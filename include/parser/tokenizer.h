#pragma once

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

class TokenizerError : public std::runtime_error {
public:
    explicit TokenizerError(std::pair<size_t, size_t> coords,
                            const std::string &msg);

private:
    char *msg_;
};

/**
 * @enum class TokenType
 * @brief Represents all types of tokens that are present in the language.
 *
 * This enum is an extensive list of all possible types of tokens of the given
 * programming language, plus several "meta-tokens" such as EOL, denting symbols
 * etc.
 */
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
    // Literals
    INTEGER,
    FLOAT,

    NONE  ///< Has no usage besides `Tokenizer::ReadToken` function
};

/**
 * @class Token
 * @brief Represents a token of the language.
 *
 * Provides minimal information about the token, information being the type of
 * token and its representation aka lexeme (if applicable and not redundant; for
 * example, representation is obviously not applicable to an indent).
 */
class Token {
public:
    /** @brief Constructs an empty token, only used for the first token of the
     * file.
     */
    Token();

    /**
     * @brief Constructs a token from its type if either the token has no
     * representation or it is redundant.
     */
    Token(TokenType type);

    /**
     * @brief Constructs a token from its type and lexeme.
     */
    Token(TokenType type, std::string lexeme);

    /**
     * @brief Gets token's type.
     */
    TokenType GetType() const;

    /**
     * @brief Gets token's lexeme if it's applicable.
     */
    const std::string &GetLexeme() const;

    bool operator==(const Token &other) const;
    bool operator!=(const Token &other) const;

private:
    TokenType type_;
    std::optional<std::string> lexeme_;
};

/**
 * @class Tokenizer
 * @brief Represents tokenizer of the source file.
 */
class Tokenizer {
public:
    /**
     * @brief Construct tokenizer entity from `std::istream` object (from where
     * to read) and a meta parameter `spaces_per_tab`.
     */
    Tokenizer(std::istream *ptr, size_t spaces_per_tab);

    /**
     * @brief Reads a new token from the stream.
     *
     * Reads a new token from the stream (if possible) and stores it in
     * `current_token_`. If provided with `expected` parameter, checks whether
     * the token just read is of same type. If not, throws an error.
     */
    void ReadToken(TokenType expected = TokenType::NONE);

    /**
     * @brief Gets the last read token.
     */
    Token GetToken() const;

    /**
     * @brief Function that returns current line and column.
     */
    std::pair<size_t, size_t> GetCoords() const;

private:
    /**
     * @brief Helper function for throwing errors.
     */
    void ThrowError(std::string msg);

    /**
     * @brief Helper function for reading a character from stream, keeping track
     * of line and column.
     */
    char StreamRead();
    /**
     * @brief Helper function for reading a token of types `TokenType::NUMBER`
     * and `TokenType::FLOAT`.
     */
    void ReadNumber();
    /**
     * @brief Helper function for reading a sequence of alphanumeric characters.
     */
    void ReadWord();

    std::istream *in_;
    Token current_token_;

    size_t spaces_per_tab_;

    size_t dedents_ = 0;

    // Context for processing indentation
    bool substruct_started_ = false;
    std::stack<size_t> indents_;
    size_t current_indent_spaces_ = 0;
    size_t indentation_level_ = 0;

    size_t line_ = 1;
    size_t column_ = 1;
};
