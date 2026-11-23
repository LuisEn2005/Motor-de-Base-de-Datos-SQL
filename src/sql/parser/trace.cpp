#include "trace.h"

#include <iomanip>
#include <ostream>

namespace ll1 {

std::string stackToString(const std::vector<Symbol>& stack) {
    std::string out;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {   // tope (back) primero
        if (!out.empty()) out += " ";
        out += toString(*it);
    }
    return out;
}

std::string remainingInput(const std::vector<Token>& tokens, size_t position) {
    std::string out;
    for (size_t i = position; i < tokens.size(); ++i) {
        if (tokens[i].type == TokenType::TOKEN_EOF) break;       // el '$' se agrega al final
        if (!out.empty()) out += " ";
        out += (tokens[i].type == TokenType::STRING_LITERAL) ? "'" + tokens[i].value + "'"
                                                             : tokens[i].lexeme;
    }
    if (!out.empty()) out += " ";
    return out + "$";
}

void printTraceHeader(std::ostream& os) {
    os << std::left << std::setw(6) << "Paso" << std::setw(46) << "Pila (tope a la izq.)"
       << std::setw(40) << "Entrada" << "  " << "Accion\n";
    os << std::string(120, '-') << "\n";
}

StepObserver makeTracer(std::ostream& os) {
    return [&os](const StepEvent& e) {
        os << std::left << std::setw(6) << e.step << std::setw(46) << stackToString(e.stack)
           << std::setw(40) << remainingInput(e.tokens, e.position) << "  " << e.action << "\n";
    };
}

} // namespace ll1
