/***************************************************************************
 *   Copyright (C) 2019 by Federico Amedeo Izzo, Federico Terraneo         *
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

#include "network_topology.h"
#include "../../util/debug_settings.h"

namespace mxnet {

//
// class NetworkTopology
//

void NetworkTopology::handleTopologies(UpdatableQueue<unsigned char,
                                       TopologyElement>& topologies) {
    // Lock mutex to access NetworkGraph (shared with ScheduleComputation).
    graph_mutex.lock();

    int numTopologies = topologies.size();
    for(int i=0; i<numTopologies; i++) {
        doReceivedTopology(topologies.dequeue());
    }

    // Unlock mutex, we finished modifying the graph
    graph_mutex.unlock();
}

void NetworkTopology::scheduleNotChanged()
{
#ifdef _MIOSIX
    miosix::Lock<miosix::Mutex> lck(graph_mutex);
#else
    std::unique_lock<std::mutex> lck(graph_mutex);
#endif
    performDelayedChangesChecks();
}

void NetworkTopology::scheduleChanged(std::set<std::pair<unsigned char,unsigned char>>&& usedLinks, std::set<std::pair<unsigned char, unsigned char>>&& newLinksCausingReschedule)
{
#ifdef _MIOSIX
    miosix::Lock<miosix::Mutex> lck(graph_mutex);
#else
    std::unique_lock<std::mutex> lck(graph_mutex);
#endif
    //First update temporary sets, then perform checks
    this->usedLinks=std::move(usedLinks);
    this->newLinksCausingReschedule = std::move(newLinksCausingReschedule);
    performDelayedChangesChecks();
}

void NetworkTopology::performDelayedChangesChecks()
{
    scheduleInProgress = false;
    for(auto removedLink : removedWhileScheduling)
    {
        if(usedLinks.find(removedLink)!=usedLinks.end())
        {
            modified_flag = true;
            break;
        }
    }
    if(!modified_flag) {
        for(auto newLink : addedWhileScheduling) {
            if(newLinksCausingReschedule.find(newLink) != newLinksCausingReschedule.end()){
                modified_flag = true;
                break;
            }
        }
    }
    removedWhileScheduling.clear();
    addedWhileScheduling.clear();
}

void NetworkTopology::doReceivedTopology(const TopologyElement& topology) {
    // Mutex already locked by caller
    unsigned char src = topology.getId();
    auto& bitset = topology.getNeighbors();
    auto& weakBitset = topology.getWeakNeighbors();
    
    if(ENABLE_TOPOLOGY_BITMASK_DBG)
    {
        std::string s;
        if(useWeakTopologies) s.reserve(bitset.bitSize()+weakBitset.bitSize()+4);
        else s.reserve(bitset.bitSize()+2);
        s+='[';
        for(unsigned int i=0;i<bitset.bitSize();i++) s+=bitset[i] ? '1' : '0';
        s+=']';
        if(useWeakTopologies) {
            s+='[';
            for(unsigned int i=0;i<weakBitset.bitSize();i++) s+=weakBitset[i] ? '1' : '0';
            s+=']';
        }
        print_dbg("[U] Topo %03d: %s\n",src,s.c_str());
    }

    /* Update graph according to received topology */
    for (unsigned i = 0; i < bitset.bitSize(); i++) {
        // Avoid auto-arcs (useless in NetworkGraph)
        if(i == src)
            continue;
        
        if(bitset[i]) {
            bool added = graph.addEdge(src, i);
            if(added) {
                if(channelSpatialReuse && !useWeakTopologies) {
                    /* In this case, the main topology graph is used for
                     * interference conflict detection. A new arc is checked
                     * against the current set to determine if rescheduling
                     * should happen.  If a rescheduling is in progress, new
                     * links are saved in a temporary set to be applied later,
                     * and the check is defered. */
                    if(!scheduleInProgress) {
                        if(newLinksCausingReschedule.find(orderLink(src,i)) !=
                                newLinksCausingReschedule.end()) {
                            modified_flag = true;
                        }
                    } else {
                        addedWhileScheduling.insert(orderLink(src,i));
                    }
                }
            }
        } else {
            bool removed = graph.removeEdge(src, i);
            
            if(removed) {
                if(scheduleInProgress == false)
                {
                    // We have an up-to-date usedLinks
                    if(usedLinks.find(orderLink(src, i))!=usedLinks.end())
                    {
                        modified_flag = true;
                    }
                } else {
                    // We don't have usedLinks, so we need to defer this check
                    removedWhileScheduling.insert(orderLink(src, i));
                }
            }
        }
    }

    if(useWeakTopologies) {
        /* Update weak graph according to received topology */
        for (unsigned i = 0; i < weakBitset.bitSize(); i++) {
            if(i == src) //no auto-arcs
                continue;
            if(weakBitset[i]) {
                bool added = weakGraph.addEdge(src, i);
                if(added) {
                    if(!scheduleInProgress) {
                        if(newLinksCausingReschedule.find(orderLink(src,i)) !=
                                newLinksCausingReschedule.end()) {
                            modified_flag = true;
                        }
                    } else {
                        addedWhileScheduling.insert(orderLink(src,i));
                    }
                }
            } else {
                weakGraph.removeEdge(src, i);
                /* NOTE : while the weak topology has changed, the 
                   removal of weak links does not cause problems to
                   existing streams, so rescheduling is not necessary.
                   Therefore we do not set the modified flag in this
                   case. */
            }

        }
    }
}

} /* namespace mxnet */
