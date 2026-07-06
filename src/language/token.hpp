#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "common.hpp"
#include "template.hpp"

enum Token_Type {
    TOKEN_TYPE_UNKNOWN = 0,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_PERCENT,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_WALRUS,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_AMPERSAND,
    TOKEN_TYPE_PAREN_OPEN,
    TOKEN_TYPE_PAREN_CLOSE,
    TOKEN_TYPE_BRACE_OPEN,
    TOKEN_TYPE_BRACE_CLOSE,
	TOKEN_TYPE_QUESTION_MARK,

    TOKEN_TYPE_EXCLAMATION,
    TOKEN_TYPE_EQUALS,
    TOKEN_TYPE_GREATER,
    TOKEN_TYPE_LESS,

    TOKEN_TYPE_EXCLAMATION_EQUALS,
    TOKEN_TYPE_EQUALS_EQUALS,
    TOKEN_TYPE_GREATER_EQUALS,
    TOKEN_TYPE_LESS_EQUALS,

    TOKEN_TYPE_IDENT,
    TOKEN_TYPE_LITERAL_INT,
    TOKEN_TYPE_LITERAL_FLOAT,
    TOKEN_TYPE_LITERAL_STRING,

    TOKEN_TYPE_BUILTIN,
    TOKEN_TYPE_PROC,
    TOKEN_TYPE_IF,
    TOKEN_TYPE_ELSE,
    TOKEN_TYPE_FOR,
    TOKEN_TYPE_WHILE,
    TOKEN_TYPE_VAR,
    TOKEN_TYPE_TYPE,
    TOKEN_TYPE_STRUCT,
    TOKEN_TYPE_ENUM,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_CONST,
    TOKEN_TYPE_IN,
    TOKEN_TYPE_OUT,

    TOKEN_TYPE_END,
};

struct Token {
    String token_string = {};
    Token_Type type = TOKEN_TYPE_UNKNOWN;
    int offset = 0;

    Token() {}
    Token(String s, Token_Type t_type, int p_offset) : token_string(s), type(t_type), offset(p_offset) {}
};

void print_token(Token& token, String_Builder* sb, bool ignore_some=false);
const char* get_token_type_string(Token_Type type);

DArray<Token> tokenize(String expression);

#endif // TOKEN_HPP
