#pragma once
#include "driver/driver.hpp"

#include "builtins.hpp"
#include "codegen/llvm_ir.hpp"
#include "format/formatter.hpp"
#include "lexer/lexer.hpp"
#include "native/native.hpp"
#include "package/sha256.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifndef NEXUS_VERSION
#define NEXUS_VERSION "dev"
#endif


namespace nexus {

    namespace fs = std::filesystem;

    // Internal interface shared by the driver translation units. None
    // of these symbols are exported from the compiler binary.
    namespace driver {


        struct ProjectConfig {

            fs::path root;

            std::string name;

            std::string version;

            std::string entry;

            std::string artifact;

            std::string target;

            int64_t windowWidth = 1280;

            int64_t windowHeight = 720;

            std::string windowTitle;
        };

        struct ModuleLoadContext {
            std::set<fs::path> active;
            std::set<fs::path> loaded;
            std::vector<fs::path> stack;
        };

        struct ParsedPackageLockfile {
            std::map<std::string, std::string> metadata;
            std::map<std::string, std::map<std::string, std::string>> sections;
        };


        enum class ReplInputResult { Line, EndOfInput, Interrupted };

        inline constexpr const char* kVersion = NEXUS_VERSION;
        inline fs::path gExecutable;
        inline fs::path gDiagnosticSource;
        inline std::string gDiagnosticText;
        inline constexpr std::size_t kReplHistoryLimit = 500;

        // One parsed `key = value` entry of a project or package manifest.
        // `section` is empty for keys that appear before any table header;
        // `instance` is 0 for a regular table and 1..N inside `[[table]]`.
        struct ManifestEntry {
            std::string section;
            std::string key;
            std::string value;
            std::size_t line = 0;
            std::size_t instance = 0;
        };

        // Parse and validate manifest structure. See support/manifest.cpp for the
        // accepted subset and the error contract.
        std::vector<ManifestEntry> parseManifestEntries(const std::string& data,
                                                        const std::string& context);

        // Read a whole file as bytes. Ownership: returns a copy, the caller owns
        // it. Errors: throws std::runtime_error("cannot open <path>") when the
        // file cannot be opened, "...: is a directory" when a directory is passed
        // (POSIX opens directories successfully, so this is checked explicitly),
        // and "cannot read <path>: ..." when a stream failure occurs mid-read.
        // Platform limits: text is returned verbatim, without newline or encoding
        // translation, on every platform.
        std::string readFile(const fs::path& p);
        std::string trim(std::string s);
        std::string formatDiagnostic(const std::exception& error,
                                     const std::string& code = "NX1001");
        // Look up a string value. Input: manifest text, section name and key;
        // section is empty for keys before any table header. Output: the raw
        // value without surrounding quotes, or "" when the key is absent, holds
        // an array, or repeats in a later `[[table]]` instance (the first
        // occurrence wins). Never throws; structural problems are the parser's
        // job, so call loadProject/parseManifestEntries first to surface them.
        std::string
        tomlString(const std::string& data, const std::string& section, const std::string& key);

        // Integer lookup built on tomlString. Returns `fallback` when the key is
        // absent, holds an array, or does not parse as a base-10 integer.
        int64_t tomlInt(const std::string& data,
                        const std::string& section,
                        const std::string& key,
                        int64_t fallback);
        fs::path findProjectRoot(fs::path start);
        ProjectConfig loadProject(const fs::path& requestedRoot = fs::current_path());
        int runCommand(const std::vector<std::string>& argv,
                       const fs::path& workDir = fs::path(),
                       bool quiet = false);
        void usage();
        std::string targetTriple(const std::string& t);
        void appendProgram(Program& dst, Program src);
        Program loadProgram(const fs::path& root, ModuleLoadContext& context);
        fs::path resolveInput(const std::string& arg);
        fs::path templateSource(const std::string& templ);
        fs::path runtimeSource();
        fs::path runtimeLibraryDir();
        void deployRuntimeLibraries(const fs::path& output);
        fs::path defaultOutput(const fs::path& input,
                               const std::string& explicitOutput,
                               bool release,
                               bool windowsTarget);
        int launchExecutable(const fs::path& executable, const fs::path& workingDirectory);
        bool hostIsX64();
        void writeBinaryRetry(const fs::path& out, const std::uint8_t* data, std::size_t size);
        int compileFile(const fs::path& in,
                        const fs::path& out,
                        const std::string& target,
                        bool emitIR,
                        int opt,
                        const std::string& backend = "auto",
                        bool quiet = false);
        void writeProjectManifest(const fs::path& dir,
                                  const std::string& name,
                                  const std::string& templ);
        fs::path sourceDataRoot();
        void seedAssets(const fs::path& dir);
        int initProject(const fs::path& dir,
                        const std::string& name,
                        const std::string& templ,
                        bool failIfNonEmpty);
        int commandInfo();
        int commandAssets();
        int commandClean();
        int commandDoctor(bool json = false);
        int commandExplainBackend(const fs::path& file, const std::string& target);
        void writeText(const fs::path& p, const std::string& data);
        void writeTextAtomically(const fs::path& p, const std::string& data);
        std::string packageTreeSha256(const fs::path& root);
        int commandFmt(const fs::path& file, bool checkOnly = false);
        int commandFmtAll(bool checkOnly);
        int commandSelfTest();
        int commandLint(const fs::path& file);
        int commandDoc();
        int commandBench(const fs::path& file, int runs);
        int commandTest();
        int commandExport(const std::vector<std::string>& args);
        int commandEditor();
        bool validPackageName(const std::string& name);
        std::string stripTomlInlineComment(const std::string& line);
        std::size_t findTomlAssignment(const std::string& line);
        std::string parsePackageMetadataString(const std::string& rawValue,
                                               const std::string& context);
        std::map<std::string, std::string> readStrictTomlStringSection(
            const std::string& data, const std::string& wantedSection, const std::string& context);
        ParsedPackageLockfile parsePackageLockfile(const std::string& data);
        void ensureSafeVendorDirectory(const ProjectConfig& cfg, bool createIfMissing);
        void updateDependency(const fs::path& manifest,
                              const std::string& name,
                              const std::string& value,
                              bool remove);
        void refreshLockfile(const ProjectConfig& cfg);
        void copyPackageTree(const fs::path& source, const fs::path& destination);
        int commandPkg(const std::vector<std::string>& args);
        int commandLsp();
        bool replStartsWith(const std::string& value, std::string_view prefix);
        int replBraceBalance(const std::string& source);
        bool replHasAssignment(const std::string& source);
        bool replIsDeclaration(const std::string& line);
        std::string replImplicitLet(const std::string& source);
        bool replIsBlockStatement(const std::string& line);
        bool replExpressionIsReplaySafe(const Expr* expression);
        bool replStatementIsReplaySafe(const Stmt* statement);
        bool replIsReplaySafeBlock(const std::string& line);
        bool replIsStoredStatement(const std::string& line);
        bool replIsDirectStatement(const std::string& line);
        bool replBareFunctionCall(const std::string& source, std::string& functionName);
        bool replIsVoidFunctionCall(const std::string& line,
                                    const std::vector<std::string>& declarations);
        std::string replBindingName(const std::string& line);
        void replWriteIndented(std::ostream& output, const std::string& source);
        fs::path replHistoryPath();
        std::string encodeReplHistoryEntry(const std::string& entry);
        std::string decodeReplHistoryEntry(const std::string& encoded);
        std::vector<std::string> loadReplHistory(const fs::path& path);
        bool saveReplHistory(const fs::path& path, const std::vector<std::string>& entries);
        std::size_t replPreviousUtf8Boundary(const std::string& value, std::size_t cursor);
        std::size_t replNextUtf8Boundary(const std::string& value, std::size_t cursor);
        std::size_t replDisplayWidth(const std::string& value);
        ReplInputResult readReplInputLine(const std::string& prompt,
                                          const std::vector<std::string>& history,
                                          const std::vector<std::string>& completionCandidates,
                                          std::string& inputLine);
        int commandRepl();

    } // namespace driver
} // namespace nexus
