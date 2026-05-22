#ifndef _MISSSION_H
#define _MISSION_H

#include "common.hpp"

struct Mission {
    String name = {};
    String objective = {};
    // @todo
    
    String_Builder buffer = {};
};

#endif // _MISSION_H