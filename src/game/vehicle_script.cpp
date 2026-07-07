#include "vehicle_script.hpp"
#include "vehicle.hpp"

void init_script(Script& s, Vehicle* vehicle)
{
    // @todo
    ASSERT(s.language == ScriptLanguage::LUA);

    lua_State* L = s.data.lua;
    if (!L) return;

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
    lua_pushcfunction(L, lookat);
    lua_setfield(L, -2, "lookat");
    lua_pushcfunction(L, getVelocity);
    lua_setfield(L, -2, "getVelocity");
    lua_pushcfunction(L, getPosition);
    lua_setfield(L, -2, "getPosition");
    lua_setglobal(L, "vehicle");

    lua_pushlightuserdata(L, &s.commands);
    lua_setfield(L, LUA_REGISTRYINDEX, "_commands");

    lua_pushlightuserdata(L, vehicle);
    lua_setfield(L, LUA_REGISTRYINDEX, "_vehicle");
}

// -- lua functions

int move(lua_State* L)
{
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, "_commands");
    DArray<VehicleCommand>* commandList = (DArray<VehicleCommand>*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    VehicleProgram program;
    program.target = cobot::vec2(x,y);

    commandList->add( { CommandMove, program } );

    return 0;
}

int lookat(lua_State* L)
{
    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, "_commands");
    DArray<VehicleCommand>* commandList = (DArray<VehicleCommand>*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    VehicleProgram program;
    program.turnTarget = cobot::vec2(x,y);

    commandList->add( { CommandTurn, program } );

    return 0;
}

int getPosition(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "_vehicle");
    Vehicle* vehicle = (Vehicle*) lua_touserdata(L, -1);

    cobot::vec2 pos = vehicle->worldPosition;

    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);

    return 2;
}

int getVelocity(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "_vehicle");
    Vehicle* vehicle = (Vehicle*) lua_touserdata(L, -1);

    cobot::vec2 vel = vehicle->velocity;

    lua_pushnumber(L, vel.x);
    lua_pushnumber(L, vel.y);

    return 2;
}
