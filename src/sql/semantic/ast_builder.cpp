#include "ast_builder.h"

#include <stdexcept>
#include <utility>

#include "grammar.h"

// Construccion del AST a partir de los tokens y de la lista de producciones del parser LL(1)
// (ver docs/sdt.md). Hay un metodo por no terminal: consume la siguiente produccion de la lista y los
// tokens que le corresponden. Atributos heredados = parametros; sintetizados = valores de retorno.

namespace sem {

  namespace {

    using ll1::NonTerminal;
    using ll1::Terminal;
    using ExprPtr = std::unique_ptr<ExprNode>;
    using NodeList = std::vector<std::unique_ptr<ASTNode>>;

    struct BuildError : std::runtime_error {
      using std::runtime_error::runtime_error;
    };

    class Builder {
      public:
        Builder(const std::vector<Token>& toks, const std::vector<int>& prods)
          : tokens(toks), prods(prods) {}

        std::unique_ptr<SelectStmtNode> build() {
          auto root = query();
          if (prodPos != prods.size())
            fail("sobran producciones sin consumir");
          return root;
        }

      private:
        const std::vector<Token>& tokens;
        const std::vector<int>& prods;
        std::size_t prodPos = 0;
        std::size_t tokPos = 0;

        [[noreturn]] void fail(const std::string& msg) const {
          throw BuildError("AST: " + msg);
        }

        // Toma la siguiente produccion (1-based) y verifica que sea de 'expected'.
        // Devuelve su numero; el llamador decide segun el numero.
        int nextProduction(NonTerminal expected) {
          if (prodPos >= prods.size())
            fail("se acabaron las producciones en " + ll1::toString(expected));
          int num = prods[prodPos++];
          const auto& all = ll1::productions();
          if (num < 1 || static_cast<std::size_t>(num) > all.size())
            fail("numero de produccion invalido: " + std::to_string(num));
          if (all[num - 1].lhs != expected)
            fail("la produccion " + std::to_string(num) + " no pertenece a " + ll1::toString(expected));
          return num;
        }

        // Consume el token actual, que debe ser de alguno de los tipos dados.
        const Token& expect(std::initializer_list<TokenType> types, const char* what) {
          if (tokPos >= tokens.size())
            fail(std::string("faltan tokens, se esperaba ") + what);
          const Token& t = tokens[tokPos];
          for (auto ty : types) {
            if (t.type == ty) { ++tokPos; return t; }
          }
          fail(std::string("se esperaba ") + what + " y se encontro '" + t.lexeme + "' en " +
              std::to_string(t.line) + ":" + std::to_string(t.column));
        }

        const Token& expect(TokenType type, const char* what) { return expect({type}, what); }

        static std::string literalText(const Token& t) { return t.value.empty() ? t.lexeme : t.value; }

        // Query -> SELECT SelectList FROM id WhereOpt ;
        std::unique_ptr<SelectStmtNode> query() {
          nextProduction(NonTerminal::QUERY);                       // 1
          const Token& sel = expect(TokenType::KEYWORD_SELECT, "SELECT");
          auto stmt = std::make_unique<SelectStmtNode>(sel.line, sel.column);
          stmt->selectList = selectList();
          expect(TokenType::KEYWORD_FROM, "FROM");
          const Token& tbl = expect(TokenType::IDENTIFIER, "nombre de tabla");
          stmt->fromTable = std::make_unique<TableRefNode>(tbl.line, tbl.column, tbl.lexeme);
          stmt->whereClause = whereOpt();
          expect(TokenType::SEMICOLON, "';'");
          return stmt;
        }

        NodeList selectList() {
          int p = nextProduction(NonTerminal::SELECT_LIST);
          NodeList list;
          if (p == 2) {                                             // SelectList -> *
            const Token& star = expect(TokenType::STAR, "'*'");
            list.push_back(std::make_unique<StarNode>(star.line, star.column));
            return list;
          }
          const Token& id = expect(TokenType::IDENTIFIER, "columna");   // SelectList -> id IdListP
          list.push_back(std::make_unique<ColumnNode>(id.line, id.column, id.lexeme));
          return idListP(std::move(list));
        }

        // IdListP.her = lista acumulada; devuelve IdListP.sin
        NodeList idListP(NodeList her) {
          while (nextProduction(NonTerminal::ID_LIST_P) == 4) {     // IdListP -> , id IdListP
            expect(TokenType::COMMA, "','");
            const Token& id = expect(TokenType::IDENTIFIER, "columna");
            her.push_back(std::make_unique<ColumnNode>(id.line, id.column, id.lexeme));
          }
          return her;                                               // 5: IdListP -> eps
        }

        ExprPtr whereOpt() {
          int p = nextProduction(NonTerminal::WHERE_OPT);
          if (p == 7) return nullptr;                               // WhereOpt -> eps
          expect(TokenType::KEYWORD_WHERE, "WHERE");                // 6
          return cond();
        }

        // Cond -> AndCond CondP
        ExprPtr cond() {
          nextProduction(NonTerminal::COND);                        // 8
          return condP(andCond());
        }

        // CondP -> OR AndCond CondP | eps   (asociativo a la izquierda)
        ExprPtr condP(ExprPtr her) {
          while (nextProduction(NonTerminal::COND_P) == 9) {
            const Token& op = expect(TokenType::KEYWORD_OR, "OR");
            ExprPtr right = andCond();
            her = std::make_unique<BinaryExprNode>(op.line, op.column, BinaryOp::OR,
                std::move(her), std::move(right));
          }
          return her;
        }

        // AndCond -> Pred AndP
        ExprPtr andCond() {
          nextProduction(NonTerminal::AND_COND);                    // 11
          return andP(pred());
        }

        // AndP -> AND Pred AndP | eps
        ExprPtr andP(ExprPtr her) {
          while (nextProduction(NonTerminal::AND_P) == 12) {
            const Token& op = expect(TokenType::KEYWORD_AND, "AND");
            ExprPtr right = pred();
            her = std::make_unique<BinaryExprNode>(op.line, op.column, BinaryOp::AND,
                std::move(her), std::move(right));
          }
          return her;
        }

        // Pred -> id RelOp Value
        ExprPtr pred() {
          nextProduction(NonTerminal::PRED);                        // 14
          const Token& id = expect(TokenType::IDENTIFIER, "columna");
          auto left = std::make_unique<ColumnNode>(id.line, id.column, id.lexeme);
          const Token* opTok = nullptr;
          BinaryOp op = relOp(opTok);
          ExprPtr right = value();
          return std::make_unique<BinaryExprNode>(opTok->line, opTok->column, op,
              std::move(left), std::move(right));
        }

        BinaryOp relOp(const Token*& opTok) {
          int p = nextProduction(NonTerminal::REL_OP);              // 15..20
          static const TokenType types[] = {
            TokenType::EQUAL, TokenType::NOT_EQUAL, TokenType::LESS_THAN,
            TokenType::GREATER_THAN, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL };
          static const BinaryOp ops[] = {
            BinaryOp::EQ, BinaryOp::NEQ, BinaryOp::LT, BinaryOp::GT, BinaryOp::LE, BinaryOp::GE };
          if (p < 15 || p > 20) fail("produccion invalida para RelOp");
          opTok = &expect(types[p - 15], "operador relacional");
          return ops[p - 15];
        }

        ExprPtr value() {
          int p = nextProduction(NonTerminal::VALUE);               // 21..24
          switch (p) {
            case 21: {
              const Token& t = expect(TokenType::INTEGER_LITERAL, "entero");
              return std::make_unique<LiteralNode>(t.line, t.column, literalText(t), DataType::INT);
            }
            case 22: {
              const Token& t = expect(TokenType::FLOAT_LITERAL, "decimal");
              return std::make_unique<LiteralNode>(t.line, t.column, literalText(t), DataType::FLOAT);
            }
            case 23: {
              const Token& t = expect(TokenType::STRING_LITERAL, "cadena");
              return std::make_unique<LiteralNode>(t.line, t.column, literalText(t), DataType::VARCHAR);
            }
            default: {
              const Token& t = expect(TokenType::IDENTIFIER, "columna");
              return std::make_unique<ColumnNode>(t.line, t.column, t.lexeme);
            }
          }
        }
    };

  } // namespace

  BuildResult buildAst(const std::vector<Token>& tokens, const std::vector<int>& productions) {
    BuildResult result;
    try {
      Builder b(tokens, productions);
      result.ast = b.build();
      result.ok = true;
    } catch (const BuildError& e) {
      result.ast.reset();
      result.ok = false;
      result.error = e.what();
    }
    return result;
  }

} // namespace sem
