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

#ifndef _KEHNETWORK_INPUTINFO_H
#define _KEHNETWORK_INPUTINFO_H 1

#include "core/input_map.h"


class kehInputData;
class kehEncDecBuffer;

// This is meant to hold information regarding which input data (based on
// project settings input mappings) should be encoded/decoded when dealing
// with the network synchronization.
class kehInputInfo
{
public:
   // Helper structure that will hold extra information regarding each registered
   // input action.
   struct ActionInfo
   {
      // Bit mask corresponding to the action, which will be used to create a "change mask"
      uint32_t mask;
      // Flag indicating if this action is a custom one or is part of an actual input map
      bool custom;
      // Flag indicating if this action is enabled or not.
      bool enabled;
   };
private:
   // Options that will be retrieved from project settings
   bool m_use_mouse_relative;
   bool m_use_mouse_speed;
   bool m_quantize_analog;
   bool m_print_debug;

   // The following maps are meant to hold the list of input data meant to be encoded/decoded,
   // thus replicated through the network. Each map corresponds to a supported data type (analog,
   // boolean, vector2 and vector3). Analog and boolean are the only ones to be automatically
   // retrieved by polling the input device state, based on the input map settings and the registered
   // input names.
   // Within those maps each entry will be keyed by the input name and will hold and instance of
   // the kehActionInfo struct.
   Map<String, ActionInfo> m_analog_list;
   Map<String, ActionInfo> m_bool_list;
   Map<String, ActionInfo> m_vec2_list;
   Map<String, ActionInfo> m_vec3_list;

   // As soon as custom input data type is registered this flag will be set to true.
   // This is mostly to help issue warnings when custom input are required but the function reference
   // used to generate the data is not set.
   bool m_has_custom_data;

private:
   // A generic function meant to create the correct entry data within the input containers
   void register_data(Map<String, ActionInfo>& container, const String& name, bool custom);

   // Write the change mask into the given EncDecBuffer. Note that when encoded, this mask use
   // a "variable number of bytes", depending on the amount of registered input data of the
   // specific type. So, if there are less than 9 analog actions, the change mask for that type
   // of data will be encoded into a single byte. This requires some checking before (re)writing
   // data. This helper internal function is meant to perform the correct writing of the mask.
   // This function assumes proper check of the size > 0 has already been done
   void write_mask(uint32_t mask, uint32_t size, Ref<kehEncDecBuffer>& into, int32_t at = -1) const;

   // This helper function is meant to read the change mask from encoded data.
   uint32_t read_mask(uint32_t size, Ref<kehEncDecBuffer>& from) const;

   // "Generic for each" - this will be called by the "exposed" function
   template<class Function>
   void for_each(const Map<String, ActionInfo>& container, Function fn)
   {
      for (const Map<String, ActionInfo>::Element* e = container.front(); e; e = e->next())
      {
         const ActionInfo& ai = e->value();
         fn(e->key(), ai.custom, ai.enabled);
      }
   }

public:
   void register_action(const String& mapped, bool is_analog, bool is_custom);

   void register_vec2(const String& mapped);
   void register_vec3(const String& mapped);

   void reset_registrations();


   void set_action_enabled(const String& mapped, bool enabled);

   void set_use_mouse_relative(bool use) { m_use_mouse_relative = use; }
   void set_use_mouse_speed(bool use) { m_use_mouse_speed = use; }

   bool use_mouse_relative() const { return m_use_mouse_relative; }
   bool use_mouse_speed() const { return m_use_mouse_speed; }

   // Create a "blank" input data object. That is, it will have no input but all the map entries will be present.
   Ref<kehInputData> make_empty() const;

   // Encode the provided InputData into the given EncDecBuffer
   void encode_to(Ref<kehEncDecBuffer>& into, const Ref<kehInputData>& input) const;

   // Use the given EncDecBuffer to decode data into an InputData object, which will be returned.
   // This function assumes the buffer is in the correct reading position
   Ref<kehInputData> decode_from(Ref<kehEncDecBuffer>& from) const;

   // Get a (const) boolean iterator pointing to the front one.
   const Map<String, ActionInfo>::Element* get_bool_iterator() const { return m_bool_list.front(); }

   // Get a (const) analog iterator pointint to the front one
   const Map<String, ActionInfo>::Element* get_analog_iterator() const { return m_analog_list.front(); }

   /*
   // Calls the provided function object for each registered boolean input. The provided function will
   // receive, in this order: Input Name (String), "is custom" (boolean), "is enabled" (boolean)
   template<class Function>
   void for_each_bool(Function fn)
   {
      for_each(m_bool_list, fn);
   }

   // Calls the provided function object for each registered analog input. The provided function will
   // receive, in this order: Input Name (String), "is custom" (boolean), "is enabled" (boolean)
   template<class Function>
   void for_each_analog(Function fn)
   {
      for_each(m_analog_list, fn);
   }*/


   kehInputInfo();
};


#endif
