/***************************************************************************
 *   Copyright (C)  2018 by Polidori Paolo              *
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


#include "assignmentphase.h"
#include "../flooding/syncstatus.h"

namespace miosix {
    AssignmentPhase::~AssignmentPhase() {
        delete forwardIds;
        delete forwardSlots;//TODO maybe useful elsewhere
    }

    void AssignmentPhase::receiveOrGetEmpty(MACContext& ctx) {
        auto* syncStatus = ctx.getSyncStatus();
        auto wakeUpTimeout = syncStatus->getWakeupAndTimeout(localFirstActivityTime);
        auto panId = ctx.getMediumAccessController().getPanId();
        auto hop = ctx.getHop();
        auto now = getTime();
        if(now >= localFirstActivityTime - syncStatus->receiverWindow)
            printf("AP start late\n");
        if (now < wakeUpTimeout.first)
            pm.deepSleepUntil(wakeUpTimeout.first);
        ledOn();
        RecvResult result;
        for(bool success = false; !(success || result.error == RecvResult::ErrorCode::TIMEOUT);
                success = isAssignmentPacket(result, panId, hop))
        {
            try {
                result = transceiver.recv(packet.data(), AssignmentPhase::assignmentPacketSize, wakeUpTimeout.second);
            } catch(std::exception& e) {
    #ifdef ENABLE_RADIO_EXCEPTION_DBG
                printf("%s\n", e.what());
    #endif /* ENABLE_RADIO_EXCEPTION_DBG */
            }
    #ifdef ENABLE_PKT_INFO_DBG
            if(result.size){
                printf("[RTT] Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
    #ifdef ENABLE_PKT_DUMP_DBG
                memDump(packet, result.size);
    #endif /* ENABLE_PKT_DUMP_DBG */
            } else printf("[RTT] No packet received, timeout reached\n");
    #endif /* ENABLE_PKT_INFO_DBG */
        }
        if(result.error != RecvResult::ErrorCode::OK) {//looks like i lost the connection to the parent hop. Let's broadcast about that.
            getEmptyPkt(panId, hop);
            measuredActivityTime = localFirstActivityTime;
        } else {
            measuredActivityTime = result.timestamp;
            packet[2] = hop;
        }
    }

    void AssignmentPhase::forwardPacket() {
        //Sending synchronization start packet
        //TODO what's better, a causal consistency or a previously measured time?
        long long sendTime = measuredActivityTime + retransmissionDelay;
        try {
            transceiver.sendAt(packet.data(), sizeof(packet), sendTime);
        } catch(std::exception& e) {
#ifdef ENABLE_RADIO_EXCEPTION_DBG
            printf("%s\n", e.what());
#endif /* ENABLE_RADIO_EXCEPTION_DBG */
        }
#ifdef ENABLE_FLOODING_INFO_DBG
        printf("Sync packet sent at %lld\n", sendTime);
#endif /* ENABLE_FLOODING_INFO_DBG */
    }
    
     void AssignmentPhase::processPacket() {
        slotCount = 0;
        for (unsigned char i = 1, j = 7, k = 0; j < assignmentPacketSize; i+=2, j++) {
            if (packet[j] & 0xF0){
                if (i == (*forwardIds)[k]) {
                    forwardSlots->push_back(slotCount);
                    k++;
                } else if (i == myId) {
                    mySlot = slotCount;
                    k++;
                }
                slotCount++;
            }
            if (packet[j] & 0x0F){
                auto nibble = i + 1;
                if (nibble == (*forwardIds)[k]) {
                    forwardSlots->push_back(slotCount);
                    k++;
                } else if (nibble == myId) {
                    mySlot = slotCount;
                    k++;
                }
                slotCount++;
            }
        }
        processed = true;
        return;
    }

}

