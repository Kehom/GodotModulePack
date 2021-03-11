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

#include "updtcontrol.h"
#include "snapentity.h"


#include "../kehgeneral/encdecbuffer.h"


uint32_t kehUpdateControl::get_signature() const
{
   ERR_FAIL_COND_V_MSG(!m_snap.is_valid(), 0, "Trying to obtain snapshot signature without starting it first.");
   return m_snap->get_signature();
}

void kehUpdateControl::start(const PoolVector<uint32_t>& snap_types)
{
   m_sig++;
   m_snap = Ref<kehSnapshot>(memnew(kehSnapshot(m_sig)));

   for (uint32_t i = 0; i < snap_types.size(); i++)
   {
      m_snap->add_type(snap_types[i]);
   }

   call_deferred("_finish");
}


void kehUpdateControl::set_custom_prop_checker(const CustomPropCheckT& pcheck)
{
   m_cpropcheck = pcheck;
}

void kehUpdateControl::set_snapshot_finished(const SnapshotFinishedT& finished)
{
   m_sfinished = finished;
}

void kehUpdateControl::set_event_dispatcher(const EventDispatcherT& evtdispatch)
{
   m_evtdispatch = evtdispatch;
}

bool kehUpdateControl::is_building() const
{
   return m_snap.is_valid() && !m_snap.is_null();
}


void kehUpdateControl::add_to_snapshot(uint32_t ehash, const Ref<kehSnapEntityBase>& entity)
{
   m_snap->add_entity(ehash, entity);
}


void kehUpdateControl::push_event(uint16_t code, const Array& params)
{
   kehNetEvent mevt;
   mevt.type = code;
   mevt.params = params;
   m_event.push_back(mevt);
}


void kehUpdateControl::reset()
{
   m_sig = 0;
   m_snap = Ref<kehSnapshot>();
   m_event.resize(0);
}


void kehUpdateControl::finish()
{
   if (!m_snap.is_valid() || m_snap.is_null())
   {
      return;
   }

   // Custom properties are sent through the reliable channel so send them first
   if (m_cpropcheck.is_valid())
      m_cpropcheck();
   // Perform the finalization tasks
   if (m_sfinished.is_valid())
      m_sfinished(m_snap);
   // Dispatch events
   if (m_evtdispatch.is_valid())
      m_evtdispatch(m_event);
   
   m_event.resize(0);

   // Reset internal snapshot reference
   m_snap = Ref<kehSnapshot>(NULL);
}


void kehUpdateControl::_bind_methods()
{
   // Doing this just so the call_deferred works.
   ClassDB::bind_method(D_METHOD("_finish"), &kehUpdateControl::finish);
}


kehUpdateControl::kehUpdateControl()
{
   m_sig = 0;
   m_encdec = Ref<kehEncDecBuffer>(memnew(kehEncDecBuffer));
}