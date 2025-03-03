#include <parser/constants.h>
#include <parser/parser.h>
#include <parser/tokenizer.h>

ParserError::ParserError(std::pair<size_t, size_t> coords,
                         const std::string& msg)
    : std::runtime_error("[" + std::to_string(coords.first) + ":" +
                         std::to_string(coords.second - 1) + "] " + msg) {
}

void Imports::AddImport(Import import) {
    auto [module_name, info] = import;
    auto [alias, functions] = info;
    if (modules_map_.count(module_name) == 0) {
        modules_map_[module_name] = {alias, functions};
    } else {
        if (modules_map_[module_name].first != alias) {
            throw std::runtime_error(
                "Alias collision while importing: tried importing a module `" +
                module_name + "` with alias `" + alias +
                "` while same module " +
                (modules_map_[module_name].first == module_name
                     ? "without alias"
                     : "with alias `" + modules_map_[module_name].first + "`") +
                " has already been imported.");
        }
        if (functions.empty()) {
            modules_map_.erase(modules_map_.find(module_name));
            modules_map_[module_name].first = alias;
        } else {
            if (!modules_map_[module_name].second.empty()) {
                modules_map_[module_name].second.insert(functions.begin(),
                                                        functions.end());
            }
        }
    }
}

const std::map<std::string, std::pair<std::string, std::set<std::string>>>&
Imports::GetImports() const {
    return modules_map_;
}

Parser::Parser(Tokenizer& tokenizer) : tokenizer_(tokenizer) {
    tokenizer_.ReadToken();
}

Module Parser::ParseModule() {
    Module module;
    while (true) {
        switch (CurrentTokenType()) {
            case TokenType::IMPORT:
                module.imports_.AddImport(ParseImport());
                break;
            case TokenType::LET:
                module.declarations_.push_back(ParseLet());
                break;
            case TokenType::MODULE:
                module.declarations_.push_back(ParseSubmodule());
                break;
            case TokenType::EOL:
            case TokenType::INDENT:
                tokenizer_.ReadToken();
                break;
            case TokenType::FILE_END:
            case TokenType::DEDENT:
                return module;
            default:
                throw ParserError(tokenizer_.GetCoords(),
                                  "Unexpected token encountered: `" +
                                      CurrentTokenLexeme() + "`.");
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

void Parser::ExpectType(TokenType type) {
    if (CurrentToken().GetType() != type) {
        throw ParserError(tokenizer_.GetCoords(),
                          "Unexpected token encountered: expected " +
                              kTokenName.at(type) + ", got " +
                              CurrentTokenLexeme() + ".");
    }
}

Import Parser::ParseImport() {
    tokenizer_.ReadToken(TokenType::IDENTIFIER);
    Imports imports;
    std::string module_name = ParseName();
    std::string alias = module_name;
    if (CurrentTokenType() == TokenType::AS) {
        tokenizer_.ReadToken(TokenType::IDENTIFIER);
        alias = ParseName();
    }
    std::set<std::string> functions;
    if (CurrentTokenType() == TokenType::L_BRACKET) {
        functions = ParseImportFunctions();
        ExpectType(TokenType::R_BRACKET);
        tokenizer_.ReadToken(TokenType::EOL);
    }
    tokenizer_.ReadToken();
    return {module_name, {alias, functions}};
}

Declaration Parser::ParseLet() {
    tokenizer_.ReadToken(TokenType::IDENTIFIER);
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
    tokenizer_.ReadToken();
    return parameters.empty()
               ? Declaration(Constant{name, std::move(value)})
               : Declaration(Function{name, parameters, std::move(value),
                                      std::move(body)});
}

Module Parser::ParseSubmodule() {
    auto [start_line, start_col] = tokenizer_.GetCoords();
    tokenizer_.ReadToken(TokenType::IDENTIFIER);
    std::string submodule_name = CurrentTokenLexeme();
    tokenizer_.ReadToken(TokenType::WHERE);
    tokenizer_.ReadToken();
    if (CurrentTokenType() == TokenType::EOL) {
        tokenizer_.ReadToken();
    }
    if (CurrentTokenType() != TokenType::INDENT) {
        throw ParserError(tokenizer_.GetCoords(),
                          "Expected an indent after substructure declaration "
                          "that started at line " +
                              std::to_string(start_line) + ".");
    }
    tokenizer_.ReadToken();
    Module submodule = ParseModule();
    if (CurrentTokenType() != TokenType::DEDENT &&
        CurrentTokenType() != TokenType::FILE_END) {
        throw ParserError(tokenizer_.GetCoords(),
                          "Expected a dedent after a substructure body that "
                          "started at line " +
                              std::to_string(start_line) + ".");
    }
    submodule.name_ = submodule_name;
    tokenizer_.ReadToken();
    return submodule;
}

const std::string Parser::ParseName() {
    std::string name = CurrentTokenLexeme();
    tokenizer_.ReadToken();
    while (CurrentTokenType() == TokenType::DOT) {
        name.append(CurrentTokenLexeme());
        tokenizer_.ReadToken(TokenType::IDENTIFIER);
        name.append(CurrentTokenLexeme());
        tokenizer_.ReadToken();
    }
    return name;
}

std::set<std::string> Parser::ParseImportFunctions() {
    tokenizer_.ReadToken(TokenType::IDENTIFIER);
    std::set<std::string> functions = {CurrentTokenLexeme()};
    tokenizer_.ReadToken();

    while (CurrentTokenType() == TokenType::COMMA) {
        tokenizer_.ReadToken(TokenType::IDENTIFIER);
        functions.insert(CurrentTokenLexeme());
        tokenizer_.ReadToken();
    }
    return functions;
}

Expression Parser::ParseExpression() {
    return ParseAddSub(Operator::ROOT);
}

Expression Parser::ParseAddSub(Operator parent_operator) {
    auto lhs = ParseMulDiv(parent_operator);
    while (CurrentTokenType() == TokenType::ADD ||
           CurrentTokenType() == TokenType::SUB) {
        auto op = CurrentTokenType() == TokenType::ADD ? Operator::ADD
                                                       : Operator::SUB;
        tokenizer_.ReadToken();
        auto rhs = ParseMulDiv(op);
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), op,
                              std::make_unique<Expression>(std::move(rhs)),
                              parent_operator};
    }
    return lhs;
}

Expression Parser::ParseMulDiv(Operator parent_operator) {
    auto lhs = ParsePow(parent_operator);
    while (CurrentTokenType() == TokenType::MUL ||
           CurrentTokenType() == TokenType::DIV) {
        auto op = CurrentTokenType() == TokenType::MUL ? Operator::MUL
                                                       : Operator::DIV;
        tokenizer_.ReadToken();
        auto rhs = ParsePow(op);
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), op,
                              std::make_unique<Expression>(std::move(rhs)),
                              parent_operator};
    }
    return lhs;
}

Expression Parser::ParseUnary(Operator parent_operator) {
    if (CurrentTokenType() == TokenType::SUB) {
        tokenizer_.ReadToken();
        Expression expr = ParseUnary(parent_operator);
        return UnaryOperation{Operator::SUB,
                              std::make_unique<Expression>(std::move(expr))};
    }
    return ParseAtom();
}

Expression Parser::ParsePow(Operator parent_operator) {
    auto lhs = ParseUnary(parent_operator);
    while (CurrentTokenType() == TokenType::POW) {
        tokenizer_.ReadToken();
        auto rhs = ParseAtom();
        lhs = BinaryOperation{
            std::make_unique<Expression>(std::move(lhs)), Operator::POW,
            std::make_unique<Expression>(std::move(rhs)), parent_operator};
    }
    return lhs;
}

Expression Parser::ParseAtom() {
    TokenType ctt = CurrentTokenType();
    switch (ctt) {
        case TokenType::IDENTIFIER: {
            std::string name = ParseName();

            if (CurrentTokenType() == TokenType::L_BRACKET) {
                tokenizer_.ReadToken();
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
        }
        case TokenType::INTEGER: {
            int value = std::stoi(CurrentTokenLexeme());
            tokenizer_.ReadToken();
            return Number{value};
        }
        case TokenType::FLOAT: {
            float value = std::stof(CurrentTokenLexeme());
            tokenizer_.ReadToken();
            return Float{value};
        }
        case TokenType::L_BRACKET: {
            tokenizer_.ReadToken();
            auto expr = ParseExpression();
            ExpectType(TokenType::R_BRACKET);
            tokenizer_.ReadToken();
            return expr;
        }
        default:
            throw ParserError(
                tokenizer_.GetCoords(),
                "Unexpected token in expression encountered: got `" +
                    CurrentTokenLexeme() +
                    "`, expected an identifier, a number, a bracket "
                    "enclosed expression.");
    }
}
