
    static std::string nx_shell_quote(const char* s) {

        std::string out = "'";

        if (!s)
            return out + "'";

        for (const char c : std::string(s)) {
            if (c == '\'')
                out += "'\\''";
            else
                out += c;
        }
        out += "'";
        return out;
    }
