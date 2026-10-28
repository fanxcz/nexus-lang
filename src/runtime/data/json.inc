
    // A bounded, strict JSON parser shared by the runtime JSON helpers. Source
    // spans let object/array values be returned as JSON without re-serializing.
    struct NxJsonValue {
        enum class Kind { Null, Boolean, Number, String, Object, Array };
        Kind kind = Kind::Null;
        size_t start = 0;
        size_t end = 0;
        std::string stringValue;
        std::vector<std::string> keys;
        std::vector<NxJsonValue> children;
    };

    class NxJsonParser {
    public:
        explicit NxJsonParser(std::string_view input) : input_(input) {}

        bool parse(NxJsonValue& root) {
            if (input_.size() > kMaxInputBytes)
                return false;
            skipWhitespace();
            if (!parseValue(root, 0))
                return false;
            skipWhitespace();
            return position_ == input_.size();
        }

    private:
        static constexpr size_t kMaxInputBytes = 16u * 1024u * 1024u;
        static constexpr size_t kMaxDepth = 128;
        static constexpr size_t kMaxNodes = 100'000;

        std::string_view input_;
        size_t position_ = 0;
        size_t nodes_ = 0;

        void skipWhitespace() {
            while (position_ < input_.size()) {
                const char c = input_[position_];
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
                    break;
                ++position_;
            }
        }

        static void appendCodePoint(std::string& out, uint32_t codePoint) {
            if (codePoint <= 0x7f) {
                out.push_back(static_cast<char>(codePoint));
            } else if (codePoint <= 0x7ff) {
                out.push_back(static_cast<char>(0xc0u | (codePoint >> 6)));
                out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
            } else if (codePoint <= 0xffff) {
                out.push_back(static_cast<char>(0xe0u | (codePoint >> 12)));
                out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3fu)));
                out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
            } else {
                out.push_back(static_cast<char>(0xf0u | (codePoint >> 18)));
                out.push_back(static_cast<char>(0x80u | ((codePoint >> 12) & 0x3fu)));
                out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3fu)));
                out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
            }
        }

        bool readHex4(uint32_t& value) {
            if (input_.size() - position_ < 4)
                return false;
            value = 0;
            for (size_t i = 0; i < 4; ++i) {
                const unsigned char c = static_cast<unsigned char>(input_[position_++]);
                value <<= 4;
                if (c >= '0' && c <= '9')
                    value |= c - '0';
                else if (c >= 'a' && c <= 'f')
                    value |= c - 'a' + 10u;
                else if (c >= 'A' && c <= 'F')
                    value |= c - 'A' + 10u;
                else
                    return false;
            }
            return true;
        }

        bool appendRawUtf8(std::string& out) {
            const size_t start = position_;
            const unsigned char first = static_cast<unsigned char>(input_[position_]);
            size_t width = 0;
            uint32_t codePoint = 0;
            uint32_t minimum = 0;
            if (first >= 0xc2 && first <= 0xdf) {
                width = 2;
                codePoint = first & 0x1fu;
                minimum = 0x80;
            } else if (first >= 0xe0 && first <= 0xef) {
                width = 3;
                codePoint = first & 0x0fu;
                minimum = 0x800;
            } else if (first >= 0xf0 && first <= 0xf4) {
                width = 4;
                codePoint = first & 0x07u;
                minimum = 0x10000;
            } else {
                return false;
            }
            if (input_.size() - position_ < width)
                return false;
            for (size_t i = 1; i < width; ++i) {
                const unsigned char next = static_cast<unsigned char>(input_[position_ + i]);
                if ((next & 0xc0u) != 0x80u)
                    return false;
                codePoint = (codePoint << 6) | (next & 0x3fu);
            }
            if (codePoint < minimum || codePoint > 0x10ffff ||
                (codePoint >= 0xd800 && codePoint <= 0xdfff))
                return false;
            out.append(input_.data() + start, width);
            position_ += width;
            return true;
        }

        bool parseString(std::string& out) {
            if (position_ >= input_.size() || input_[position_] != '"')
                return false;
            ++position_;
            out.clear();
            while (position_ < input_.size()) {
                const unsigned char c = static_cast<unsigned char>(input_[position_++]);
                if (c == '"')
                    return true;
                if (c < 0x20)
                    return false;
                if (c >= 0x80) {
                    --position_;
                    if (!appendRawUtf8(out))
                        return false;
                    continue;
                }
                if (c != '\\') {
                    out.push_back(static_cast<char>(c));
                    continue;
                }
                if (position_ >= input_.size())
                    return false;
                const char escaped = input_[position_++];
                switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    uint32_t codePoint = 0;
                    if (!readHex4(codePoint))
                        return false;
                    if (codePoint >= 0xd800 && codePoint <= 0xdbff) {
                        if (input_.size() - position_ < 6 || input_[position_] != '\\' ||
                            input_[position_ + 1] != 'u')
                            return false;
                        position_ += 2;
                        uint32_t low = 0;
                        if (!readHex4(low) || low < 0xdc00 || low > 0xdfff)
                            return false;
                        codePoint = 0x10000u + ((codePoint - 0xd800u) << 10) + (low - 0xdc00u);
                    } else if (codePoint >= 0xdc00 && codePoint <= 0xdfff) {
                        return false;
                    }
                    appendCodePoint(out, codePoint);
                    break;
                }
                default:
                    return false;
                }
            }
            return false;
        }

        bool parseNumber() {
            if (position_ < input_.size() && input_[position_] == '-')
                ++position_;
            if (position_ >= input_.size())
                return false;
            if (input_[position_] == '0') {
                ++position_;
                if (position_ < input_.size() && input_[position_] >= '0' &&
                    input_[position_] <= '9')
                    return false;
            } else {
                if (input_[position_] < '1' || input_[position_] > '9')
                    return false;
                while (position_ < input_.size() && input_[position_] >= '0' &&
                       input_[position_] <= '9')
                    ++position_;
            }
            if (position_ < input_.size() && input_[position_] == '.') {
                ++position_;
                const size_t fractionStart = position_;
                while (position_ < input_.size() && input_[position_] >= '0' &&
                       input_[position_] <= '9')
                    ++position_;
                if (position_ == fractionStart)
                    return false;
            }
            if (position_ < input_.size() &&
                (input_[position_] == 'e' || input_[position_] == 'E')) {
                ++position_;
                if (position_ < input_.size() &&
                    (input_[position_] == '+' || input_[position_] == '-'))
                    ++position_;
                const size_t exponentStart = position_;
                while (position_ < input_.size() && input_[position_] >= '0' &&
                       input_[position_] <= '9')
                    ++position_;
                if (position_ == exponentStart)
                    return false;
            }
            return true;
        }

        bool parseLiteral(std::string_view literal) {
            if (input_.substr(position_, literal.size()) != literal)
                return false;
            position_ += literal.size();
            return true;
        }

        bool parseValue(NxJsonValue& value, size_t depth) {
            skipWhitespace();
            if (depth > kMaxDepth || ++nodes_ > kMaxNodes || position_ >= input_.size())
                return false;
            value.start = position_;
            const char c = input_[position_];
            if (c == '"') {
                value.kind = NxJsonValue::Kind::String;
                if (!parseString(value.stringValue))
                    return false;
            } else if (c == '{') {
                value.kind = NxJsonValue::Kind::Object;
                ++position_;
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == '}') {
                    ++position_;
                } else {
                    while (true) {
                        skipWhitespace();
                        std::string key;
                        if (!parseString(key))
                            return false;
                        skipWhitespace();
                        if (position_ >= input_.size() || input_[position_++] != ':')
                            return false;
                        NxJsonValue child;
                        if (!parseValue(child, depth + 1))
                            return false;
                        value.keys.push_back(std::move(key));
                        value.children.push_back(std::move(child));
                        skipWhitespace();
                        if (position_ >= input_.size())
                            return false;
                        if (input_[position_] == '}') {
                            ++position_;
                            break;
                        }
                        if (input_[position_++] != ',')
                            return false;
                    }
                }
            } else if (c == '[') {
                value.kind = NxJsonValue::Kind::Array;
                ++position_;
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == ']') {
                    ++position_;
                } else {
                    while (true) {
                        NxJsonValue child;
                        if (!parseValue(child, depth + 1))
                            return false;
                        value.children.push_back(std::move(child));
                        skipWhitespace();
                        if (position_ >= input_.size())
                            return false;
                        if (input_[position_] == ']') {
                            ++position_;
                            break;
                        }
                        if (input_[position_++] != ',')
                            return false;
                    }
                }
            } else if (c == 't') {
                value.kind = NxJsonValue::Kind::Boolean;
                if (!parseLiteral("true"))
                    return false;
            } else if (c == 'f') {
                value.kind = NxJsonValue::Kind::Boolean;
                if (!parseLiteral("false"))
                    return false;
            } else if (c == 'n') {
                value.kind = NxJsonValue::Kind::Null;
                if (!parseLiteral("null"))
                    return false;
            } else {
                value.kind = NxJsonValue::Kind::Number;
                if (!parseNumber())
                    return false;
            }
            value.end = position_;
            return true;
        }
    };

    static bool nx_parse_json_source(const char* input,
                                    std::string& source,
                                    NxJsonValue& root) {
        if (!input)
            return false;
        const size_t length = std::strlen(input);
        if (length > 16u * 1024u * 1024u)
            return false;
        source.assign(input, length);
        return NxJsonParser(source).parse(root);
    }

    static const NxJsonValue* nx_json_find_member(const NxJsonValue& object, const char* key) {
        if (object.kind != NxJsonValue::Kind::Object || !key)
            return nullptr;
        // For duplicate keys, prefer the last occurrence, matching common parsers.
        for (size_t i = object.keys.size(); i > 0; --i)
            if (object.keys[i - 1] == key)
                return &object.children[i - 1];
        return nullptr;
    }

    static std::string nx_json_value_text(const std::string& source, const NxJsonValue& value) {
        if (value.kind == NxJsonValue::Kind::String)
            return value.stringValue;
        return source.substr(value.start, value.end - value.start);
    }

    static const char* nx_json_kind_name(NxJsonValue::Kind kind) {
        switch (kind) {
        case NxJsonValue::Kind::Null: return "null";
        case NxJsonValue::Kind::Boolean: return "boolean";
        case NxJsonValue::Kind::Number: return "number";
        case NxJsonValue::Kind::String: return "string";
        case NxJsonValue::Kind::Object: return "object";
        case NxJsonValue::Kind::Array: return "array";
        }
        return "invalid";
    }
