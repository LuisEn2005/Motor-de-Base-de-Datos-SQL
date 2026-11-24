#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
private:
    std::string src;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    std::unordered_map<std::string, TokenType> keywords;
    std::vector<Diagnostic> diags;

    char peek(size_t offset = 0) const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();
    static std::string toUpper(const std::string& str);

    void addError(const std::string& message, int atLine, int atColumn);
    void addWarning(const std::string& message, int atLine, int atColumn);
 
    std::optional<Token> scanToken();
    std::optional<Token> scanIdentifier(int startLine, int startColumn);
    std::optional<Token> scanNumber(int startLine, int startColumn);
    std::optional<Token> scanString(int startLine, int startColumn);
    std::optional<Token> scanOperator(int startLine, int startColumn);
    std::optional<Token> scanInvalid(int startLine, int startColumn);

public:
    explicit Lexer(std::string source);

    Token nextToken();
    std::vector<Token> tokenizeAll();

    const std::vector<Diagnostic> & diagnostics() const { return diags; }
    
    bool hasErrors() const;
};

#endif // LEXER_H
