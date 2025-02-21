#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#include "tokenizer.h"

class Module;
struct BinaryOperation;
struct FunctionCall;
struct Function;

class Imports {
public:
    void AddImport(const std::string& module_name, const std::string& alias,
                   std::set<std::string> functions);

    void Print() const;

private:
    std::map<std::string, std::pair<std::string, std::set<std::string>>> modules_map_;
};

struct Variable {
    std::string name_;

    void Print() const;
};
struct Number {
    int value_;

    void Print() const;
};
struct Float {
    double value_;

    void Print() const;
};
using Expression = std::variant<BinaryOperation, FunctionCall, Variable, Number, Float>;
enum class Operator { ADD, SUB, MUL, DIV, POW };
struct BinaryOperation {
    std::unique_ptr<Expression> lhs_;
    Operator op_;
    std::unique_ptr<Expression> rhs_;

    void Print() const;
};
struct FunctionCall {
    std::string name_;
    std::vector<Expression> args_;

    void Print() const;
};

struct Constant {  // declarations like `let var_name := ...`
    void Print() const;

    std::string name_;
    Expression value_;
};
struct Function {  // declarations like `let var_name := ... where\n ...`
    void Print() const;

    std::string name_;
    std::vector<std::string> parameters_;
    Expression value_;
    std::unique_ptr<Module> body_ = nullptr;
};

using Declaration = std::variant<Constant, Function, Module>;

struct Module {
    void Print() const;

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
    Expression ParseAddSub();
    Expression ParseMulDiv();
    Expression ParsePow();
    Expression ParseAtom();

    Tokenizer& tokenizer_;
};

struct DeclarationPrintVisitor {
    void operator()(const Constant& c) {
        c.Print();
    }

    void operator()(const Function& f) {
        f.Print();
    }

    void operator()(const Module& m) {
        m.Print();
    }
};

// using Expression = std::variant<BinaryOperation, FunctionCall, Variable, Number, Float>;
struct ExpressionPrintVisitor {
    void operator()(const BinaryOperation& op) {
        op.Print();
    }

    void operator()(const FunctionCall& call) {
        call.Print();
    }

    void operator()(const Variable& var) {
        var.Print();
    }

    void operator()(const Number& n) {
        n.Print();
    }

    void operator()(const Float& f) {
        f.Print();
    }
};
