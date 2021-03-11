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

#include "pinginfo.h"

// Wait one second between ping requests
const float kehPingInfo::PING_INTERVAL = 1.0f;
// Wait five seconds before considering a ping request as lost
const float kehPingInfo::PING_TIMEOUT = 5.0f;


float kehPingInfo::calculate_and_restart(uint32_t sig)
{
   float ret = -1.0f;
   if (m_signature == sig)
   {
      // Obtain the amount of time and convert to milliseconds
      m_last_ping = (PING_TIMEOUT - m_timeout->get_time_left()) * 1000.0f;
      ret = m_last_ping;
      m_timeout->stop();
      m_interval->start();
   }

   return ret;
}




void kehPingInfo::request_ping(uint32_t pid)
{
   m_signature += 1;
   m_interval->stop();
   m_timeout->start();
   m_parent->rpc_unreliable_id(pid, "_client_ping", m_signature, m_last_ping);
}

void kehPingInfo::on_ping_timeout(uint32_t pid)
{
   // The last ping request has timed out. No answer received, so assume the packet has been lost
   m_lost_packets++;

   // Request a new ping - no need to wait through interval since there was already 5 seconds from
   // the previous request.
   request_ping(pid);
}

void kehPingInfo::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("_request_ping", "pid"), &kehPingInfo::request_ping);
   ClassDB::bind_method(D_METHOD("_on_ping_timeout", "pid"), &kehPingInfo::on_ping_timeout);
}


kehPingInfo::kehPingInfo(uint32_t pid, Node* parent)
{
   m_signature = 0;
   m_lost_packets = 0;
   m_last_ping = 0.0f;
   m_parent = parent;

   // This vector will be used to add a payload to the connected functions
   Vector<Variant> payload;
   payload.push_back(pid);

   // Initialize the two timers
   m_interval = memnew(Timer());
   m_interval->set_wait_time(PING_INTERVAL);
   m_interval->set_timer_process_mode(Timer::TimerProcessMode::TIMER_PROCESS_IDLE);
   m_interval->set_name("net_ping_interval");
   m_interval->connect("timeout", this, "_request_ping", payload);

   m_timeout = memnew(Timer());
   m_timeout->set_wait_time(PING_TIMEOUT);
   m_interval->set_timer_process_mode(Timer::TimerProcessMode::TIMER_PROCESS_IDLE);
   m_interval->set_name("net_ping_timeout");
   m_interval->connect("timeout", this, "_on_ping_timeout", payload);

   // Timers must be added into the tree otherwise they aren't updated
   m_parent->add_child(m_interval);
   m_parent->add_child(m_timeout);

   // Make sure the timeout is stopped while the interval is running
   m_timeout->stop();
   m_interval->start();
}