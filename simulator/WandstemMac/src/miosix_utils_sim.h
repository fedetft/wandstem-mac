/***************************************************************************
 *   Copyright (C)  2018 by Polidori Paolo                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef MIOSIX_UTILS_SIM_H_
#define MIOSIX_UTILS_SIM_H_

#ifndef UNITTEST

#include <omnetpp.h>
#include "MiosixStaticInterface.h"

namespace mxnet {
void print_dbg_(const char *fmt, ...);
#define print_dbg print_dbg_
}

#endif //UNITTEST

namespace miosix {

void ledOn();
void ledOff();
long long getTime();
void memDump(const void *start, int len);

#ifndef UNITTEST

class Thread : public MiosixStaticInterface {
public:
    static void nanoSleep(long long delta);
    static void nanoSleepUntil(long long when);
};

#else //UNITTEST

class Thread {
public:
    static void nanoSleep(long long delta);
    static void nanoSleepUntil(long long when);
};

#endif //UNITTEST

struct Leds {
    static bool greenOn;
    static bool redOn;
    static void print();
};

class greenLed {
public:
    static void high();
    static void low();
};

}



#endif /* MIOSIX_UTILS_SIM_H_ */
