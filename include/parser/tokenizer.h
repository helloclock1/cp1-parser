#pragma once

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stack>
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
    NUMBER,
    FLOAT,

    NONE
};

// TODO(helloclock): perhaps do something when information about token lexeme is redundanat
class Token {
public:
    Token();
    Token(TokenType type);
    Token(TokenType type, std::string lexeme);

    TokenType GetType() const;

    const std::string &GetLexeme() const;

    bool operator==(const Token &other) const;
    bool operator!=(const Token &other) const;

private:
    TokenType type_;
    std::optional<std::string> lexeme_;
};

class Tokenizer {
public:
    Tokenizer(std::istream *ptr, size_t spaces_per_tab);
    void ReadToken(TokenType expected = TokenType::NONE);
    Token GetToken() const;

private:
    void ReadNumber();
    void ReadWord();

    std::istream *in_;
    Token current_token_;

    size_t spaces_per_tab_;

    // Context for processing indentation
    bool substruct_started_ = false;
    std::stack<size_t> indents_;
    size_t current_indent_spaces_ = 0;
    size_t indentation_level_ = 0;
};
