#include <parser/tokenizer.h>

TokenizerError::TokenizerError(const std::string &msg) : std::runtime_error(msg) {
}

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
    throw std::logic_error(
        "Trying to access a lexeme of a non-identifier-like token. This is most likely an error on program's side.\n");
}

Tokenizer::Tokenizer(std::istream *ptr, size_t spaces_per_tab) : in_(ptr), spaces_per_tab_(spaces_per_tab) {
}

void Tokenizer::ReadToken(TokenType expected) {
    if (dedents_ > 0) {
        --dedents_;
        current_token_ = Token(TokenType::DEDENT);
        return;
    }
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
            if (in_->peek() == '\t') {
                new_indent += spaces_per_tab_;
            } else {
                ++new_indent;
            }
            in_->get();
        }
        if (in_->peek() == '\n') {
            in_->get();
            return;
        }
        if (new_indent > current_indent_spaces_) {
            if (new_indent >= 2 && new_indent - 2 >= current_indent_spaces_ && substruct_started_) {
                indents_.push(new_indent - current_indent_spaces_);
                current_indent_spaces_ = new_indent;
                ++indentation_level_;
                current_token_ = Token(TokenType::INDENT);
                return;
            } else {
                throw TokenizerError("Encountered an indent greater than the indent of the block.\n");
            }
        } else if (new_indent < current_indent_spaces_) {
            if (indentation_level_ != 0) {
                size_t space_diff = current_indent_spaces_ - new_indent;
                while (new_indent < current_indent_spaces_) {
                    current_indent_spaces_ -= indents_.top();
                    indents_.pop();
                    ++dedents_;
                }
                if (new_indent != current_indent_spaces_) {
                    throw TokenizerError("Unexpected indentation encountered.\n");
                }
                --dedents_;
                --indentation_level_;
                current_token_ = Token(TokenType::DEDENT);
                return;
            } else {
                throw TokenizerError("Encountered more dedents than there were indents prior.\n");
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
            throw TokenizerError("Unknown symbol encountered while tokenizing.\n");
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
        throw TokenizerError("Unknown symbol encountered while tokenizing.\n");
    }
    if (expected != TokenType::NONE && current_token_.GetType() != expected) {
        throw TokenizerError("Unexpected token encountered.\n");
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

std::unordered_map<std::string, Token> kStringToToken = {{"import", Token(TokenType::IMPORT, "import")},
                                                         {"let", Token(TokenType::LET, "let")},
                                                         {"as", Token(TokenType::AS, "as")},
                                                         {"where", Token(TokenType::WHERE, "where")},
                                                         {"module", Token(TokenType::MODULE, "module")}};

void Tokenizer::ReadNumber() {
    std::string token_string;
    while (std::isdigit(in_->peek())) {
        token_string += in_->get();
    }
    bool is_float = false;
    if (in_->peek() == '.') {
        is_float = true;
        token_string += in_->get();
        while (std::isdigit(in_->peek())) {
            token_string += in_->get();
        }
    }
    if (std::isalpha(in_->peek())) {
        throw TokenizerError("Encountered a token starting with a number that is not a number itself.\n");
    }
    current_token_ = is_float ? Token(TokenType::FLOAT, token_string) : Token(TokenType::NUMBER, token_string);
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
