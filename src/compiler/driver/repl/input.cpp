#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        constexpr int kReplKeyUp = 0x1001;

        constexpr int kReplKeyDown = 0x1002;

        constexpr int kReplKeyLeft = 0x1003;

        constexpr int kReplKeyRight = 0x1004;

        constexpr int kReplKeyHome = 0x1005;

        constexpr int kReplKeyEnd = 0x1006;

        constexpr int kReplKeyDelete = 0x1007;


        std::size_t replPreviousUtf8Boundary(const std::string& value, std::size_t cursor) {
            if (cursor == 0)
                return 0;
            --cursor;
            while (cursor > 0 &&
                   (static_cast<unsigned char>(value[cursor]) & 0xc0) == 0x80)
                --cursor;
            return cursor;
        }


        std::size_t replNextUtf8Boundary(const std::string& value, std::size_t cursor) {
            if (cursor >= value.size())
                return value.size();
            ++cursor;
            while (cursor < value.size() &&
                   (static_cast<unsigned char>(value[cursor]) & 0xc0) == 0x80)
                ++cursor;
            return cursor;
        }


        std::size_t replDisplayWidth(const std::string& value) {
            std::size_t width = 0;
            for (const unsigned char ch : value) {
                if ((ch & 0xc0) != 0x80)
                    ++width;
            }
            return width;
        }


#ifdef _WIN32
        struct ReplTerminalMode {
            HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
            DWORD originalMode = 0;
            bool active = false;

            ReplTerminalMode() {
                if (input == INVALID_HANDLE_VALUE || !GetConsoleMode(input, &originalMode))
                    return;
                const DWORD rawMode =
                    originalMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
                active = SetConsoleMode(input, rawMode) != 0;
            }

            ~ReplTerminalMode() {
                if (active)
                    SetConsoleMode(input, originalMode);
            }
        };

#else
        struct ReplTerminalMode {
            termios original{};
            bool active = false;

            ReplTerminalMode() {
                if (::tcgetattr(STDIN_FILENO, &original) != 0)
                    return;
                termios raw = original;
                raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
                raw.c_cc[VMIN] = 1;
                raw.c_cc[VTIME] = 0;
                active = ::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
            }

            ~ReplTerminalMode() {
                if (active)
                    ::tcsetattr(STDIN_FILENO, TCSANOW, &original);
            }
        };

#endif

        ReplInputResult readReplInputLine(const std::string& prompt,
                                          const std::vector<std::string>& history,
                                          const std::vector<std::string>& completionCandidates,
                                          std::string& inputLine) {
#ifdef _WIN32
            if (_isatty(_fileno(stdin)) == 0) {
                if (std::getline(std::cin, inputLine))
                    return ReplInputResult::Line;
                return ReplInputResult::EndOfInput;
            }
            ReplTerminalMode terminalMode;
            if (!terminalMode.active) {
                if (std::getline(std::cin, inputLine))
                    return ReplInputResult::Line;
                return ReplInputResult::EndOfInput;
            }
            auto readKey = []() -> int {
                const int ch = _getch();
                if (ch == 0 || ch == 224) {
                    switch (_getch()) {
                    case 72: return kReplKeyUp;
                    case 80: return kReplKeyDown;
                    case 75: return kReplKeyLeft;
                    case 77: return kReplKeyRight;
                    case 71: return kReplKeyHome;
                    case 79: return kReplKeyEnd;
                    case 83: return kReplKeyDelete;
                    default: return -2;
                    }
                }
                return ch;
            };
#else
            if (::isatty(STDIN_FILENO) == 0) {
                if (std::getline(std::cin, inputLine))
                    return ReplInputResult::Line;
                return ReplInputResult::EndOfInput;
            }
            ReplTerminalMode terminalMode;
            if (!terminalMode.active) {
                if (std::getline(std::cin, inputLine))
                    return ReplInputResult::Line;
                return ReplInputResult::EndOfInput;
            }
            const auto readByte = [](int timeoutMs) -> int {
                pollfd descriptor{STDIN_FILENO, POLLIN, 0};
                if (::poll(&descriptor, 1, timeoutMs) <= 0)
                    return -1;
                unsigned char byte = 0;
                if (::read(STDIN_FILENO, &byte, 1) != 1)
                    return -1;
                return byte;
            };
            auto readKey = [&]() -> int {
                unsigned char byte = 0;
                if (::read(STDIN_FILENO, &byte, 1) != 1)
                    return -1;
                if (byte != 27)
                    return byte;
                const int opening = readByte(35);
                if (opening != '[' && opening != 'O')
                    return 27;
                const int final = readByte(35);
                switch (final) {
                case 'A': return kReplKeyUp;
                case 'B': return kReplKeyDown;
                case 'C': return kReplKeyRight;
                case 'D': return kReplKeyLeft;
                case 'H': return kReplKeyHome;
                case 'F': return kReplKeyEnd;
                case '3':
                    return readByte(35) == '~' ? kReplKeyDelete : -2;
                default: return -2;
                }
            };
#endif
            std::string line;
            std::string draft;
            std::size_t cursor = 0;
            std::size_t historyIndex = history.size();
            std::size_t displayedColumns = 0;
            const auto redraw = [&]() {
                const std::size_t lineColumns = replDisplayWidth(line);
                const std::size_t cursorColumns = replDisplayWidth(line.substr(0, cursor));
                const std::size_t width = std::max(displayedColumns, lineColumns);
                std::cout << '\r' << prompt << line;
                if (width > lineColumns)
                    std::cout << std::string(width - lineColumns, ' ');
                if (width > cursorColumns)
                    std::cout << std::string(width - cursorColumns, '\b');
                std::cout.flush();
                displayedColumns = lineColumns;
            };

            for (;;) {
                const int key = readKey();
                if (key == -2)
                    continue;
                if (key == -1) {
                    if (line.empty())
                        return ReplInputResult::EndOfInput;
                    std::cout << "\r\n" << std::flush;
                    inputLine = std::move(line);
                    return ReplInputResult::Line;
                }
                if (key == '\r' || key == '\n') {
                    std::cout << "\r\n" << std::flush;
                    inputLine = std::move(line);
                    return ReplInputResult::Line;
                }
                if (key == 3) {
                    std::cout << "^C\r\n" << std::flush;
                    return ReplInputResult::Interrupted;
                }
                if (key == 4) {
                    if (line.empty())
                        return ReplInputResult::EndOfInput;
                    if (cursor < line.size()) {
                        line.erase(cursor, replNextUtf8Boundary(line, cursor) - cursor);
                        redraw();
                    }
                    continue;
                }
                if (key == 1) { // Ctrl+A
                    cursor = 0;
                    redraw();
                    continue;
                }
                if (key == 5) { // Ctrl+E
                    cursor = line.size();
                    redraw();
                    continue;
                }
                if (key == 21) { // Ctrl+U
                    line.erase(0, cursor);
                    cursor = 0;
                    redraw();
                    continue;
                }
                if (key == 11) { // Ctrl+K
                    line.erase(cursor);
                    redraw();
                    continue;
                }
                if (key == kReplKeyLeft) {
                    cursor = replPreviousUtf8Boundary(line, cursor);
                    redraw();
                    continue;
                }
                if (key == kReplKeyRight) {
                    cursor = replNextUtf8Boundary(line, cursor);
                    redraw();
                    continue;
                }
                if (key == kReplKeyHome) {
                    cursor = 0;
                    redraw();
                    continue;
                }
                if (key == kReplKeyEnd) {
                    cursor = line.size();
                    redraw();
                    continue;
                }
                if (key == kReplKeyDelete) {
                    if (cursor < line.size()) {
                        line.erase(cursor, replNextUtf8Boundary(line, cursor) - cursor);
                        redraw();
                    }
                    continue;
                }
                if (key == kReplKeyUp) {
                    if (history.empty())
                        continue;
                    if (historyIndex == history.size())
                        draft = line;
                    if (historyIndex > 0)
                        --historyIndex;
                    line = history[historyIndex];
                    cursor = line.size();
                    redraw();
                    continue;
                }
                if (key == kReplKeyDown) {
                    if (historyIndex < history.size()) {
                        ++historyIndex;
                        line = historyIndex == history.size() ? draft : history[historyIndex];
                        cursor = line.size();
                        redraw();
                    }
                    continue;
                }
                if (key == '\t') {
                    const auto isIdentifierByte = [](unsigned char ch) {
                        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                               (ch >= '0' && ch <= '9') || ch == '_';
                    };
                    std::size_t wordStart = cursor;
                    while (wordStart > 0 &&
                           isIdentifierByte(static_cast<unsigned char>(line[wordStart - 1])))
                        --wordStart;
                    std::size_t wordEnd = cursor;
                    while (wordEnd < line.size() &&
                           isIdentifierByte(static_cast<unsigned char>(line[wordEnd])))
                        ++wordEnd;
                    const std::string prefix = line.substr(wordStart, cursor - wordStart);
                    std::vector<std::string> matches;
                    for (const auto& candidate : completionCandidates) {
                        if (candidate.compare(0, prefix.size(), prefix) == 0)
                            matches.push_back(candidate);
                    }
                    if (!matches.empty()) {
                        std::size_t commonLength = matches.front().size();
                        for (std::size_t i = 1; i < matches.size(); ++i) {
                            commonLength = std::min(commonLength, matches[i].size());
                            std::size_t j = 0;
                            while (j < commonLength && matches.front()[j] == matches[i][j])
                                ++j;
                            commonLength = j;
                        }
                        const std::size_t replacementLength = matches.size() == 1
                                                                  ? matches.front().size()
                                                                  : std::max(commonLength, prefix.size());
                        if (replacementLength > prefix.size() || matches.size() == 1) {
                            const std::string replacement =
                                matches.front().substr(0, replacementLength);
                            line.replace(wordStart, wordEnd - wordStart, replacement);
                            cursor = wordStart + replacement.size();
                        }
                        if (matches.size() == 1) {
                            redraw();
                        } else {
                            if (replacementLength > prefix.size())
                                redraw();
                            std::cout << "\r\n";
                            for (const auto& match : matches)
                                std::cout << match << "  ";
                            std::cout << "\r\n";
                            displayedColumns = 0;
                            redraw();
                        }
                    }
                    continue;
                }
                if (key == 8 || key == 127) {
                    if (cursor > 0) {
                        const std::size_t previous = replPreviousUtf8Boundary(line, cursor);
                        line.erase(previous, cursor - previous);
                        cursor = previous;
                        redraw();
                    }
                    continue;
                }
                if (key >= 32 && key <= 255) {
                    line.insert(cursor, 1, static_cast<char>(key));
                    ++cursor;
                    redraw();
                }
            }
        }


    } // namespace driver
} // namespace nexus
