#include "parser.h"

namespace ll1 {

Terminal terminalFromToken(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_SELECT:  return Terminal::SELECT;
        case TokenType::KEYWORD_FROM:    return Terminal::FROM;
        case TokenType::KEYWORD_WHERE:   return Terminal::WHERE;
        case TokenType::KEYWORD_AND:     return Terminal::AND;
        case TokenType::KEYWORD_OR:      return Terminal::OR;
        case TokenType::IDENTIFIER:      return Terminal::ID;
        case TokenType::INTEGER_LITERAL: return Terminal::INT;
        case TokenType::FLOAT_LITERAL:   return Terminal::FLOAT;
        case TokenType::STRING_LITERAL:  return Terminal::STRING;
        case TokenType::EQUAL:           return Terminal::EQ;
        case TokenType::NOT_EQUAL:       return Terminal::NEQ;
        case TokenType::LESS_THAN:       return Terminal::LT;
        case TokenType::GREATER_THAN:    return Terminal::GT;
        case TokenType::LESS_EQUAL:      return Terminal::LE;
        case TokenType::GREATER_EQUAL:   return Terminal::GE;
        case TokenType::COMMA:           return Terminal::COMMA;
        case TokenType::SEMICOLON:       return Terminal::SEMI;
        case TokenType::STAR:            return Terminal::STAR;
        case TokenType::TOKEN_EOF:       return Terminal::END;
    }
    return Terminal::END;
}

namespace {

// Como se muestra el token encontrado en los mensajes de error.
std::string displayToken(const Token& tok) {
    if (tok.type == TokenType::STRING_LITERAL) return "'" + tok.value + "'";
    return tok.lexeme;
}

SyntaxError makeError(const Token& look, bool atEnd, const std::vector<Terminal>& expected) {
    SyntaxError e;
    e.line = look.line;
    e.column = look.column;
    e.found = atEnd ? "fin de entrada" : displayToken(look);
    for (auto t : expected) e.expected.push_back(describe(t));

    e.message = "Error sintactico en " + std::to_string(e.line) + ":" + std::to_string(e.column) +
                ": se encontro " + (atEnd ? e.found : "'" + e.found + "'") + " pero se esperaba ";
    for (size_t i = 0; i < e.expected.size(); ++i) {
        if (i > 0) e.message += (i + 1 == e.expected.size()) ? " o " : ", ";
        e.message += e.expected[i];
    }
    return e;
}

} // namespace

ParseResult parseTokens(const std::vector<Token>& tokens, const ParseTable& table,
                        const StepObserver& observer) {
    ParseResult result;
    const auto& prods = productions();

    // Si el vector no trae TOKEN_EOF (o esta vacio), se usa uno sintetico tras el ultimo token conocido.
    Token syntheticEnd{TokenType::TOKEN_EOF, "", "", 1, 1};
    if (!tokens.empty()) {
        syntheticEnd.line = tokens.back().line;
        syntheticEnd.column = tokens.back().column + static_cast<int>(tokens.back().lexeme.size());
    }

    // Pila sintactica explicita: '$' en el fondo, simbolo inicial encima.
    std::vector<Symbol> stack;
    stack.push_back(Symbol::T(Terminal::END));
    stack.push_back(Symbol::N(startSymbol()));

    size_t pos = 0;
    int step = 0;
    auto notify = [&](const std::string& action) {
        if (observer) observer(StepEvent{++step, stack, tokens, pos, action});
    };

    while (true) {
        const bool pastEnd = pos >= tokens.size();
        const Token& look = pastEnd ? syntheticEnd : tokens[pos];
        const Terminal a = pastEnd ? Terminal::END : terminalFromToken(look.type);
        const bool atEnd = (a == Terminal::END);
        const Symbol top = stack.back();

        if (top.isTerminal) {
            if (top.terminal() == a) {
                if (a == Terminal::END) {              // '$' contra '$': aceptacion
                    notify("ACEPTAR");
                    result.accepted = true;
                    return result;
                }
                notify("match " + toString(top));
                stack.pop_back();
                ++pos;
            } else {                                   // terminal esperado distinto del token actual
                notify("ERROR: se esperaba " + describe(top.terminal()));
                result.error = makeError(look, atEnd, {top.terminal()});
                return result;
            }
        } else {
            const int p = table.lookup(top.nonTerminal(), a);
            if (p < 0) {                               // celda vacia en la tabla
                notify("ERROR: M[" + toString(top.nonTerminal()) + ", " + toString(a) + "] vacia");
                result.error = makeError(look, atEnd, table.expected(top.nonTerminal()));
                return result;
            }
            notify(toString(prods[p]));
            result.productions.push_back(p + 1);
            stack.pop_back();                          // expansion: se apila el lado derecho al reves
            const auto& rhs = prods[p].rhs;
            for (auto it = rhs.rbegin(); it != rhs.rend(); ++it) stack.push_back(*it);
        }
    }
}

} // namespace ll1
