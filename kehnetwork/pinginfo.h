/**
 * Copyright (c) 2021 Yuri Sarudiansky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


// Initially my intention was to perform a "true ping" using ENet. While researching
// I stumbled into this post:
// http://lists.cubik.org/pipermail/enet-discuss/2008-October/000980.html
// If the link is broken, here is the contents:
//
// "The roundTripTime field should never be interpreted as a reliable all
// purpose ping. It's solely for ENet's internal use. YMMV. Buyer beware.
// Caveat emptor. Etc.
// 
// The best way to get an accurate ping for specific purposes it to implement
// a ping tailored for that purpose. Send a time value to the other end, have
// the other end send it back, check the difference, and do with it as you
// will."
//
// That said, that's basically how the internal ping measurement will occur.

#ifndef _KEHNETWORK_PINGINFO_H
#define _KEHNETWORK_PINGINFO_H 1

#include "scene/main/node.h"
#include "scene/main/timer.h"

class kehPingInfo : public Reference
{
   GDCLASS(kehPingInfo, Reference)
private:
   static const float PING_INTERVAL;
   static const float PING_TIMEOUT;

   Timer* m_interval;              // Timer to control interval between ping requests
   Timer* m_timeout;               // Timer to control ping timeouts
   uint32_t m_signature;           // Signature of the ping request
   uint32_t m_lost_packets;        // Number of packets considered lost
   float m_last_ping;              // Last measured ping, in milliseconds
   Node* m_parent;                 // Node to hold the timers and remote functions. It should be a kehPlayerNode

private:
   void request_ping(uint32_t pid);

   void on_ping_timeout(uint32_t pid);

protected:
   static void _bind_methods();

public:
   float calculate_and_restart(uint32_t sig);

   kehPingInfo(uint32_t pid, Node* parent);
};


#endif
