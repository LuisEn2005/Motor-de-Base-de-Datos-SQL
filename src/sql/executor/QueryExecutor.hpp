#pragma once

// ============================================================
// QueryExecutor.hpp
// Conecta el plan validado (SelectStmtNode del analizador
// semántico) con el motor de almacenamiento (RecordManager +
// BTree) para producir resultados reales.
// ============================================================

#include "../semantic/ast.h"
#include "../semantic/catalog.h"
#include "../../storage/RecordManager.hpp"
#include "../../storage/Tuple.hpp"
#include "../../index/BTree.hpp"
#include "../../index/KeyExtractor.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace db::sql {

// Una fila de resultado: pares (nombre_columna, valor_string)
struct ResultRow {
    std::vector<std::pair<std::string, std::string>> columns;
};

struct QueryResult {
    std::vector<std::string>  columnNames;   // encabezado
    std::vector<ResultRow>    rows;
    std::string               error;         // vacío si todo OK
    bool                      ok = false;
};

// ----------------------------------------------------------------
// Convierte un Value de Tuple a string para mostrar
// ----------------------------------------------------------------
inline std::string valueToString(const storage::Value& v)
{
    return std::visit([](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::int64_t>)
            return std::to_string(x);
        else if constexpr (std::is_same_v<T, double>)
            return std::to_string(x);
        else
            return x;   // std::string
    }, v);
}

// ----------------------------------------------------------------
// Evalúa un predicado WHERE sobre una Tuple.
// Devuelve true si la tupla pasa el filtro.
// ----------------------------------------------------------------
class PredicateEvaluator : public sem::AstVisitor {
public:
    const storage::Tuple& tuple;
    const sem::CatalogManager& catalog;
    const std::string& tableName;

    // resultado de la evaluación del nodo actual
    bool   boolResult  = true;
    double numResult   = 0.0;
    bool   isNum       = false;
    std::string strResult;
    bool   isStr       = false;

    PredicateEvaluator(const storage::Tuple& t,
                       const sem::CatalogManager& cat,
                       const std::string& tbl)
        : tuple(t), catalog(cat), tableName(tbl) {}

    // Obtiene el valor de una columna de la tupla por nombre
    // storage::Tuple = std::vector<Value>, indexado por orden de columna en el catálogo
    std::optional<storage::Value> columnValue(const std::string& colName) const {
        const sem::TableInfo* tbl = catalog.findTable(tableName);
        if (!tbl) return std::nullopt;
        auto it = tbl->columnIndex.find(colName);
        if (it == tbl->columnIndex.end()) return std::nullopt;
        std::size_t idx = it->second;
        if (idx >= tuple.size()) return std::nullopt;
        return tuple[idx];
    }

    void visit(sem::SelectStmtNode&) override {}
    void visit(sem::TableRefNode&)  override {}
    void visit(sem::StarNode&)      override {}

    void visit(sem::ColumnNode& n) override {
        auto val = columnValue(n.columnName);
        if (!val) { isNum = false; isStr = false; return; }
        std::visit([this](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::int64_t>) {
                numResult = static_cast<double>(x); isNum = true; isStr = false;
            } else if constexpr (std::is_same_v<T, double>) {
                numResult = x; isNum = true; isStr = false;
            } else {
                strResult = x; isStr = true; isNum = false;
            }
        }, *val);
    }

    void visit(sem::LiteralNode& n) override {
        if (n.literalType == sem::DataType::INT) {
            numResult = std::stod(n.value); isNum = true; isStr = false;
        } else if (n.literalType == sem::DataType::FLOAT) {
            numResult = std::stod(n.value); isNum = true; isStr = false;
        } else {
            strResult = n.value; isStr = true; isNum = false;
        }
    }

    void visit(sem::BinaryExprNode& n) override {
        if (sem::isLogical(n.op)) {
            PredicateEvaluator left(tuple, catalog, tableName);
            n.left->accept(left);
            PredicateEvaluator right(tuple, catalog, tableName);
            n.right->accept(right);
            if (n.op == sem::BinaryOp::AND)
                boolResult = left.boolResult && right.boolResult;
            else
                boolResult = left.boolResult || right.boolResult;
            isNum = false; isStr = false;
            return;
        }
        // Operador relacional
        PredicateEvaluator left(tuple, catalog, tableName);
        n.left->accept(left);
        PredicateEvaluator right(tuple, catalog, tableName);
        n.right->accept(right);

        if (left.isNum && right.isNum) {
            double l = left.numResult, r = right.numResult;
            switch (n.op) {
                case sem::BinaryOp::EQ:  boolResult = l == r; break;
                case sem::BinaryOp::NEQ: boolResult = l != r; break;
                case sem::BinaryOp::LT:  boolResult = l <  r; break;
                case sem::BinaryOp::GT:  boolResult = l >  r; break;
                case sem::BinaryOp::LE:  boolResult = l <= r; break;
                case sem::BinaryOp::GE:  boolResult = l >= r; break;
                default: boolResult = false;
            }
        } else if (left.isStr && right.isStr) {
            const std::string& l = left.strResult;
            const std::string& r = right.strResult;
            switch (n.op) {
                case sem::BinaryOp::EQ:  boolResult = l == r; break;
                case sem::BinaryOp::NEQ: boolResult = l != r; break;
                case sem::BinaryOp::LT:  boolResult = l <  r; break;
                case sem::BinaryOp::GT:  boolResult = l >  r; break;
                case sem::BinaryOp::LE:  boolResult = l <= r; break;
                case sem::BinaryOp::GE:  boolResult = l >= r; break;
                default: boolResult = false;
            }
        } else {
            boolResult = false;
        }
        isNum = false; isStr = false;
    }
};

// ----------------------------------------------------------------
// QueryExecutor
// ----------------------------------------------------------------
class QueryExecutor {
public:
    explicit QueryExecutor(const sem::CatalogManager& catalog,
                           storage::RecordManager&    records)
        : catalog_(catalog), records_(records) {}

    QueryResult execute(sem::SelectStmtNode& plan)
    {
        QueryResult result;

        if (!plan.fromTable) {
            result.error = "El plan no tiene clausula FROM";
            return result;
        }
        const std::string& tableName = plan.fromTable->tableName;

        // 1. Construir encabezado a partir del select list (ya expandido por el semántico)
        for (const auto& item : plan.selectList) {
            if (item->kind() == sem::NodeKind::COLUMN) {
                auto* col = static_cast<sem::ColumnNode*>(item.get());
                result.columnNames.push_back(col->columnName);
            } else if (item->kind() == sem::NodeKind::STAR) {
                result.columnNames.push_back("*");
            }
        }

        // 2. Full table scan sobre RecordManager
        const auto records = records_.scan();

        for (const auto& record : records) {
            const storage::Tuple& tuple = record.tuple;

            // 3. Filtrar por WHERE (si existe)
            if (plan.whereClause) {
                PredicateEvaluator evaluator(tuple, catalog_, tableName);
                plan.whereClause->accept(evaluator);
                if (!evaluator.boolResult) continue;
            }

            // 4. Proyectar columnas seleccionadas
            ResultRow row;
            const sem::TableInfo* tblInfo = catalog_.findTable(tableName);

            for (const auto& item : plan.selectList) {
                if (item->kind() == sem::NodeKind::COLUMN) {
                    auto* colNode = static_cast<sem::ColumnNode*>(item.get());
                    std::string val;
                    if (tblInfo) {
                        auto it = tblInfo->columnIndex.find(colNode->columnName);
                        if (it != tblInfo->columnIndex.end() && it->second < tuple.size())
                            val = valueToString(tuple[it->second]);
                    }
                    row.columns.push_back({colNode->columnName, val});
                }
            }
            result.rows.push_back(std::move(row));
        }

        result.ok = true;
        return result;
    }

private:
    const sem::CatalogManager& catalog_;
    storage::RecordManager&    records_;
};

} // namespace db::sql
