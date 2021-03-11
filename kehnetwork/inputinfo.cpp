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


#include "inputinfo.h"
#include "inputdata.h"

#include "../kehgeneral/encdecbuffer.h"
#include "../kehgeneral/quantize.h"

#include "core/project_settings.h"


void kehInputInfo::register_action(const String& mapped, bool is_analog, bool is_custom)
{
   if (m_print_debug)
   {
      print_line(vformat("Registering%snetwork input '%s' | analog: %s", is_custom ? " custom " : " ", mapped, is_analog));
   }

   if (is_analog)
   {
      register_data(m_analog_list, mapped, is_custom);
   }
   else
   {
      register_data(m_bool_list, mapped, is_custom);
   }
}


void kehInputInfo::register_vec2(const String& mapped)
{
   if (m_print_debug)
   {
      print_line(vformat("Registering custom vector2 network input data '%s'", mapped));
   }

   register_data(m_vec2_list, mapped, true);
}

void kehInputInfo::register_vec3(const String& mapped)
{
   if (m_print_debug)
   {
      print_line(vformat("Registering custom vector3 network input data '%s'", mapped));
   }

   register_data(m_vec3_list, mapped, true);
}


void kehInputInfo::reset_registrations()
{
   m_analog_list.clear();
   m_bool_list.clear();
   m_vec2_list.clear();
   m_vec3_list.clear();
   m_has_custom_data = false;
}


Ref<kehInputData> kehInputInfo::make_empty() const
{
   Ref<kehInputData> ret = memnew(kehInputData(0));

   if (m_use_mouse_relative)
      ret->set_mouse_relative(Vector2());
   if (m_use_mouse_speed)
      ret->set_mouse_speed(Vector2());
   
   for (const Map<String, ActionInfo>::Element* e = m_analog_list.front(); e; e = e->next())
      ret->set_analog(e->key(), 0.0f);
   
   for (const Map<String, ActionInfo>::Element* e = m_bool_list.front(); e; e = e->next())
      ret->set_pressed(e->key(), false);
   
   for (const Map<String, ActionInfo>::Element* e = m_vec2_list.front(); e; e = e->next())
      ret->set_custom_vec2(e->key(), Vector2());
   
   for (const Map<String, ActionInfo>::Element* e = m_vec3_list.front(); e; e = e->next())
      ret->set_custom_vec3(e->key(), Vector3());


   return ret;
}


void kehInputInfo::encode_to(Ref<kehEncDecBuffer>& into, const Ref<kehInputData>& input) const
{
   // Encode the signature of the input object. Use uint, 4 bytes
   into->write_uint(input->get_signature());

   // Encode the flag indicating if this object has input or not
   into->write_bool(input->has_input());

   // If there is input, encode the data
   if (input->has_input())
   {
      // If mouse relative is enabled, encode it. It's a Vector2, so 8 bytes
      if (m_use_mouse_relative)
      {
         into->write_vector2(input->get_mouse_relative());
      }

      // If mouse speed is enabled encode it. It's a Vector2, so 8 bytes
      if (m_use_mouse_speed)
      {
         into->write_vector2(input->get_mouse_speed());
      }

      // Encode analog data if there is at least one registered
      if (m_analog_list.size() > 0)
      {
         // Current writing index - this will be used to rewrite the change mask if necessary
         const uint32_t windex = into->get_current_size();
         // Assume nothing has changed
         uint32_t cmask = 0;
         // Write into the buffer at least to reserve space
         write_mask(cmask, m_analog_list.size(), into);

         for (const Map<String, ActionInfo>::Element* e = m_analog_list.front(); e; e = e->next())
         {
            const float val = input->get_analog(e->key());
            if (val != 0.0f)
            {
               cmask |= e->value().mask;
               // Since this analog input is not zero, encode it
               if (m_quantize_analog)
               {
                  // Quantization is enabled. Analog input is always in the range [0..1]
                  const uint8_t q = kehQuantize::get_singleton()->quantize_float(val, 0.0, 1.0, 8);
                  into->write_byte(q);
               }
               else
               {
                  // Quantize is disabled. Encode normally.
                  into->write_float(val);
               }
            }
         }

         // All relevant analogs have been encoded. If something changed the mask must be updated within
         // the encoded byte array. The correct index is stored in the windex variable
         if (cmask != 0)
         {
            write_mask(cmask, m_analog_list.size(), into, windex);
         }
      }

      // Encode boolean data
      if (m_bool_list.size() > 0)
      {
         uint32_t cmask = 0;

         for (const Map<String, ActionInfo>::Element* e = m_bool_list.front(); e; e = e->next())
         {
            if (input->is_pressed(e->key()))
            {
               cmask |= e->value().mask;
            }
         }

         write_mask(cmask, m_bool_list.size(), into);
      }

      // Encode custom Vector2 data
      if (m_vec2_list.size() > 0)
      {
         const uint32_t windex = into->get_current_size();
         uint32_t cmask = 0;
         write_mask(cmask, m_vec2_list.size(), into);

         for (const Map<String, ActionInfo>::Element* e = m_vec2_list.front(); e; e = e->next())
         {
            const Vector2 val(input->get_custom_vec2(e->key()));
            if (val.x != 0.0f || val.y != 0.0f)
            {
               cmask |= e->value().mask;
               into->write_vector2(val);
            }
         }

         if (cmask != 0)
         {
            write_mask(cmask, m_vec2_list.size(), into, windex);
         }
      }

      // Encode custom Vector3 data
      if (m_vec3_list.size() > 0)
      {
         const uint32_t windex = into->get_current_size();
         uint32_t cmask = 0;
         write_mask(cmask, m_vec3_list.size(), into);

         for (const Map<String, ActionInfo>::Element* e = m_vec3_list.front(); e; e = e->next())
         {
            const Vector3 val(input->get_custom_vec3(e->key()));
            if (val.x != 0.0f || val.y != 0.0f || val.z != 0.0f)
            {
               cmask |= e->value().mask;
               into->write_vector3(val);
            }
         }

         if (cmask != 0)
         {
            write_mask(cmask, m_vec3_list.size(), into, windex);
         }
      }
   }
}


Ref<kehInputData> kehInputInfo::decode_from(Ref<kehEncDecBuffer>& from) const
{
   // Decode the signature - while at the same time creating the return object
   Ref<kehInputData> ret = memnew(kehInputData(from->read_uint()));

   // Decode the "has_input" flag
   const bool has_input = from->read_bool();

   if (has_input)
   {
      // Decode mouse relative data if it's enabled
      if (m_use_mouse_relative)
      {
         ret->set_mouse_relative(from->read_vector2());
      }

      // Decode mouse speed data if it's enabled
      if (m_use_mouse_speed)
      {
         ret->set_mouse_speed(from->read_vector2());
      }

      // Decode analog data
      if (m_analog_list.size() > 0)
      {
         // First the change mask, indicating which of the analog inputs were encoded
         const uint32_t cmask = read_mask(m_analog_list.size(), from);
         for (const Map<String, ActionInfo>::Element* e = m_analog_list.front(); e; e = e->next())
         {
            if (cmask & e->value().mask)
            {
               if (m_quantize_analog)
               {
                  // Analog quantization is enabled, so extract the quantized value first
                  const uint8_t q = from->read_byte();
                  // Then restore the float
                  ret->set_analog(e->key(), kehQuantize::get_singleton()->restore_float(q, 0.0, 1.0, 8));
               }
               else
               {
                  ret->set_analog(e->key(), from->read_float());
               }
            }
         }
      }

      // Decode boolean data
      if (m_bool_list.size() > 0)
      {
         const uint32_t cmask = read_mask(m_bool_list.size(), from);
         for (const Map<String, ActionInfo>::Element* e = m_bool_list.front(); e; e = e->next())
         {
            ret->set_pressed(e->key(), (cmask & e->value().mask));
         }
      }

      // Decode vector2 data
      if (m_vec2_list.size() > 0)
      {
         const uint32_t cmask = read_mask(m_vec2_list.size(), from);
         for (const Map<String, ActionInfo>::Element* e = m_vec2_list.front(); e; e = e->next())
         {
            if (cmask & e->value().mask)
            {
               ret->set_custom_vec2(e->key(), from->read_vector2());
            }
         }
      }

      // Decode Vector3 data
      if (m_vec3_list.size() > 0)
      {
         const uint32_t cmask = read_mask(m_vec3_list.size(), from);
         for (const Map<String, ActionInfo>::Element* e = m_vec3_list.front(); e; e = e->next())
         {
            if (cmask & e->value().mask)
            {
               ret->set_custom_vec3(e->key(), from->read_vector3());
            }
         }
      }
   }
   
   return ret;
}



void kehInputInfo::set_action_enabled(const String& mapped, bool enabled)
{
   if (m_analog_list.has(mapped))
   {
      m_analog_list[mapped].enabled = enabled;
   }
   else if(m_bool_list.has(mapped))
   {
      m_bool_list[mapped].enabled = enabled;
   }
   else
   {
      WARN_PRINT(String("Trying to set action enabled state on non-existent action '{0}'").format(mapped));
   }
}



void kehInputInfo::register_data(Map<String, kehInputInfo::ActionInfo>& container, const String& name, bool custom)
{
   if (!container.has(name))
   {
      ActionInfo nentry;
      nentry.mask = 1 << container.size();
      nentry.custom = custom;
      nentry.enabled = true;

      container[name] = nentry;

      m_has_custom_data |= custom;
   }
}

void kehInputInfo::write_mask(uint32_t mask, uint32_t size, Ref<kehEncDecBuffer>& into, int32_t at) const
{
   if (size <= 8)
   {
      if (at < 0)
      {
         into->write_byte(mask);
      }
      else
      {
         into->rewrite_byte(mask, at);
      }
   }
   else if (size <= 16)
   {
      if (at < 0)
      {
         into->write_ushort(mask);
      }
      else
      {
         into->rewrite_ushort(mask, at);
      }
   }
   else
   {
      if (at < 0)
      {
         into->write_uint(mask);
      }
      else
      {
         into->rewrite_uint(mask, at);
      }
   }
}

uint32_t kehInputInfo::read_mask(uint32_t size, Ref<kehEncDecBuffer>& from) const
{
   if (size <= 8)
   {
      return from->read_byte();
   }
   else if (size <= 16)
   {
      return from->read_ushort();
   }

   return from->read_uint();
}




kehInputInfo::kehInputInfo() :
   m_has_custom_data(false)
{
   m_use_mouse_relative = GLOBAL_GET("keh_modules/network/input/use_mouse_relative");
   m_use_mouse_speed = GLOBAL_GET("keh_modules/network/input/use_mouse_speed");
   m_quantize_analog = GLOBAL_GET("keh_modules/network/input/quantize_analog_data");
   m_print_debug = GLOBAL_GET("keh_modules/network/general/print_debug_info");
}
