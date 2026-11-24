#pragma once

// ============================================================
// SQLEngine.hpp
// Fachada del SQL Server: recibe una cadena SQL, la pasa por
// Lexer → Parser LL(1) → AST Builder → Analizador Semántico
// → Executor, y devuelve el resultado o los errores.
// ============================================================

#include "../lexer/lexer.h"
#include "../parser/grammar.h"
#include "../parser/parse_table.h"
#include "../parser/parser.h"
#include "../semantic/ast_builder.h"
#include "../semantic/ast_printer.h"
#include "../semantic/catalog.h"
#include "../semantic/semantic_analyzer.h"
#include "QueryExecutor.hpp"

#include "../../storage/RecordManager.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace db::sql {

struct SQLResponse {
    bool        ok = false;
    std::string stage;      // etapa donde se detectó el error
    std::string errorMsg;
    QueryResult result;     // válido si ok == true
};

class SQLEngine {
public:
    // catalog: catálogo de tablas/columnas (puede cargarse desde archivo).
    // records: instancia del RecordManager del motor de almacenamiento.
    SQLEngine(sem::CatalogManager catalog, storage::RecordManager& records)
        : catalog_(std::move(catalog))
        , records_(records)
        , parseTable_(ll1::defaultParseTable())
    {}

    // Ejecuta una consulta SQL y devuelve la respuesta.
    SQLResponse query(const std::string& sql)
    {
        SQLResponse resp;

        // ── 1. LÉXICO ──────────────────────────────────────────
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenizeAll();

        if (lexer.hasErrors()) {
            resp.stage    = "Lexico";
            resp.errorMsg = formatDiagnostics(lexer.diagnostics());
            return resp;
        }

        // ── 2. SINTÁCTICO (LL(1)) ──────────────────────────────
        ll1::ParseResult parseResult = ll1::parseTokens(tokens, parseTable_);

        if (!parseResult.accepted) {
            resp.stage = "Sintactico";
            if (parseResult.error) {
                resp.errorMsg = parseResult.error->message;
            } else {
                resp.errorMsg = "Error sintactico desconocido";
            }
            return resp;
        }

        // ── 3. CONSTRUCCIÓN DEL AST ────────────────────────────
        sem::BuildResult buildResult = sem::buildAst(tokens, parseResult.productions);

        if (!buildResult.ok || !buildResult.ast) {
            resp.stage    = "AST";
            resp.errorMsg = buildResult.error;
            return resp;
        }

        // ── 4. ANÁLISIS SEMÁNTICO ──────────────────────────────
        sem::SemanticAnalyzer semAnalyzer(catalog_);
        semAnalyzer.analyze(*buildResult.ast);

        if (semAnalyzer.hasErrors()) {
            resp.stage    = "Semantico";
            resp.errorMsg = formatSemDiagnostics(semAnalyzer.diagnostics());
            return resp;
        }

        // ── 5. EJECUCIÓN ───────────────────────────────────────
        QueryExecutor executor(catalog_, records_);
        resp.result = executor.execute(*buildResult.ast);

        if (!resp.result.ok) {
            resp.stage    = "Ejecucion";
            resp.errorMsg = resp.result.error;
            return resp;
        }

        resp.ok = true;
        return resp;
    }

    // Acceso de solo lectura al catálogo (para agregar tablas en runtime)
    sem::CatalogManager& catalog() { return catalog_; }

private:
    sem::CatalogManager      catalog_;
    storage::RecordManager&  records_;
    const ll1::ParseTable&   parseTable_;

    static std::string formatDiagnostics(const std::vector<Diagnostic>& diags)
    {
        std::ostringstream out;
        for (const auto& d : diags)
            out << severityToString(d.severity) << " [" << d.line << ":" << d.column << "] "
                << d.message << "\n";
        return out.str();
    }

    static std::string formatSemDiagnostics(const std::vector<sem::SemanticDiagnostic>& diags)
    {
        std::ostringstream out;
        for (const auto& d : diags)
            out << sem::format(d) << "\n";
        return out.str();
    }
};

} // namespace db::sql
