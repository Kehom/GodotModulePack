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

// This is meant to be a "lightweight data object". Basically when input is gathered
// through the network input system, an object of this class will be generated.
// When encoding data, it will be retrieved from one instance of this class. When
// decoding, an instance of this class will be generated.
// Instead of using the normal input polling, when input is necessary it should be
// requested from the network singleton, which will then provide an instance of
// this class.

#ifndef _KEHNETWORK_INPUTDATA_H
#define _KEHNETWORK_INPUTDATA_H 1

#include "core/reference.h"

class kehInputData : public Reference
{
   GDCLASS(kehInputData, Reference);
private:
   Map<String, Vector2> m_vec2;
   Map<String, Vector3> m_vec3;
   Map<String, float> m_analog;
   Map<String, bool> m_action;
   bool m_has_input;
   uint32_t m_signature;


protected:

   static void _bind_methods();

public:
   uint32_t get_signature() const { return m_signature; }
   bool has_input() const { return m_has_input; }

   Vector2 get_custom_vec2(const String& name) const;
   void set_custom_vec2(const String& name, const Vector2& val);

   Vector3 get_custom_vec3(const String& name) const;
   void set_custom_vec3(const String& name, const Vector3& val);

   bool get_custom_bool(const String& name) const;
   void set_custom_bool(const String& name, bool val);

   Vector2 get_mouse_relative() const;
   void set_mouse_relative(const Vector2& mr);

   Vector2 get_mouse_speed() const;
   void set_mouse_speed(const Vector2& ms);

   float get_analog(const String& name) const;
   void set_analog(const String& name, float val);

   bool is_pressed(const String& name) const;
   void set_pressed(const String& name, bool p);


   kehInputData(uint32_t s = 0);
};


#endif
