#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "common.hpp"
#include "math_util.hpp"

#include "language/lang.hpp"

#include "lua.hpp"


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

    Script() : language(ScriptLanguage::LUA), data{} {}
    Script(lua_State* lua) : language(ScriptLanguage::LUA)
    {
        data.lua = lua;
    }
    Script(Interp* interpreter) : language(ScriptLanguage::LANGUAGE) {
        data.interp = interpreter;
    }
    bool set_source(ScriptLanguage language, String source);
};

void run_script(Script& s);

struct VehicleProgram {
    vec2 target = {};
};

// make a lua state
lua_State* init_lua();

// functions
int move(lua_State* L);

#endif // _SCRIPT_H
