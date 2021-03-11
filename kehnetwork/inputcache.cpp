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


#include "inputcache.h"
#include "inputdata.h"


uint32_t kehInputCache::get_used_input_in_snap(uint32_t snap_sig) const
{
   const Map<uint32_t, uint32_t>::Element* e = m_snapinpu.find(snap_sig);
   return e ? e->value() : 0;
}



void kehInputCache::associate(uint32_t snapsig, uint32_t isig)
{
   m_snapinpu[snapsig] = isig;
   if (isig == 0)
   {
      m_no_input_count++;
   }
}


void kehInputCache::acknowledge(uint32_t snapsig)
{
   // Depending on how the synchronization occurred, it's possible some older data didn't
   // get a direct acknowledgement but those are now irrelevant. So, the cleanup must be
   // performed through a loop that goes from the last acknowledged up to the one specified.
   // During this loop the "no input count" must be correctly updated
   for (uint32_t sig = m_last_ack_snap + 1; sig < snapsig + 1; sig++)
   {
      const Map<uint32_t, uint32_t>::Element* e = m_snapinpu.find(sig);
      if (e)
      {
         if (e->value() == 0)
         {
            m_no_input_count -= 1;
         }
         m_snapinpu.erase(sig);
      }
   }

   m_last_ack_snap = snapsig;
}


void kehInputCache::cache_local_input(const Ref<kehInputData>& input)
{
   m_cbuffer.push_back(input);
}

void kehInputCache::cache_remote_input(const Ref<kehInputData>& input)
{
   m_sbuffer[input->get_signature()] = input;
}

Ref<kehInputData> kehInputCache::get_input_data(uint32_t index) const
{
   return m_cbuffer[index];
}


void kehInputCache::clear_older(uint32_t isig)
{
   while (m_cbuffer.size() > 0 && m_cbuffer[0]->get_signature() <= isig)
   {
      m_cbuffer.remove(0);
   }
}


Ref<kehInputData> kehInputCache::get_client_input()
{
   Ref<kehInputData> ret;
   const uint32_t using_sig = m_last_sig + 1;
   Map<uint32_t, Ref<kehInputData>>::Element* mel = m_sbuffer.find(using_sig);

   if (mel)
   {
      // There is a valid input object in the cache, so update the last used signature
      m_last_sig++;
      // Assign the retrieved object into the return value
      ret = mel->value();
      // The object is not needed within the container anymore, so remove it
      m_sbuffer.erase(using_sig);
   }

   return ret;
}


void kehInputCache::reset()
{
   m_sbuffer.clear();
   m_cbuffer = PoolVector<Ref<kehInputData>>();
   m_last_sig = 0;
   m_snapinpu.clear();
   m_no_input_count = 0;
   m_last_ack_snap = 0;
}



kehInputCache::kehInputCache() :
   m_last_sig(0),
   m_no_input_count(0),
   m_last_ack_snap(0)
{

}