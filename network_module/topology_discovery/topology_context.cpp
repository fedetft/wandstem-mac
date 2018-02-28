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
#include "../maccontext.h"
#include "topology_context.h"
#include "../debug_settings.h"
#include <stdexcept>

namespace miosix {

unsigned short DynamicTopologyContext::getBestPredecessor() {
    return (ctx.getHop() > 1)? min_element(predecessorsRSSIUnseenSince.begin(),
            predecessorsRSSIUnseenSince.end(),
            CompareRSSI(ctx.getNetworkConfig()->maxRoundsUnreliableParent
                    ))->first: 0;
}

unsigned short DynamicMeshTopologyContext::receivedMessage(unsigned char* pkt, unsigned short len,
        unsigned short nodeIdByTopologySlot, short rssi) {
    auto config = ctx.getNetworkConfig();
    auto size = NeighborMessage::getMaxSize(config->maxNodes, config->networkIdBits, config->hopBits);
    unsigned short i;
    if (len < size)
        throw std::runtime_error("Received invalid length topology packet");
    for (i = 0; i + size < len;) {
        auto* newData = NeighborMessage::fromPkt(config->maxNodes, config->networkIdBits, config->hopBits, pkt, len - i, i);
        if (newData == nullptr) throw std::runtime_error("Wrongly checked received invalid length topology packet");
        if (i == 0 && newData->getSender() != nodeIdByTopologySlot) {
            unreceivedMessage(nodeIdByTopologySlot);
            throw std::runtime_error("Received topology packet from a node whose timeslot is different");
        }
        i += newData->getSize();
        //add it as neighbor and update last seen
        neighborsUnseenSince[newData->getSender()] = 0;
        if (newData->getHop() < ctx.getHop())
            //if it comes from the previous hop, set its RSSI for choosing the best assignee
            predecessorsRSSIUnseenSince[newData->getSender()] = std::make_pair(rssi, 0);
        if (newData->getAssignee() == ctx.getNetworkId()) {
            //The node chosen me for forwarding the data
            if (enqueuedTopologyMessages.hasKey(newData->getSender())) {
                //if i already know the node
                auto oldData = enqueuedTopologyMessages.getByKey(newData->getSender());
                //if i have old data
                if (*((NeighborMessage*) oldData) != *newData) {
                    //if it's different, update it and reset its position in the queue
                    enqueuedTopologyMessages.removeElement(newData->getSender());
                    enqueuedTopologyMessages.enqueue(newData->getSender(), newData);
                }
            } else {//new neighbor's data
                enqueuedTopologyMessages.enqueue(newData->getSender(), newData);
            }
        }
    }
    return i;
}

void DynamicMeshTopologyContext::unreceivedMessage(unsigned short nodeIdByTopologySlot) {
    auto it = neighborsUnseenSince.find(nodeIdByTopologySlot);
    if (it != neighborsUnseenSince.end()) {
        if (it->second >= ctx.getNetworkConfig()->maxRoundsUnavailableBecomesDead)
            neighborsUnseenSince.erase(it);
        else
            it->second++;
    }
    auto it2 = predecessorsRSSIUnseenSince.find(nodeIdByTopologySlot);
    if (it2 != predecessorsRSSIUnseenSince.end()) {
        if (it2->second.second >= ctx.getNetworkConfig()->maxRoundsUnreliableParent)
            predecessorsRSSIUnseenSince.erase(it2);
        else
            it2->second.second++;
    }
}

std::vector<TopologyMessage*> DynamicMeshTopologyContext::dequeueMessages(unsigned short count) {
    std::vector<TopologyMessage*> retval;
    for (unsigned short i = 0; i < count && !enqueuedTopologyMessages.isEmpty(); i++) {
        retval.push_back(enqueuedTopologyMessages.dequeue());
    }
    return retval;
}

TopologyMessage* DynamicMeshTopologyContext::getMyTopologyMessage() {
    auto* config = ctx.getNetworkConfig();
    std::vector<bool> neighbors(config->maxNodes - 1);
    for (unsigned short i = 0, j = 0; i < config->maxNodes - 1; i++){
        if (i == ctx.getNetworkId()) continue;
        else neighbors[j++] = neighborsUnseenSince.find(i) != neighborsUnseenSince.end();
    }
    if (ctx.getHop() == 1) neighbors[0] = true;
    return hasPredecessor()? new NeighborMessage(config->maxNodes, config->networkIdBits, config->hopBits,
            ctx.getNetworkId(), ctx.getHop(), getBestPredecessor(), std::move(neighbors)) : nullptr;
}

unsigned short MasterMeshTopologyContext::receivedMessage(unsigned char* pkt, unsigned short len, unsigned short nodeIdByTopologySlot, short rssi) {
    auto config = ctx.getNetworkConfig();
    auto size = NeighborMessage::getMaxSize(config->maxNodes, config->networkIdBits, config->hopBits);
    if (len < size)
        throw std::runtime_error("Received invalid length topology packet");
    unsigned short i;
    for (i = 0; i + size < len;) {
        auto* newData = NeighborMessage::fromPkt(config->maxNodes, config->networkIdBits, config->hopBits, pkt, len - i, i);
        if (newData == nullptr) throw std::runtime_error("Wrongly checked received invalid length topology packet");
        if (i == 0 && newData->getSender() != nodeIdByTopologySlot) {
            unreceivedMessage(nodeIdByTopologySlot);
            throw std::runtime_error("Received topology packet from a node whose timeslot is different");
        }
        i += newData->getSize();
        for (unsigned short j = 0, node = 0; node < config->maxNodes; j++, node++) {
            if (node == nodeIdByTopologySlot) node++;
            if (newData->getNeighbors(j)) {
                if (!topology.hasEdge(nodeIdByTopologySlot, node))
                    topology.addEdge(nodeIdByTopologySlot, node);
            } else {
                if (topology.hasEdge(nodeIdByTopologySlot, node))
                    topology.removeEdge(nodeIdByTopologySlot, node);
            }
        }
    }
    return i;
}

void MasterMeshTopologyContext::unreceivedMessage(unsigned short nodeIdByTopologySlot) {
    //TODO remove nodes if not seen for long time, avoiding to rely on their neighbors only
}

void MasterMeshTopologyContext::print() {
    for (auto it : topology.getEdges())
        print_dbg("[%d] - [%d]\n", it.first, it.second);
}

bool DynamicTopologyContext::hasPredecessor() {
    return ctx.getHop() < 2 || predecessorsRSSIUnseenSince.size();
}

} /* namespace miosix */