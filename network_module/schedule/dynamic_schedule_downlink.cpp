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

#include "../debug_settings.h"
#include "dynamic_schedule_downlink.h"
#include "schedule_context.h"
#include "../timesync/timesync_downlink.h"

using namespace miosix;

namespace mxnet {

void DynamicScheduleDownlinkPhase::execute(long long slotStart) {
    auto arrivalTime = slotStart + (ctx.getHop() - 1) * rebroadcastInterval;
    auto wakeupTimeout = timesync->getWakeupAndTimeout(arrivalTime);
    if (ENABLE_SCHEDULE_DL_INFO_DBG)
        print_dbg("[S] WU=%lld TO=%lld\n", wakeupTimeout.first, wakeupTimeout.second);
    //Transceiver configured with non strict timeout
    transceiver.configure(ctx.getTransceiverConfig());
    transceiver.turnOn();
    bool success = false;
    auto now = getTime();
    //check if we skipped the synchronization time
    if (now + timesync->getReceiverWindow() >= arrivalTime) {
        if (ENABLE_FLOODING_ERROR_DBG)
            print_dbg("[S] started too late\n");
        return;
    }
    if(now < wakeupTimeout.first)
        pm.deepSleepUntil(wakeupTimeout.first);
    ledOn();
    for (; !(success || rcvResult.error == miosix::RecvResult::TIMEOUT);
            success = rcvResult.error == miosix::RecvResult::OK) {
        try {
            //uncorrected TS needed for computing the correction with flopsync
            rcvResult = transceiver.recv(packet.data(), MediumAccessController::maxPktSize, wakeupTimeout.second);
        } catch(std::exception& e) {
            if (ENABLE_RADIO_EXCEPTION_DBG)
                print_dbg("%s\n", e.what());
        }
        if (ENABLE_PKT_INFO_DBG) {
            if(rcvResult.size) {
                print_dbg("Received packet, error %d, size %d, timestampValid %d: ", rcvResult.error, rcvResult.size, rcvResult.timestampValid);
                if (ENABLE_PKT_DUMP_DBG)
                    memDump(packet.data(), rcvResult.size);
            } else print_dbg("No packet received, timeout reached\n");
        }
    }
    
    transceiver.idle(); //Save power waiting for rebroadcast time
    
    if (!success) return;

    //This conversion is really necessary to get the corrected time in NS, to pass to transceiver
    //Rebroadcast the sync packet
    rebroadcast(rcvResult.timestamp);
    transceiver.turnOff();
    auto dSctx = dynamic_cast<DynamicScheduleContext*>(scheduleContext);
    auto data = std::vector<unsigned char>(packet.begin(), packet.begin() + rcvResult.size);
    dSctx->parseSchedule(data);
}

void DynamicScheduleDownlinkPhase::rebroadcast(long long rcvTime) {
    if(ctx.getHop() >= networkConfig->getMaxHops()) return;
    try {
        transceiver.sendAt(packet.data(), packet.size(), rcvTime + rebroadcastInterval);
    } catch(std::exception& e) {
        if (ENABLE_RADIO_EXCEPTION_DBG)
            print_dbg("%s\n", e.what());
    }
}

}
