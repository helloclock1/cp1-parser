#pragma once

#include <string>
#include <unordered_map>

#include "parser.h"
#include "tokenizer.h"

inline const std::unordered_map<Operator, std::string> kOperatorRepr{
    {Operator::ADD, "+"},
    {Operator::SUB, "-"},
    {Operator::MUL, "*"},
    {Operator::DIV, "/"},
    {Operator::POW, "^"}};

inline const std::unordered_map<Operator, int> kOperatorPrecedence{
    {Operator::ADD, 1},
    {Operator::SUB, 1},
    {Operator::MUL, 2},
    {Operator::DIV, 2},
    {Operator::POW, 3}};

inline const std::unordered_map<std::string, Token> kStringToToken{
    {"import", Token(TokenType::IMPORT, "import")},
    {"let", Token(TokenType::LET, "let")},
    {"as", Token(TokenType::AS, "as")},
    {"where", Token(TokenType::WHERE, "where")},
    {"module", Token(TokenType::MODULE, "module")}};

inline const std::unordered_map<TokenType, std::string> kTokenName = {
    {TokenType::IMPORT, "import"},
    {TokenType::AS, "as"},
    {TokenType::MODULE, "module"},
    {TokenType::LET, "let"},
    {TokenType::WHERE, "where"},
    {TokenType::DOT, "dot"},
    {TokenType::COMMA, "comma"},
    {TokenType::L_BRACKET, "opening bracket"},
    {TokenType::R_BRACKET, "closing bracket"},
    {TokenType::ASSIGN, "assignment operator"},
    {TokenType::INDENT, "indent"},
    {TokenType::DEDENT, "dedent"},
    {TokenType::EOL, "end of line"},
    {TokenType::FILE_END, "end of file"},
    {TokenType::ADD, "addition"},
    {TokenType::SUB, "subtraction"},
    {TokenType::MUL, "multiplication"},
    {TokenType::DIV, "division"},
    {TokenType::POW, "power"},
    {TokenType::IDENTIFIER, "identifier"},
    {TokenType::INTEGER, "integer"},
    {TokenType::FLOAT, "float"}};
