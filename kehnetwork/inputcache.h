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


#ifndef _KEHNETWORK_INPUTCACHE_H
#define _KEHNETWORK_INPUTCACHE_H 1

#include "core/reference.h"

class kehInputData;

// The input cache is used in two different ways, depending on which machine it's
// running and which player the node owning the cache belongs to.
// Running on server:
// - If the node corresponds to the local player (the server), then the cache does
//   nothing because there is no need to validate its data.
// - If this node corresponds to a client then the cache will hold the received input
//   data, which will be retrieved from it when iterating the game state. At that
//   moment the input is removed from the buffer and a secondary container holds
//   information that maps form the snapshot signature to the used input signature.
//   With this information when encoding snapshot data it's possible to attach to it
//   the signature of the input data used to simulate the game. Another thing to keep
//   in mind is that incoming input data may be out of order or duplicated. To that
//   end it's a lot simpler to deal with a map to hold the objects.
// Running on client:
// - If the owning node corresponds to a remote player, then the cache does nothing
//   because client know nothing about other clients nor the server in regards to
//   input data.
// - If the owning node corresponds to the local player, then the cache must hold
//   a sequential (ascending) set of input objects that are mostly meant for
//   validation when snapshot data arrives. When validating, any input older than the
//   one just checked must be removed from the container, thus keeping this data in
//   ascending order makes things a lot easier. In this case it's better to have an
//   array rather than map to hold this data.
// With all this information in mind, this class is meant to make things a bit easier
// to deal with those differences.

class kehInputCache
{
private:
   // Meant for the server, map from input signature to instance of kehInputData
   Map<uint32_t, Ref<kehInputData>> m_sbuffer;
   // Meant for the (local) client, holds non acknowledged input data
   PoolVector<Ref<kehInputData>> m_cbuffer;
   // Keep track of the last input signature used by this machine
   uint32_t m_last_sig;
   // Used only on server, this creates association of snapshot (key) and input (value)
   Map<uint32_t, uint32_t> m_snapinpu;
   // Count number of 0-input snapshots that weren't acknowledged by the client. If
   // this value is bigger than 0 then the server will send the newest full snapshot
   // within its history rather than calculating delta snapshot.
   uint32_t m_no_input_count;
   // Holds the signature of the last acknowledged snapshot signature. This will be used
   // as reference to cleanup older data.
   uint32_t m_last_ack_snap;

public:
   uint32_t get_last_sig() const { return m_last_sig; }
   uint32_t get_used_input_in_snap(uint32_t snap_sig) const;
   uint32_t get_last_ack_snap() const { return m_last_ack_snap; }
   uint32_t get_non_acked_scount() const { return m_snapinpu.size(); }
   bool has_no_input() const { return m_no_input_count > 0; }

   // Creates the association of snapshot signature with input for the corresponding player.
   // This will automatically take care of the "no input count"
   void associate(uint32_t snapsig, uint32_t isig);

   // Acknowledges the specified snapshot signature by removing it from the snapinpu container.
   // It automatically updates the "no input count" property by subtracting from it if the
   // given snapshot didn't use any input.
   void acknowledge(uint32_t snapsig);

   // Increments the internal "last signature", which will be used to build new input data
   // objects for the local player
   uint32_t increment_input() { return ++m_last_sig; }


   // Adds local input object into the internal buffer
   void cache_local_input(const Ref<kehInputData>& input);

   // Adds a client input data into the relevenat container
   void cache_remote_input(const Ref<kehInputData>& input);

   uint32_t get_cache_size() const { return m_cbuffer.size(); }

   Ref<kehInputData> get_input_data(uint32_t index) const;

   // Removes all input objects that are older and equal to the specified input signature
   void clear_older(uint32_t isig);

   // When server requires client input data, use this. This will automatically remove the returned
   // object from the internal container as it will not be needed anymore.
   Ref<kehInputData> get_client_input();

   // Reset the internal state.
   void reset();

   kehInputCache();
};


#endif
