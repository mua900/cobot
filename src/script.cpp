#include "script.hpp"
#include "log.hpp"

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

lua_State* init_lua(VehicleProgram* program)
{
    lua_State* L = luaL_newstate();
    if (!L) return nullptr;
    luaL_openselectedlibs(L, LUA_MATHLIBK | LUA_TABLIBK | LUA_STRLIBK, 0);

    lua_newtable(L);
    lua_pushcfunction(L, move);
    lua_setfield(L, -2, "move");
    lua_newtable(L);
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "y");
    lua_setfield(L, -2, "target");
    lua_pushcfunction(L, move);
    lua_setfield(L, -2, "move");
    lua_setglobal(L, "vehicle");

    lua_pushlightuserdata(L, program);
    lua_setfield(L, LUA_REGISTRYINDEX, "program");

    return L;
}

// -- lua functions

int move(lua_State* L)
{
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, "program");
    VehicleProgram* program = (VehicleProgram*) lua_topointer(L, -1);
    log_info("%p", program);

    return 0;
}
