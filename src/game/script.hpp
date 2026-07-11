#ifndef SCRIPT_HPP
#define SCRIPT_HPP

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"

#include "language/lang.hpp"

#include "lua.hpp"

struct VehicleProgram {
    cobot::vec2 target = {};
    cobot::vec2 turnTarget = {};
};

// @todo
enum CommandType {
    CommandMove,
    CommandTurn,
};

struct VehicleCommand {
    CommandType type = {};
    VehicleProgram program = {};

    VehicleCommand() {}
    VehicleCommand(CommandType commandType, VehicleProgram prog)
        :
        type(commandType), program(prog)
    {}
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

    DArray<VehicleCommand> commands = {};
    int activeCommand = 0;

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
    bool is_valid() { return data.lua != nullptr || data.interp != nullptr; }
};

void run_script(Script& s);

#endif // SCRIPT_HPP
