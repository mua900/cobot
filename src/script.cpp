#include "script.hpp"

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
    luaL_openlibs(L);

    lua_newuserdata(L, 1);
    lua_register(L, "add", add);

    return L;
}

// -- lua functions

int add(lua_State* L)
{
    int a = lua_tointeger(L, 1);
    int b = lua_tointeger(L, 2);

    lua_pushinteger(L, a + b);

    return 1;
}

int forward(lua_State* L)
{
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float delta = lua_tonumber(L, 3);

    float mag = std::sqrtf(x * x + y * y);
    
    x = x + (x / mag) * delta;
    y = y + (y / mag) * delta;

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);

    return 2;
}

int back(lua_State* L)
{
    lua_checkstack(L, 3);

    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    float delta = lua_tonumber(L, 3);

    float mag = std::sqrtf(x * x + y * y);
    
    x = x + (x / mag) * delta;
    y = y + (y / mag) * delta;

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);

    return 2;
}
