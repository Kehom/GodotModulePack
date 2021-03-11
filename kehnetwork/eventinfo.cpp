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

#include "eventinfo.h"
#include "../kehgeneral/encdecbuffer.h"
#include "snapentity.h"


bool kehEventInfo::check_types(const Array& ptypes)
{
   for (uint32_t i = 0; i < ptypes.size(); i++)
   {
      uint32_t type = ptypes[i];
      switch (type)
      {
         // Fall through on purpose. All supported types must enter an "empty" case. Otherwise just
         // return false because an unsupported type is in the list
         case Variant::BOOL:
         case Variant::INT:
         case Variant::VECTOR2:
         case Variant::RECT2:
         case Variant::QUAT:
         case Variant::COLOR:
         case Variant::VECTOR3:
         case kehSnapEntityBase::CTYPE_UINT:
         case kehSnapEntityBase::CTYPE_BYTE:
         case kehSnapEntityBase::CTYPE_USHORT:
         {} break;

         default:
            return false;
      }
   }

   return true;
}


void kehEventInfo::attach_handler(const Object* obj, const String& fname)
{
   // When this is called this is assuming the verification of Object validity and existence of the function name
   // have already been done
   evt_handler eh(obj, fname);
   m_event_handler.push_back(eh);
}

void kehEventInfo::clear_handlers()
{
   m_event_handler.resize(0);
}

void kehEventInfo::encode(Ref<kehEncDecBuffer>& into, const Array& params) const
{
   ERR_FAIL_COND_MSG(params.size() != m_param_type.size(), vformat("Trying to encode event type '%d' but given parameter number (%d) does not match that of the registration (%d).", m_type_id, params.size(), m_param_type.size()));

   // Write the parameters
   for (uint32_t i = 0; i < m_param_type.size(); i++)
   {
      switch (m_param_type[i])
      {
         case Variant::BOOL:
         {
            into->write_bool(params[i]);
         } break;

         case Variant::INT:
         {
            into->write_int(params[i]);
         } break;

         case Variant::REAL:
         {
            into->write_float(params[i]);
         } break;

         case Variant::VECTOR2:
         {
            into->write_vector2(params[i]);
         } break;

         case Variant::RECT2:
         {
            into->write_rect2(params[i]);
         } break;

         case Variant::QUAT:
         {
            into->write_quat(params[i]);
         } break;

         case Variant::COLOR:
         {
            into->write_color(params[i]);
         } break;

         case Variant::VECTOR3:
         {
            into->write_vector3(params[i]);
         } break;

         case kehSnapEntityBase::CTYPE_UINT:
         {
            into->write_uint(params[i]);
         } break;

         case kehSnapEntityBase::CTYPE_BYTE:
         {
            into->write_byte(params[i]);
         } break;

         case kehSnapEntityBase::CTYPE_USHORT:
         {
            into->write_ushort(params[i]);
         } break;
      }
   }
}

void kehEventInfo::decode(Ref<kehEncDecBuffer>& from) const
{
   // At this point the "from" should already have gone past the type ID of this event. So just decode the parameters
   Array params;

   for (uint32_t i = 0; i < m_param_type.size(); i++)
   {
      switch (m_param_type[i])
      {
         case Variant::BOOL:
         {
            params.push_back(from->read_bool());
         } break;

         case Variant::INT:
         {
            params.push_back(from->read_int());
         } break;

         case Variant::REAL:
         {
            params.push_back(from->read_float());
         } break;

         case Variant::VECTOR2:
         {
            params.push_back(from->read_vector2());
         } break;

         case Variant::RECT2:
         {
            params.push_back(from->read_rect2());
         } break;

         case Variant::QUAT:
         {
            params.push_back(from->read_quat());
         } break;

         case Variant::COLOR:
         {
            params.push_back(from->read_color());
         } break;

         case Variant::VECTOR3:
         {
            params.push_back(from->read_vector3());
         } break;

         case kehSnapEntityBase::CTYPE_UINT:
         {
            params.push_back(from->read_uint());
         } break;

         case kehSnapEntityBase::CTYPE_BYTE:
         {
            params.push_back(from->read_byte());
         } break;

         case kehSnapEntityBase::CTYPE_USHORT:
         {
            params.push_back(from->read_ushort());
         } break;
      }
   }

   call_handlers(params);
}


void kehEventInfo::call_handlers(const Array& params) const
{
   for (uint32_t i = 0; i < m_event_handler.size(); i++)
   {
      // FIXME: the correct check for the Object validity
      if (m_event_handler[i].obj)
      {
         Object* o = (Object*)m_event_handler[i].obj;
         o->callv(m_event_handler[i].fname, params);
      }
   }

}


kehEventInfo::kehEventInfo(uint16_t type, const Array& param_list)
{
   m_type_id = type;
   for (uint32_t i = 0; i < param_list.size(); i++)
   {
      m_param_type.push_back(param_list[i]);
   }
}
