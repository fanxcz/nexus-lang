#pragma once
#include <cstddef>
#include <string>

namespace nexus {

    enum class TokenKind {

        End,
        Identifier,
        Integer,
        Float,
        String,
        KwFn,
        KwLet,
        KwConst,
        KwMut,
        KwReturn,
        KwIf,
        KwElse,
        KwWhile,
        KwFor,
        KwIn,
        KwBreak,
        KwContinue,
        KwDefer,
        KwStruct,
        KwEnum,
        KwImport,
        KwExtern,
        KwUnsafe,
        KwAs,
        KwMatch,
        KwTrue,
        KwFalse,
        KwPrint,
        Plus,
        Minus,
        Star,
        Slash,
        Percent,
        PlusEqual,
        MinusEqual,
        StarEqual,
        SlashEqual,
        PercentEqual,
        Range,
        Equal,
        EqualEqual,
        BangEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        Bang,
        AndAnd,
        OrOr,
        Ampersand,
        Pipe,
        Caret,
        ShiftLeft,
        ShiftRight,
        AmpEqual,
        PipeEqual,
        CaretEqual,
        ShiftLeftEqual,
        ShiftRightEqual,
        LParen,
        RParen,
        LBrace,
        RBrace,
        LBracket,
        RBracket,
        Comma,
        Colon,
        ColonColon,
        Dot,
        Arrow,
        FatArrow,
        Semicolon,
        Question,
    };

    struct SourcePos {
        std::size_t line = 1, column = 1, offset = 0;
    };

    struct Token {
        TokenKind kind;
        std::string text;
        SourcePos pos;
        // Lexer-recorded exclusive end; token.text may be decoded (for example, strings).
        SourcePos endPos{0, 0, 0};
        bool hasEndPos = false;
    };

    const char* tokenName(TokenKind kind);

} // namespace nexus
