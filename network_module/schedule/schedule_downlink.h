/***************************************************************************
 *   Copyright (C)  2017 by Terraneo Federico, Polidori Paolo              *
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

#pragma once

#include "dynamic_schedule_information.h"
#include "../mac_phase.h"
#include "../mac_context.h"
#include "interfaces-impl/transceiver.h"
#include "interfaces-impl/power_manager.h"
#include <utility>

/**
 * Represents the phase in which the schedule is flooded, in order to reach all the nodes and make it available for
 * all of them, so they start operating it when configured.
 */
namespace mxnet {
class ScheduleDownlinkPhase : public MACPhase {
public:
    ScheduleDownlinkPhase() = delete;
    ScheduleDownlinkPhase(const ScheduleDownlinkPhase& orig) = delete;
    virtual ~ScheduleDownlinkPhase() {};
    unsigned long long getDuration() const override {
        return phaseStartupTime + networkConfig.getMaxHops() * rebroadcastInterval;
    }

    unsigned long long getSlotsCount() const override { return networkConfig.getScheduleDownlinkPerSlotframeCount(); }
    bool isScheduleEnd(std::set<DynamicScheduleElement*>::iterator it) const { return nodeSchedule.empty() || nodeSchedule.end() == it; }
    std::set<DynamicScheduleElement*>::iterator getFirstSchedule() { return nodeSchedule.begin(); };
    std::queue<std::vector<unsigned char>>* getQueueForId(unsigned short id) {
        std::map<unsigned short, std::queue<std::vector<unsigned char>>>::iterator retval = forwardQueues.find(id);
        if (retval == forwardQueues.end()) return nullptr;
        return &(retval->second);
    }
    static const int phaseStartupTime = 450000;
    static const int rebroadcastInterval = 5000000; //32us per-byte + 600us total delta
    
protected:
    ScheduleDownlinkPhase(MACContext& ctx) :
        MACPhase(ctx),
        networkConfig(ctx.getNetworkConfig()) {};
    
    const NetworkConfiguration& networkConfig;
    std::set<DynamicScheduleElement*> nodeSchedule;
    std::map<unsigned short, std::queue<std::vector<unsigned char>>> forwardQueues;
};
}

