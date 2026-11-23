#include "parse_table.h"

#include <algorithm>
#include <iomanip>
#include <ostream>

namespace ll1 {

namespace {

// Agrega la produccion 'prod' (0-based) a la celda M[A, a]. Si la celda ya tenia OTRA produccion
// distinta, se registra un conflicto (la gramatica dejaria de ser LL(1)).
void addCell(ParseTable& table, NonTerminal a, Terminal t, int prod) {
    auto key = std::make_pair(a, t);
    auto it = table.cells.find(key);
    if (it == table.cells.end()) {
        table.cells[key] = prod;
        return;
    }
    if (it->second != prod) {
        table.conflicts.push_back("M[" + toString(a) + ", " + toString(t) + "]: producciones " +
                                  std::to_string(it->second + 1) + " y " + std::to_string(prod + 1));
    }
}

} // namespace

// Algoritmo clasico de construccion de la tabla LL(1):
//   para cada produccion A -> alpha:
//     1) para cada a en FIRST(alpha)            : M[A, a] = A -> alpha
//     2) si alpha deriva epsilon, para cada b en FOLLOW(A) : M[A, b] = A -> alpha
ParseTable buildParseTable(const FirstFollowSets& sets) {
    ParseTable table;
    const auto& prods = productions();

    for (size_t i = 0; i < prods.size(); ++i) {
        const Production& p = prods[i];

        bool alphaNullable = false;
        std::set<Terminal> first = firstOfSequence(p.rhs, sets, alphaNullable);

        for (Terminal a : first) addCell(table, p.lhs, a, static_cast<int>(i));

        if (alphaNullable) {
            for (Terminal b : sets.follow.at(p.lhs)) addCell(table, p.lhs, b, static_cast<int>(i));
        }
    }
    return table;
}

const ParseTable& defaultParseTable() {
    static const ParseTable table = buildParseTable(computeFirstFollow());
    return table;
}

// Imprime M[A, a] como matriz: una fila por no terminal, una columna por terminal.
// Cada celda muestra el numero de produccion; vacia = error sintactico.
void printParseTable(std::ostream& os, const ParseTable& table) {
    const int firstCol = 12;   // ancho de la columna con el nombre del no terminal
    const int colW = 7;        // ancho de cada columna de terminal

    os << "Tabla de parseo LL(1) M[A, a]   (celda = N de produccion; vacia = error)\n";
    os << std::left << std::setw(firstCol) << "";
    for (int j = 0; j < kNumTerminals; ++j) {
        os << std::setw(colW) << toString(static_cast<Terminal>(j));
    }
    os << "\n" << std::string(firstCol + colW * kNumTerminals, '-') << "\n";

    for (int i = 0; i < kNumNonTerminals; ++i) {
        auto nt = static_cast<NonTerminal>(i);
        os << std::left << std::setw(firstCol) << toString(nt);
        for (int j = 0; j < kNumTerminals; ++j) {
            int p = table.lookup(nt, static_cast<Terminal>(j));
            os << std::setw(colW) << (p < 0 ? std::string(".") : std::to_string(p + 1));
        }
        os << "\n";
    }

    if (table.isLL1()) {
        os << "\nSin conflictos: la gramatica es LL(1).\n";
    } else {
        os << "\nCONFLICTOS (la gramatica NO es LL(1)):\n";
        for (const auto& c : table.conflicts) os << "  - " << c << "\n";
    }
}

} // namespace ll1
