#ifndef PARSE_TABLE_H
#define PARSE_TABLE_H

// Tabla de parseo LL(1): M[A, a] -> numero de produccion.
//
// Este header es el CONTRATO entre la tabla y el motor (parser.cpp):
//   - La estructura ParseTable y sus consultas (lookup / expected) ya estan definidas.
//   - TODO (Persona B): implementar en parse_table.cpp la construccion de la tabla a partir de
//     FIRST/FOLLOW (buildParseTable, con deteccion de conflictos), defaultParseTable() y
//     printParseTable(); y agregar parse_table.cpp al CMake (parser_core).

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "grammar.h"

namespace ll1 {

struct ParseTable {
    // (A, a) -> indice 0-based de la produccion (numero de produccion = indice + 1).
    std::map<std::pair<NonTerminal, Terminal>, int> cells;
    // Descripcion de cada celda con mas de una produccion candidata. Vacio <=> gramatica LL(1).
    std::vector<std::string> conflicts;

    bool isLL1() const { return conflicts.empty(); }

    // Indice de la produccion en M[A, a], o -1 si la celda esta vacia (error sintactico).
    int lookup(NonTerminal a, Terminal t) const {
        auto it = cells.find({a, t});
        return it == cells.end() ? -1 : it->second;
    }

    // Terminales con celda no vacia en la fila A (los tokens "esperados" para A), en orden del enum.
    std::vector<Terminal> expected(NonTerminal a) const {
        std::vector<Terminal> out;
        for (int i = 0; i < kNumTerminals; ++i) {
            auto t = static_cast<Terminal>(i);
            if (lookup(a, t) >= 0) out.push_back(t);
        }
        return out;
    }
};

// --- Pendiente (Persona B): declaradas aqui, se implementan en parse_table.cpp ---

// M[A, a] = A -> alpha  para cada a en FIRST(alpha);  y si alpha deriva eps, tambien para cada a en FOLLOW(A).
ParseTable buildParseTable(const FirstFollowSets& sets);

// Tabla de la gramatica del equipo (se construye una sola vez).
const ParseTable& defaultParseTable();

// Impresion de la tabla M[A, a] para consola / informe.
void printParseTable(std::ostream& os, const ParseTable& table);

} // namespace ll1

#endif // PARSE_TABLE_H
