#include <parser/constants.h>
#include <parser/tokenizer.h>

TokenizerError::TokenizerError(std::pair<size_t, size_t> coords,
                               const std::string &msg)
    : std::runtime_error("[" + std::to_string(coords.first) + ":" +
                         std::to_string(coords.second - 1) + "] " + msg) {
}

Token::Token() : type_(TokenType::EOL) {
}

Token::Token(TokenType type) : type_(type) {
}

Token::Token(TokenType type, std::string lexeme)
    : type_(type), lexeme_(lexeme) {
}

TokenType Token::GetType() const {
    return type_;
}

const std::string &Token::GetLexeme() const {
    if (lexeme_.has_value()) {
        return lexeme_.value();
    }
    throw std::logic_error(
        "Trying to access a lexeme of a non-identifier-like token. This is "
        "most likely an error on program's side.");
}

Tokenizer::Tokenizer(std::istream *ptr, size_t spaces_per_tab)
    : in_(ptr), spaces_per_tab_(spaces_per_tab) {
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
        StreamRead();
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
            StreamRead();
        }
        if (in_->peek() == '\n') {
            StreamRead();
            return;
        }
        if (new_indent > current_indent_spaces_) {
            if (new_indent >= 2 && new_indent - 2 >= current_indent_spaces_ &&
                substruct_started_) {
                indents_.push(new_indent - current_indent_spaces_);
                current_indent_spaces_ = new_indent;
                ++indentation_level_;
                current_token_ = Token(TokenType::INDENT);
                return;
            } else {
                throw TokenizerError(GetCoords(),
                                     "Encountered an indent greater than the "
                                     "indent of the block.");
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
                    throw TokenizerError(GetCoords(),
                                         "Unexpected indentation encountered.");
                }
                --dedents_;
                --indentation_level_;
                current_token_ = Token(TokenType::DEDENT);
                return;
            } else {
                throw TokenizerError(
                    GetCoords(),
                    "Encountered more dedents than there were indents prior.");
            }
        }
        substruct_started_ = false;
    } else {
        // processing useless whitespace characters
        while (std::isspace(in_->peek()) && in_->peek() != '\n') {
            StreamRead();
        }
    }
    std::string token_string;
    char next = in_->peek();
    if (std::isdigit(next)) {
        ReadNumber();
    } else if (std::isalpha(next) || next == '_') {
        ReadWord();
    } else if (next == '.') {
        StreamRead();
        current_token_ = Token(TokenType::DOT);
    } else if (next == ',') {
        StreamRead();
        current_token_ = Token(TokenType::COMMA);
    } else if (next == ':') {
        StreamRead();
        if (StreamRead() == '=') {
            current_token_ = Token(TokenType::ASSIGN);
        } else {
            throw TokenizerError(GetCoords(),
                                 "Unknown symbol encountered while tokenizing. "
                                 "Maybe you meant `:=`?");
        }
    } else if (next == '\n') {
        StreamRead();
        current_token_ = Token(TokenType::EOL);
    } else if (next == '(') {
        StreamRead();
        current_token_ = Token(TokenType::L_BRACKET);
    } else if (next == ')') {
        StreamRead();
        current_token_ = Token(TokenType::R_BRACKET);
    } else if (next == '+') {
        StreamRead();
        current_token_ = Token(TokenType::ADD);
    } else if (next == '-') {
        StreamRead();
        current_token_ = Token(TokenType::SUB);
    } else if (next == '/') {
        StreamRead();
        current_token_ = Token(TokenType::DIV);
    } else if (next == '*') {
        StreamRead();
        current_token_ = Token(TokenType::MUL);
    } else if (next == '^') {
        StreamRead();
        current_token_ = Token(TokenType::POW);
    } else if (next == -1) {
    } else {
        throw TokenizerError(
            GetCoords(), "Unknown symbol encountered while tokenizing: `" +
                             std::string(1, static_cast<char>(next)) + "`.");
    }
    if (expected != TokenType::NONE && current_token_.GetType() != expected) {
        throw TokenizerError(GetCoords(),
                             "Unexpected token encountered: expected " +
                                 kTokenName.at(expected) + ", got " +
                                 kTokenName.at(current_token_.GetType()) + ".");
    }
}

Token Tokenizer::GetToken() const {
    return current_token_;
}

std::pair<size_t, size_t> Tokenizer::GetCoords() const {
    return {line_, column_};
}

bool Token::operator==(const Token &other) const {
    return type_ == other.type_ && lexeme_ == other.lexeme_;
}

bool Token::operator!=(const Token &other) const {
    return !(*this == other);
}

char Tokenizer::StreamRead() {
    char result = in_->get();
    if (result == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return result;
}

void Tokenizer::ReadNumber() {
    std::string token_string;
    while (std::isdigit(in_->peek())) {
        token_string += StreamRead();
    }
    bool is_float = false;
    if (in_->peek() == '.') {
        is_float = true;
        token_string += StreamRead();
        while (std::isdigit(in_->peek())) {
            token_string += StreamRead();
        }
    }
    if (std::isalpha(in_->peek())) {
        throw TokenizerError(GetCoords(),
                             "Encountered a token starting with a number that "
                             "is not a number itself: `" +
                                 token_string + "` and on.");
    }
    current_token_ = is_float ? Token(TokenType::FLOAT, token_string)
                              : Token(TokenType::INTEGER, token_string);
}

void Tokenizer::ReadWord() {
    std::string token_string;
    while (std::isalnum(in_->peek()) || in_->peek() == '_') {
        token_string += StreamRead();
    }
    if (kStringToToken.count(token_string) != 0) {
        current_token_ = kStringToToken.at(token_string);
    } else {
        current_token_ = Token(TokenType::IDENTIFIER, token_string);
    }
    if (current_token_.GetType() == TokenType::WHERE) {
        substruct_started_ = true;
    }
}
