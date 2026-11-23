#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "data_type.h"

namespace sem {

  enum class NodeKind { SELECT_STMT, TABLE_REF, STAR, COLUMN, LITERAL, BINARY_EXPR };
  enum class BinaryOp { EQ, NEQ, LT, GT, LE, GE, AND, OR };

  inline bool isLogical(BinaryOp op)    { return op == BinaryOp::AND || op == BinaryOp::OR; }
  inline bool isRelational(BinaryOp op) { return !isLogical(op); }
  inline bool isEquality(BinaryOp op)   { return op == BinaryOp::EQ || op == BinaryOp::NEQ; }

  class SelectStmtNode;
  class TableRefNode;
  class StarNode;
  class ColumnNode;
  class LiteralNode;
  class BinaryExprNode;

  class AstVisitor {
    public:
      virtual ~AstVisitor() = default;
      virtual void visit(SelectStmtNode& n) = 0;
      virtual void visit(TableRefNode& n) = 0;
      virtual void visit(StarNode& n) = 0;
      virtual void visit(ColumnNode& n) = 0;
      virtual void visit(LiteralNode& n) = 0;
      virtual void visit(BinaryExprNode& n) = 0;
  };

  class ASTNode {
    public:
      int line = 0;
      int column = 0;
      DataType inferredType = DataType::UNKNOWN; // Decoración del AST

      virtual ~ASTNode() = default;
      virtual NodeKind kind() const = 0;
      virtual void accept(AstVisitor& v) = 0;

    protected:
      ASTNode(int l, int c) : line(l), column(c) {}
  };

  class ExprNode : public ASTNode {
    protected:
      ExprNode(int l, int c) : ASTNode(l, c) {}
  };

  class TableRefNode : public ASTNode {
    public:
      std::string tableName;
      TableRefNode(int l, int c, std::string name) : ASTNode(l, c), tableName(std::move(name)) {}
      NodeKind kind() const override { return NodeKind::TABLE_REF; }
      void accept(AstVisitor& v) override { v.visit(*this); }
  };

  class StarNode : public ExprNode {
    public:
      StarNode(int l, int c) : ExprNode(l, c) {}
      NodeKind kind() const override { return NodeKind::STAR; }
      void accept(AstVisitor& v) override { v.visit(*this); }
  };

  class ColumnNode : public ExprNode {
    public:
      std::string columnName;
      ColumnNode(int l, int c, std::string name) : ExprNode(l, c), columnName(std::move(name)) {}
      NodeKind kind() const override { return NodeKind::COLUMN; }
      void accept(AstVisitor& v) override { v.visit(*this); }
  };

  class LiteralNode : public ExprNode {
    public:
      std::string value;
      DataType literalType;
      LiteralNode(int l, int c, std::string val, DataType type) 
        : ExprNode(l, c), value(std::move(val)), literalType(type) { this->inferredType = type; }
      NodeKind kind() const override { return NodeKind::LITERAL; }
      void accept(AstVisitor& v) override { v.visit(*this); }
  };

  class BinaryExprNode : public ExprNode {
    public:
      BinaryOp op;
      std::unique_ptr<ExprNode> left;
      std::unique_ptr<ExprNode> right;
      BinaryExprNode(int l, int c, BinaryOp o, std::unique_ptr<ExprNode> lft, std::unique_ptr<ExprNode> rgt)
        : ExprNode(l, c), op(o), left(std::move(lft)), right(std::move(rgt)) {}
      NodeKind kind() const override { return NodeKind::BINARY_EXPR; }
      void accept(AstVisitor& v) override { v.visit(*this); }
  };

  class SelectStmtNode : public ASTNode {
    public:
      std::vector<std::unique_ptr<ASTNode>> selectList;
      std::unique_ptr<TableRefNode> fromTable;
      std::unique_ptr<ExprNode> whereClause;

      SelectStmtNode(int l, int c) : ASTNode(l, c) {}
      NodeKind kind() const override { return NodeKind::SELECT_STMT; }
      void accept(AstVisitor& v) override { v.visit(*this); }
  };

}
#endif
