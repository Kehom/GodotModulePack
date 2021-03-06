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

#include "nodespawner.h"


void kehNetNodeSpawner::_bind_methods()
{
   BIND_VMETHOD(MethodInfo(PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "Node"), "spawn"));
}



Node* kehNetDefaultSpawner::spawn()
{
   Node* ret = NULL;

   if (m_scene_class.is_valid())
   {
      ret = m_scene_class->instance();
   }

   return ret;
}


void kehNetDefaultSpawner::set_scene_class(const Ref<PackedScene>& sc)
{
   m_scene_class = sc;
}

Ref<PackedScene> kehNetDefaultSpawner::get_scene_class() const
{
   return m_scene_class;
}


void kehNetDefaultSpawner::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("spawn"), &kehNetDefaultSpawner::spawn);

   ClassDB::bind_method(D_METHOD("set_scene_class", "scene_class"), &kehNetDefaultSpawner::set_scene_class);
   ClassDB::bind_method(D_METHOD("get_scene_class"), &kehNetDefaultSpawner::get_scene_class);

   ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene_class", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene", NULL), "set_scene_class", "get_scene_class");
}


kehNetDefaultSpawner::kehNetDefaultSpawner(const Ref<PackedScene>& scene_class) :
   m_scene_class(scene_class)
{}
kehNetDefaultSpawner::~kehNetDefaultSpawner()
{}