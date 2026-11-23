#include "grammar.h"

#include <iomanip>
#include <ostream>

namespace ll1 {

namespace {

using T = Terminal;
using N = NonTerminal;

Symbol t(Terminal x)    { return Symbol::T(x); }
Symbol n(NonTerminal x) { return Symbol::N(x); }

std::vector<Production> buildProductions() {
    return {
        /*  1 */ {N::QUERY,       {t(T::SELECT), n(N::SELECT_LIST), t(T::FROM), t(T::ID), n(N::WHERE_OPT), t(T::SEMI)}},
        /*  2 */ {N::SELECT_LIST, {t(T::STAR)}},
        /*  3 */ {N::SELECT_LIST, {t(T::ID), n(N::ID_LIST_P)}},
        /*  4 */ {N::ID_LIST_P,   {t(T::COMMA), t(T::ID), n(N::ID_LIST_P)}},
        /*  5 */ {N::ID_LIST_P,   {}},
        /*  6 */ {N::WHERE_OPT,   {t(T::WHERE), n(N::COND)}},
        /*  7 */ {N::WHERE_OPT,   {}},
        /*  8 */ {N::COND,        {n(N::AND_COND), n(N::COND_P)}},
        /*  9 */ {N::COND_P,      {t(T::OR), n(N::AND_COND), n(N::COND_P)}},
        /* 10 */ {N::COND_P,      {}},
        /* 11 */ {N::AND_COND,    {n(N::PRED), n(N::AND_P)}},
        /* 12 */ {N::AND_P,       {t(T::AND), n(N::PRED), n(N::AND_P)}},
        /* 13 */ {N::AND_P,       {}},
        /* 14 */ {N::PRED,        {t(T::ID), n(N::REL_OP), n(N::VALUE)}},
        /* 15 */ {N::REL_OP,      {t(T::EQ)}},
        /* 16 */ {N::REL_OP,      {t(T::NEQ)}},
        /* 17 */ {N::REL_OP,      {t(T::LT)}},
        /* 18 */ {N::REL_OP,      {t(T::GT)}},
        /* 19 */ {N::REL_OP,      {t(T::LE)}},
        /* 20 */ {N::REL_OP,      {t(T::GE)}},
        /* 21 */ {N::VALUE,       {t(T::INT)}},
        /* 22 */ {N::VALUE,       {t(T::FLOAT)}},
        /* 23 */ {N::VALUE,       {t(T::STRING)}},
        /* 24 */ {N::VALUE,       {t(T::ID)}},
    };
}

} // namespace

const std::vector<Production>& productions() {
    static const std::vector<Production> prods = buildProductions();
    return prods;
}

NonTerminal startSymbol() { return NonTerminal::QUERY; }

std::string toString(Terminal x) {
    switch (x) {
        case T::SELECT: return "SELECT";
        case T::FROM:   return "FROM";
        case T::WHERE:  return "WHERE";
        case T::AND:    return "AND";
        case T::OR:     return "OR";
        case T::ID:     return "id";
        case T::INT:    return "int";
        case T::FLOAT:  return "float";
        case T::STRING: return "string";
        case T::EQ:     return "=";
        case T::NEQ:    return "<>";
        case T::LT:     return "<";
        case T::GT:     return ">";
        case T::LE:     return "<=";
        case T::GE:     return ">=";
        case T::COMMA:  return ",";
        case T::SEMI:   return ";";
        case T::STAR:   return "*";
        case T::END:    return "$";
        default:        return "?";
    }
}

std::string toString(NonTerminal x) {
    switch (x) {
        case N::QUERY:       return "Query";
        case N::SELECT_LIST: return "SelectList";
        case N::ID_LIST_P:   return "IdListP";
        case N::WHERE_OPT:   return "WhereOpt";
        case N::COND:        return "Cond";
        case N::COND_P:      return "CondP";
        case N::AND_COND:    return "AndCond";
        case N::AND_P:       return "AndP";
        case N::PRED:        return "Pred";
        case N::REL_OP:      return "RelOp";
        case N::VALUE:       return "Value";
        default:             return "?";
    }
}

std::string toString(const Symbol& s) {
    return s.isTerminal ? toString(s.terminal()) : toString(s.nonTerminal());
}

std::string toString(const Production& p) {
    std::string out = toString(p.lhs) + " ->";
    if (p.rhs.empty()) return out + " eps";
    for (const auto& s : p.rhs) out += " " + toString(s);
    return out;
}

std::string describe(Terminal x) {
    switch (x) {
        case T::ID:     return "identificador";
        case T::INT:    return "entero";
        case T::FLOAT:  return "decimal";
        case T::STRING: return "cadena";
        case T::END:    return "fin de entrada";
        case T::EQ: case T::NEQ: case T::LT: case T::GT: case T::LE: case T::GE:
        case T::COMMA: case T::SEMI: case T::STAR:
            return "'" + toString(x) + "'";
        default:        return toString(x);
    }
}

std::set<Terminal> firstOfSequence(const std::vector<Symbol>& seq,
                                   const FirstFollowSets& sets,
                                   bool& nullable) {
    std::set<Terminal> result;
    nullable = true;
    for (const auto& s : seq) {
        if (s.isTerminal) {
            result.insert(s.terminal());
            nullable = false;
            return result;
        }
        auto it = sets.first.find(s.nonTerminal());
        if (it != sets.first.end()) result.insert(it->second.begin(), it->second.end());
        if (!sets.nullable.count(s.nonTerminal())) {
            nullable = false;
            return result;
        }
    }
    return result;
}

FirstFollowSets computeFirstFollow() {
    FirstFollowSets sets;
    const auto& prods = productions();

    for (int i = 0; i < kNumNonTerminals; ++i) {
        auto nt = static_cast<NonTerminal>(i);
        sets.first[nt];
        sets.follow[nt];
    }

    // NULLABLE y FIRST: punto fijo.
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : prods) {
            bool seqNullable = false;
            std::set<Terminal> f = firstOfSequence(p.rhs, sets, seqNullable);
            auto& dst = sets.first[p.lhs];
            for (auto x : f) if (dst.insert(x).second) changed = true;
            if (seqNullable && sets.nullable.insert(p.lhs).second) changed = true;
        }
    }

    // FOLLOW: punto fijo.
    sets.follow[startSymbol()].insert(Terminal::END);
    changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : prods) {
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                if (p.rhs[i].isTerminal) continue;
                NonTerminal b = p.rhs[i].nonTerminal();
                std::vector<Symbol> beta(p.rhs.begin() + i + 1, p.rhs.end());
                bool betaNullable = false;
                std::set<Terminal> f = firstOfSequence(beta, sets, betaNullable);
                auto& dst = sets.follow[b];
                for (auto x : f) if (dst.insert(x).second) changed = true;
                if (betaNullable) {
                    for (auto x : sets.follow[p.lhs]) if (dst.insert(x).second) changed = true;
                }
            }
        }
    }
    return sets;
}


void printGrammar(std::ostream& os) {
    const auto& prods = productions();
    os << "Gramatica LL(1) (simbolo inicial: " << toString(startSymbol()) << ")\n";
    for (size_t i = 0; i < prods.size(); ++i) {
        os << "  " << std::setw(2) << (i + 1) << ". " << toString(prods[i]) << "\n";
    }
}

namespace {
std::string setToString(const std::set<Terminal>& s, bool withEps) {
    std::string out = "{ ";
    bool first = true;
    for (auto t : s) {
        if (!first) out += ", ";
        out += (t == Terminal::COMMA) ? "','" : toString(t);   // la coma se muestra entre comillas
        first = false;
    }
    if (withEps) {
        if (!first) out += ", ";
        out += "eps";
        first = false;
    }
    return out + (first ? "}" : " }");
}
} // namespace

void printFirstFollow(std::ostream& os, const FirstFollowSets& sets) {
    os << "Conjuntos FIRST\n";
    for (int i = 0; i < kNumNonTerminals; ++i) {
        auto nt = static_cast<NonTerminal>(i);
        os << "  FIRST(" << toString(nt) << ") = "
           << setToString(sets.first.at(nt), sets.nullable.count(nt) > 0) << "\n";
    }
    os << "Conjuntos FOLLOW\n";
    for (int i = 0; i < kNumNonTerminals; ++i) {
        auto nt = static_cast<NonTerminal>(i);
        os << "  FOLLOW(" << toString(nt) << ") = " << setToString(sets.follow.at(nt), false) << "\n";
    }
}

} // namespace ll1
