#ifndef VEHICLE_SCRIPT_HPP
#define VEHICLE_SCRIPT_HPP

#include "script.hpp"

// functions
int move(lua_State* L);   // (x : float, y : float) -> ()
int lookat(lua_State* L); // (x : float, y : float) -> ()

int getPosition(lua_State* L);  // () -> (x : float, y : float)
int getVelocity(lua_State* L);  // () -> (x : float, y : float)


#endif // VEHICLE_SCRIPT_HPP