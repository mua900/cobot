#include "script.hpp"
#include "util/log.hpp"

bool Script::set_source(ScriptLanguage language, String source) {
	commands.discard_data();

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
