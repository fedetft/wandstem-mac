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

#pragma once

#include "timesync_downlink.h"
#include "controller/flopsync2.h"
#include "controller/synchronizer.h"
#include "roundtrip/asking_roundtrip.h"
#include "interfaces-impl/virtual_clock.h"
#include "kernel/timeconversion.h"
#include "../../util/packet.h"
#include <limits>

namespace mxnet {
class DynamicTimesyncDownlink : public TimesyncDownlink {
public:
    DynamicTimesyncDownlink() = delete;
    explicit DynamicTimesyncDownlink(MACContext& ctx) :
            TimesyncDownlink(ctx, DESYNCHRONIZED, std::numeric_limits<unsigned>::max()),
            //askingRTP(ctx),
            tc(new miosix::TimeConversion(EFM32_HFXO_FREQ)),
            vt(miosix::VirtualClock::instance()),
            synchronizer(new Flopsync2()),
            measuredFrameStart(0),
            computedFrameStart(0),
            theoreticalFrameStart(0),
            clockCorrection(0),
            missedPackets(0) {
                vt.setSyncPeriod(networkConfig.getClockSyncPeriod());
        };
    DynamicTimesyncDownlink(const DynamicTimesyncDownlink& orig) = delete;
    virtual ~DynamicTimesyncDownlink() {
        delete tc;
        delete synchronizer;
    };
    inline void execute(long long slotStart) override;
    std::pair<long long, long long> getWakeupAndTimeout(long long tExpected) override;
    //    long long getDelayToMaster() const override { return askingRTP.getDelayToMaster(); }
    virtual long long getSlotframeStart() const { return measuredFrameStart - (ctx.getHop() - 1) * rebroadcastInterval; }
protected:
    void rebroadcast(const Packet& pkt, long long arrivalTs);
    /* Similar to isSyncPacket but checks also rssi to be above the threshold */
    bool isResyncPacket(const Packet& packet, miosix::RecvResult r) {
        if(r.rssi < ctx.getNetworkConfig().getMinNeighborRSSI())
            return false;
        return isSyncPacket(packet, r, false);
    }
    bool isSyncPacket(const Packet& packet, miosix::RecvResult r, bool synchronized) {
        auto panId = networkConfig.getPanId();
        // Ignore low RSSI packets if not syncronyzed
        if(synchronized == false && r.rssi<networkConfig.getMinNeighborRSSI())
            return false;
        if((r.timestampValid && packet.size() == syncPacketSize
            && packet[0] == 0x46 && packet[1] == 0x08
            && packet[3] == static_cast<unsigned char>(panId >> 8)
            && packet[4] == static_cast<unsigned char>(panId & 0xff)
            && packet[5] == 0xff && packet[6] == 0xff) == false) return false;
        if(synchronized) {
            // If synchronized, the hop can't change
            if(ctx.getHop() != packet[2] + 1) return false;
        } else {
            if(networkConfig.getStaticHop()>0)
            {
                // If not synchronized and forced hop selected, ignore other hops
                if(packet[2] + 1 != networkConfig.getStaticHop()) return false;
            } else {
                // If not synchronized and no forced hop, ignore hops>maxNumHop
                // NOTE: this is important because other phases send packets
                // with the same header which can be disambiguated by the hop field
                if(packet[2] + 1 > networkConfig.getMaxHops()) return false;
            }
        }
        return true;
    }
    /**
     * Master node do not need reset since it never loses synchronization
     */
    void reset() override {};

    /**
     * Since the node is synchronized, it performs the step in the FLOPSYNC-2 controller
     */
    void periodicSync();

    /**
     * Since the node is not synchronizes, it listens to the channel for an undefined time
     * to reinitialize the FLOPSYNC-2 controller
     */
    void resync();

    /**
     * Resets the data calculated by and useful for the controller
     */
    void reset(long long hookPktTime);
    void next() override;
    long long correct(long long int uncorrected) override;

    /**
     * Marks the packet as missed, altering the internal state of the synchronization automaton
     */
    unsigned char missedPacket();

    /**
     * Updates the VirtualClock data to perform corrections based on the freshly calculated FLOPSYNC-2 data
     */
    void updateVt() {
        vt.update(
                tc->ns2tick(theoreticalFrameStart),
                tc->ns2tick(computedFrameStart), clockCorrection
        );
    }

    /**
     * To be called after setHop, every time the MAC resynchronizes
     * calls reset methods in every mac_phase containing some status
     * to avoid working with old data
     * NOTE: used only on dynamic mac_phases since the master node
     * can never be desynchronized since it's the one giving the time.
     */
    void resetMAC();

    //AskingRoundtripPhase askingRTP;
    miosix::TimeConversion* const tc;
    miosix::VirtualClock& vt;
    Synchronizer* const synchronizer;

    /**
     * Contains the corrected timestamp of arrival of the synchronization packet.
     * If no packet arrived, it is just incremented of a sync period.
     */
    long long measuredFrameStart;

    /**
     * Contains the calculated timestamp of arrival of the synchronization packet
     * including the clock correction.
     */
    long long computedFrameStart;

    /**
     * Contains the calculated timestamp of arrival of the synchronization packet,
     * by taking the first timesync as zero and incrementing it every sync period.
     */
    long long theoreticalFrameStart;

    int clockCorrection;
    unsigned char missedPackets;
};
}

