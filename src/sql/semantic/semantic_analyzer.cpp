#include "semantic_analyzer.h"
#include <unordered_set>

namespace sem {
  class SemanticVisitor : public AstVisitor {
    const CatalogManager& catalog;
    std::vector<SemanticDiagnostic>& diags;
    std::string currentTable;

    void addError(int line, int col, SemanticErrorKind kind, const std::string& msg, const std::string& subj) {
      diags.push_back({Severity::ERROR, kind, msg, subj, line, col});
    }

    public:
    SemanticVisitor(const CatalogManager& cat, std::vector<SemanticDiagnostic>& d) 
      : catalog(cat), diags(d) {}

    void visit(SelectStmtNode& n) override {
      if (n.fromTable) {
        n.fromTable->accept(*this);
      }
      std::unordered_set<std::string> seenColumns;
      for (auto& item : n.selectList) {
        item->accept(*this);
        if (item->kind() == NodeKind::COLUMN) {
          auto col = static_cast<ColumnNode*>(item.get());
          if (!seenColumns.insert(col->columnName).second) {
            addError(col->line, col->column, SemanticErrorKind::DUPLICATE_COLUMN, 
                "Columna proyectada mas de una vez", col->columnName);
          }
        }
      }
      if (n.whereClause) {
        n.whereClause->accept(*this);
        if (n.whereClause->inferredType != DataType::BOOLEAN && n.whereClause->inferredType != DataType::UNKNOWN) {
          addError(n.whereClause->line, n.whereClause->column, SemanticErrorKind::TYPE_MISMATCH,
              "La clausula WHERE debe evaluar a BOOLEAN", "");
        }
      }
      std::vector<std::unique_ptr<ASTNode>> newSelectList;
      for (auto& item : n.selectList) {
        const TableInfo* tableInfo = currentTable.empty() ? nullptr : catalog.findTable(currentTable);
        if (item->kind() == NodeKind::STAR && tableInfo) {
          for (const auto& col : tableInfo->columns) {
            auto colNode = std::make_unique<ColumnNode>(item->line, item->column, col.name);
            colNode->inferredType = col.type;
            newSelectList.push_back(std::move(colNode));
          }
        } else {
          // Cualquier otro nodo, o un '*' cuya tabla no existe: se conserva (no se descarta).
          newSelectList.push_back(std::move(item));
        }
      }
      n.selectList = std::move(newSelectList);
    }

    void visit(TableRefNode& n) override {
      if (!catalog.findTable(n.tableName)) {
        addError(n.line, n.column, SemanticErrorKind::TABLE_NOT_FOUND, 
            "La tabla no existe en el catalogo", n.tableName);
      } else {
        currentTable = n.tableName;
      }
    }

    void visit(StarNode& n) override {
      if (currentTable.empty()) return;
      auto tableInfo = catalog.findTable(currentTable);
      if (tableInfo) {
        n.inferredType = DataType::UNKNOWN; 
      }
    }

    void visit(ColumnNode& n) override {
      if (currentTable.empty()) return;
      auto colInfo = catalog.findColumn(currentTable, n.columnName);
      if (!colInfo) {
        addError(n.line, n.column, SemanticErrorKind::COLUMN_NOT_FOUND, 
            "La columna no pertenece a la tabla " + currentTable, n.columnName);
      } else {
        n.inferredType = colInfo->type;
      }
    }

    void visit(LiteralNode& n) override {
      // El tipo ya viene asignado desde el parser
    }

    void visit(BinaryExprNode& n) override {
      n.left->accept(*this);
      n.right->accept(*this);

      DataType lt = n.left->inferredType;
      DataType rt = n.right->inferredType;

      if (lt == DataType::UNKNOWN || rt == DataType::UNKNOWN) {
        n.inferredType = DataType::UNKNOWN;
        return;
      }

      if (isRelational(n.op) || isEquality(n.op)) {
        if (isNumeric(lt) && isNumeric(rt)) {
          n.inferredType = DataType::BOOLEAN;
        } else if (lt == rt) {
          n.inferredType = DataType::BOOLEAN;
        } else {
          addError(n.line, n.column, SemanticErrorKind::TYPE_MISMATCH, 
              "Tipos incompatibles en comparacion", toString(lt) + " vs " + toString(rt));
          n.inferredType = DataType::UNKNOWN;
        }
      } else if (isLogical(n.op)) {
        if (lt != DataType::BOOLEAN || rt != DataType::BOOLEAN) {
          addError(n.line, n.column, SemanticErrorKind::LOGICAL_OPERAND_NOT_BOOLEAN, 
              "Operadores logicos requieren operandos BOOLEAN", "");
          n.inferredType = DataType::UNKNOWN;
        } else {
          n.inferredType = DataType::BOOLEAN;
        }
      }
    }
  };

  bool SemanticAnalyzer::analyze(SelectStmtNode& ast) {
    diags.clear();
    SemanticVisitor visitor(catalog, diags);
    ast.accept(visitor);
    return !hasErrors();
  }
}
