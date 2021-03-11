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

#ifndef _KEHNETWORK_EVENTINFO_H
#define _KEHNETWORK_EVENTINFO_H 1


#include "core/reference.h"

class kehEncDecBuffer;

struct kehNetEvent
{
   uint16_t type;
   Array params;
};

class kehEventInfo
{
   struct evt_handler
   {
      String fname;
      const Object* obj;

      evt_handler(const Object* o = NULL, const String& fn = "") : fname(fn), obj(o) {}
      evt_handler(const evt_handler& other) : fname(other.fname), obj(other.obj) {}
      bool operator == (const evt_handler& other) const { return fname == other.fname && obj == other.obj; }
   };
private:
   // Type ID of events described by this EventInfo
   uint16_t m_type_id;

   // List of types of the parametes expected by events of this type
   PoolVector<uint32_t> m_param_type;

   // List of handlers for this event
   PoolVector<evt_handler> m_event_handler;
   
public:
   static bool check_types(const Array& ptypes);

   void attach_handler(const Object* obj, const String& fname);

   void clear_handlers();

   void encode(Ref<kehEncDecBuffer>& into, const Array& params) const;

   void decode(Ref<kehEncDecBuffer>& from) const;

   void call_handlers(const Array& params) const;

   kehEventInfo(uint16_t type = 0, const Array& param_list = Array());
};

#endif
