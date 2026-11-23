#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    KEYWORD_SELECT,
    KEYWORD_FROM,
    KEYWORD_WHERE,
    KEYWORD_AND,
    KEYWORD_OR,

    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    EQUAL,            // =
    NOT_EQUAL,        // <> o !=
    LESS_THAN,        // <
    GREATER_THAN,     // >
    LESS_EQUAL,       // <=
    GREATER_EQUAL,    // >=

    COMMA,            // ,
    SEMICOLON,        // ;
    STAR,             // *

    TOKEN_EOF,        // Fin de entrada
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::string value;
    int line;
    int column;
};

enum class Severity {
  WARNING,
  ERROR
};

struct Diagnostic {
  Severity severity;
  std::string message;
  int line;
  int column;
};

std::string tokenTypeToString(TokenType type);
std::string severityToString(Severity severity);

#endif // TOKEN_H
