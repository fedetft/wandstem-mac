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

#include "master_topology_discovery_phase.h"
#include "topology_context.h"
#include "../debug_settings.h"
#include <limits>

namespace miosix {

MasterTopologyDiscoveryPhase::~MasterTopologyDiscoveryPhase() {
    // TODO Auto-generated destructor stub
}

void MasterTopologyDiscoveryPhase::execute(MACContext& ctx) {
    if (ENABLE_TOPOLOGY_INFO_DBG)
        print_dbg("[T] GFAT=%llu\n", globalFirstActivityTime);
    unsigned char packet[ctx.maxPacketSize];
    transceiver.configure(ctx.getTransceiverConfig());
    auto* cfg = ctx.getNetworkConfig();
    auto* topology = ctx.getTopologyContext();
    RecvResult result;
    transceiver.turnOn();
    for (unsigned short nodeId = cfg->maxNodes - 1; nodeId > 0; nodeId--) {
        try {
            result = transceiver.recv(packet, ctx.maxPacketSize,
                    getNodeTransmissionTime(nodeId) + MediumAccessController::maxPropagationDelay + packetTime + MediumAccessController::maxAdmittableResyncReceivingWindow);
        } catch(std::exception& e) {
            if (ENABLE_RADIO_EXCEPTION_DBG)
                print_dbg("%s\n", e.what());
        }
        if (ENABLE_PKT_INFO_DBG) {
            if(result.size) {
                print_dbg("Received packet, error %d, size %d, timestampValid %d: ",
                        result.error, result.size, result.timestampValid);
                if (ENABLE_PKT_DUMP_DBG)
                    memDump(packet, result.size);
            } else print_dbg("No packet received, timeout reached\n");
        }
        if (result.error == RecvResult::ErrorCode::OK) {
            topology->receivedMessage(packet, result.size * std::numeric_limits<unsigned char>::digits, nodeId, result.rssi);
            if (ENABLE_TOPOLOGY_INFO_DBG)
                print_dbg("[T] <- N=%u @%llu\n", nodeId, result.timestamp);
        } else {
            topology->unreceivedMessage(nodeId);
        }
    }
    if (ENABLE_TOPOLOGY_INFO_DBG) {
        dynamic_cast<MasterMeshTopologyContext*>(topology)->print();
    }
}

} /* namespace miosix */