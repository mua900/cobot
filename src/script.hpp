#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "common.hpp"

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
    };

    Script() {
        // because you can't default initialize unions in C++.
        memset(this, 0, sizeof(*this));
    }
    bool set_source(ScriptLanguage language, String source);
};

void run_script(Script& s);

#endif // _SCRIPT_H
