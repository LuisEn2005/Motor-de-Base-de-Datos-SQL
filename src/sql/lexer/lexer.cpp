#include "lexer.h"
#include <cctype>
#include <cstdio>
#include <utility>

namespace {

bool isAlphaAscii(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isDigitAscii(char c) {
    return c >= '0' && c <= '9';
}

bool isIdentStart(char c) { return isAlphaAscii(c) || c == '_'; }
bool isIdentPart(char c)  { return isAlphaAscii(c) || isDigitAscii(c) || c == '_'; }

bool isNonAscii(char c) { return static_cast<unsigned char>(c) >= 0x80; }

std::string describeChar(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    if (u < 0x20 || u == 0x7F) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\x%02X", u);
        return buf;
    }
    return std::string(1, c);
}

} // namespace

Lexer::Lexer(std::string source) : src(std::move(source)) {
    keywords = {
        {"SELECT", TokenType::KEYWORD_SELECT},
        {"FROM",   TokenType::KEYWORD_FROM},
        {"WHERE",  TokenType::KEYWORD_WHERE},
        {"AND",    TokenType::KEYWORD_AND},
        {"OR",     TokenType::KEYWORD_OR}
    };
}

bool Lexer::isAtEnd() const {
    return pos >= src.length();
}

char Lexer::peek(size_t offset) const {
    if (pos + offset >= src.length()) return '\0';
    return src[pos + offset];
}

char Lexer::advance() {
    char current = peek();
    pos++;
    if (current == '\n') {
        line++;
        column = 1;
    } else if ((static_cast<unsigned char>(current) & 0xC0) != 0x80) {
        column++;
    }
    return current;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

std::string Lexer::toUpper(const std::string& str) {
    std::string upperStr = str;
    for (char& c : upperStr) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return upperStr;
}

void Lexer::addError(const std::string& message, int atLine, int atColumn) {
    diags.push_back({Severity::ERROR, message, atLine, atColumn});
}

void Lexer::addWarning(const std::string& message, int atLine, int atColumn) {
    diags.push_back({Severity::WARNING, message, atLine, atColumn});
}

bool Lexer::hasErrors() const {
    for (const auto& d : diags) {
        if (d.severity == Severity::ERROR) return true;
    }
    return false;
}

std::optional<Token> Lexer::scanIdentifier(int startLine, int startColumn) {
    std::string lexeme;
    while (!isAtEnd() && isIdentPart(peek())) {
        lexeme += advance();
    }

    auto it = keywords.find(toUpper(lexeme));
    if (it != keywords.end()) {
        return Token{it->second, lexeme, lexeme, startLine, startColumn};
    }
    return Token{TokenType::IDENTIFIER, lexeme, lexeme, startLine, startColumn};
}

std::optional<Token> Lexer::scanNumber(int startLine, int startColumn) {
    std::string lexeme;
    bool negative = false;

    if (peek() == '-') {
        negative = true;
        lexeme += advance();
    }

    std::string intPart, fracPart;
    bool hasDot = false;
    int dots = 0;

    while (!isAtEnd() && (isDigitAscii(peek()) || peek() == '.')) {
        char c = advance();
        lexeme += c;
        if (c == '.') {
            dots++;
            hasDot = true;
        } else if (!hasDot) {
            intPart += c;
        } else {
            fracPart += c;
        }
    }

    bool trailingLetters = false;
    while (!isAtEnd() && isIdentPart(peek())) {
        trailingLetters = true;
        lexeme += advance();
    }

    if (dots > 1) {
        addError("Number wrongly done '" + lexeme + "': more than one decimal dot",
                 startLine, startColumn);
        return std::nullopt;
    }
    if (trailingLetters) {
        addError("Token wrongly done '" + lexeme +
                 "': a identifier cannot start by a digit",
                 startLine, startColumn);
        return std::nullopt;
    }

    std::string value = negative ? "-" : "";
    value += intPart.empty() ? "0" : intPart;
    if (hasDot) {
        value += ".";
        value += fracPart.empty() ? "0" : fracPart;
        if (intPart.empty() || fracPart.empty()) {
            addWarning("Literal decimal no conventional '" + lexeme +
                       "', it is interpreted as '" + value + "'",
                       startLine, startColumn);
        }
    }

    TokenType type = hasDot ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL;
    return Token{type, lexeme, value, startLine, startColumn};
}

std::optional<Token> Lexer::scanString(int startLine, int startColumn) {
    advance();

    std::string raw, value;
    bool closed = false;

    while (!isAtEnd()) {
        char c = peek();

        if (c == '\n' || c == '\r') break;

        if (c == '\'') {
            advance();
            closed = true;
            break;
        }

        if (c == '\\') {
            char next = peek(1);
            if (next == '\0' || next == '\n' || next == '\r') {
                raw += advance();
                break;
            }
            int escLine = line, escColumn = column;
            advance();
            char esc = advance();
            raw += '\\';
            raw += esc;
            if (esc == '\'' || esc == '\\') {
                value += esc;
            } else {
                addWarning("Escape sequence unknown '\\" + describeChar(esc) +
                           "' on text string", escLine, escColumn);
                value += '\\';
                value += esc;
            }
            continue;
        }

        raw += c;
        value += c;
        advance();
    }

    if (!closed) {
        addError("Text string not closed", startLine, startColumn);
        return std::nullopt;
    }
    return Token{TokenType::STRING_LITERAL, raw, value, startLine, startColumn};
}

std::optional<Token> Lexer::scanOperator(int startLine, int startColumn) {
    char c = advance();
    switch (c) {
        case ',': return Token{TokenType::COMMA,     ",", ",", startLine, startColumn};
        case '*': return Token{TokenType::STAR,      "*", "*", startLine, startColumn};
        case ';': return Token{TokenType::SEMICOLON, ";", ";", startLine, startColumn};
        case '=': return Token{TokenType::EQUAL,     "=", "=", startLine, startColumn};
        case '>':
            if (peek() == '=') {
                advance();
                return Token{TokenType::GREATER_EQUAL, ">=", ">=", startLine, startColumn};
            }
            return Token{TokenType::GREATER_THAN, ">", ">", startLine, startColumn};
        case '<':
            if (peek() == '=') {
                advance();
                return Token{TokenType::LESS_EQUAL, "<=", "<=", startLine, startColumn};
            }
            if (peek() == '>') {
                advance();
                return Token{TokenType::NOT_EQUAL, "<>", "<>", startLine, startColumn};
            }
            return Token{TokenType::LESS_THAN, "<", "<", startLine, startColumn};
        case '!':
            if (peek() == '=') {
                advance();
                return Token{TokenType::NOT_EQUAL, "!=", "!=", startLine, startColumn};
            }
            addError("Character not valid '!' (only admitted '!=')", startLine, startColumn);
            return std::nullopt;
        default:
            break;
    }
    return std::nullopt;
}

std::optional<Token> Lexer::scanInvalid(int startLine, int startColumn) {
    if (isNonAscii(peek())) {
        std::string bytes;
        while (!isAtEnd() && isNonAscii(peek())) {
            bytes += advance();
        }
        addError("Character not ASCII not allowed '" + bytes + "'", startLine, startColumn);
        return std::nullopt;
    }

    char c = advance();
    if (c == '-') {
        addError("'-' only valid if attached to a number", startLine, startColumn);
    } else {
        addError("Character not valid '" + describeChar(c) + "'", startLine, startColumn);
    }
    return std::nullopt;
}

std::optional<Token> Lexer::scanToken() {
    int startLine = line;
    int startColumn = column;
    char c = peek();

    if (isIdentStart(c)) return scanIdentifier(startLine, startColumn);
    if (isDigitAscii(c)) return scanNumber(startLine, startColumn);
    if (c == '.' && isDigitAscii(peek(1))) return scanNumber(startLine, startColumn);
    if (c == '-' && (isDigitAscii(peek(1)) || (peek(1) == '.' && isDigitAscii(peek(2))))) {
        return scanNumber(startLine, startColumn);
    }
    if (c == '\'') return scanString(startLine, startColumn);

    switch (c) {
        case ',': case ';': case '*': case '=': case '<': case '>': case '!':
            return scanOperator(startLine, startColumn);
        default:
            return scanInvalid(startLine, startColumn);
    }
}

Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();
        if (isAtEnd()) {
            return Token{TokenType::TOKEN_EOF, "EOF", "EOF", line, column};
        }
        std::optional<Token> token = scanToken();
        if (token) return *token;
    }
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    Token t;
    do {
        t = nextToken();
        tokens.push_back(t);
    } while (t.type != TokenType::TOKEN_EOF);
    return tokens;
}
