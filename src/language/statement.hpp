#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "common.hpp"
#include "template.hpp"
#include "expr.hpp"
#include "lang_common.hpp"

enum class StatementKind {
    DeclVar,
    DeclProc,
    DeclType,
    Assignment,
    Block,
    If_,
    For_,
    While_,
    Expression,
};

struct Statement {
    StatementKind kind;
};

struct ProgramTree {
    DArray<Statement*> statements = {};

    ~ProgramTree() {
        statements.reset();
    }
};

struct StmtBlock : Statement {
    DArray<Statement*> statements;
    StmtBlock() {
        kind = StatementKind::Block;
    }
    StmtBlock(DArray<Statement*> stmts) : statements(stmts) {}
};

struct StmtDeclVar : Statement {
    String name = {};
    Expr* rhs = nullptr;
    String type_name = {};
    bool infer_type = false;
    StmtDeclVar() {
        kind = StatementKind::DeclVar;
    }
    StmtDeclVar(String name, Expr* rhs, String type, bool infer) : name(name), rhs(rhs), type_name(type), infer_type(infer) {
        kind = StatementKind::DeclVar;
    }
};

using ParameterFlags = u8;
#define PARAMETER_IS_INPUT  BIT(0)
#define PARAMETER_IS_OUTPUT BIT(1)
#define PARAMETER_IS_CONST  BIT(2)
struct Parameter {
    String name = {};
    String type = {};
    ParameterFlags flags = {};

    Parameter() {}
    Parameter(String name, String type, ParameterFlags flags)
        : name(name), type(type), flags(flags)
    {}
};

using ProcFlags = u8;
#define PROC_IS_BUILTIN       BIT(0)
#define PROC_HAS_SIDE_EFFECTS BIT(1)
struct StmtDeclProc : Statement {
    String name = {};
    DArray<Parameter> parameters = {};
    DArray<Statement*> body = {};
    ProcFlags flags = {};
    StmtDeclProc() {
        kind = StatementKind::DeclProc;
    }
    StmtDeclProc(String name, DArray<Parameter> params, DArray<Statement*> body, ProcFlags flags)
    : name(name), parameters(params), body(body), flags(flags)
    {
        kind = StatementKind::DeclProc;
    }
};

using TypeFlags = u8;
// @todo distint types (type A = int)
#define TypeIsComposite BIT(0)
#define TypeIsEnum      BIT(1)
// #define TypeIsUnion     BIT(2)
struct StmtDeclType : Statement {
    String name = {};
    DArray<Variable> fields = {};
    TypeFlags flags = {};
    StmtDeclType() {
        kind = StatementKind::DeclType;
    }
    StmtDeclType(String name, DArray<Variable> field, TypeFlags flags) : name(name), fields(field), flags(flags) {
        kind = StatementKind::DeclType;
    }
};

struct StmtAssignment : Statement {
    Expr* lhs = nullptr;
    Expr* rhs = nullptr;
    StmtAssignment() {
        kind = StatementKind::Assignment;
    }
    StmtAssignment(Expr* l, Expr* r) : lhs(l), rhs(r) {
        kind = StatementKind::Assignment;
    }
};

struct StmtIf : Statement {
    Expr* condition = nullptr;
    Statement* then_ = nullptr;
    Statement* else_ = nullptr;
    StmtIf() {
        kind = StatementKind::If_;
    }
    StmtIf(Expr* cond, Statement* th, Statement* els) : condition(cond), then_(th), else_(els) {
        kind = StatementKind::If_;
    }
};

struct StmtFor : Statement {
    Expr* start = nullptr;
    Expr* condition = nullptr;
    Expr* end = nullptr;
    Statement* body = nullptr;
    StmtFor() {
        kind = StatementKind::For_;
    }
    StmtFor(Expr* start, Expr* cond, Expr* end, Statement* body) : start(start), condition(cond), end(end), body(body) {
        kind = StatementKind::For_;
    }
};

struct StmtWhile : Statement {
    Expr* condition = nullptr;
    Statement* body = nullptr;
    StmtWhile() {
        kind = StatementKind::While_;
    }
    StmtWhile(Expr* cond, Statement* body) : condition(cond), body(body) {
        kind = StatementKind::While_;
    }
};

struct StmtExpression : Statement {
    Expr* expr = nullptr;
    StmtExpression() {
        kind = StatementKind::Expression;
    }
    StmtExpression(Expr* expr) : expr(expr) {
        kind = StatementKind::Expression;
    }
};

#endif // STATEMENT_HPP
