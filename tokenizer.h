#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>

enum class TokenType {
    // Keywords
    IMPORT,
    AS,
    MODULE,
    LET,
    WHERE,

    // Punctuation
    DOT,
    COMMA,
    L_BRACKET,
    R_BRACKET,
    ASSIGN,
    INDENT,
    DEDENT,
    EOL,
    FILE_END,

    // Math operators
    ADD,
    SUB,
    MUL,
    DIV,
    POW,

    // Entities
    IDENTIFIER,
    // LITERAL
    NUMBER
};

// TODO(helloclock): perhaps do something when information about token lexeme is redundanat
class Token {
public:
    Token() : type_(TokenType::EOL), lexeme_("\n") {
    }
    Token(TokenType type, std::string lexeme) : type_(type), lexeme_(lexeme) {
    }

    TokenType GetType() const {
        return type_;
    }

    const std::string &GetLexeme() const {
        return lexeme_;
    }

    bool operator==(const Token &other) const;
    bool operator!=(const Token &other) const;

private:
    TokenType type_;
    std::string lexeme_;
};

class Tokenizer {
public:
    Tokenizer(std::istream *ptr) : in_(ptr) {
    }
    void ReadToken();
    Token GetToken() const;

private:
    void ReadNumber();
    void ReadWord();

    std::istream *in_;
    Token current_token_;
    size_t current_indent_ = 0;
};

void ParseImports(Tokenizer &tokenizer);
