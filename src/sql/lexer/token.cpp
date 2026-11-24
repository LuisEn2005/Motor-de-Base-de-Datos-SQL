#include "token.h" 

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_SELECT: return "KEYWORD_SELECT";
        case TokenType::KEYWORD_FROM:   return "KEYWORD_FROM";
        case TokenType::KEYWORD_WHERE:  return "KEYWORD_WHERE";
        case TokenType::KEYWORD_AND:    return "KEYWORD_AND";
        case TokenType::KEYWORD_OR:     return "KEYWORD_OR";
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL:   return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::EQUAL:          return "EQUAL";
        case TokenType::NOT_EQUAL:      return "NOT_EQUAL";
        case TokenType::LESS_THAN:      return "LESS_THAN";
        case TokenType::GREATER_THAN:   return "GREATER_THAN";
        case TokenType::LESS_EQUAL:     return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL:  return "GREATER_EQUAL";
        case TokenType::COMMA:          return "COMMA";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::STAR:           return "STAR";
        case TokenType::TOKEN_EOF:      return "EOF";
    }
    return "UNKNOWN";
}

std::string severityToString(Severity severity) {
  switch (severity) {
    case Severity::WARNING: return "WARNING";
    case Severity::ERROR:   return "ERROR";
  }
  return "UNKNOWN";
}
