#include "script.hpp"
#include "log.hpp"

bool Script::set_source(ScriptLanguage language, String source) {
    if (language == ScriptLanguage::LANGUAGE) {
        Interp* interp = interp_create();
        if (!interp) return false;
        if (!interp_set_program(interp, source.data, source.size)) return false;

        script.clear_and_append(source);

        return true;
    }
    else if (language == ScriptLanguage::LUA) {
        script.clear_and_append(source);
        return true;
    }
    else {
        ASSERT(false);
        return false;
    }
}

void Script::set_program_data()
{
    if (language == ScriptLanguage::LUA)
    {
        lua_State* L = data.lua;

        lua_pushlightuserdata(L, &program);
        lua_setfield(L, LUA_REGISTRYINDEX, "program");
    }
}

void run_script(Script& s)
{
    if (s.script.cursor == 0)
    {
        // there is no program to run
        return;
    }

    if (s.language == ScriptLanguage::LUA)
    {
        int status = luaL_dostring(s.data.lua, s.script.c_string());
    }
    else if (s.language == ScriptLanguage::LANGUAGE)
    {
        ASSERT("Not implemented");
    }
}

lua_State* init_lua()
{
    lua_State* L = luaL_newstate();
    if (!L) return nullptr;
    luaL_openselectedlibs(L, LUA_MATHLIBK | LUA_TABLIBK | LUA_STRLIBK, 0);

    lua_newtable(L);
    lua_newtable(L);
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "target");
    lua_pushcfunction(L, move);
    lua_setfield(L, -2, "move");
    lua_setglobal(L, "vehicle");

    return L;
}

// -- lua functions

int move(lua_State* L)
{
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, "program");
    VehicleProgram* program = (VehicleProgram*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    log_info("%f, %f", program->target.x, program->target.y);

    return 0;
}
