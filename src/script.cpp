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