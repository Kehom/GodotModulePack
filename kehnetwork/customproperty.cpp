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


#include "customproperty.h"

#include "../kehgeneral/encdecbuffer.h"



bool kehCustomProperty::encode_to(Ref<kehEncDecBuffer>& into, const String& pname, uint32_t expected_type, bool force_encode)
{
   if (!m_dirty && !force_encode)
      return false;
   
   // Write the property name
   into->write_string(pname);

   bool e = false;
   // Then the value
   switch (expected_type)
   {
      case Variant::BOOL:
      {
         into->write_bool(m_value);
         e = true;
      } break;

      case Variant::INT:
      {
         into->write_int(m_value);
         e = true;
      } break;

      case Variant::REAL:
      {
         into->write_float(m_value);
         e = true;
      } break;

      case Variant::RECT2:
      {
         into->write_rect2(m_value);
         e = true;
      } break;

      case Variant::VECTOR3:
      {
         into->write_vector3(m_value);
         e = true;
      } break;

      case Variant::QUAT:
      {
         into->write_quat(m_value);
         e = true;
      } break;

      case Variant::COLOR:
      {
         into->write_color(m_value);
         e = true;
      } break;

      case Variant::STRING:
      {
         into->write_string(m_value);
         e = true;
      } break;
   }

   if (e)
      m_dirty = false;

   return e;
}


bool kehCustomProperty::decode_from(Ref<kehEncDecBuffer>& from, uint32_t expected_type, bool make_dirty)
{
   bool d = false;
   switch (expected_type)
   {
      case Variant::BOOL:
      {
         m_value = from->read_bool();
         d = true;
      } break;

      case Variant::INT:
      {
         m_value = from->read_int();
         d = true;
      } break;

      case Variant::REAL:
      {
         m_value = from->read_float();
         d = true;
      } break;

      case Variant::VECTOR2:
      {
         m_value = from->read_vector2();
         d = true;
      } break;

      case Variant::RECT2:
      {
         m_value = from->read_rect2();
         d = true;
      } break;

      case Variant::VECTOR3:
      {
         m_value = from->read_vector3();
         d = true;
      } break;

      case Variant::QUAT:
      {
         m_value = from->read_quat();
         d = true;
      } break;

      case Variant::COLOR:
      {
         m_value = from->read_color();
         d = true;
      } break;

      case Variant::STRING:
      {
         m_value = from->read_string();
         d = true;
      } break;
   }

   if (make_dirty && d)
      m_dirty = true;
      
   return d;
}


void kehCustomProperty::set_value(const Variant& v)
{
   if (v != m_value)
   {
      m_value = v;
      m_dirty = true;
   }
}



void kehCustomProperty::_bind_methods()
{
   BIND_ENUM_CONSTANT(None);
   BIND_ENUM_CONSTANT(ServerOnly);
   BIND_ENUM_CONSTANT(ServerBroadcast);
}



kehCustomProperty::kehCustomProperty(const Variant& initial, ReplicationMode mode) :
   m_value(initial),
   m_replicate(mode),
   m_dirty(false)
{
}