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

#ifndef INTERFACES_IMPL_VIRTUAL_CLOCK_H_
#define INTERFACES_IMPL_VIRTUAL_CLOCK_H_

#include "../MiosixInterface.h"
#include "../NodeBase.h"
#include <mutex>
#include <map>

namespace miosix {

class VirtualClock : public MiosixInterface {
public:
    VirtualClock();
    virtual ~VirtualClock();

    /**
     * \return an instance to the virtual clock
     */
    static VirtualClock& instance();
    static void deinstance(NodeBase* ref);

    long long corrected2uncorrected(long long tick){
        return tick;
    }

    long long uncorrected2corrected(long long tick){
        return tick;
    }

    void update(long long baseTheoretical, long long baseComputed, long long clockCorrection) {}

    void setSyncPeriod(unsigned long long syncPeriod) {}

private:

    static std::mutex instanceMutex;
    static std::map<NodeBase*, VirtualClock*> instances;
};

} /* namespace miosix */

#endif /* INTERFACES_IMPL_VIRTUAL_CLOCK_H_ */
