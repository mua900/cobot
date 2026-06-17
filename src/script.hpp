#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"

#include "language/lang.hpp"

#include "lua.hpp"

struct VehicleProgram {
    cobot::vec2 target = {};
    cobot::vec2 turnTarget = {};
};

enum CommandType {
    CommandMove, CommandTurn,
};

struct VehicleCommand {
    CommandType type = {};
    VehicleProgram program = {};
};

enum class ScriptLanguage {
    LUA,
    LANGUAGE,  // @todo find a name for this
};

struct Script {
    ScriptLanguage language;
    String_Builder script;

    union {
        lua_State* lua;
        Interp* interp;
    } data = {};

    List<VehicleCommand> commands = {};

    Script() : language(ScriptLanguage::LUA), data{} {}
    Script(ScriptLanguage lang) : language(lang) {}
    Script(lua_State* lua) : language(ScriptLanguage::LUA)
    {
        data.lua = lua;
    }
    Script(Interp* interpreter) : language(ScriptLanguage::LANGUAGE) {
        data.interp = interpreter;
    }

    bool set_source(ScriptLanguage language, String source);

    void set_program_data(int index);
};

void run_script(Script& s);

// make a lua state
lua_State* init_lua();

// functions
int move(lua_State* L);   // float x, y
int lookat(lua_State* L); // float x, y

#endif // _SCRIPT_H
