#ifndef ALIEN_CHARUTILS_H
#define ALIEN_CHARUTILS_H

#include <stdexcept>

#include "u8string.h"
#include "utf8proc.h"

namespace alien::util {

    u8char get_class(u8char c);

    bool isspace(u8char c);

}

#endif //ALIEN_CHARUTILS_H