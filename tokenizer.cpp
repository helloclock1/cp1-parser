// TODO(helloclock): fix tokenizer entering an infinite loop on empty lines (fixed itself somehow
// lol)
#include "tokenizer.h"

Token::Token() : type_(TokenType::EOL), lexeme_("\n") {
}

Token::Token(TokenType type, std::string lexeme) : type_(type), lexeme_(lexeme) {
}

TokenType Token::GetType() const {
    return type_;
}

const std::string &Token::GetLexeme() const {
    return lexeme_;
}

Tokenizer::Tokenizer(std::istream *ptr) : in_(ptr) {
}

void Tokenizer::ReadToken(TokenType expected) {
    if (in_->peek() == -1) {
        std::cout << "Tokenizing done, exiting...\n";
        current_token_ = Token(TokenType::FILE_END, "");
    }
    if (current_token_.GetType() == TokenType::EOL && std::isspace(in_->peek())) {
        /* TODO(helloclock):
         * Currently only Python-like indentation is supported (i.e., indentation in the input file
         * must be fixed, in this case 2, spaces per level)
         */
        size_t new_indent = 0;
        while (std::isspace(in_->peek()) && in_->peek() != '\n') {
            in_->get();
            ++new_indent;
        }
        new_indent /= 2;
        bool dent_symbol = false;
        if (new_indent > current_indent_) {
            current_token_ = Token(TokenType::INDENT, "  ");
            dent_symbol = true;
        } else if (new_indent < current_indent_) {
            current_token_ = Token(TokenType::DEDENT, "");
            dent_symbol = true;
        }
        current_indent_ = new_indent;
        if (dent_symbol) {
            return;
        }
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
            throw std::runtime_error("bruh");
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
        throw std::runtime_error("Unknown symbol encountered.");
    }
    if (expected != TokenType::NONE && current_token_.GetType() != expected) {
        throw std::runtime_error("Unknown token encountered.");
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
        throw std::runtime_error(
            "Encountered a token starting with a number that is not a number itself.");
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
}
