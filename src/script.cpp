#include "script.hpp"

void run_script(Script& s)
{
    if (s.language == ScriptLanguage::LUA)
    {
        int status = luaL_dostring(s.lua, s.script.c_string());
    }
    else if (s.language == ScriptLanguage::LANGUAGE)
    {
        ASSERT("Not implemented");
    }
}