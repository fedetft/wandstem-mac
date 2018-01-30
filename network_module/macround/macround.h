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

#ifndef NETWORKROUND_H
#define NETWORKROUND_H

#include "../maccontext.h"
#include <memory>

namespace miosix {
    class FloodingPhase;
    class RoundtripPhase;
    class ReservationPhase;
    class AssignmentPhase;
    class MACRound {
    public:
        MACRound(FloodingPhase* flooding, RoundtripPhase* roundtrip, ReservationPhase* reservation, AssignmentPhase* assignment) :
                flooding(flooding), roundtrip(roundtrip), reservation(reservation), assignment(assignment) {};
        MACRound(const MACRound& orig) = delete;
        virtual ~MACRound();

        void setFloodingPhase(FloodingPhase* fp);
        inline FloodingPhase* getFloodingPhase() { return flooding; }

        void setRoundTripPhase(RoundtripPhase* rtp);
        inline RoundtripPhase* getRoundtripPhase() { return roundtrip; }

        void setReservationPhase(ReservationPhase* rp);
        inline ReservationPhase* getReservationPhase() { return reservation; }

        void setAssignmentPhase(AssignmentPhase* as);
        inline AssignmentPhase* getAssignmentPhase() { return assignment; }

        virtual void run(MACContext& ctx);
        static const long long roundDuration = 10000000000LL; //10s
    protected:
        MACRound() {};
        FloodingPhase* flooding = nullptr;
        RoundtripPhase* roundtrip = nullptr;
        ReservationPhase* reservation = nullptr;
        AssignmentPhase* assignment = nullptr;

    };
}

#endif /* NETWORKROUND_H */

