#ifndef SEMANTIC_ERROR_H
#define SEMANTIC_ERROR_H

#include <string>

#include "../lexer/token.h"

namespace sem {

  enum class SemanticErrorKind {
    TABLE_NOT_FOUND,
    COLUMN_NOT_FOUND,
    TYPE_MISMATCH,
    LOGICAL_OPERAND_NOT_BOOLEAN,
    DUPLICATE_COLUMN
  };

  struct SemanticDiagnostic {
    Severity severity = Severity::ERROR;
    SemanticErrorKind kind = SemanticErrorKind::TABLE_NOT_FOUND;
    std::string message;
    std::string subject;
    int line = 0;
    int column = 0;
  };

  inline std::string toString(SemanticErrorKind k) {
    switch (k) {
      case SemanticErrorKind::TABLE_NOT_FOUND:             return "TABLE_NOT_FOUND";
      case SemanticErrorKind::COLUMN_NOT_FOUND:            return "COLUMN_NOT_FOUND";
      case SemanticErrorKind::TYPE_MISMATCH:               return "TYPE_MISMATCH";
      case SemanticErrorKind::LOGICAL_OPERAND_NOT_BOOLEAN: return "LOGICAL_OPERAND_NOT_BOOLEAN";
      case SemanticErrorKind::DUPLICATE_COLUMN:            return "DUPLICATE_COLUMN";
    }
    return "UNKNOWN";
  }

  inline std::string format(const SemanticDiagnostic& d) {
    std::string prefix = (d.severity == Severity::ERROR) ? "[ERROR semantico] " : "[ADVERTENCIA semantica] ";
    std::string out = prefix + std::to_string(d.line) + ":" + std::to_string(d.column) + " " + toString(d.kind) +
      ": " + d.message;
    if (!d.subject.empty()) out += " [" + d.subject + "]";   // identificador o expresion conflictiva
    return out;
  }

}

#endif // SEMANTIC_ERROR_H
