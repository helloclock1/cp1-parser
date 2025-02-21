// WARNING(helloclock): for ease of debugging, formatted code is PRINTED, NOT WRITTEN TO FILE
#include "parser.h"

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include "tokenizer.h"

const std::unordered_map<Operator, std::string> kOperatorRepr = {{Operator::ADD, "+"},
                                                                 {Operator::SUB, "-"},
                                                                 {Operator::MUL, "*"},
                                                                 {Operator::DIV, "/"},
                                                                 {Operator::POW, "^"}};

void Imports::AddImport(const std::string& module_name, const std::string& alias = "",
                        std::set<std::string> functions = {}) {
    if (modules_map_.count(module_name) ==
        0) {  // No module named `module_name` has been imported yet
        modules_map_[module_name] = {alias, functions};
    } else {  // Just append functions to an already imported module
        modules_map_[module_name].second.insert(functions.begin(), functions.end());
    }
}

void BinaryOperation::Print() const {
    std::cout << "(";
    std::visit(ExpressionPrintVisitor{}, *lhs_);
    std::cout << " " << kOperatorRepr.at(op_) << " ";
    std::visit(ExpressionPrintVisitor{}, *rhs_);
    std::cout << ")";
}

void FunctionCall::Print() const {
    std::cout << name_ << "(";
    for (const Expression& expr : args_) {
        std::visit(ExpressionPrintVisitor{}, expr);
        std::cout << ", ";
    }
    std::cout << ")";
}

void Variable::Print() const {
    std::cout << name_;
}

void Number::Print() const {
    std::cout << value_;
}

void Float::Print() const {
    std::cout << value_;
}

void Constant::Print() const {
    std::cout << "let " << name_ << " := ";
    std::visit(ExpressionPrintVisitor{}, value_);
    std::cout << std::endl;
}

void Function::Print() const {
    std::cout << "let " << name_ << "(";
    for (const auto& param : parameters_) {
        std::cout << param << ", ";
    }
    std::cout << ") := ";
    std::visit(ExpressionPrintVisitor{}, value_);
    if (body_) {
        std::cout << " where" << std::endl;
        body_->Print();
    }
    std::cout << std::endl;
}

void Imports::Print() const {
    for (auto const& [name, info] : modules_map_) {
        std::cout << "import " << name << " ";
        const auto& [alias, functions] = info;
        if (name != alias) {
            std::cout << "as " << alias << " ";
        }
        if (!functions.empty()) {
            std::cout << "(";
            for (const auto& f : functions) {
                std::cout << f << ", ";
            }
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}

void Module::Print() const {
    if (!name_.empty()) {
        std::cout << "module " << name_ << " where" << std::endl;
    }
    // print imports
    imports_.Print();
    for (const Declaration& decl : declarations_) {
        // print declaration
        std::visit(DeclarationPrintVisitor{}, decl);
    }
}

Parser::Parser(Tokenizer& tokenizer) : tokenizer_(tokenizer) {
    tokenizer_.ReadToken();
}

Module Parser::ParseModule() {
    Module module;
    while (true) {
        switch (CurrentTokenType()) {
            case TokenType::IMPORT:
                ParseImport(module);
                break;
            case TokenType::LET:
                module.declarations_.push_back(ParseLet());
                break;
            case TokenType::MODULE:
                module.declarations_.push_back(ParseSubmodule());
                break;
            case TokenType::FILE_END:
            case TokenType::DEDENT:
                return module;
            case TokenType::EOL:
                tokenizer_.ReadToken();
                break;
            case TokenType::INDENT:
                tokenizer_.ReadToken();
                break;
            default:
                throw std::runtime_error("Unexpected token in module encountered.");
        }
    }
}

Token Parser::CurrentToken() const {
    return tokenizer_.GetToken();
}

TokenType Parser::CurrentTokenType() const {
    return CurrentToken().GetType();
}

std::string Parser::CurrentTokenLexeme() const {
    return CurrentToken().GetLexeme();
}

void Parser::Expect(const Token& token) {
    if (CurrentToken() != token) {
        throw std::runtime_error("Unexpected token encountered.");
    }
}

void Parser::ExpectType(TokenType type) {
    if (CurrentToken().GetType() != type) {
        throw std::runtime_error("Unexpected token encountered.");
    }
}

void Parser::ParseImport(Module& module) {
    ExpectType(TokenType::IMPORT);
    tokenizer_.ReadToken();
    Imports imports;
    std::string module_name = ParseName();
    std::string alias = module_name;
    if (CurrentToken().GetType() == TokenType::AS) {
        tokenizer_.ReadToken();
        alias = ParseName();
    }
    std::set<std::string> functions;
    if (CurrentToken().GetType() == TokenType::L_BRACKET) {
        functions = ParseImportFunctions();
        ExpectType(TokenType::R_BRACKET);
        tokenizer_.ReadToken();
    }
    tokenizer_.ReadToken();
    module.imports_.AddImport(module_name, alias, functions);
}

Declaration Parser::ParseLet() {
    ExpectType(TokenType::LET);
    tokenizer_.ReadToken();

    ExpectType(TokenType::IDENTIFIER);
    std::string name = CurrentTokenLexeme();
    tokenizer_.ReadToken();

    std::vector<std::string> parameters;
    if (CurrentTokenType() == TokenType::L_BRACKET) {
        tokenizer_.ReadToken();
        while (CurrentTokenType() != TokenType::R_BRACKET) {
            ExpectType(TokenType::IDENTIFIER);
            parameters.push_back(CurrentTokenLexeme());
            tokenizer_.ReadToken();
            if (CurrentTokenType() == TokenType::COMMA) {
                tokenizer_.ReadToken();
            }
        }
        tokenizer_.ReadToken();
    }
    ExpectType(TokenType::ASSIGN);
    tokenizer_.ReadToken();
    Expression value = ParseExpression();
    std::unique_ptr<Module> body = nullptr;
    if (CurrentTokenType() == TokenType::WHERE) {
        tokenizer_.ReadToken();
        body = std::make_unique<Module>(ParseModule());
    }
    return parameters.empty()
               ? Declaration(Constant{name, std::move(value)})
               : Declaration(Function{name, parameters, std::move(value), std::move(body)});
}

Module Parser::ParseSubmodule() {
    ExpectType(TokenType::MODULE);
    tokenizer_.ReadToken();
    ExpectType(TokenType::IDENTIFIER);
    std::string submodule_name = CurrentTokenLexeme();
    tokenizer_.ReadToken();
    ExpectType(TokenType::WHERE);
    tokenizer_.ReadToken();
    if (CurrentTokenType() == TokenType::EOL) {
        tokenizer_.ReadToken();
    }
    if (CurrentTokenType() != TokenType::INDENT) {
        throw std::runtime_error("Expected indent after submodule declaration");
    }
    tokenizer_.ReadToken();
    Module submodule = ParseModule();
    if (CurrentTokenType() != TokenType::DEDENT) {
        throw std::runtime_error("Expected dedent after submodule body");
    }
    tokenizer_.ReadToken();
    submodule.name_ = submodule_name;
    return submodule;
}

const std::string Parser::ParseName() {
    ExpectType(TokenType::IDENTIFIER);

    std::string name = CurrentTokenLexeme();
    tokenizer_.ReadToken();
    while (CurrentToken().GetType() == TokenType::DOT) {
        name.append(CurrentTokenLexeme());
        tokenizer_.ReadToken();
        ExpectType(TokenType::IDENTIFIER);
        name.append(CurrentTokenLexeme());
        tokenizer_.ReadToken();
    }
    return name;
}

std::set<std::string> Parser::ParseImportFunctions() {
    tokenizer_.ReadToken();
    ExpectType(TokenType::IDENTIFIER);
    std::set<std::string> functions = {CurrentTokenLexeme()};
    tokenizer_.ReadToken();

    while (CurrentToken().GetType() == TokenType::COMMA) {
        tokenizer_.ReadToken();
        ExpectType(TokenType::IDENTIFIER);
        functions.insert(CurrentTokenLexeme());
        tokenizer_.ReadToken();
    }
    return functions;
}

Expression Parser::ParseExpression() {
    return ParseAddSub();
}

Expression Parser::ParseAddSub() {
    auto lhs = ParseMulDiv();
    while (CurrentTokenType() == TokenType::ADD || CurrentTokenType() == TokenType::SUB) {
        auto op = CurrentTokenType() == TokenType::ADD ? Operator::ADD : Operator::SUB;
        tokenizer_.ReadToken();
        auto rhs = ParseMulDiv();
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), op,
                              std::make_unique<Expression>(std::move(rhs))};
    }
    return lhs;
}

Expression Parser::ParseMulDiv() {
    auto lhs = ParsePow();
    while (CurrentTokenType() == TokenType::MUL || CurrentTokenType() == TokenType::DIV) {
        auto op = CurrentTokenType() == TokenType::MUL ? Operator::MUL : Operator::DIV;
        tokenizer_.ReadToken();
        auto rhs = ParsePow();
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), op,
                              std::make_unique<Expression>(std::move(rhs))};
    }
    return lhs;
}

Expression Parser::ParsePow() {
    auto lhs = ParseAtom();
    while (CurrentTokenType() == TokenType::POW) {
        tokenizer_.ReadToken();
        auto rhs = ParseAtom();
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), Operator::POW,
                              std::make_unique<Expression>(std::move(rhs))};
    }
    return lhs;
}

Expression Parser::ParseAtom() {
    if (CurrentTokenType() == TokenType::IDENTIFIER) {
        std::string name = ParseName();

        if (CurrentTokenType() == TokenType::L_BRACKET) {
            tokenizer_.ReadToken();
            // Function call
            std::vector<Expression> args;
            while (CurrentTokenType() != TokenType::R_BRACKET) {
                args.push_back(ParseExpression());
                if (CurrentTokenType() == TokenType::COMMA) {
                    tokenizer_.ReadToken();
                }
            }
            tokenizer_.ReadToken();
            return FunctionCall{name, std::move(args)};
        } else {
            return Variable{name};
        }
    } else if (CurrentTokenType() == TokenType::NUMBER) {
        int value = std::stoi(CurrentTokenLexeme());
        tokenizer_.ReadToken();
        return Number{value};
    } else if (CurrentTokenType() == TokenType::L_BRACKET) {
        tokenizer_.ReadToken();
        auto expr = ParseExpression();
        ExpectType(TokenType::R_BRACKET);
        tokenizer_.ReadToken();
        return expr;
    } else {
        throw std::runtime_error("Unexpected token in expression encountered.");
    }
}
