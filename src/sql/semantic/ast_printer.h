#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <iosfwd>
#include <string>

#include "ast.h"

namespace sem {

  void printAst(std::ostream& os, const SelectStmtNode& root);
  std::string astToString(const SelectStmtNode& root);

}

#endif // AST_PRINTER_H
