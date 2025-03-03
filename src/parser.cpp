#include <parser/parser.h>
#include <parser/tokenizer.h>

ParserError::ParserError(std::pair<size_t, size_t> coords, const std::string& msg)
    : std::runtime_error("[" + std::to_string(coords.first) + ":" + std::to_string(coords.second - 1) + "] " + msg) {
}

const std::unordered_map<Operator, std::string> kOperatorRepr = {
    {Operator::ADD, "+"}, {Operator::SUB, "-"}, {Operator::MUL, "*"}, {Operator::DIV, "/"}, {Operator::POW, "^"}};

void Imports::AddImport(const std::string& module_name, const std::string& alias = "",
                        std::set<std::string> functions = {}) {
    if (modules_map_.count(module_name) == 0) {
        modules_map_[module_name] = {alias, functions};
    } else {
        modules_map_[module_name].second.insert(functions.begin(), functions.end());
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
                throw ParserError(tokenizer_.GetCoords(),
                                  "Unexpected token encountered: `" + CurrentTokenLexeme() + "`.");
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

// TODO(helloclock): move this and other maps to separate file
std::unordered_map<TokenType, std::string> kTokenNames = {{TokenType::IMPORT, "import"},
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
                                                          {TokenType::NUMBER, "integer"},
                                                          {TokenType::FLOAT, "float"}};

void Parser::ExpectType(TokenType type) {
    if (CurrentToken().GetType() != type) {
        throw ParserError(tokenizer_.GetCoords(), "Unexpected token encountered: expected " + kTokenNames[type] +
                                                      ", got " + CurrentTokenLexeme() + ".");
    }
}

void Parser::ParseImport(Module& module) {
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
        tokenizer_.ReadToken();
    }
    tokenizer_.ReadToken();
    module.imports_.AddImport(module_name, alias, functions);
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
    return parameters.empty() ? Declaration(Constant{name, std::move(value)})
                              : Declaration(Function{name, parameters, std::move(value), std::move(body)});
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
                          "Expected an indent after substructure declaration that started at line " +
                              std::to_string(start_line) + ".");
    }
    tokenizer_.ReadToken();
    Module submodule = ParseModule();
    if (CurrentTokenType() != TokenType::DEDENT && CurrentTokenType() != TokenType::FILE_END) {
        throw ParserError(tokenizer_.GetCoords(), "Expected a dedent after a substructure body that started at line " +
                                                      std::to_string(start_line) + ".");
    }
    tokenizer_.ReadToken();
    submodule.name_ = submodule_name;
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

const std::unordered_map<Operator, int> kOperatorPrecedence = {
    {Operator::ADD, 1}, {Operator::SUB, 1}, {Operator::MUL, 2}, {Operator::DIV, 2}, {Operator::POW, 3}};

Expression Parser::ParseExpression() {
    return ParseAddSub(Operator::ROOT);
}

Expression Parser::ParseAddSub(Operator parent_operator) {
    auto lhs = ParseMulDiv(parent_operator);
    while (CurrentTokenType() == TokenType::ADD || CurrentTokenType() == TokenType::SUB) {
        auto op = CurrentTokenType() == TokenType::ADD ? Operator::ADD : Operator::SUB;
        tokenizer_.ReadToken();
        auto rhs = ParseMulDiv(op);
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), op,
                              std::make_unique<Expression>(std::move(rhs)), parent_operator};
    }
    return lhs;
}

Expression Parser::ParseMulDiv(Operator parent_operator) {
    auto lhs = ParsePow(parent_operator);
    while (CurrentTokenType() == TokenType::MUL || CurrentTokenType() == TokenType::DIV) {
        auto op = CurrentTokenType() == TokenType::MUL ? Operator::MUL : Operator::DIV;
        tokenizer_.ReadToken();
        auto rhs = ParsePow(op);
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), op,
                              std::make_unique<Expression>(std::move(rhs)), parent_operator};
    }
    return lhs;
}

Expression Parser::ParsePow(Operator parent_operator) {
    auto lhs = ParseAtom();
    while (CurrentTokenType() == TokenType::POW) {
        tokenizer_.ReadToken();
        auto rhs = ParseAtom();
        lhs = BinaryOperation{std::make_unique<Expression>(std::move(lhs)), Operator::POW,
                              std::make_unique<Expression>(std::move(rhs)), parent_operator};
    }
    return lhs;
}

Expression Parser::ParseAtom() {
    if (CurrentTokenType() == TokenType::IDENTIFIER) {
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
    } else if (CurrentTokenType() == TokenType::NUMBER) {
        int value = std::stoi(CurrentTokenLexeme());
        tokenizer_.ReadToken();
        return Number{value};
    } else if (CurrentTokenType() == TokenType::FLOAT) {
        float value = std::stof(CurrentTokenLexeme());
        tokenizer_.ReadToken();
        return Float{value};
    } else if (CurrentTokenType() == TokenType::L_BRACKET) {
        tokenizer_.ReadToken();
        auto expr = ParseExpression();
        ExpectType(TokenType::R_BRACKET);
        tokenizer_.ReadToken();
        return expr;
    } else {
        throw ParserError(tokenizer_.GetCoords(),
                          "Unexpected token in expression encountered: got `" + CurrentTokenLexeme() +
                              "`, expected an identifier, a number, a bracket enclosed expression.");
    }
}
