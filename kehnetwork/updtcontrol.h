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

#ifndef _KEHNETWORK_UPDATECONTROL_H
#define _KEHNETWORK_UPDATECONTROL_H 1

#include "core/object.h"
#include "core/reference.h"

#include "functoid.h"
#include "eventinfo.h"
#include "snapshot.h"

class kehEncDecBuffer;


// This inner class will be used to control the overall update flow. It holds the snapshot
// that is being built during the frame. When the "start snapshot" gets called from the game
// code, the interface code will relay the call to the instance of this call, which will take
// care of setting up a deferred call to the function that will perform the final tasks of
// "post game update", which is finishing the snapshot, encoding and sending the data to the
// clients as well as dispatching all accumulated events and custom property changes.
class kehUpdateControl : public Object
{
   GDCLASS(kehUpdateControl, Object);

public:
   typedef kehFunctoid<void()> CustomPropCheckT;
   typedef kehFunctoid<void(Ref<kehSnapshot>&)> SnapshotFinishedT;
   typedef kehFunctoid<void(const PoolVector<kehNetEvent>&)> EventDispatcherT;

private:
   // Signature of the snapshot being built. This also is used to "calculate" the signature of the next snapshot.
   uint32_t m_sig;
   // The snapshot that is being built during a frame
   Ref<kehSnapshot> m_snap;
   // Accumulate events here
   PoolVector<kehNetEvent> m_event;

   // This will be used to encode and decode snapshot data.
   Ref<kehEncDecBuffer> m_encdec;

   // When the finish() function is running this function will be called and it's meant to combine
   // (encode) custom properties into a single RPC call
   CustomPropCheckT m_cpropcheck;
   // This is meant to perform "snapshot finished" actions. The function will be called when the
   // finish() function is running.
   SnapshotFinishedT m_sfinished;
   // Will be called during when snapshot is finished. The function should dispatch accumulated events
   EventDispatcherT m_evtdispatch;


private:
   // Called through a defer, will perform tasks to "finalize" the snapshot
   void finish();

protected:
   static void _bind_methods();

public:
   uint32_t get_signature() const;
   Ref<kehEncDecBuffer> get_enc_dec() { return m_encdec; }

   void start(const PoolVector<uint32_t>& snap_types);

   void set_custom_prop_checker(const CustomPropCheckT& pcheck);
   void set_snapshot_finished(const SnapshotFinishedT& finished);
   void set_event_dispatcher(const EventDispatcherT& evtdispatch);

   bool is_building() const;


   void add_to_snapshot(uint32_t ehash, const Ref<kehSnapEntityBase>& entity);

   void push_event(uint16_t code, const Array& params);

   void reset();

   
   kehUpdateControl();
};

#endif
