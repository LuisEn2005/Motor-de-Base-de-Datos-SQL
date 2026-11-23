#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <vector>

#include "ast.h"
#include "catalog.h"
#include "semantic_error.h"

namespace sem {
  class SemanticAnalyzer {
    public:
      explicit SemanticAnalyzer(const CatalogManager& catalog) : catalog(catalog) {}

      bool analyze(SelectStmtNode& ast);

      const std::vector<SemanticDiagnostic>& diagnostics() const { return diags; }

      bool hasErrors() const {
        for (const auto& d : diags) if (d.severity == Severity::ERROR) return true;
        return false;
      }

    private:
      const CatalogManager& catalog;
      std::vector<SemanticDiagnostic> diags;
  };

} // namespace sem

#endif // SEMANTIC_ANALYZER_H
