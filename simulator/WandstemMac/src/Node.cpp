//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Node.h"
#include <iostream>

Define_Module(Node);

void Node::activity()
{
    unsigned char pkt[127];
    auto result = receive(pkt, sizeof(pkt), SimTime(100, SIMTIME_S), false);
    EV_INFO << "ERROR: " << result.error << " AT " << result.timestamp << endl;
    if(result.error == RecvResult::OK) {
        EV_INFO << "[ ";
        for (int i = 0; i < result.size; i++)
            EV_INFO << std::hex << (unsigned int) pkt[i] << " ";
        EV_INFO << std::dec << "]"<< endl;
    }
    simtime_t precPacketTime;
    for(;;)
    {
        precPacketTime = SimTime(result.timestamp, SIMTIME_NS);
        auto sleepTime = precPacketTime + SimTime(5, SIMTIME_MS) - simTime();
        EV_INFO << "Sleeping for " << sleepTime << endl;
        waitAndDeletePackets(sleepTime);
        //trying with a 5 ms window
        auto timeout = precPacketTime + SimTime(15, SIMTIME_MS);
        EV_INFO << "Will listen for packets from " << simTime().inUnit(SIMTIME_NS) << " to " << timeout.inUnit(SIMTIME_NS) << endl;
        result = receive(pkt, sizeof(pkt), timeout, true);
        EV_INFO << "ERROR: " << result.error << " AT " << result.timestamp << endl;
        if(result.error == RecvResult::OK) {
            EV_INFO << "[ ";
            for (int i = 0; i < result.size; i++)
                EV_INFO << std::hex << (unsigned int) pkt[i] << " ";
            EV_INFO << std::dec << "]"<< endl;
        }
    }
}
