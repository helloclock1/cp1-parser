#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

#include "tokenizer.h"

class Module;
struct BinaryOperation;
struct FunctionCall;
struct Function;

struct Imports {
    void AddImport(const std::string& module_name, const std::string& alias,
                   std::set<std::string> functions);

    std::map<std::string, std::pair<std::string, std::set<std::string>>> modules_map_;
};

struct Variable {
    std::string name_;
};
struct Number {
    int value_;
};
struct Float {
    double value_;
};
using Expression = std::variant<BinaryOperation, FunctionCall, Variable, Number, Float>;
enum class Operator { ADD, SUB, MUL, DIV, POW, ROOT };
struct BinaryOperation {
    std::unique_ptr<Expression> lhs_;
    Operator op_;
    std::unique_ptr<Expression> rhs_;
    Operator parent_operator_ = Operator::ROOT;
};
struct FunctionCall {
    std::string name_;
    std::vector<Expression> args_;
};

struct Constant {  // declarations like `let var_name := ...`
    std::string name_;
    Expression value_;
};
struct Function {  // declarations like `let var_name := ... where\n ...`
    std::string name_;
    std::vector<std::string> parameters_;
    Expression value_;
    std::unique_ptr<Module> body_ = nullptr;
};

using Declaration = std::variant<Constant, Function, Module>;

struct Module {
    std::string name_ = "";
    Imports imports_;
    std::vector<Declaration> declarations_;
    std::unique_ptr<Module> parent_ = nullptr;
};

class Parser {
public:
    Parser(Tokenizer& tokenizer);

    Module ParseModule();
    void ParseImport(Module& module);
    Declaration ParseLet();
    Module ParseSubmodule();

private:
    Token CurrentToken() const;
    TokenType CurrentTokenType() const;
    std::string CurrentTokenLexeme() const;
    void Expect(const Token& token);
    void ExpectType(TokenType type);

    const std::string ParseName();
    std::set<std::string> ParseImportFunctions();

    Expression ParseExpression();
    Expression ParseAddSub(Operator parent_operator);
    Expression ParseMulDiv(Operator parent_operator);
    Expression ParsePow(Operator parent_operator);
    Expression ParseAtom();

    Tokenizer& tokenizer_;
};
