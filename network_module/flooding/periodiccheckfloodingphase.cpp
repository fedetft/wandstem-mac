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

#include "periodiccheckfloodingphase.h"
#include "hookingfloodingphase.h"
#include <stdio.h>
#include "../debug_settings.h"

namespace miosix{
PeriodicCheckFloodingPhase::~PeriodicCheckFloodingPhase() {
}
void PeriodicCheckFloodingPhase::execute(MACContext& ctx)
{   
    syncStatus = ctx.getSyncStatus();
    auto networkConfig = *ctx.getNetworkConfig();
    
    //This is fully corrected
    auto wakeupTimeout = syncStatus->getWakeupAndTimeout(localFirstActivityTime);
    auto timeoutTime = syncStatus->correct(wakeupTimeout.second);
    
    //Transceiver configured with non strict timeout
    transceiver.configure(ctx.getTransceiverConfig());
    unsigned char packet[syncPacketSize];
    
    if (ENABLE_FLOODING_INFO_DBG)
        print_dbg("[F] WU=%lld TO=%lld\n", wakeupTimeout.first, timeoutTime);

    transceiver.turnOn();
    RecvResult result;
    bool success = false;
    
    auto now = getTime();
    //check if we skipped the synchronization time
    if (now >= localFirstActivityTime - syncStatus->receiverWindow) {
        if (ENABLE_FLOODING_ERROR_DBG)
            print_dbg("[F] started too late\n");
        syncStatus->missedPacket();
        return;
    }
    if(now < wakeupTimeout.first)
        pm.deepSleepUntil(wakeupTimeout.first);
    
    ledOn();
    
    for (; !(success || result.error == RecvResult::ErrorCode::TIMEOUT);
            success = isSyncPacket(result, packet, networkConfig.panId, ctx.getHop())) {
        try {
            //uncorrected TS needed for computing the correction with flopsync
            result = transceiver.recv(packet, syncPacketSize, timeoutTime, Transceiver::Unit::NS, Transceiver::Correct::UNCORR);
        } catch(std::exception& e) {
            if (ENABLE_RADIO_EXCEPTION_DBG)
                print_dbg("%s\n", e.what());
        }
        if (ENABLE_PKT_INFO_DBG) {
            if(result.size){
                print_dbg("Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
                if (ENABLE_PKT_DUMP_DBG)
                    memDump(packet, result.size);
            } else print_dbg("No packet received, timeout reached\n");
        }
    }
    
    transceiver.idle(); //Save power waiting for rebroadcast time
    
    if (success) {
        //This conversion is really necessary to get the corrected time in NS, to pass to transceiver
        long long correctedMeasuredFrameStart = syncStatus->correct(result.timestamp);
        measuredGlobalFirstActivityTime = correctedMeasuredFrameStart - packet[2] * rebroadcastInterval;
        packet[2]++;
        //Rebroadcast the sync packet
        if (success) rebroadcast(correctedMeasuredFrameStart, packet, networkConfig.maxHops);
        syncStatus->receivedPacket(result.timestamp);
        transceiver.turnOff();
        if (ENABLE_FLOODING_INFO_DBG)
            print_dbg("[F] RT=%lld\ne=%lld u=%d w=%d rssi=%d\n",
                    result.timestamp,
                    syncStatus->getError(),
                    syncStatus->clockCorrection,
                    syncStatus->receiverWindow,
                   result.rssi);
    } else {
        measuredGlobalFirstActivityTime = syncStatus->computedFrameStart - ctx.getHop() * rebroadcastInterval;
        if (ENABLE_FLOODING_INFO_DBG) {
            print_dbg("[F] miss u=%d w=%d\n", syncStatus->clockCorrection, syncStatus->receiverWindow);
            if (syncStatus->missedPacket() >= maxMissedPackets)
                print_dbg("[F] lost sync\n");
        }
    }
    print_dbg("[F] MGFAT %llu\n", measuredGlobalFirstActivityTime);
    ledOff();
}
}

