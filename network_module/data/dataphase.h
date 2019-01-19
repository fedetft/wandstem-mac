/***************************************************************************
 *   Copyright (C)  2018 by Federico Amedeo Izzo                           *
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

#include "../mac_phase.h"
#include "../mac_context.h"
#include "../schedule/schedule_distribution.h"
#include "../timesync/networktime.h"

namespace mxnet {
/**
 * Represents the data phase, which transfers data from the streams among the nodes
 * in a TDMA way, by following the schedule that has been distributed.
 */


class DataPhase : public MACPhase {
public:
    DataPhase(MACContext& ctx) : MACPhase(ctx) {};
    
    virtual ~DataPhase() {}

    /* Called during data slots, it does the schedule playback,
       in fact, it executes the action written in the explicit schedule slot
       corresponding to the current tile slot */
    virtual void execute(long long slotStart) override;
    /* Needs to be called at every tile slot, when DataPhase::execute() is not
       called, must me called every tile slot even when the node
       is not synchronized, to keep track with the current tile slot */
    void advance(long long slotStart) override {
        nextSlot();
    }
    static unsigned long long getDuration() {
        return packetArrivalAndProcessingTime + transmissionInterval;
    }
    static const int transmissionInterval = 1000000; //1ms
    static const int packetArrivalAndProcessingTime = 5000000;//32 us * 127 B + tp = 5ms
    static const int packetTime = 4256000;//physical time for transmitting/receiving the packet: 4256us
    void sleep();
    void sendFromStream(unsigned int SrcPort);
    void receiveToStream(unsigned int DstPort);
    void sendFromBuffer();
    void receiveToBuffer();
    /* Called from ScheduleDownlinkPhase class on the first downlink slot
     * of the new schedule, to replace the currentSchedule,
     * taking effect in the next dataphase */
    void setSchedule(const std::vector<ExplicitScheduleElement>& newSchedule) { 
        currentSchedule = newSchedule;
    }
    /* Called from ScheduleDownlinkPhase class on the first downlink slot
     * of the new schedule, to set the schedule lenght or DataSuperframeSize */
    void setScheduleTiles(unsigned int newScheduleTiles) {
        scheduleTiles = newScheduleTiles;
        auto slotsInTile = ctx.getSlotsInTileCount();
        scheduleSlots = scheduleTiles * slotsInTile;
    }
    /* Called from ScheduleDownlinkPhase class on the first downlink slot
     * of the new schedule, to set the global time number when the scedule
     * becomes active, useful to know the current dataslot after resync */
    void setScheduleActivationTile(unsigned long newScheduleActivationTile) {
        scheduleActivationTile = newScheduleActivationTile;
    }

    /**
     * Calculates slot number in current schedule (dataSlot) after resyncing
     */
    void alignToNetworkTime(NetworkTime nt);

private:
    void nextSlot() {
        if (++tileSlot >= scheduleSlots) {
            tileSlot = 0;
        }
    }
    unsigned short tileSlot = 0;

    unsigned long scheduleID = 0;
    unsigned long scheduleTiles = 0;
    unsigned long scheduleSlots = 0;
    unsigned long scheduleActivationTile = 0;
    std::vector<ExplicitScheduleElement> currentSchedule;
    Packet buffer;
};

}

