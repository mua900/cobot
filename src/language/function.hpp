#ifndef FUNCTION_HPP
#define FUNCTION_HPP

#include "util/string_util.hpp"
#include "expr.hpp"

typedef void (*InterpFunction)(const Value* parameters, Value* return_values);

#define VARIABLE_TYPE_REAL    String("real")
#define VARIABLE_TYPE_INTEGER String("int")
#define VARIABLE_TYPE_BOOLEAN String("bool")
#define VARIABLE_TYPE_STRING  String("string")

struct FunctionSignature {
    String name = {};
    DArray<Variable_Type> return_types = {};
    DArray<Variable_Type> parameter_types = {};

    FunctionSignature()
    {}
    FunctionSignature(String n, DArray<Variable_Type> ret_types, DArray<Variable_Type> param_types)
        :
        name(n), return_types(ret_types), parameter_types(param_types)
    {}

    bool operator==(const FunctionSignature& other) const {
        if (return_types.size() != other.return_types.size()) return false;
        if (parameter_types.size() != other.parameter_types.size()) return false;
        for (int i = 0; i < return_types.size(); i++) {
            if (return_types[i] != other.return_types[i]) return false;
        }
        for (int i = 0; i < parameter_types.size(); i++) {
            if (parameter_types[i] != other.parameter_types[i]) return false;
        }
        return  name == other.name;
    }
};

struct Function {
    InterpFunction implementation = nullptr;
    FunctionSignature signature = {};

    Function() {}
    Function(FunctionSignature sign) : signature(sign) {}
    Function(InterpFunction impl, FunctionSignature sign) : implementation(impl), signature(sign) {}
};

// results should be an array of length of func.signature.return_types.size
void call_function(Function func, const Value* parameters, Value* results);

// --- Builtin functions

// to add a new function:
// Add an enum entry here.
// Go to get_function_id function and add the name of your function to be matched and return your enum entry on a match.
// Implement a function with the signature (name is convention, replace the func_name part) st_func_name(Value* inputs, Value* outputs).
// Go to get_default_builtin_functions function and add the implementation of your function to the list. It needs a signature with typed input and outputs. You will get the idea if you look at the function.
// And it would be nice if you also add the name of your function to get_builtin_function_name as well.
enum Builtin_Func_Type {
    BUILTIN_FUNC_EXP,
    BUILTIN_FUNC_ABS,
    BUILTIN_FUNC_SIGN,
    BUILTIN_FUNC_CEIL,
    BUILTIN_FUNC_FLOOR,
    BUILTIN_FUNC_SIN,
    BUILTIN_FUNC_COS,
    BUILTIN_FUNC_TAN,
    BUILTIN_FUNC_ARCSIN,
    BUILTIN_FUNC_ARCCOS,
    BUILTIN_FUNC_ARCTAN,
    BUILTIN_FUNC_FRACT,
    BUILTIN_FUNC_SMOOTHSTEP,
    BUILTIN_FUNC_MIX,
    BUILTIN_FUNC_SAW,
    BUILTIN_FUNC_SQUARE,
    BUILTIN_FUNC_TRIANGLE,
    BUILTIN_FUNC_CLAMP,
    BUILTIN_FUNC_POW,
    BUILTIN_FUNC_LOG,  // natural log
    BUILTIN_FUNC_MIN,
    BUILTIN_FUNC_MAX,

    // @todo variadic functions

    BUILTIN_FUNC_COUNT,

    BUILTIN_FUNC_UNKNOWN,
};

const char* get_builtin_function_name(Builtin_Func_Type builtin);

enum Builtin_Variable : unsigned int {
    BUILTIN_VARIABLE_TIME = 0,

    // avalible if there is any bound input streams
    BUILTIN_VARIABLE_INPUT_SAMPLE = 1,

    BUILTIN_VARIABLE_COUNT
};

enum Builtin_Constants {
    // constant
    BUILTIN_CONSTANT_PI,
	BUILTIN_CONSTANT_TAU,
    BUILTIN_CONSTANT_E,

    BUILTIN_CONSTANT_COUNT,
};

using Function_ID = u32;
using BuiltinVar_ID = u32;

#define BUILTIN_VAR_ID_INVALID (BuiltinVar_ID)(-1)
#define FUNC_ID_INVALID (Function_ID)(-1)

using Builtin_Function_List = Function[BUILTIN_FUNC_COUNT];

// the argument must be an array of pointers of size BUILTIN_FUNC_COUNT
void get_default_builtin_functions(Function* func_list);

bool is_builtin_function(const Expr_Call* call);

Function_ID get_function_id(String name);
double get_builtin_constant(String name);
BuiltinVar_ID get_builtin_var_id(String name);

#endif // FUNCTION_HPP
