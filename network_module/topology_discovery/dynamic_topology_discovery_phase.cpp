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

#include "dynamic_topology_discovery_phase.h"
#include "topology_message.h"
#include "../debug_settings.h"
#include "../flooding/syncstatus.h"
#include <limits>

namespace miosix {

DynamicTopologyDiscoveryPhase::~DynamicTopologyDiscoveryPhase() {
    // TODO Auto-generated destructor stub
}

void DynamicTopologyDiscoveryPhase::receiveByNode(MACContext& ctx, unsigned short nodeId) {
    auto* status = ctx.getSyncStatus();
    auto timestampFrom = getNodeTransmissionTime(nodeId);
    auto wakeUpTimeout = status->getWakeupAndTimeout(timestampFrom);
    auto* topology = ctx.getTopologyContext();
    RecvResult result;
    auto now = getTime();
    if (now >= timestampFrom - status->receiverWindow)
        print_dbg("TC start late\n");
    if (now < wakeUpTimeout.first)
        pm.deepSleepUntil(wakeUpTimeout.first);
    if(nodeId < 4)
        print_dbg("[T] expect %u from %llu to %llu\n", nodeId, now, wakeUpTimeout.second - MediumAccessController::packetPreambleTime + packetTime);
    try {
        result = transceiver.recv(packet, ctx.maxPacketSize, wakeUpTimeout.second - MediumAccessController::packetPreambleTime + packetTime);
    } catch(std::exception& e) {
        if (ENABLE_RADIO_EXCEPTION_DBG)
            print_dbg("%s\n", e.what());
    }
    if (ENABLE_PKT_INFO_DBG) {
        if(result.size) {
            print_dbg("Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
            if (ENABLE_PKT_DUMP_DBG)
                memDump(packet, result.size);
        } else print_dbg("No packet received, timeout reached\n");
    }
    //TODO filter by a minimum RSSI
    if (result.error == RecvResult::ErrorCode::OK) {
        topology->receivedMessage(packet, result.size * std::numeric_limits<unsigned char>::digits, nodeId, result.rssi);
        if (ENABLE_TOPOLOGY_INFO_DBG)
            print_dbg("[T] <- N=%u @%llu\n", nodeId, result.timestamp);
    } else {
        topology->unreceivedMessage(nodeId);
    }
}

void DynamicTopologyDiscoveryPhase::sendMyTopology(MACContext& ctx) {
    auto* topology = dynamic_cast<DynamicTopologyContext*>(ctx.getTopologyContext());
    TopologyMessage* msg = topology->getMyTopologyMessage();
    if (msg == nullptr) return; //I still dunno anything about any predecessor
    auto msgdata = msg->getPkt();
    auto size = msg->getSize();
    ctx.bitwisePopulateBitArrTop<unsigned char>(packet, ctx.maxPacketSize, msgdata.data(), msgdata.size(), 0, size);
    auto pktSize = size;
    for (auto* msg : topology->dequeueMessages(cfg->forwardedTopologies)) {
        size = msg->getSize();
        msgdata = msg->getPkt();
        ctx.bitwisePopulateBitArrTop<unsigned char>(packet, ctx.maxPacketSize, msgdata.data(), msgdata.size(), pktSize, size);
        pktSize += size;
    }
    auto time = getNodeTransmissionTime(ctx.getNetworkId());
    if (ENABLE_TOPOLOGY_INFO_DBG)
        print_dbg("[T] N=%u -> @%llu\n", ctx.getNetworkId(), time);
    transceiver.sendAt(packet, (pktSize - 1) / std::numeric_limits<unsigned char>::digits + 1, time);
}

void DynamicTopologyDiscoveryPhase::execute(MACContext& ctx) {
    print_dbg("[TDP] GFAT=%llu\n", globalFirstActivityTime);
    cfg = ctx.getNetworkConfig();
    transceiver.configure(ctx.getTransceiverConfig());
    ledOn();
    transceiver.turnOn();
    for (unsigned short nodeId = cfg->maxNodes - 1; nodeId > 0; nodeId--) {
        if (nodeId == ctx.getNetworkId()) sendMyTopology(ctx);
        else receiveByNode(ctx, nodeId);
    }
}

} /* namespace miosix */