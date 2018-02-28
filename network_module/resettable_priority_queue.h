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

#ifndef NETWORK_MODULE_RESETTABLE_PRIORITY_QUEUE_H_
#define NETWORK_MODULE_RESETTABLE_PRIORITY_QUEUE_H_

#include <utility>
#include <map>
#include <deque>
#include <algorithm>
#include <stdexcept>

namespace miosix {

template<class keyType, class valType>
class ResettablePriorityQueue {
public:
    ResettablePriorityQueue() {
        // TODO Auto-generated constructor stub

    }
    virtual ~ResettablePriorityQueue() {
        // TODO Auto-generated destructor stub
    }
    valType& dequeue();
    valType& top();
    bool enqueue(keyType key, const valType& val);
    bool removeElement(keyType key);
    bool update(keyType key, const valType& val);
    bool resetPriority(keyType key);
    valType& getByKey(keyType key);
    bool hasKey(keyType key);
    bool isEmpty();
private:
    std::map<keyType, valType> data;
    std::deque<keyType> queue;
};

template<class keyType, class valType>
valType& ResettablePriorityQueue<keyType, valType>::dequeue() {
    if (queue.empty()) throw std::runtime_error("no element in queue");
    auto key = queue.back();
    queue.pop_back();
    auto& retval = data[key];
    data.erase(key);
    return retval;
}

template<class keyType, class valType>
valType& ResettablePriorityQueue<keyType, valType>::top() {
    if (queue.empty()) throw std::runtime_error("no element in queue");
    return data[queue.back()];
}

template<class keyType, class valType>
bool miosix::ResettablePriorityQueue<keyType, valType>::enqueue(keyType key, const valType& val) {
    if(hasKey(key)) return false;
    data[key] = val;
    queue.push_front(key);
    return true;
}

template<class keyType, class valType>
bool miosix::ResettablePriorityQueue<keyType, valType>::removeElement(keyType key) {
    if(!hasKey(key)) return false;
    data.erase(key);
    queue.erase(std::find(queue.begin(), queue.end(), key));
    return true;
}

template<class keyType, class valType>
bool miosix::ResettablePriorityQueue<keyType, valType>::update(keyType key, const valType& val) {
    if(!hasKey(key)) return false;
    data[key] = val;
    return true;
}

template<class keyType, class valType>
bool miosix::ResettablePriorityQueue<keyType, valType>::resetPriority(keyType key) {
    if(!hasKey(key)) return false;
    queue.erase(std::find(queue.begin(), queue.end(), key));
    queue.push_front(key);
    return true;
}

template<class keyType, class valType>
inline valType& miosix::ResettablePriorityQueue<keyType, valType>::getByKey(keyType key) {
    if (!hasKey(key)) throw new std::runtime_error("empty");
    return data[key];
}

template<class keyType, class valType>
inline bool ResettablePriorityQueue<keyType, valType>::hasKey(keyType key) {
    return data.count(key);
}

template<class keyType, class valType>
bool ResettablePriorityQueue<keyType, valType>::isEmpty() {
    return data.size() == 0;
}

} /* namespace miosix */

#endif /* NETWORK_MODULE_RESETTABLE_PRIORITY_QUEUE_H_ */