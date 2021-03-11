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

#ifndef _KEHNETWORK_NODESPAWNER_H
#define _KEHNETWORK_NODESPAWNER_H

#include "core/reference.h"
#include "scene/resources/packed_scene.h"



class kehNetNodeSpawner : public Reference
{
   GDCLASS(kehNetNodeSpawner, Reference);
private:

protected:
   static void _bind_methods();
public:
};


class kehNetDefaultSpawner : public kehNetNodeSpawner
{
   GDCLASS(kehNetDefaultSpawner, kehNetNodeSpawner);
private:
   Ref<PackedScene> m_scene_class;

protected:
   static void _bind_methods();

public:
   Node* spawn();

   void set_scene_class(const Ref<PackedScene>& sc);
   Ref<PackedScene> get_scene_class() const;


   // Unfortunately can't create a constructor with non default arguments. Instantiating from script will not work!
   kehNetDefaultSpawner(const Ref<PackedScene>& scene_class = NULL);
   ~kehNetDefaultSpawner();
};


#endif
