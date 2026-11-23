#ifndef PARSER_H
#define PARSER_H

// Motor LL(1) no recursivo basado en pila explicita.
// Consume los tokens del lexer (Practica 4) sin modificarlos.

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "grammar.h"
#include "parse_table.h"
#include "../lexer/token.h"

namespace ll1 {

struct SyntaxError {
    int line = 0;
    int column = 0;
    std::string found;                  // token encontrado, tal como aparece en la entrada
    std::vector<std::string> expected;  // tokens que se esperaban en ese estado
    std::string message;                // mensaje completo listo para imprimir
};

struct ParseResult {
    bool accepted = false;
    std::optional<SyntaxError> error;   // presente si y solo si !accepted
    std::vector<int> productions;       // numeros de produccion aplicados, en orden (1-based)
};

// Evento que el motor emite en CADA paso del algoritmo (antes de aplicarlo).
// Es el gancho para construir la traza (paso, pila, entrada, accion) sin tocar el bucle del motor.
struct StepEvent {
    int step;                           // 1, 2, 3, ...
    const std::vector<Symbol>& stack;   // la pila; el TOPE es el ultimo elemento (back)
    const std::vector<Token>& tokens;   // tokens de entrada
    size_t position;                    // indice del token de lookahead (puede ser == tokens.size(): fin)
    std::string action;                 // "Query -> SELECT ...", "match SELECT", "ACEPTAR" o "ERROR: ..."
};
using StepObserver = std::function<void(const StepEvent&)>;

// Correspondencia TokenType (lexer) -> Terminal (gramatica).
Terminal terminalFromToken(TokenType type);

// Analiza una secuencia de tokens (idealmente terminada en TOKEN_EOF; si falta, se asume el fin).
// Nunca lanza excepciones ni accede fuera de rango: ante un error devuelve accepted = false y se detiene
// en el primer error (modo "todo o nada"). 'observer' (opcional) recibe un evento por paso.
ParseResult parseTokens(const std::vector<Token>& tokens, const ParseTable& table,
                        const StepObserver& observer = nullptr);

} // namespace ll1

#endif // PARSER_H
