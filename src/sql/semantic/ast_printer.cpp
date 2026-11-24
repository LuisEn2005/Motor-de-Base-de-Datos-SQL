#include "ast_printer.h"

#include <ostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
// Imprime el AST como arbol (formato de docs/sdt.md, seccion 5). Una vez corrido el analizador semantico,
// los nodos con tipo inferido lo muestran como sufijo: "ColumnNode(edad) : INT".
// Recorre con kind() + static_cast (solo lectura), sin necesitar el Visitor no constante.

namespace sem {

  namespace {

    const char* const BRANCH = "├─ ";   // ├─
    const char* const LAST   = "└─ ";   // └─
    const char* const PIPE   = "│  ";         // │
    const char* const BLANK  = "   ";

    std::string opSymbol(BinaryOp op) {
      switch (op) {
        case BinaryOp::EQ:  return "=";
        case BinaryOp::NEQ: return "<>";
        case BinaryOp::LT:  return "<";
        case BinaryOp::GT:  return ">";
        case BinaryOp::LE:  return "<=";
        case BinaryOp::GE:  return ">=";
        case BinaryOp::AND: return "AND";
        case BinaryOp::OR:  return "OR";
      }
      return "?";
    }

    std::string typeSuffix(const ASTNode& n) {
      return n.inferredType == DataType::UNKNOWN ? "" : " : " + toString(n.inferredType);
    }

    // Texto de una linea para un nodo (sin hijos).
    std::string describe(const ASTNode& n) {
      switch (n.kind()) {
        case NodeKind::SELECT_STMT:
          return "SelectStmtNode";
        case NodeKind::TABLE_REF:
          return "TableRefNode(" + static_cast<const TableRefNode&>(n).tableName + ")";
        case NodeKind::STAR:
          return "StarNode";
        case NodeKind::COLUMN:
          return "ColumnNode(" + static_cast<const ColumnNode&>(n).columnName + ")" + typeSuffix(n);
        case NodeKind::LITERAL: {
          const auto& l = static_cast<const LiteralNode&>(n);
          std::string v = (l.literalType == DataType::VARCHAR) ? "'" + l.value + "'" : l.value;
          return "LiteralNode(" + v + ", " + toString(l.literalType) + ")";
        }
        case NodeKind::BINARY_EXPR:
          return "BinaryExprNode(" + opSymbol(static_cast<const BinaryExprNode&>(n).op) + ")" + typeSuffix(n);
      }
      return "?";
    }

    // Imprime los hijos de una expresion binaria bajo 'prefix'.
    void printChildren(std::ostream& os, const ASTNode& n, const std::string& prefix) {
      if (n.kind() != NodeKind::BINARY_EXPR) return;
      const auto& b = static_cast<const BinaryExprNode&>(n);
      const ASTNode* kids[2] = { b.left.get(), b.right.get() };
      for (int i = 0; i < 2; ++i) {
        if (!kids[i]) continue;
        bool last = (i == 1) || !kids[1];
        os << prefix << (last ? LAST : BRANCH) << describe(*kids[i]) << "\n";
        printChildren(os, *kids[i], prefix + (last ? BLANK : PIPE));
      }
    }

  } // namespace

  void printAst(std::ostream& os, const SelectStmtNode& root) {
    os << describe(root) << "\n";
    const bool hasWhere = root.whereClause != nullptr;

    // selectList: un solo elemento va en la misma linea; varios, como lista de hijos.
    const char* selBranch = (root.fromTable || hasWhere) ? BRANCH : LAST;
    if (root.selectList.size() == 1) {
      os << selBranch << "selectList: " << describe(*root.selectList[0]) << "\n";
    } else {
      os << selBranch << "selectList:" << (root.selectList.empty() ? " (vacia)" : "") << "\n";
      for (std::size_t i = 0; i < root.selectList.size(); ++i) {
        bool last = (i + 1 == root.selectList.size());
        os << (std::string(*selBranch == BRANCH[0] ? PIPE : BLANK)) << (last ? LAST : BRANCH)
           << describe(*root.selectList[i]) << "\n";
      }
    }

    if (root.fromTable) {
      os << (hasWhere ? BRANCH : LAST) << "fromTable:  " << describe(*root.fromTable) << "\n";
    }

    if (hasWhere) {
      os << LAST << "whereClause:\n";
      os << BLANK << describe(*root.whereClause) << "\n";
      printChildren(os, *root.whereClause, BLANK);
    }
  }

  std::string astToString(const SelectStmtNode& root) {
    std::ostringstream os;
    printAst(os, root);
    return os.str();
  }

}
//[Console]::OutputEncoding = [System.Text.Encoding]::UTF8