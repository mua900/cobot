#pragma once

#include "lang_common.hpp"
#include "lang.hpp"
#include "statement.hpp"
#include "function.hpp"

#include "util/common.hpp"
#include "util/template.hpp"

/*
Instruction layout:
	opcode(16 bit), operand0(16 bit), operand1(16 bit), operand2(16 bit)
	64 bit fixed length instructions.

	All instructions take 3 arguments but they aren't required to use any of them.
	The arguments can be registers or identifiers for known constants or functions.

Registers:
	The registers are seperated into independent integer and floating point files.
	There are no hard limits on register file size.

Constant Block:
	The constant values used in the program that doesn't fit into the instruction.
	There is a constant block created when compiling the program that contains all constants used in the program.
	They are accessed via instructions that load values from the constant block into registers.
*/

enum Bytecode_Opcode : u16
{
	// destination source syntax
	// arithmetic operations are destructive and overwrite the first argument register
	INSTR_LOAD,			// load reg, const_int
	INSTR_LOADF,		// load freg, const_float
	INSTR_LOAD_BUILTIN,	// load_builtin freg builtin_id
	INSTR_LOAD_VAR,		// load_var freg var_id
	INSTR_LOAD_SAMPLE,  // load_sample freg
	INSTR_LOAD_I_TO_F,  // load freg, const_int
	INSTR_LOAD_F_TO_I,  // load reg, const_float

	INSTR_MOV,			// mov  reg, reg
	INSTR_MOVF,			// mov  freg, freg
	INSTR_MOV_I_TO_F,	// mov  freg, reg
	INSTR_MOV_F_TO_I,	// mov  reg, freg

	// -- fused multiply add, computes (a x b) + c and stores it in a
	INSTR_FMA,          // fma freg(a), freg(b), freg(c)

	// -- binary operations, the result of operation(b, c) is stored in a

	INSTR_ADD,			// add reg(a), reg(b), reg(c)
	INSTR_SUB,			// sub reg(a), reg(b), reg(c)
	INSTR_MUL,			// mul reg(a), reg(b), reg(c)
	INSTR_DIV,			// div reg(a), reg(b), reg(c)
	INSTR_MOD,			// mod reg(a), reg(b), reg(c)

	INSTR_ADDF,			// add freg(a), freg(b), freg(c)
	INSTR_SUBF,			// sub freg(a), freg(b), freg(c)
	INSTR_MULF,			// mul freg(a), freg(b), freg(c)
	INSTR_DIVF,			// div freg(a), freg(b), freg(c)
	INSTR_MODF,			// mod freg(a), freg(b), freg(c)

	INSTR_AND,          // and reg(a), reg(b), reg(c)
	INSTR_OR,           // or reg(a), reg(b), reg(c)
	INSTR_XOR,          // xor reg(a), reg(b), reg(c)

	INSTR_NOT,			// not 	  reg

	INSTR_NEGATE,		// negate reg
	INSTR_NEGATE_F,		// negate freg

	// the result of the comparison is stored in the relevant bits of the flags register.
	INSTR_CMP,			// cmp reg(a), reg(b)
	INSTR_CMPF,			// cmpf freg(a), freg(b)

	// test if the value stored in the argument register is 0 and set the condition bit to 0 if it is and 1 if it is not.
	INSTR_TEST,         // test reg
	INSTR_TEST_F,       // test freg

	// commit the result of the previous generic compare operation for a specific case to the condition bit
	INSTR_TEST_RESULT,	// test immediate

	// jump
	INSTR_JMP,			// jmp  address_low, address_high
	// jump if the condition bit is true (1)
	INSTR_JMP_COND,		// cjmp address_low, address_high

	// Call the builtin function identified by func_id and place the result in freg.
	// This is for simple builtin functions that take one argument and return a single value.
	// The register carrying the argument gets passed in the instruction
	// and the register is overwriten with the result of the call to return the value.
	INSTR_CALL_BUILTIN_SIMPLE,	// call_builtin func_id freg
	// calls the function identified by func_id and place the result in the freg
	// the arguments are placed on the stack per call and used by the called function
	INSTR_CALL_BUILTIN,			// call func_id freg
	// calls the function identified by func_id. Read arguments from the stack.
	// and place the results to the stack
	INSTR_CALL_BUILTIN_STACK_RETURN,  // call func_id

	// the same as above but the called function is also in bytecode and the return location and flags (in that order) are saved to stack
	INSTR_CALL_SIMPLE,
	INSTR_CALL,
	INSTR_CALL_STACK_RETURN,

	// push a value to the value stack
	INSTR_PUSH,			// push freg
	// pop a value from the value stack
	INSTR_POP,          // pop freg

	// pop the flags and return address (in that order) from the stack and jump to the return address.
	INSTR_RET,			// ret freg

	INSTR_HALT,         // halt

	INSTR_COUNT,

	INSTR_SENTINEL,
};

const char* opcode_string(Bytecode_Opcode opcode);

enum Constant_Type {
	CONSTANT_TYPE_INTEGER,
	CONSTANT_TYPE_REAL,
	CONSTANT_TYPE_BUILTIN,
};

struct Constant_Id {
	Constant_Type constant_type;
	u16 constant_index;
};

using Func_Id = u16;

enum Value_Location_Type {
	INTEGER_REGISTER,
	REAL_REGISTER,
};

struct Value_Location_Info {
	u32 integer_register = 0;
	u32 real_register = 0;
	Constant_Id const_id = {};

	Value_Location_Type location_type = {};
};

struct Bytecode_Instr {
	Bytecode_Opcode opcode = {};
	u16 op0 = 0;
	u16 op1 = 0;
	u16 op2 = 0;

	Bytecode_Instr() {}
	Bytecode_Instr(Bytecode_Opcode p_opcode)
		: opcode(p_opcode)
	{}

	Bytecode_Instr(Bytecode_Opcode p_opcode, u16 operand)
		: opcode(p_opcode), op0(operand)
	{}

	Bytecode_Instr(Bytecode_Opcode p_opcode, u16 operand_0, u16 operand_1)
		: opcode(p_opcode), op0(operand_0), op1(operand_1)
	{}

	Bytecode_Instr(Bytecode_Opcode p_opcode, u16 operand_0, u16 operand_1, u16 operand_2)
		: opcode(p_opcode), op0(operand_0), op1(operand_1), op2(operand_2)
	{}
};

static_assert(sizeof(Bytecode_Instr) == 8);

struct Constant_Block {
	DArray<double> real = {};
	DArray<s64> integer = {};
	double time = 0.0;
	long sample_rate = 0;
	long sample_index = 0;
	Builtin_Function_List builtin_function = {};

	Constant_Id add_constant(Value value);
};

struct Bytecode_Code {
	DArray<Bytecode_Instr> code;

	u32 index() const {
		return code.size() - 1;
	}

	u32 size() const {
		return code.size();
	}
};

#define CONDITION_RESULT				BIT(0)

#define COMPARISON_RESULT_EQUALS		BIT(4)
#define COMPARISON_RESULT_NOT_EQUALS	BIT(5)
#define COMPARISON_RESULT_GREATER_THAN	BIT(6)
#define COMPARISON_RESULT_LESS_THAN		BIT(7)

struct Bytecode_Processor {
	DArray<s64> regs = {};
	DArray<double> fregs = {};

	DArray<Value> stack = {};

	u32 result_flags = 0;

	Bytecode_Processor()
		: regs(8), fregs(8)
	{}
};

struct InputStream {
	float* samples = nullptr;
	size_t sample_count = 0;
	int stride = 0;
	int sample_index = 0;

	InputStream() {}
	InputStream(float* p_samples, size_t p_sample_count, int p_stride)
		:
		samples(p_samples), sample_count(p_sample_count), stride(p_stride)
	{}
};

struct Bytecode_Function {
	String name = {};
	Bytecode_Code code = {};
};

struct Var {
	Variable variable = {};
	Value value = {};

	Var() {}
	Var(Variable v) : variable(v) {}
};

struct Bytecode_Program {
	Bytecode_Processor processor = {};
	Bytecode_Code code = {};
	Constant_Block constant_block = {};
	InputStream input_stream = {};

	DArray<Var> variables = {};
	DArray<Bytecode_Function> functions = {};

	Bytecode_Program() : processor(), code(), constant_block() {
		get_default_builtin_functions(constant_block.builtin_function);
	}

	void reset();

	u16 allocate_integer_register();
	u16 allocate_float_register();

	void copy_value_to_float_register(Value_Location_Info val_loc, u16 dest_reg);
	u16 get_value_to_float_register(Value_Location_Info val_info);
	u16 get_value_to_integer_register(Value_Location_Info val_info);

	void emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1, u16 arg2);

	Value_Location_Info emit_load_constant(Constant_Id value_id);

    int add_symbol(Variable var) {
        return variables.add(Var(var));
    }

	Func_Id add_function(Bytecode_Function func);

	void set_input_stream(InputStream istream);

	void set_builtin_function(InterpFunction  implementation, u32 builtin_function);

	void print_program() const;
};

bool bytecode_compile_program(Bytecode_Program& bytecode, const ProgramTree& tree);
void bytecode_run(Bytecode_Program& program);
