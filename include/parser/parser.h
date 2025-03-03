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

class ParserError : public std::runtime_error {
public:
    ParserError(std::pair<size_t, size_t> coords, const std::string& msg);
};

// Forward declarations
struct Module;
struct BinaryOperation;
struct FunctionCall;
struct Function;

// TODO(helloclock): horrible implementation, fix asap then write docs
struct Imports {
    void AddImport(const std::string& module_name, const std::string& alias, std::set<std::string> functions);

    std::map<std::string, std::pair<std::string, std::set<std::string>>> modules_map_;
};

/**
 * @struct Variable
 * @brief Stores information of a variable token.
 */
struct Variable {
    std::string name_;
};

/**
 * @struct Number
 * @brief Stores information of an integer token.
 */
struct Number {
    int value_;
};

/**
 * @struct Float
 * @brief Stores information of a floating point number token.
 */
struct Float {
    double value_;
};

/**
 * @brief Alias for all the parts of an expression (sequence of atoms and operators).
 */
using Expression = std::variant<BinaryOperation, FunctionCall, Variable, Number, Float>;

/**
 * @enum class Operator
 * @brief Stores all possible mathematical operators.
 */
enum class Operator { ADD, SUB, MUL, DIV, POW, ROOT };

/**
 * @struct BinaryOperation
 * @brief Stores LHS, operator and RHS of the expression. For the sake of formatting (bracket placement to be specific)
 * also contains context for parent binary operation.
 */
struct BinaryOperation {
    std::unique_ptr<Expression> lhs_;
    Operator op_;
    std::unique_ptr<Expression> rhs_;
    Operator parent_operator_ = Operator::ROOT;
};

/**
 * @struct FunctionCall
 * @brief Stores information on function call and its arguments.
 */
struct FunctionCall {
    std::string name_;
    std::vector<Expression> args_;
};

/**
 * @struct Constant
 * @brief Stores information of declarations like `let var_name := `.
 */
struct Constant {  // declarations like `let var_name := ...`
    std::string name_;
    Expression value_;
};

/**
 * @struct Function
 * @brief Stores information of declarations like `let func_name(arg1, ..., argn) := expression` (possibly with a
 * `where` block).
 */
struct Function {  // declarations like `let var_name := ... where\n ...`
    std::string name_;
    std::vector<std::string> parameters_;
    Expression value_;
    std::unique_ptr<Module> body_ = nullptr;
};

/**
 * @brief Alias for all possible types of declarations in the language.
 */
using Declaration = std::variant<Constant, Function, Module>;

/**
 * @struct Module
 * @brief Represents a module of a language, which is a name (unless the module is source file), a (possibly empty) list
 * of imports, a (possibly empty) list of declarations.
 */
struct Module {
    std::string name_ = "";
    Imports imports_;
    std::vector<Declaration> declarations_;
};

/**
 * @class Parser
 * @brief Represents a parser of a source file, depends on `Tokenizer` class.
 *
 * The class parses the source file in a recursive manner with the help of `Tokenizer` class.
 */
class Parser {
public:
    /**
     * @brief Constructs `Parser` entity using a reference to tokenizer.
     */
    Parser(Tokenizer& tokenizer);

    /**
     * @brief Parses a module entity. As the provided file is technically a module too, this function gets called to
     * start parsing of the entire file.
     */
    Module ParseModule();

    /**
     * @brief Parses an import statement line and adds it to `module`.
     */
    void ParseImport(Module& module);

    /**
     * @brief Parses a let-declaration (be it constant or function).
     */
    Declaration ParseLet();

    /**
     * @brief Parses a submodule by parsing its header and then recursively calling `ParseModule`.
     */
    Module ParseSubmodule();

private:
    /**
     * @brief Helper function for getting current token.
     */
    Token CurrentToken() const;

    /**
     * @brief Helper function for getting type of the current token.
     */
    TokenType CurrentTokenType() const;

    /**
     * @brief Helper function for getting lexeme of the current token.
     */
    std::string CurrentTokenLexeme() const;

    /**
     * @brief Helper function that checks whether type of the current token equals to `type`.
     *
     * @throws Throws an error if `CurrentTokenType()` and `type` don't match.
     */
    void ExpectType(TokenType type);

    /**
     * @brief Parses a name (perhaps "indented", like `module.submodule.entity`).
     */
    const std::string ParseName();

    /**
     * @brief Parses functions getting imported in an import statement.
     */
    std::set<std::string> ParseImportFunctions();

    /*
     * @brief Parses an expression.
     */
    Expression ParseExpression();

    // Next several functions parse binary expression of the given priority.

    Expression ParseAddSub(Operator parent_operator);
    Expression ParseMulDiv(Operator parent_operator);
    Expression ParsePow(Operator parent_operator);
    Expression ParseAtom();

    Tokenizer& tokenizer_;
};
