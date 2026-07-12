#include "lang.hpp"
#include "parse.hpp"
#include "token.hpp"
#include "statement.hpp"
#include "bytecode.hpp"

#include "util/math_util.hpp"

static const char* interpLastError = nullptr;

struct Interp {
    ProgramTree tree = {};
    Bytecode_Program program = {};
    String_Builder buffer = {};
};

const char* interp_get_last_error() { return interpLastError; }
Interp* interp_create() {
    Interp* interp = new Interp;
    return interp;
}

void interp_destroy(Interp* interp) {
    delete interp;
}

Interp* interp_copy(Interp* interp) {
    Interp* intr = new Interp;
    intr->tree = interp->tree;
    intr->program = interp->program;
    intr->buffer = interp->buffer;
    return intr;
}

bool interp_syntax_check(const char* program_string, int length) {
    Parser parser = Parser();
    String program = String(program_string, length);

    return parser.syntax_check(program);
}

bool interp_set_program(Interp* interp, const char* program_string, int length) {
    Parser parser = Parser();
    String program = String(program_string, length);
    ProgramTree tree = {};
    if (!parser.parse(program, tree)) {
        return false;
    }

    Bytecode_Program bytecode = {};

    if (!bytecode_compile_program(bytecode, tree)) {
        return false;
    }
    interp->tree = tree;
    interp->program = bytecode;
    return true;
}

void interp_run_program(Interp* interp) {
    bytecode_run(interp->program);
}

int interp_register_variable(Interp* interp, const char* name, int length, String type) {
    String symbol = String(name, length);
    symbol = interp->buffer.put_string(symbol);
    Variable var = Variable(symbol, type);

    for (int i = 0; i < interp->program.variables.size(); i++) {
        String var_name = interp->program.variables.get(i).variable.name;
        if (string_compare(var_name, symbol)) {
            interpLastError = "Trying to register the same variable name more than once.";
            return i;
        }
    }

    int var_id = interp->program.add_symbol(var);
    return var_id;
}

bool interp_set_variable_value(Interp* interp, int variable, Value value) {
    if (!interp->program.variables.in_bounds(variable)) {
        interpLastError = "Trying to set a variable out of bounds";
        return false;
    }

    interp->program.variables.get_ref(variable).value = value;
    return true;
}

Value interp_get_variable_value(const Interp* interp, int variable) {
    if (!interp->program.variables.in_bounds(variable)) {
        interpLastError = "Trying to get a variable out of bounds";
        return Value();
    }

    return interp->program.variables.get(variable).value;
}

InterpString interp_get_variable_name(const Interp* interp, int variable) {
    InterpString name = {};
    if (!interp->program.variables.in_bounds(variable)) {
        interpLastError = "Trying to get a variable's name out of bounds";
        return name;
    }

    String n = interp->program.variables.get(variable).variable.name;
    name.data = n.data;
    name.size = n.size;
    return name;
}

int interp_get_variable_count(const Interp* interp) {
    return interp->program.variables.size();
}

int interp_register_function(Interp* interp, const char* name, int length, Variable_Type type) {
    // @todo
    ASSERT(false);
    return 0;
}

bool interp_set_function_callback(Interp* interp, int variable, LangFunction callback) {
    // @todo
    ASSERT(false);
    return 0;
}

LangFunction interp_get_function_callback(const Interp* interp, int variable) {
    // @todo
    ASSERT(false);
    return nullptr;
}

const char* interp_get_function_name_at_index(const Interp* interp, int index) {
    // @todo
    ASSERT(false);
    return nullptr;
}

int interp_get_function_count(const Interp* interp) {
    // @todo
    ASSERT(false);
    return 0;
}

void interp_set_input_stream(Interp* interp, float* input_stream, int input_stream_size, int stride) {
    interp->program.set_input_stream(InputStream(input_stream, input_stream_size, stride));
}

void interp_clear_input_stream(Interp* interp) {
    interp->program.input_stream = InputStream();
}

bool Parser::parse(String program, ProgramTree& tree) {
    cursor = 0;
    tokens = tokenize(program);

    while (cursor < tokens.size()) {
        Statement* stmt = parse_statement();

        if (!stmt) {
            return false;
        }

        tree.statements.add(stmt);
    }

    return true;
}

bool Parser::syntax_check(String string) {
    tokens = tokenize(string);

    while (cursor < tokens.size()) {
        Statement* stmt = parse_statement();
        if (!stmt) return false;
    }

    return true;
}

Statement* Parser::parse_statement() {
    bool builtin = false;

    switch (tokens.get(cursor).type) {
        case TOKEN_TYPE_IF: {
            return parse_if();
        }
        case TOKEN_TYPE_FOR: {
            return parse_for();
        }
        case TOKEN_TYPE_WHILE: {
            return parse_while();
        }
        case TOKEN_TYPE_BUILTIN: {
            ASSERT(consume(TOKEN_TYPE_BUILTIN));
            builtin = true;
        }  // fallthrough
        case TOKEN_TYPE_PROC: {
            return parse_procedure_declaration(builtin);
        }
        case TOKEN_TYPE_TYPE: {
            return parse_type_declaration();
        }
        case TOKEN_TYPE_BRACE_OPEN: {
            return parse_block_statement();
        }
        case TOKEN_TYPE_VAR: {
            return parse_variable_declaration();
        }
        default: {
            Expr* expr = parse_expression();
            if (tokens.get(cursor).type == TOKEN_TYPE_EQUALS) {
                cursor ++;
                Expr* rhs = parse_expression();
                if (!consume(TOKEN_TYPE_SEMICOLON)) {
                    parser_error = Error("Missing ; at the end of assignment", tokens.get(cursor).offset);
                }

                return new StmtAssignment(expr, rhs);
            }
            else {
                return new StmtExpression(expr);
            }
        }
    }
}

StmtBlock* Parser::parse_block_statement() {
    if (!consume(TOKEN_TYPE_BRACE_OPEN)) {
        return nullptr;
    }

    DArray<Statement*> body;
    while (cursor < tokens.size() && tokens.get(cursor).type != TOKEN_TYPE_BRACE_CLOSE)
    {
        Statement* stmt = parse_statement();
        body.add(stmt);
    }

    if (tokens.get(cursor).type != TOKEN_TYPE_BRACE_CLOSE)
    {
        return nullptr;
    }

    cursor ++;

    return new StmtBlock(body);
}

StmtDeclVar* Parser::parse_variable_declaration() {
    if (!consume(TOKEN_TYPE_VAR)) { return nullptr; }

    if (tokens.get(cursor).type != TOKEN_TYPE_IDENT)
    {
        parser_error = Error("Expected variable name after var keyword", tokens.get(cursor).offset);
        return nullptr;
    }

    String name = tokens.get(cursor).token_string;
    cursor ++;

    bool infer_type = false;
    String type_name = {};
    if (tokens.get(cursor).type == TOKEN_TYPE_WALRUS) {
        infer_type = true;
        cursor ++;
    }
    else if (tokens.get(cursor).type == TOKEN_TYPE_COLON) {
        cursor ++;
        if (tokens.get(cursor).type != TOKEN_TYPE_IDENT) {
            return nullptr;
        }
        type_name = tokens.get(cursor).token_string;
        cursor ++;
        if (!consume(TOKEN_TYPE_EQUALS)) {
            return nullptr;
        }
    }
    else {
        return nullptr;
    }

    Expr* rhs = parse_expression();
    if (!rhs)
    {
        return nullptr;
    }

    if (!consume(TOKEN_TYPE_SEMICOLON)) {
        parser_error = Error("Missing ; at the end of variable declaration", tokens.get(cursor).offset);
        return nullptr;
    }

    return new StmtDeclVar(name, rhs, type_name, infer_type);
}

StmtDeclProc* Parser::parse_procedure_declaration(bool builtin) {
    if (!consume(TOKEN_TYPE_PROC)) { return nullptr; }

    if (tokens.get(cursor).type != TOKEN_TYPE_IDENT)
    {
        parser_error = Error("Expeceted procedure name after proc keyword", tokens.get(cursor).offset);
        return nullptr;
    }

    String name = tokens.get(cursor).token_string;
    cursor ++;

    ProcFlags flags = builtin ? PROC_IS_BUILTIN : 0;

    DArray<Parameter> params = {};

    if (!consume(TOKEN_TYPE_PAREN_OPEN)) { return nullptr; }
    while (cursor < tokens.size() && tokens.get(cursor).type != TOKEN_TYPE_PAREN_CLOSE)
    {
        ParameterFlags paramFlags = 0;

        if (tokens.get(cursor).type == TOKEN_TYPE_CONST) {
            paramFlags |= PARAMETER_IS_CONST;
            cursor ++;
        }
        if (tokens.get(cursor).type == TOKEN_TYPE_IN) {
            paramFlags |= PARAMETER_IS_INPUT;
            cursor ++;
        }
        if (tokens.get(cursor).type == TOKEN_TYPE_OUT) {
            paramFlags |= PARAMETER_IS_OUTPUT;
            cursor ++;
        }

        if (!(paramFlags & PARAMETER_IS_INPUT) && !(paramFlags & PARAMETER_IS_OUTPUT)) {
            paramFlags |= PARAMETER_IS_INPUT | PARAMETER_IS_OUTPUT;
        }

        if (tokens.get(cursor).type != TOKEN_TYPE_IDENT) {
            return nullptr;
        }
        String param_name = tokens.get(cursor).token_string;

        if (!consume(TOKEN_TYPE_COLON)) {
            return nullptr;
        }

        Token type_token = tokens.get(cursor);
        if (!is_type_token(type_token.type)) {
            return nullptr;
        }

        if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_CLOSE)
            break;

        if (!consume(TOKEN_TYPE_COMMA)) {
            return nullptr;
        }

        params.add(Parameter(param_name, type_token.token_string, paramFlags));
    }
    if (!consume(TOKEN_TYPE_PAREN_CLOSE)) { return nullptr; }

    if (!consume(TOKEN_TYPE_BRACE_OPEN)) { return nullptr; }

    DArray<Statement*> body;
    while (cursor < tokens.size() && tokens.get(cursor).type != TOKEN_TYPE_BRACE_CLOSE) {
        Statement* stmt = parse_statement();
        if (!stmt) {
            return nullptr;
        }

        body.add(stmt);
    }

    if (!consume(TOKEN_TYPE_BRACE_CLOSE)) { return nullptr; }

    return new StmtDeclProc(name, params, body, flags);
}

StmtDeclType* Parser::parse_type_declaration() {
    if (!consume(TOKEN_TYPE_TYPE)) { return nullptr; }

    if (tokens.get(cursor).type != TOKEN_TYPE_IDENT) {
        return nullptr;
    }

    String name = tokens.get(cursor).token_string;

    TypeFlags flags = 0;

    auto kind = tokens.get(cursor).type;
    if (kind == TOKEN_TYPE_STRUCT) {
        flags |= TypeIsComposite;
    }
    else if (kind == TOKEN_TYPE_ENUM) {
        flags |= TypeIsEnum;
    }

    if (!consume(TOKEN_TYPE_BRACE_OPEN)) {
        return nullptr;
    }

    DArray<Variable> field;
    while (cursor < tokens.size() && tokens.get(cursor).type != TOKEN_TYPE_BRACE_CLOSE) {
        if (tokens.get(cursor).type != TOKEN_TYPE_IDENT) {
            return nullptr;
        }

        Variable variable = {};
        variable.name = tokens.get(cursor).token_string;
        cursor ++;

        if (!consume(TOKEN_TYPE_COLON)) {
            return nullptr;
        }

        if (!is_type_token(tokens.get(cursor).type)) {
            return nullptr;
        }

        variable.type_name = tokens.get(cursor).token_string;
        cursor ++;

        field.add(variable);
    }

    if (!consume(TOKEN_TYPE_BRACE_CLOSE)) {
        return nullptr;
    }

    return new StmtDeclType(name, field, flags);
}

StmtIf* Parser::parse_if() {
    if (!consume(TOKEN_TYPE_IF)) {
        return nullptr;
    }

    Expr* condition = parse_expression();
    if (!condition) {
        return nullptr;
    }

    if (!consume(TOKEN_TYPE_BRACE_OPEN)) {
        return nullptr;
    }

    Statement* then_ = parse_statement();
    if (!then_) return nullptr;
    Statement* else_ = nullptr;
    if (tokens.get(cursor).type == TOKEN_TYPE_ELSE) {
        cursor ++;
        else_ = parse_statement();
        if (!else_) return nullptr;
    }

    return new StmtIf(condition, then_, else_);
}

StmtFor* Parser::parse_for() {
    if (!consume(TOKEN_TYPE_FOR)) {
        return nullptr;
    }

    Expr* start = parse_expression();
    if (!start) return nullptr;
    if (!consume(TOKEN_TYPE_SEMICOLON)) return nullptr;
    Expr* condition = parse_expression();
    if (!condition) return nullptr;
    if (!consume(TOKEN_TYPE_SEMICOLON)) return nullptr;
    Expr* end = parse_expression();
    if (!end) return nullptr;

    Statement* body = parse_statement();

    return new StmtFor(start, condition, end, body);
}

StmtWhile* Parser::parse_while() {
    if (!consume(TOKEN_TYPE_WHILE)) return nullptr;
    Expr* condition = parse_expression();
    if (!condition) return nullptr;

    Statement* body = parse_statement();
    return new StmtWhile(condition, body);
}

Expr* Parser::parse_expression()
{
    Expr* expr = parse_ternary_expr();
    if (expr)
    {
        const char* error_string = nullptr;
        expr = collapse_expr(expr, &error_string);

        if (!expr)
        {
			parser_error.message = error_string;
			parser_error.offset = 0;  // @todo hold location information inside the expression
        }
    }

    return expr;
}

Expr* Parser::parse_ternary_expr() {
	Expr* expr = parse_equality_expr();
	if (!expr) return nullptr;

	if (tokens.get(cursor).type == TOKEN_TYPE_QUESTION_MARK) {
		cursor++;  // ?

		Expr* then_branch = parse_expression();
		if (!then_branch)
		{
            free_tree(expr);
            return nullptr;
		}
		if (tokens.get(cursor).type != TOKEN_TYPE_COLON) {
            parser_error = Error("Expected ':' in ternary expression", tokens.get(cursor).offset);
			free_tree(expr);
			free_tree(then_branch);
			return nullptr;
		}

		cursor++;  // :

		Expr* else_branch = parse_expression();
		if (!else_branch) {
			free_tree(expr);
			free_tree(then_branch);
			return nullptr;
		}

		return new Expr_Ternary(expr, then_branch, else_branch);
	}
	else {
		return expr;
	}
}

Expr* Parser::parse_equality_expr()
{
    Expr* left = parse_comparison_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_EQUALS_EQUALS || type == TOKEN_TYPE_EXCLAMATION_EQUALS)
    {
        cursor++;

        Expr* right = parse_comparison_expr();
        if (!right)
        {
            if (!parser_error.message)
            {
                parser_error = Error("Expected expression after equality operator", tokens.get(cursor).offset);
            }

            free_tree(left);
            return nullptr;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return nullptr;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_comparison_expr()
{
    Expr* left = parse_arithmetic_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_GREATER || type == TOKEN_TYPE_LESS)
    {
        cursor++;

        Expr* right = parse_arithmetic_expr();
        if (!right)
        {
            if (!parser_error.message)
            {
                parser_error = Error("Expected expression after comparison operator", tokens.get(cursor).offset);
            }

            free_tree(left);
            return nullptr;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return nullptr;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_arithmetic_expr()
{
    Expr* left = parse_factor_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_PLUS || type == TOKEN_TYPE_MINUS)
    {
        cursor++;

        Expr* right = parse_factor_expr();
        if (!right)
        {
            if (!parser_error.message)
            {
                parser_error = Error("Expected expression after arithmetic operator", tokens.get(cursor).offset);
            }

            free_tree(left);
            return nullptr;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return nullptr;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_factor_expr()
{
    Expr* left = parse_mod_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_STAR || type == TOKEN_TYPE_SLASH)
    {
        cursor++;

        Expr* right = parse_mod_expr();
        if (!right) {
            if (!parser_error.message)
            {
                parser_error = Error("Expected expression after arithmetic operator", tokens.get(cursor).offset);
            }

			free_tree(left);
            return nullptr;
        }

        Op_Binary op = get_binop(type);
		ASSERT(op != Binop_Unknown);

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_mod_expr() {
	Expr* left = parse_unary_expr();

	Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_PERCENT)
	{
		cursor++;

		Expr* right = parse_unary_expr();
		if (!right) {
            if (!parser_error.message)
            {
                parser_error = Error("Expected expression after mod operator", tokens.get(cursor).offset);
            }

			free_tree(left);
			return nullptr;
		}

		// Op_Binary op = Binop_Mod;
		Op_Binary op = get_binop(type);
		ASSERT(op != Binop_Unknown);

		left = new Expr_Binary(left, right, op);

		type = tokens.get(cursor).type;
	}

	return left;
}

Expr* Parser::parse_unary_expr()
{
    Token_Type type = tokens.get(cursor).type;
    Expr* operand = nullptr;
    while (type == TOKEN_TYPE_MINUS || type == TOKEN_TYPE_EXCLAMATION || type == TOKEN_TYPE_PLUS)
    {
        cursor++;

        operand = parse_unary_expr();
        if (!operand)
        {
            if (!parser_error.message)
            {
                parser_error = Error("Expected expression after unary operator", tokens.get(cursor).offset);
            }
            return nullptr;
        }

        Op_Unary op = (type == TOKEN_TYPE_MINUS) ? Unop_Negate : ((type == TOKEN_TYPE_PLUS) ? Unop_Plus : Unop_Not);
        operand = new Expr_Unary(op, operand);

        type = tokens.get(cursor).type;
    }

    if (operand)
    {
        return operand;
    }
    else {
        return parse_call_expr();
    }
}

Expr* Parser::parse_call_expr()
{
    if (!(tokens.get(cursor).type == TOKEN_TYPE_IDENT && tokens.get(cursor + 1).type == TOKEN_TYPE_PAREN_OPEN))
    {
        return parse_primary_expr();
    }

    String name = tokens.get(cursor).token_string;
    Function_ID fn_id = get_function_id(name);
    if (fn_id == FUNC_ID_INVALID)
    {
        parser_error = Error("Unknown function", tokens.get(cursor).offset);
        return nullptr;
    }

    cursor++;  // identifier (function name)

    // function call
    cursor++;  // (

    DArray<Expr*> arguments;

    int num_open_parens = 1;
    while (num_open_parens != 0)
    {
        if (tokens.get(cursor).type == TOKEN_TYPE_END) {
            parser_error = Error("Missing ')'", tokens.get(cursor).offset);

            arguments.free();
            return nullptr;
        }

        Expr* expression = parse_expression();
        if (!expression) {
            for (auto arg : arguments)
            {
                free_tree(arg);
            }
            arguments.free();
            return nullptr;
        }

        arguments.add(expression);

        if (tokens.get(cursor).type != TOKEN_TYPE_COMMA && tokens.get(cursor).type != TOKEN_TYPE_PAREN_CLOSE) {
            parser_error = Error("Expected ',' to seperate or otherwise ')' to close argument list to function call", tokens.get(cursor).offset);
            arguments.free();

            return nullptr;
        }

        if (tokens.get(cursor).type == TOKEN_TYPE_COMMA) {
            cursor += 1;
        }

        if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_CLOSE) {
            num_open_parens -= 1;
        }
        else if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_OPEN) {
            num_open_parens += 1;
        }
    }

    ASSERT(tokens.get(cursor).type == TOKEN_TYPE_PAREN_CLOSE);
    cursor++; // )

    return new Expr_Call(name, arguments, fn_id);
}

Expr* Parser::parse_primary_expr()
{
    Token token = tokens.get(cursor);
    Token_Type type = token.type;
    switch (type)
    {
        case TOKEN_TYPE_LITERAL_INT:
        {
            cursor++;
            bool success = false;
            long long i = string_to_integer(token.token_string, &success);
            if (!success)
            {
                parser_error.message = "Invalid integer literal";
                parser_error.offset = token.offset;
                return nullptr;
            }
            return new Expr_Literal(i);
        }
        case TOKEN_TYPE_LITERAL_FLOAT:
        {
            cursor++;
            bool success = false;
            double real = string_to_real(token.token_string, &success);
            if (!success)
            {
                parser_error.message = "Invalid float literal";
                parser_error.offset = token.offset;
                return nullptr;
            }
            return new Expr_Literal(real);
        }
        case TOKEN_TYPE_IDENT:
        {
            cursor++;
            BuiltinVar_ID builtin_var_id = get_builtin_var_id(token.token_string);
            double builtin_constant = get_builtin_constant(token.token_string);
            if (builtin_var_id != BUILTIN_VAR_ID_INVALID) {
                // the variable is a builtin

                switch (builtin_var_id) {
                    case BUILTIN_VARIABLE_TIME: {
                        Expr* time = new Expr_Variable(token.token_string, builtin_var_id, VARIABLE_TYPE_REAL, true);

                        return time;
                    }

                    default: {
                        panic("Invalid variable id");  // bug case
                    }
                }
            }
            else if (builtin_constant != 0.0) {
                return new Expr_Literal(builtin_constant);
            }
            else {
                for (int i = 0; i < symbols.size(); i++)
                {
                    Variable var = symbols[i];
                    if (string_compare(token.token_string, var.name)) {
                        return new Expr_Variable(var.name, i, var.type_name, false);
                    }
                }

                // if it's not a known constant, function or variable it's an unknown identifier
                parser_error = Error("Unknown identifier", token.offset);
                return nullptr;
            }
        }
        case TOKEN_TYPE_PAREN_OPEN:
        {
            cursor++;

            Expr* expr = parse_expression();

            if (!expr)
                return nullptr;
            if (!consume(TOKEN_TYPE_PAREN_CLOSE))
            {
                parser_error = Error("Expected ')'", tokens.get(cursor).offset);
                return nullptr;
            }

            return new Expr_Grouping(expr);
        }
        default: {
            parser_error = Error("Expected expression", token.offset);
            return nullptr;
        }
    }
}

Op_Binary get_binop(Token_Type type)
{
    switch (type)
    {
    case TOKEN_TYPE_PLUS:               return Binop_Add;
    case TOKEN_TYPE_MINUS:              return Binop_Sub;
    case TOKEN_TYPE_STAR:               return Binop_Mul;
    case TOKEN_TYPE_SLASH:              return Binop_Div;
    case TOKEN_TYPE_PERCENT:            return Binop_Mod;
    case TOKEN_TYPE_EQUALS_EQUALS:      return Binop_Eq;
    case TOKEN_TYPE_EXCLAMATION_EQUALS: return Binop_Neq;
    case TOKEN_TYPE_GREATER:            return Binop_Gt;
    case TOKEN_TYPE_GREATER_EQUALS:     return Binop_Ge;
    case TOKEN_TYPE_LESS:               return Binop_Lt;
    case TOKEN_TYPE_LESS_EQUALS:        return Binop_Le;
    default:
        return Binop_Unknown;
    }
}

bool binop_is_arithmetic(Op_Binary op) {
    return op >= Binop_Add && op <= Binop_Mod;
}

bool binop_is_comparison(Op_Binary op) {
    return op >= Binop_Eq && op <= Binop_Le;
}

const char* get_binop_string(Op_Binary op)
{
    switch (op)
    {
    case Binop_Unknown: return "Binop_Unknown";
    case Binop_Add: return "Binop_Add";
    case Binop_Sub: return "Binop_Sub";
    case Binop_Mul: return "Binop_Mul";
    case Binop_Div: return "Binop_Div";
    case Binop_Mod: return "Binop_Mod";
    case Binop_Eq: return "Binop_Eq";
    case Binop_Neq: return "Binop_Neq";
    case Binop_Gt: return "Binop_Gt";
    case Binop_Ge: return "Binop_Ge";
    case Binop_Lt: return "Binop_Lt";
    case Binop_Le: return "Binop_Le";
    }

    return NULL;
}

bool is_type_token(Token_Type type)
{
    return type == TOKEN_TYPE_INT || type == TOKEN_TYPE_FLOAT || type == TOKEN_TYPE_BOOL
        || type == TOKEN_TYPE_IDENT;
}

bool Parser::consume(Token_Type type)
{
    bool match = (tokens.get(cursor).type == type);
    if (match)
    {
        cursor++;
    }
    return match;
}

void print_expr(const Expr* expr, int indent);
void print_expression(const Expr* expr){
  print_expr(expr, 0);
}

void print_expr(const Expr* expr, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        printf("    ");
    }

    switch (expr->type)
    {
        case ExprKind::Literal:
        {
            const auto literal = static_cast<const Expr_Literal*>(expr);
            switch (literal->value.type)
            {
                case Var_Type_Integer:
                    printf("Integer Literal: %li\n", literal->value.integer); break;
                case Var_Type_Real:
                    printf("Float Literal: %f\n", literal->value.real); break;
                case Var_Type_Boolean:
                    printf("Boolean Literal: %s\n", BOOL_STRING(literal->value.boolean)); break;
            }
            break;
        }
        case ExprKind::Variable:
        {
            const auto var = static_cast<const Expr_Variable*>(expr);
            SCOPE_STRING(var->name, name);
            printf("%s\n", name);
            break;
        }
        case ExprKind::Unary:
        {
            const auto unary = static_cast<const Expr_Unary*>(expr);
            const char* operator_string = (unary->op == Unop_Negate) ? "Negate" : ((unary->op == Unop_Plus) ? "Plus" : "Not");
            printf("Unary Expression %s\n", operator_string);
            print_expr(unary->operand, indent + 1);
            break;
        }
        case ExprKind::Binary:
        {
            const auto binary = static_cast<const Expr_Binary*>(expr);
            printf("Binary Expression %s\n", get_binop_string(binary->op));
            print_expr(binary->left, indent + 1);
            print_expr(binary->right, indent + 1);
            break;
        }
        case ExprKind::Grouping:
        {
            const auto grouping = static_cast<const Expr_Grouping*>(expr);
            printf("Grouping Expression\n");
            print_expr(grouping->expr, indent + 1);
            break;
        }
        case ExprKind::Call:
        {
            const auto call = static_cast<const Expr_Call*>(expr);
			printf("Call Expression\n");
            SCOPE_STRING(call->function_name, fname);
            printf("%s\n", fname);
            for (int i = 0; i < call->arguments.size(); i++)
            {
                print_expr(call->arguments.get(i), indent + 1);
            }

            break;
        }
        case ExprKind::Ternary: {
            const auto ternary = static_cast<const Expr_Ternary*>(expr);
            printf("Ternary Expression\n");
            print_expr(ternary->condition, indent + 1);
            print_expr(ternary->then_, indent + 2);
            print_expr(ternary->else_, indent + 2);

            break;
        }
        case ExprKind::Tuple: {
            const auto tuple = static_cast<const Expr_Tuple*>(expr);
            printf("Tuple expression\n");
            for (const Expr* e : tuple->expressions) {
                print_expr(e, indent + 1);
            }

            break;
        }
        default: {
            panic("Unknown expression type");
        }
    }
}

BuiltinVar_ID get_builtin_var_id(String name)
{
    if (string_compare(make_string("time"), name) ||
        string_compare(make_string("t"), name)
        )
    {
        return BUILTIN_VARIABLE_TIME;
    }
    else if (string_compare(make_string("sample"), name) ||
             string_compare(make_string("s"), name)
            )
    {
        return BUILTIN_VARIABLE_INPUT_SAMPLE;
    }
    else
    {
        return BUILTIN_VAR_ID_INVALID;
    }
}

// returns 0 if no constant matches
double get_builtin_constant(String name) {
    if (string_compare(name, make_string("pi")) || string_compare(name, make_string("PI"))) {
        return CONSTANT_PI;
    }
    else if (string_compare(name, make_string("e")) || string_compare(name, make_string("E"))) {
        return CONSTANT_E;
    }
	else if (string_compare(name, make_string("TAU")) || string_compare(name, make_string("tau"))) {
		return CONSTANT_TAU;
	}
    else {
        return 0.0;
    }
}

Function_ID get_function_id(String name)
{
    if (string_compare(name, make_string("sin"))) {
        return BUILTIN_FUNC_SIN;
    }
    else if (string_compare(name, make_string("cos"))) {
        return BUILTIN_FUNC_COS;
    }
    else if (string_compare(name, make_string("tan"))) {
        return BUILTIN_FUNC_TAN;
    }
    else if (string_compare(name, make_string("abs"))) {
        return BUILTIN_FUNC_ABS;
    }
    else if (string_compare(name, make_string("sign"))) {
        return BUILTIN_FUNC_SIGN;
    }
    else if (string_compare(name, make_string("asin"))) {
        return BUILTIN_FUNC_ARCSIN;
    }
    else if (string_compare(name, make_string("acos"))) {
        return BUILTIN_FUNC_ARCCOS;
    }
    else if (string_compare(name, make_string("atan"))) {
        return BUILTIN_FUNC_ARCTAN;
    }
    else if (string_compare(name, make_string("ceil"))) {
        return BUILTIN_FUNC_CEIL;
    }
    else if (string_compare(name, make_string("floor"))) {
        return BUILTIN_FUNC_FLOOR;
    }
    else if (string_compare(name, make_string("smoothstep"))) {
        return BUILTIN_FUNC_SMOOTHSTEP;
    }
    else if (string_compare(name, make_string("clamp"))) {
        return BUILTIN_FUNC_CLAMP;
    }
    else if (string_compare(name, make_string("exp"))) {
        return BUILTIN_FUNC_EXP;
    }
    else if (string_compare(name, make_string("log"))) {
        return BUILTIN_FUNC_LOG;
    }
    else if (string_compare(name, make_string("pow"))) {
    	return BUILTIN_FUNC_POW;
    }
	else if (string_compare(name, make_string("fract"))) {
		return BUILTIN_FUNC_FRACT;
	}
	else if (string_compare(name, make_string("mix"))) {
	   return BUILTIN_FUNC_MIX;
	}
	else if (string_compare(name, make_string("saw"))) {
	   return BUILTIN_FUNC_SAW;
	}
	else if (string_compare(name, make_string("square"))) {
	   return BUILTIN_FUNC_SQUARE;
	}
	else if (string_compare(name, make_string("triangle"))) {
	   return BUILTIN_FUNC_TRIANGLE;
	}
    else if (string_compare(name, make_string("max"))) {
        return BUILTIN_FUNC_MAX;
    }
    else if (string_compare(name, make_string("min"))) {
        return BUILTIN_FUNC_MIN;
    }

    return FUNC_ID_INVALID; // @todo user defined functions
}

void free_tree(Expr* node) {
	if (!node) {
		return;
	}

	if (node->type == ExprKind::Binary) {
		free_tree(static_cast<Expr_Binary*>(node)->left);
		free_tree(static_cast<Expr_Binary*>(node)->right);
	}
	else if (node->type == ExprKind::Grouping) {
		free_tree(static_cast<Expr_Grouping*>(node)->expr);
	}
	else if (node->type == ExprKind::Unary) {
		free_tree(static_cast<Expr_Unary*>(node)->operand);
	}
	else if (node->type == ExprKind::Call) {
		auto call = static_cast<Expr_Call*>(node);
		for (auto arg : call->arguments) {
			free_tree(arg);
		}
	}
    else if (node->type == ExprKind::Ternary) {
        auto ternary = static_cast<Expr_Ternary*>(node);
        free_tree(ternary->condition);
        free_tree(ternary->then_);
        free_tree(ternary->else_);
    }
    else if (node->type == ExprKind::Tuple) {
        auto tuple = static_cast<Expr_Tuple*>(node);
        for (auto expr : tuple->expressions) {
            free_tree(expr);
        }
    }

	delete node;
}

Expr* collapse_expr_real(Expr* root, Function* builtin_functions, const char** error_string);
Expr* collapse_expr(Expr* root, const char** error_string)
{
    static bool inited = false;
    static Builtin_Function_List builtin_functions;
    if (!inited)
    {
        get_default_builtin_functions(builtin_functions);
        inited = true;
    }

    return collapse_expr_real(root, builtin_functions, error_string);
}

Expr* collapse_expr_real(Expr* root, Function* builtin_functions, const char** error_string)
{
    Expr* expr = root;
    switch (expr->type)
    {
        case ExprKind::Grouping:
		{
			auto group = static_cast<Expr_Grouping*>(expr);
			Expr* expr = group->expr;
			delete group;
			return collapse_expr_real(expr, builtin_functions, error_string);
		}
        case ExprKind::Binary:
		{
			auto binary = static_cast<Expr_Binary*>(expr);

            ASSERT(binary->left || binary->right);

			if (!binary->left)
			{
                Expr* right = binary->right;
                delete binary;
				return collapse_expr_real(right, builtin_functions, error_string);
			}
			if (!binary->right)
			{
                Expr* left = binary->left;
                delete binary;
				return collapse_expr_real(left, builtin_functions, error_string);
			}

			auto left = collapse_expr_real(binary->left, builtin_functions, error_string);
			auto right = collapse_expr_real(binary->right, builtin_functions, error_string);

            if (!(left && right))
            {
                free_tree(left);
                free_tree(right);
                return nullptr;
            }

            Variable_Type arithmetic_type = (left->result_type == Var_Type_Integer && right->result_type == Var_Type_Integer) ? Var_Type_Integer : Var_Type_Real;

			// the result type depends on the operation
			Variable_Type res_type;
            switch (binary->op)
			{
    			case Binop_Unknown:
    				*error_string = "Unknown binary operator";
                    free_tree(left);
                    free_tree(right);
                    return nullptr;
    			case Binop_Add: res_type = arithmetic_type;  break;
    			case Binop_Sub: res_type = arithmetic_type;  break;
    			case Binop_Mul: res_type = arithmetic_type;  break;
    			case Binop_Div: res_type = arithmetic_type;  break;
    			case Binop_Mod: res_type = arithmetic_type;  break;
    			case Binop_Eq:  res_type = Var_Type_Boolean; break;
    			case Binop_Neq: res_type = Var_Type_Boolean; break;
    			case Binop_Gt:  res_type = Var_Type_Boolean; break;
    			case Binop_Ge:  res_type = Var_Type_Boolean; break;
    			case Binop_Lt:  res_type = Var_Type_Boolean; break;
    			case Binop_Le:  res_type = Var_Type_Boolean; break;
    			default:
    				panic("Unknown binary operator");
			}

            if (is_numeric(left->result_type) && is_numeric(right->result_type))
            {
                if (!is_numeric(res_type))
                {
                    free_tree(left);
                    free_tree(right);

                    *error_string = "Non arithmetic operation is provided numeric arguments";

                    return nullptr;
                }
            }

			binary->left = left;
			binary->right = right;
			binary->result_type = res_type;

			if (left->type == ExprKind::Literal && right->type == ExprKind::Literal)
			{
				Value left_value = static_cast<Expr_Literal*>(left)->value;
				Value right_value = static_cast<Expr_Literal*>(right)->value;

                Op_Binary operation = binary->op;

                free_tree(binary);

				double left_numeric = (left_value.type == Var_Type_Integer) ? left_value.integer : left_value.real;
				double right_numeric = (right_value.type == Var_Type_Integer) ? right_value.integer : right_value.real;

                switch (operation)
				{
				case Binop_Unknown:
					*error_string = "Unknown binary operator";
					return nullptr;
				case Binop_Add:     return new Expr_Literal(left_numeric + right_numeric);
				case Binop_Sub:     return new Expr_Literal(left_numeric - right_numeric);
				case Binop_Mul:     return new Expr_Literal(left_numeric * right_numeric);
				case Binop_Div:     return new Expr_Literal(left_numeric / right_numeric);
				case Binop_Mod:     return new Expr_Literal(fmod(left_numeric, right_numeric));
				case Binop_Eq:      return new Expr_Literal(left_numeric == right_numeric);
				case Binop_Neq:     return new Expr_Literal(left_numeric != right_numeric);
				case Binop_Gt:      return new Expr_Literal(left_numeric > right_numeric);
				case Binop_Ge:      return new Expr_Literal(left_numeric >= right_numeric);
				case Binop_Lt:      return new Expr_Literal(left_numeric < right_numeric);
				case Binop_Le:      return new Expr_Literal(left_numeric <= right_numeric);
				default:
					panic("Unknown binary operator");
				}
			}

			break;
		}
        case ExprKind::Call:
		{
			auto call = static_cast<Expr_Call*>(expr);

			bool all_literals = true;
			bool all_reals = true;
			for (int i = 0; i < call->arguments.size(); i++)
			{
                call->arguments[i] = collapse_expr_real(call->arguments[i], builtin_functions, error_string);
				if (!call->arguments[i])
				{
				    free_tree(call);
				    return nullptr;
				}
				if (call->arguments[i]->type == ExprKind::Literal)
				{
					if (static_cast<Expr_Literal*>(call->arguments[i])->value.type != Var_Type_Real)
					{
						all_reals = false;
					}
				}
				else
				{
					all_literals = false;
				}
			}

			bool computable_now = is_builtin_function(call) && all_literals && all_reals;

			if (is_builtin_function(call))
			{
                Function builtin = builtin_functions[call->fn_id];

				if (call->arguments.size() != builtin.signature.parameter_types.size()) {
				    SCOPE_STRING(builtin.signature.name, function_name);
                    static char error_buffer [128];
                    snprintf(error_buffer, sizeof(error_buffer),
                            "Function %s expects %d arguments but %d given.",
                            function_name,
                            builtin.signature.parameter_types.size(),
                            call->arguments.size());
                    *error_string = error_buffer;
                    free_tree(call);
                    return nullptr;
				}

				int arg_count = call->arguments.size();
				int ret_count = builtin.signature.return_types.size();

				for (int i = 0; i < arg_count; i++)
				{
					if (call->arguments.get(i)->result_type != builtin.signature.parameter_types.get(i))
					{
						*error_string = "Wrong argument type in function call";
						free_tree(call);
						return nullptr;
					}
				}

				if (arg_count == 1 && computable_now) {
					if (call->arguments.get(0)->type == ExprKind::Literal) {
						Expr_Literal* lit = static_cast<Expr_Literal*>(call->arguments.get(0));

						if (ret_count == 1) {
							Value arg = Value(lit->value);
							Value value;
							call_function(builtin, &arg, &value);
							free_tree(call);
							return new Expr_Literal(value);
						}
					}
				}

				if (computable_now)
				{
					DArray<Value> arguments(arg_count);
					for (int i = 0; i < arg_count; i++) {
						// we know that all of them should be literals
						ASSERT(static_cast<Expr_Literal*>(call->arguments.get(i))->type == ExprKind::Literal);
						Value value = static_cast<Expr_Literal*>(call->arguments.get(i))->value;
						arguments.add(value);
					}

					Expr* ret = nullptr;

					if (builtin.signature.return_types.size() == 0) {
                        // keep the node on the tree
                        return call;
					}
					else if (builtin.signature.return_types.size() == 1) {
						Value result = {};
						call_function(builtin, arguments.data(), &result);
						ret = new Expr_Literal(result);
					}
					else {
                        int ret_count = builtin.signature.return_types.size();
                        Value* res = new Value[ret_count];
						call_function(builtin, arguments.data(), res);

						DArray<Expr*> return_expressions(ret_count);
						for (int i = 0; i < ret_count; i++) {
							return_expressions.get_ref(i) = new Expr_Literal(res[i]);
						}

                        delete[] res;

						ret = new Expr_Tuple(return_expressions);
					}

					arguments.reset();
					free_tree(call);
					return ret;
				}
				else {
                    if (builtin.signature.return_types.size() == 1)
                    {
    				    call->result_type = builtin.signature.return_types.get(0);
    					return call;
                    }
                    else
                    {
                        *error_string = "Builtin functions returning more than a single value not implemented";
                        return nullptr;
                    }
				}
			}
			else {
				panic("User defined functions not implemented");
			}

			break;
		}
	    case ExprKind::Literal:
		{
			return expr;
		}
        case ExprKind::Unary:
		{
			Expr_Unary* unary = static_cast<Expr_Unary*>(expr);
			auto operand = collapse_expr_real(unary->operand, builtin_functions, error_string);
			if (!operand)
				return nullptr;
			unary->operand = operand;
			unary->result_type = operand->result_type;

			if (unary->op == Unop_Plus)
			{
                if (operand->result_type != Var_Type_Real && operand->result_type != Var_Type_Integer)
                {
                    *error_string = "Can not use unary plus on non numeric value";
                    delete unary;
                    return nullptr;
                }

                delete unary;
                return operand;
			}

			if (unary->operand->type == ExprKind::Literal)
			{
                Op_Unary unop = unary->op;
				auto literal = static_cast<Expr_Literal*>(unary->operand);

                delete unary;

				switch (unop)
				{
                    case Unop_Negate:
					{
						if (!is_numeric(literal->value.type))
						{
							*error_string = "Can not negate non numeric value";
							return nullptr;
						}

						if (literal->value.type == Var_Type_Integer)
						{
							literal->value.integer = -literal->value.integer;
						}
						else if (literal->value.type == Var_Type_Real)
						{
							literal->value.real = -literal->value.real;
						}

						return literal;
					}
                    case Unop_Not:
					{
						if (literal->value.type != Var_Type_Boolean)
						{
							*error_string = "Can not apply the operator Not to non boolean value";
							return nullptr;
						}

						literal->value.boolean = !literal->value.boolean;
						return literal;
					}
					default:
					   panic("Unhandled unary operation type in collapse_expr");
				}
			}

			break;
		}
        case ExprKind::Variable:
		{
			auto var = static_cast<Expr_Variable*>(expr);
			if (var->var_id == BUILTIN_VAR_ID_INVALID)
			{
				fprintf(stderr, "Internal Error: Invalid variable id in collapse_expr.\n");
				return nullptr;
			}
			return expr;
		}

    	case ExprKind::Ternary:
		{
			auto ternary = static_cast<Expr_Ternary*>(expr);
			auto cond = collapse_expr_real(ternary->condition, builtin_functions, error_string);
			if (!cond) {
				return nullptr;
			}

			if (cond->type == ExprKind::Literal) {
				bool thruth_value = static_cast<Expr_Literal*>(cond)->value.evaluate_truth_value();

				Expr* path = nullptr;
				if (thruth_value) {
					path = collapse_expr_real(ternary->then_, builtin_functions, error_string);
					ternary->then_ = nullptr;
				}
				else {
					path = collapse_expr_real(ternary->else_, builtin_functions, error_string);
					ternary->else_ = nullptr;
				}

				free_tree(ternary);

				return path;
			}

			ternary->condition = cond;

			auto then_ = collapse_expr_real(ternary->then_, builtin_functions, error_string);
			auto else_ = collapse_expr_real(ternary->else_, builtin_functions, error_string);

			if (!(then_ && else_)) {
				return nullptr;
			}

			ternary->then_ = then_;
			ternary->else_ = else_;

			if (then_->result_type != else_->result_type)
			{
                if (!(is_numeric(then_->result_type) && is_numeric(else_->result_type)))
                {
                    *error_string = "Ternary branches must produce results of the same type.";
                    return nullptr;
                }
			}

			ternary->result_type = (then_->result_type == Var_Type_Integer && else_->result_type == Var_Type_Integer) ? Var_Type_Integer : Var_Type_Real;

			return ternary;
		}
        case ExprKind::Tuple:
        {
            auto tuple = static_cast<Expr_Tuple*>(expr);
            for (auto& e : tuple->expressions) {
                e = collapse_expr_real(e, builtin_functions, error_string);
                if (!e)
                {
                    free_tree(tuple);
                    return nullptr;
                }
            }
            // result_type ???
            return tuple;
        }
    }
    return expr;
}

bool Value::evaluate_truth_value() {
    switch (type) {
        case Var_Type_Boolean: return boolean;
        case Var_Type_Integer: return integer != 0;
        case Var_Type_Real: return real != 0.0;
    }

    panic("Unknown value type");
}



DArray<Token> tokenize(String expression)
{
    auto tokens = DArray<Token>(8);

#define ADD_TOKEN(p_token_type, p_token_string) tokens.add(Token(make_string(p_token_string), p_token_type, cursor)); cursor++;

    int cursor = 0;
    while (cursor < expression.size)
    {
        char ch = expression.data[cursor];

        switch (ch)
        {
            case '+':
                ADD_TOKEN(TOKEN_TYPE_PLUS, "+")
                break;
            case '-':
                ADD_TOKEN(TOKEN_TYPE_MINUS, "-")
                break;
            case '*':
                ADD_TOKEN(TOKEN_TYPE_STAR, "*")
                break;
            case '/':
                ADD_TOKEN(TOKEN_TYPE_SLASH, "/")
                break;
            case '%':
                ADD_TOKEN(TOKEN_TYPE_PERCENT, "%")
                break;
            case ',':
                ADD_TOKEN(TOKEN_TYPE_COMMA, ",")
                break;
            case ':':
                ADD_TOKEN(TOKEN_TYPE_COLON, ":")
                break;
            case ';':
                ADD_TOKEN(TOKEN_TYPE_SEMICOLON, ";")
                break;
            case '&':
                ADD_TOKEN(TOKEN_TYPE_AMPERSAND, "&")
                break;
            case '(':
                ADD_TOKEN(TOKEN_TYPE_PAREN_OPEN, "(")
                break;
            case ')':
                ADD_TOKEN(TOKEN_TYPE_PAREN_CLOSE, ")")
                break;
            case '{':
                ADD_TOKEN(TOKEN_TYPE_BRACE_OPEN, "{")
                break;
            case '}':
                ADD_TOKEN(TOKEN_TYPE_BRACE_CLOSE, "}")
                break;
            case '?':
				ADD_TOKEN(TOKEN_TYPE_QUESTION_MARK, "?")
				break;
            case '!': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_EXCLAMATION_EQUALS, "!=")
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_EXCLAMATION, "!")
                }
                break;
            }
            case '=': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_EQUALS_EQUALS, "==")
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_EQUALS, "=")
                }
                break;
            }
            case '>': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_GREATER_EQUALS, ">=");
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_GREATER, ">")
                }
                break;
            }
            case '<': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_LESS_EQUALS, "<=");
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_LESS, "<");
                }
                break;
            }
            default:
            {
                if (is_space(ch))
                {
                    while (is_space(ch))
                    {
                        ch = expression.data[cursor];
                        cursor++;
                    }
                    continue;
                }

                const auto is_ident_character = [](char ch){return is_alpha(ch) || ch == '_';};
                if (is_ident_character(ch))
                {
                    int start = cursor;
                    while (is_ident_character(ch) || is_digit(ch))
                    {
                        cursor++;
                        ch = expression.data[cursor];
                    }

                    String ident = string_slice(expression, start, cursor);
                    String builtin_ = String("builtin");
                    String proc_ = String("proc");
                    String if_ = String("if");
                    String else_ = String("else");
                    String for_ = String("for");
                    String while_ = String("while");
                    String var_ = String("var");
                    String type_ = String("type");
                    String struct_ = String("struct");
                    String enum_ = String("enum");
                    String int_ = String("int");
                    String float_ = String("float");
                    String bool_ = String("bool");

                    if (string_compare(builtin_, ident)) {
                        tokens.add(Token(builtin_, TOKEN_TYPE_BUILTIN, cursor));
                    }
                    else if (string_compare(proc_, ident)) {
                        tokens.add(Token(proc_, TOKEN_TYPE_PROC, cursor));
                    }
                    else if (string_compare(if_, ident)) {
                        tokens.add(Token(if_, TOKEN_TYPE_IF, cursor));
                    }
                    else if (string_compare(else_, ident)) {
                        tokens.add(Token(else_, TOKEN_TYPE_ELSE, cursor));
                    }
                    else if (string_compare(for_, ident)) {
                        tokens.add(Token(for_, TOKEN_TYPE_FOR, cursor));
                    }
                    else if (string_compare(while_, ident)) {
                        tokens.add(Token(while_, TOKEN_TYPE_WHILE, cursor));
                    }
                    else if (string_compare(var_, ident)) {
                        tokens.add(Token(var_, TOKEN_TYPE_VAR, cursor));
                    }
                    else if (string_compare(type_, ident)) {
                        tokens.add(Token(type_, TOKEN_TYPE_TYPE, cursor));
                    }
                    else if (string_compare(struct_, ident)) {
                        tokens.add(Token(struct_, TOKEN_TYPE_STRUCT, cursor));
                    }
                    else if (string_compare(enum_, ident)) {
                        tokens.add(Token(enum_, TOKEN_TYPE_ENUM, cursor));
                    }
                    else if (string_compare(int_, ident)) {
                        tokens.add(Token(int_, TOKEN_TYPE_INT, cursor));
                    }
                    else if (string_compare(float_, ident)) {
                        tokens.add(Token(float_, TOKEN_TYPE_FLOAT, cursor));
                    }
                    else if (string_compare(bool_, ident)) {
                        tokens.add(Token(bool_, TOKEN_TYPE_BOOL, cursor));
                    }
                    else {
                        tokens.add(Token(ident, TOKEN_TYPE_IDENT, cursor));
                    }
                }
                else if (is_digit(ch))
                {
                    int start = cursor;
                    while (is_digit(ch)) { cursor++; ch = expression.data[cursor]; }

                    if (ch == '.')
                    {
                        cursor++;
                        ch = expression.data[cursor];
                        while (is_digit(ch)) { cursor++; ch = expression.data[cursor]; }
                        String ident = string_slice(expression, start, cursor);
                        tokens.add(Token(ident, TOKEN_TYPE_LITERAL_FLOAT, cursor));
                    }
                    else
                    {
                        String ident = string_slice(expression, start, cursor);
                        tokens.add(Token(ident, TOKEN_TYPE_LITERAL_INT, cursor));
                    }
                }
                else if (ch == '"')
                {
                    int start = cursor + 1;
                    cursor++;
                    while (ch != '"')
                    {
                        if (cursor >= expression.size)
                        {
                            fprintf(stderr, "Unterminated string literal\n");
                            break;
                        }
                        cursor++;
                        ch = expression.data[cursor];
                    }

                    String s_literal = string_slice(expression, start, cursor);
                    tokens.add(Token(s_literal, TOKEN_TYPE_LITERAL_STRING, start));
                }
                else {
                    fprintf(stderr, "Unknown character %c", ch);
                    cursor++;
                }
            }
        }
    }

    tokens.add(Token(make_string("END"), TOKEN_TYPE_END, cursor));
    tokens.add(Token(make_string("END"), TOKEN_TYPE_END, cursor));

    return tokens;
}
