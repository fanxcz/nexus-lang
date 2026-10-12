#pragma once
#include "lexer/token.hpp"
#include <string>
#include <vector>

namespace nexus {

    class Lexer {

        public:
        explicit Lexer(std::string source): source_(std::move(source)) {
        }
        std::vector<Token> tokenize();

        private:
        std::string source_;
        std::size_t i_=0;
        SourcePos pos_{
        };

        char peek(std::size_t n=0) const;

        char take();

        void skipWhitespaceAndComments();

        Token identOrKeyword();
        Token number();
        Token string();

    };

}
