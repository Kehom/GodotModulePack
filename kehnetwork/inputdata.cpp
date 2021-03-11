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

#include "inputdata.h"

Vector2 kehInputData::get_custom_vec2(const String& name) const
{
   const Map<String, Vector2>::Element* e = m_vec2.find(name);
   return (e ? e->value() : Vector2());
}

void kehInputData::set_custom_vec2(const String& name, const Vector2& val)
{
   m_vec2[name] = val;
   m_has_input = (val.x != 0.0f || val.y != 0.0f || m_has_input);
}


Vector3 kehInputData::get_custom_vec3(const String& name) const
{
   const Map<String, Vector3>::Element* e = m_vec3.find(name);
   return (e ? e->value() : Vector3());
}

void kehInputData::set_custom_vec3(const String& name, const Vector3& val)
{
   m_vec3[name] = val;
   m_has_input = (val.x != 0.0f || val.y != 0.0f || val.z != 0.0f || m_has_input);
}


bool kehInputData::get_custom_bool(const String& name) const
{
   const Map<String, bool>::Element* e = m_action.find(name);
   return (e ? e->value() : false);
}

void kehInputData::set_custom_bool(const String& name, bool val)
{
   m_action[name] = val;
   m_has_input = (val || m_has_input);
}


Vector2 kehInputData::get_mouse_relative() const
{
   return get_custom_vec2("_mouse_relative");
}

void kehInputData::set_mouse_relative(const Vector2& mr)
{
   set_custom_vec2("_mouse_relative", mr);
}


Vector2 kehInputData::get_mouse_speed() const
{
   return get_custom_vec2("_mouse_speed");
}

void kehInputData::set_mouse_speed(const Vector2& ms)
{
   set_custom_vec2("_mouse_speed", ms);
}


float kehInputData::get_analog(const String& name) const
{
   const Map<String, float>::Element* e = m_analog.find(name);
   return (e ? e->value() : 0.0f);
}

void kehInputData::set_analog(const String& name, float val)
{
   m_analog[name] = val;
   m_has_input = (val != 0.0f || m_has_input);
}


bool kehInputData::is_pressed(const String& name) const
{
   const Map<String, bool>::Element* e = m_action.find(name);
   return (e ? e->value() : false);
}

void kehInputData::set_pressed(const String& name, bool p)
{
   m_action[name] = p;
   m_has_input = (p || m_has_input);
}


void kehInputData::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("get_custom_vec2", "name"), &kehInputData::get_custom_vec2);
   ClassDB::bind_method(D_METHOD("set_custom_vec2", "name", "val"), &kehInputData::set_custom_vec2);

   ClassDB::bind_method(D_METHOD("get_custom_vec3", "name"), &kehInputData::get_custom_vec3);
   ClassDB::bind_method(D_METHOD("set_custom_vec3", "name", "val"), &kehInputData::set_custom_vec3);

   ClassDB::bind_method(D_METHOD("get_custom_bool", "name"), &kehInputData::get_custom_bool);
   ClassDB::bind_method(D_METHOD("set_custom_bool", "name", "val"), &kehInputData::set_custom_bool);

   ClassDB::bind_method(D_METHOD("get_mouse_relative"), &kehInputData::get_mouse_relative);
   ClassDB::bind_method(D_METHOD("get_mouse_speed"), &kehInputData::get_mouse_speed);

   ClassDB::bind_method(D_METHOD("get_analog", "name"), &kehInputData::get_analog);
   ClassDB::bind_method(D_METHOD("is_pressed", "name"), &kehInputData::is_pressed);
}


kehInputData::kehInputData(uint32_t s) :
   m_has_input(false),
   m_signature(s)
{

}