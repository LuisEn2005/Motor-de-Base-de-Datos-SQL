#ifndef GRAMMAR_H
#define GRAMMAR_H

// Gramatica LL(1) del subconjunto SQL (SELECT) y calculo de FIRST / FOLLOW.
// Ver parser_ll1/docs/gramatica.md para la derivacion a mano.

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ll1 {

// Terminales de la gramatica (uno por cada clase de token del lexer).
enum class Terminal {
    SELECT, FROM, WHERE, AND, OR,
    ID, INT, FLOAT, STRING,
    EQ, NEQ, LT, GT, LE, GE,
    COMMA, SEMI, STAR,
    END,                       // '$': fin de entrada
    COUNT_                     // centinela, no es un terminal
};

enum class NonTerminal {
    QUERY, SELECT_LIST, ID_LIST_P, WHERE_OPT,
    COND, COND_P, AND_COND, AND_P,
    PRED, REL_OP, VALUE,
    COUNT_                     // centinela, no es un no terminal
};

constexpr int kNumTerminals    = static_cast<int>(Terminal::COUNT_);
constexpr int kNumNonTerminals = static_cast<int>(NonTerminal::COUNT_);

struct Symbol {
    bool isTerminal;
    int id;                    // Terminal o NonTerminal segun isTerminal

    static Symbol T(Terminal t)    { return {true,  static_cast<int>(t)}; }
    static Symbol N(NonTerminal n) { return {false, static_cast<int>(n)}; }

    Terminal terminal() const       { return static_cast<Terminal>(id); }
    NonTerminal nonTerminal() const { return static_cast<NonTerminal>(id); }
    bool operator==(const Symbol& o) const { return isTerminal == o.isTerminal && id == o.id; }
};

// A -> rhs   (rhs vacio = epsilon)
struct Production {
    NonTerminal lhs;
    std::vector<Symbol> rhs;
};

// Las 24 producciones, en el orden de docs/gramatica.md.
// El indice 0-based i corresponde a la produccion numero i + 1.
const std::vector<Production>& productions();
NonTerminal startSymbol();

// Nombres para tablas y trazas ("id", "*", "$", "WhereOpt", ...).
std::string toString(Terminal t);
std::string toString(NonTerminal n);
std::string toString(const Symbol& s);
std::string toString(const Production& p);          // "Query -> SELECT SelectList ..."
// Descripcion legible para mensajes de error ("identificador", "';'", ...).
std::string describe(Terminal t);

struct FirstFollowSets {
    std::map<NonTerminal, std::set<Terminal>> first;
    std::map<NonTerminal, std::set<Terminal>> follow;
    std::set<NonTerminal> nullable;                 // no terminales que derivan epsilon
};

// Calcula FIRST y FOLLOW por punto fijo. FOLLOW(Query) contiene '$'.
FirstFollowSets computeFirstFollow();

// FIRST de una secuencia de simbolos; 'nullable' indica si la secuencia deriva epsilon.
std::set<Terminal> firstOfSequence(const std::vector<Symbol>& seq,
                                   const FirstFollowSets& sets,
                                   bool& nullable);

// Salidas formateadas para consola / informe.
void printGrammar(std::ostream& os);
void printFirstFollow(std::ostream& os, const FirstFollowSets& sets);

} // namespace ll1

#endif // GRAMMAR_H
