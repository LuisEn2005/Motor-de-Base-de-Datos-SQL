#ifndef TRACE_H
#define TRACE_H

// Trazador del parseo: imprime en consola (paso, pila, entrada, accion) en cada paso del motor.
// Se engancha al motor por medio del StepObserver de parser.h, sin tocar el bucle del parser.

#include <iosfwd>
#include <string>

#include "parser.h"

namespace ll1 {

// Texto de la pila con el TOPE a la izquierda: "Query $".
std::string stackToString(const std::vector<Symbol>& stack);

// Entrada restante desde el lookahead, terminada en '$': "a FROM t ; $".
std::string remainingInput(const std::vector<Token>& tokens, size_t position);

// Imprime la cabecera de la tabla de traza.
void printTraceHeader(std::ostream& os);

// Devuelve un observador que imprime una fila por paso en 'os'.
StepObserver makeTracer(std::ostream& os);

} // namespace ll1

#endif // TRACE_H
