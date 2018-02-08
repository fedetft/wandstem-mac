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

#include "NodeBase.h"

void NodeBase::waitAndDeletePackets(simtime_t time)
{
    cQueue queue;
    waitAndEnqueue(time,&queue);
    //TODO: is this efficient? And most important, why can't they use std::list?
    while(!queue.isEmpty()) delete queue.pop();
}

void NodeBase::radioSend()
{
    for(int i=0;i<gateSize("wireless");i++)
        send(new cMessage("job"),"wireless$o",i);
}
