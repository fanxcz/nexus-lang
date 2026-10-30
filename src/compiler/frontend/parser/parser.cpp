#include "parser/parser.hpp"

#include "parser/parser_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace nexus {

    Program Parser::parseProgram() {
        Program p;
        recoveredErrors_.clear();
        std::vector<std::string> errors;

        const auto isDeclarationStart = [](TokenKind kind) {
            return kind == TokenKind::KwFn || kind == TokenKind::KwStruct ||
                   kind == TokenKind::KwEnum || kind == TokenKind::KwImport ||
                   kind == TokenKind::KwExtern;
        };

        while (cur().kind != TokenKind::End) {
            const std::size_t declarationStart = i_;
            try {
                if (accept(TokenKind::KwImport)) {
                    if (cur().kind == TokenKind::String) {
                        p.imports.push_back(expect(TokenKind::String).text);
                    } else {
                        std::string name = expectIdentifier();
                        while (accept(TokenKind::Dot))
                            name += "." + expectIdentifier();
                        p.imports.push_back(std::move(name));
                    }
                    accept(TokenKind::Semicolon);
                    continue;
                }
                if (accept(TokenKind::KwStruct)) {
                    --i_;
                    auto declaration = parseStruct();
                    declaration.span = spanForTokens(declarationStart, i_);
                    p.structs.push_back(std::move(declaration));
                    continue;
                }
                if (accept(TokenKind::KwEnum)) {
                    --i_;
                    auto declaration = parseEnum();
                    declaration.span = spanForTokens(declarationStart, i_);
                    p.enums.push_back(std::move(declaration));
                    continue;
                }
                if (accept(TokenKind::KwExtern)) {
                    const Token abi = expect(TokenKind::String);
                    if (abi.text != "C")
                        throw std::runtime_error("unsupported extern ABI '" + abi.text +
                                                 "'; only extern \"C\" is supported at " +
                                                 std::to_string(abi.pos.line) + ":" +
                                                 std::to_string(abi.pos.column));
                    auto declaration = parseFunction(true);
                    declaration.span = spanForTokens(declarationStart, i_);
                    p.functions.push_back(std::move(declaration));
                    continue;
                }
                auto declaration = parseFunction(false);
                declaration.span = spanForTokens(declarationStart, i_);
                p.functions.push_back(std::move(declaration));
            } catch (const std::exception& error) {
                errors.emplace_back(error.what());

                // Synchronize at the next declaration instead of abandoning the
                // entire file at the first syntax error. NEXUS does not allow
                // nested function/type declarations, so these keywords form a
                // safe recovery set for malformed top-level declarations.
                std::size_t candidate = i_;
                if (candidate <= declarationStart)
                    candidate = declarationStart + 1;
                while (candidate < tokens_.size() && tokens_[candidate].kind != TokenKind::End &&
                       !isDeclarationStart(tokens_[candidate].kind)) {
                    ++candidate;
                }

                if (candidate >= tokens_.size() || tokens_[candidate].kind == TokenKind::End) {
                    i_ = tokens_.size() - 1;
                    break;
                }

                // Avoid repeatedly retrying the token that produced this error.
                if (candidate <= declarationStart) {
                    ++candidate;
                    while (candidate < tokens_.size() &&
                           tokens_[candidate].kind != TokenKind::End &&
                           !isDeclarationStart(tokens_[candidate].kind)) {
                        ++candidate;
                    }
                }
                i_ = candidate;
            }
        }

        errors.insert(errors.end(), recoveredErrors_.begin(), recoveredErrors_.end());

        if (!errors.empty()) {
            std::ostringstream message;
            message << "NEXUS found " << errors.size() << " syntax error";
            if (errors.size() != 1)
                message << 's';
            message << ':';
            for (const auto& error : errors)
                message << "\n  - " << error;
            throw std::runtime_error(message.str());
        }

        return p;
    }
    StructDecl Parser::parseStruct() {
        expect(TokenKind::KwStruct);
        StructDecl s;
        s.name = expectIdentifier();
        expect(TokenKind::LBrace);
        while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End) {
            const std::size_t fieldStart = i_;
            auto n = expectIdentifier();
            expect(TokenKind::Colon);
            auto t = parseType();
            StructField field{n, t};
            field.span = spanForTokens(fieldStart, i_);
            s.fields.push_back(std::move(field));
            accept(TokenKind::Comma);
            accept(TokenKind::Semicolon);
        }
        expect(TokenKind::RBrace);
        accept(TokenKind::Semicolon);
        return s;
    }
    EnumDecl Parser::parseEnum() {
        expect(TokenKind::KwEnum);
        EnumDecl e;
        e.name = expectIdentifier();
        expect(TokenKind::LBrace);
        while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End) {
            EnumVariant variant;
            variant.name = expectIdentifier();
            if (accept(TokenKind::LParen)) {
                variant.payload = parseType();
                expect(TokenKind::RParen);
            }
            e.variants.push_back(std::move(variant));
            accept(TokenKind::Comma);
            accept(TokenKind::Semicolon);
        }
        expect(TokenKind::RBrace);
        accept(TokenKind::Semicolon);
        return e;
    }
    Function Parser::parseFunction(bool external) {
        expect(TokenKind::KwFn);
        auto name = expectIdentifier();
        expect(TokenKind::LParen);
        std::vector<Param> ps;
        if (cur().kind != TokenKind::RParen) {
            for (;;) {
                const std::size_t paramStart = i_;
                auto n = expectIdentifier();
                expect(TokenKind::Colon);
                auto type = parseType();
                Param param{n, std::move(type)};
                param.span = spanForTokens(paramStart, i_);
                ps.push_back(std::move(param));
                if (!accept(TokenKind::Comma))
                    break;
            }
        }
        expect(TokenKind::RParen);
        Type ret = Type::void_();
        if (accept(TokenKind::Arrow))
            ret = parseType();
        if (external) {
            accept(TokenKind::Semicolon);
            return {name, std::move(ps), ret, nullptr, true};
        }
        auto body = parseBlock();
        return {name, std::move(ps), ret, std::move(body), false};
    }

} // namespace nexus
