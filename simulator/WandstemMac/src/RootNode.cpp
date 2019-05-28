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

#include "RootNode.h"

#include "network_module/master_tdmh.h"
#include "network_module/network_configuration.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <list>

Define_Module(RootNode);

using namespace std;
using namespace mxnet;

struct Data
{
    Data() {}
    Data(int id, unsigned int counter) : id(id), counter(counter){}
    unsigned char id;
    unsigned int counter;
}__attribute__((packed));

void RootNode::activity()
{
    using namespace miosix;
    print_dbg("Master node\n");
    const NetworkConfiguration config(
            hops,            //maxHops
            nodes,           //maxNodes
            address,       //networkId
            false,         //staticHop
            6,             //panId
            5,             //txPower
            2460,          //baseFrequency
            10000000000,   //clockSyncPeriod
            maxForwardedTopologiesFromMaxNumNodes(nodes), //maxForwardedTopologies
            1,             //numUplinkPackets
            100000000,     //tileDuration
            150000,        //maxAdmittedRcvWindow
            3,             //maxRoundsUnavailableBecomesDead
            -75,           //minNeighborRSSI
            3              //maxMissedTimesyncs
    );
    MasterMediumAccessController controller(Transceiver::instance(), config);

    tdmh = &controller;
    auto *t = new thread(&RootNode::application, this);
    try {
        controller.run();
    } catch(exception& e) {
        //Note: omnet++ seems to terminate coroutines with an exception
        //of type cStackCleanupException. Squelch these
        if(string(typeid(e).name()).find("cStackCleanupException")==string::npos)
            cerr<<"\nException thrown: "<<e.what()<<endl;
        quit.store(true);
        t->join();
        throw;
    } catch(...) {
        quit.store(true);
        t->join();
        cerr<<"\nUnnamed exception thrown"<<endl;
        throw;
    }
    quit.store(true);
    t->join();
}

void RootNode::application() {
    /* Wait for TDMH to become ready */
    MACContext* ctx = tdmh->getMACContext();
    while(!ctx->isReady()) {
    }
    // NOTE: we can't use Stream API functions in simulator
    // so we have to get a pointer to StreamManager
    StreamManager* mgr = ctx->getStreamManager();
    auto params = StreamParameters(Redundancy::TRIPLE_SPATIAL, // Redundancy
                                   Period::P1,                 // Period          
                                   1,                          // Payload size
                                   Direction::TX);             // Direction
    unsigned char port = 1;
    printf("[A] Opening server on port %d\n", port);
    /* Open a Server to listen for incoming streams */
    int server = mgr->listen(port,              // Destination port
                        params);           // Server parameters
    if(server < 0) {                
        printf("[A] Server opening failed! error=%d\n", server);
        return;
    }
    while(mgr->getInfo(server).getStatus() == StreamStatus::LISTEN) {
        int stream = mgr->accept(server);
        pair<int, StreamManager*> arg = make_pair(stream, mgr);
        thread t1(&RootNode::streamThread, this, arg);
        t1.detach();
    }
}

void RootNode::streamThread(pair<int, StreamManager*> arg) {
    try{
        int stream = arg.first;
        StreamManager* mgr = arg.second;
        StreamInfo info = mgr->getInfo(stream);
        StreamId id = info.getStreamId();
        printf("[A] Master node: Stream (%d,%d) accepted\n", id.src, id.dst);
        while(mgr->getInfo(stream).getStatus() == StreamStatus::ESTABLISHED) {
            Data data;
            int len = mgr->read(stream, &data, sizeof(data));
            if(len != sizeof(data))
                printf("[E] Received wrong size data from Stream (%d,%d): %d\n",
                        id.src, id.dst, len);
            else
                printf("[A] Received data from Stream (%d,%d): ID=%d Counter=%u\n",
                       id.src, id.dst, data.id, data.counter);
        }
    }catch(...){
        printf("Exception thrown in streamThread\n");
    }
}
