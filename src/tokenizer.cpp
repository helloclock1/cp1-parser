#include <parser/tokenizer.h>

Token::Token() : type_(TokenType::EOL), lexeme_("\\n") {
}

Token::Token(TokenType type) : type_(type) {
}

Token::Token(TokenType type, std::string lexeme) : type_(type), lexeme_(lexeme) {
}

TokenType Token::GetType() const {
    return type_;
}

const std::string &Token::GetLexeme() const {
    if (lexeme_.has_value()) {
        return lexeme_.value();
    }
    std::cerr << "Trying to access a lexeme of a non-identifier-like token. This is most likely an "
                 "error on program's side.\n";
    exit(1);
}

Tokenizer::Tokenizer(std::istream *ptr) : in_(ptr) {
}

void Tokenizer::ReadToken(TokenType expected) {
    if (in_->peek() == -1) {
        current_token_ = Token(TokenType::FILE_END, "file_end");
        return;
    }
    while (current_token_.GetType() == TokenType::EOL && in_->peek() == '\n') {
        in_->get();
        return;
    }
    if (current_token_.GetType() == TokenType::EOL) {
        size_t new_indent = 0;
        while (std::isspace(in_->peek()) && in_->peek() != '\n') {
            in_->get();
            ++new_indent;
        }
        if (new_indent > current_indent_spaces_) {
            if (new_indent >= 2 && new_indent - 2 >= current_indent_spaces_ && substruct_started_) {
                current_indent_spaces_ = new_indent;
                indents_.push(new_indent);
                ++indentation_level_;
                current_token_ = Token(TokenType::INDENT);
                return;
            } else {
                std::cerr << "Encountered an indent greater than the indent of the block.\n";
                exit(1);
            }
        } else if (new_indent < current_indent_spaces_) {
            if (indentation_level_ != 0) {
                current_indent_spaces_ -= indents_.top();
                indents_.pop();
                --indentation_level_;
                current_token_ = Token(TokenType::DEDENT);
                return;
            } else {
                std::cerr << "Encountered more dedents than there were indents prior.\n";
                exit(1);
            }
        }
        substruct_started_ = false;
    } else {
        // processing useless whitespace characters
        while (std::isspace(in_->peek()) && in_->peek() != '\n') {
            in_->get();
        }
    }
    std::string token_string;
    if (std::isdigit(in_->peek())) {
        ReadNumber();
    } else if (std::isalpha(in_->peek()) || in_->peek() == '_') {
        ReadWord();
    } else if (in_->peek() == '.') {
        in_->get();
        current_token_ = Token(TokenType::DOT, ".");
    } else if (in_->peek() == ',') {
        in_->get();
        current_token_ = Token(TokenType::COMMA, ",");
    } else if (in_->peek() == ':') {
        in_->get();
        if (in_->get() == '=') {
            current_token_ = Token(TokenType::ASSIGN, ":=");
        } else {
            std::cerr << "Unknown symbol encountered while tokenizing.\n";
            exit(1);
        }
    } else if (in_->peek() == '\n') {
        in_->get();
        current_token_ = Token(TokenType::EOL, "\\n");
    } else if (in_->peek() == '(') {
        in_->get();
        current_token_ = Token(TokenType::L_BRACKET, "(");
    } else if (in_->peek() == ')') {
        in_->get();
        current_token_ = Token(TokenType::R_BRACKET, ")");
    } else if (in_->peek() == '+') {
        in_->get();
        current_token_ = Token(TokenType::ADD, "+");
    } else if (in_->peek() == '-') {
        in_->get();
        current_token_ = Token(TokenType::SUB, "-");
    } else if (in_->peek() == '/') {
        in_->get();
        current_token_ = Token(TokenType::DIV, "/");
    } else if (in_->peek() == '*') {
        in_->get();
        current_token_ = Token(TokenType::MUL, "*");
    } else if (in_->peek() == '^') {
        in_->get();
        current_token_ = Token(TokenType::POW, "^");
    } else if (in_->peek() == -1) {
    } else {
        std::cerr << "Unknown symbol encountered while tokenizing.\n";
        exit(1);
    }
    if (expected != TokenType::NONE && current_token_.GetType() != expected) {
        std::cerr << "Unexpected token encountered.\n";
        exit(1);
    }
}

Token Tokenizer::GetToken() const {
    return current_token_;
}

bool Token::operator==(const Token &other) const {
    return type_ == other.type_ && lexeme_ == other.lexeme_;
}

bool Token::operator!=(const Token &other) const {
    return !(*this == other);
}

std::unordered_map<std::string, Token> kStringToToken = {
    {"import", Token(TokenType::IMPORT, "import")},
    {"let", Token(TokenType::LET, "let")},
    {"as", Token(TokenType::AS, "as")},
    {"where", Token(TokenType::WHERE, "where")},
    {"module", Token(TokenType::MODULE, "module")}};

void Tokenizer::ReadNumber() {
    // TODO(helloclock): add float support
    std::string token_string;
    while (std::isdigit(in_->peek())) {
        token_string += in_->get();
    }
    if (std::isalpha(in_->peek())) {
        std::cerr << "Encountered a token starting with a number that is not a number itself.\n";
        exit(1);
    }
    current_token_ = Token(TokenType::NUMBER, token_string);
}

void Tokenizer::ReadWord() {
    std::string token_string;
    while (std::isalnum(in_->peek()) || in_->peek() == '_') {
        token_string += in_->get();
    }
    if (kStringToToken.count(token_string) != 0) {
        current_token_ = kStringToToken[token_string];
    } else {
        current_token_ = Token(TokenType::IDENTIFIER, token_string);
    }
    if (current_token_.GetType() == TokenType::WHERE) {
        substruct_started_ = true;
    }
}
