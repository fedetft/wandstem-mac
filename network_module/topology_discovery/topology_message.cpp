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
#include "topology_message.h"
#include <cstring>
#include <stdexcept>
#include <limits>

namespace miosix {

std::vector<unsigned char> NeighborMessage::getPkt()  {
    auto bitSize = getMaxSize();
    auto byteSize = (bitSize - 1) / (std::numeric_limits<unsigned char>::digits) + 1;
    std::vector<unsigned char> retval(byteSize);
    auto pkt = retval.data();
    auto idx = 0;
    short firstBitPow;
    for (firstBitPow = nodesBits - std::numeric_limits<unsigned char>::digits;
            firstBitPow >= 0;
            firstBitPow -= std::numeric_limits<unsigned char>::digits)
        pkt[idx++] = sender >> firstBitPow;
    pkt[idx] |= sender << -firstBitPow;
    for (firstBitPow = hopBits + firstBitPow; firstBitPow >= 0; firstBitPow -= std::numeric_limits<unsigned char>::digits)
        pkt[idx++] |= hop >> firstBitPow;
    pkt[idx] |= hop << -firstBitPow;
    for (firstBitPow = nodesBits + firstBitPow; firstBitPow >= 0; firstBitPow -= std::numeric_limits<unsigned char>::digits)
        pkt[idx++] |= assignee >> firstBitPow;
    pkt[idx] |= assignee << -firstBitPow;
    firstBitPow += std::numeric_limits<unsigned char>::digits;
    pkt[idx] &= (~0) >> firstBitPow;
    memset(pkt + idx + 1, 0, byteSize - (idx + 1));
    for (unsigned short j = 0; j < numNodes - 1; j++)
        if (neighbors[j]) {
            pkt[idx + (j + firstBitPow) / (std::numeric_limits<unsigned char>::digits)] |=
                    1 << (std::numeric_limits<unsigned char>::digits - 1 - firstBitPow - j % (std::numeric_limits<unsigned char>::digits));
        }
    return retval;
}

NeighborMessage* NeighborMessage::fromPkt(unsigned short numNodes, unsigned short nodesBits, unsigned char hopBits,
        unsigned char* pkt, unsigned short bitLen, unsigned short startBit) {
    auto size = NeighborMessage::getMaxSize(numNodes, nodesBits, hopBits);
    if (bitLen < size)
        return nullptr; //throw std::runtime_error("Invalid data, unparsable packet");
    auto trailingBits = startBit % (std::numeric_limits<unsigned char>::digits);
    auto idx = startBit / (std::numeric_limits<unsigned char>::digits);
    unsigned short ones = ~0;
    short firstBitPow;
    unsigned short sender = 0;
    for (firstBitPow = nodesBits - std::numeric_limits<unsigned char>::digits + trailingBits;
            firstBitPow >= 0;
            firstBitPow -= std::numeric_limits<unsigned char>::digits)
        sender |= pkt[idx++] << firstBitPow;
    sender |= pkt[idx] >> -firstBitPow;
    sender &= ones >> (std::numeric_limits<unsigned short>::digits - nodesBits);
    unsigned short hop = 0;
    for (firstBitPow = hopBits + firstBitPow; firstBitPow >= 0; firstBitPow -= std::numeric_limits<unsigned char>::digits)
        hop |= pkt[idx++] << firstBitPow;
    hop |= pkt[idx] >> -firstBitPow;
    hop &= ones >> (std::numeric_limits<unsigned short>::digits - hopBits);
    unsigned short assignee = 0;
    for (firstBitPow = nodesBits + firstBitPow; firstBitPow >= 0; firstBitPow -= std::numeric_limits<unsigned char>::digits)
        assignee |= pkt[idx++] << firstBitPow;
    assignee |= pkt[idx] >> -firstBitPow;
    assignee &= ones >> (std::numeric_limits<unsigned short>::digits - nodesBits);
    firstBitPow += std::numeric_limits<unsigned char>::digits;
    std::vector<bool> neighbors(numNodes - 1);
    for (unsigned short j = 0; j < numNodes - 1; j++) {
        neighbors[j] = (pkt[idx + (j + firstBitPow) / (std::numeric_limits<unsigned char>::digits)] &
            (1 << (std::numeric_limits<unsigned char>::digits - 1 - (j + firstBitPow) % (std::numeric_limits<unsigned char>::digits)))) > 0;
    }
    return new NeighborMessage(numNodes, nodesBits, hopBits, sender, hop, assignee, std::move(neighbors));

}

bool NeighborMessage::operator ==(const NeighborMessage &b) const {
    return this->sender == b.sender && this->assignee == b.assignee && this->hop == b.hop && this->neighbors == b.neighbors;
}

} /* namespace miosix */