#ifndef EXPR_HPP
#define EXPR_HPP

#include "lang.hpp"
#include "token.hpp"
#include "template.hpp"

enum Op_Unary {
    Unop_Negate,
    Unop_Plus,
    Unop_Not,
};

enum Op_Binary {
    Binop_Unknown = 0,
    Binop_Add,
    Binop_Sub,
    Binop_Mul,
    Binop_Div,
    Binop_Mod,
    Binop_Logic_And,
    Binop_Logic_Or,
    Binop_Bitwise_And,
    Binop_Bitwise_Or,
    Binop_Bitwise_Xor,
    Binop_Eq,
    Binop_Neq,
    Binop_Gt,
    Binop_Ge,
    Binop_Lt,
    Binop_Le,
};

enum class ExprKind {
    Literal,
    Variable,
    Unary,
    Binary,
    Grouping,
    Call,
	Ternary,
	Tuple,
};

using ExprFlags = u8;

struct Expr {
    ExprKind type;
    Variable_Type result_type = {};
    ExprFlags flags = 0;  // unused currently
};

void print_expr(const Expr* expr, int indent);

struct Expr_Literal : Expr {
    Value value;

    Expr_Literal(bool b) : value(b)
	{
        type = ExprKind::Literal;
    }
    Expr_Literal(long long integer) : value(integer)
	{
        type = ExprKind::Literal;
    }
    Expr_Literal(double real) : value(real)
	{
        type = ExprKind::Literal;
    }
    Expr_Literal(Value val) : value(val) {
        type = ExprKind::Literal;
    }
};

struct Expr_Unary : Expr {
    Op_Unary op;
    Expr* operand = NULL;

    Expr_Unary(Op_Unary p_op, Expr* p_operand) : op(p_op), operand(p_operand)
	{
        type = ExprKind::Unary;
        flags = p_operand->flags;
    }
};

struct Expr_Binary : Expr {
    Expr* left = NULL;
    Expr* right = NULL;
    Op_Binary op;

    Expr_Binary(Expr* l, Expr* r, Op_Binary p_op) : left(l), right(r), op(p_op)
	{
        type = ExprKind::Binary;
        if (l)
            flags |= l->flags;
        if (r)
            flags |= r->flags;
    }
};

Op_Binary get_binop(Token_Type type);
const char* get_binop_string(Op_Binary op);
bool binop_is_arithmetic(Op_Binary op);
bool binop_is_comparison(Op_Binary op);

struct Expr_Grouping : Expr {
    Expr* expr = NULL;

    Expr_Grouping(Expr* p_expr) : expr(p_expr)
	{
        type = ExprKind::Grouping;
        flags = p_expr->flags;
    }
};

struct Expr_Call : Expr {
    String function_name;  // @note do we need expressions that can return functions? Not currently.
    DArray<Expr*> arguments;
    int fn_id = 0;

    Expr_Call(String f_name, DArray<Expr*>& args, int func_id) : function_name(f_name), arguments(args), fn_id(func_id)
	{
        type = ExprKind::Call;
        for (const auto arg : args)
        {
            flags |= arg->flags;
        }
    }
};

struct Expr_Variable : Expr {
    String name;
    unsigned int var_id = 0;
    String variable_type;
    bool is_builtin = false;

    Expr_Variable(String var_name, int id, String var_type, bool builtin) : name(var_name), var_id(id), variable_type(var_type), is_builtin(builtin)
	{
        type = ExprKind::Variable;
    }
};

struct Expr_Ternary : Expr {
	Expr* condition = nullptr;
	Expr* then_ = nullptr;
	Expr* else_ = nullptr;

	Expr_Ternary(Expr* condition, Expr* then_, Expr* else_) : condition(condition), then_(then_), else_(else_)
	{
		type = ExprKind::Ternary;
		flags |= condition->flags;
		flags |= then_->flags;
		flags |= else_->flags;
	}
};

struct Expr_Tuple : Expr {
	DArray<Expr*> expressions;

	Expr_Tuple(DArray<Expr*>& exprs)
		: expressions(exprs)
	{
		type = ExprKind::Tuple;
		for (const auto expr : expressions)
		{
		  flags |= expr->flags;
		}
	}
};

Expr* collapse_expr(Expr* root, const char** error_string);
void free_tree(Expr* node);

#endif // EXPR_HPP
