#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include <memory>
#include <string>
#include <vector>

#include "ast.h"
#include "../lexer/token.h"

namespace sem {

  struct BuildResult {
    std::unique_ptr<SelectStmtNode> ast;
    bool ok = false;
    std::string error;
  };

  BuildResult buildAst(const std::vector<Token>& tokens, const std::vector<int>& productions);

}

#endif // AST_BUILDER_H
