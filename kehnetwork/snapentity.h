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

#ifndef _KEHNETWORK_SNAPENTITY_H
#define _KEHNETWORK_SNAPENTITY_H 1

#include "core/reference.h"

class kehSnapEntityBase : public Reference
{
   GDCLASS(kehSnapEntityBase, Reference);

public:
   const static uint32_t CTYPE_UINT = 65538;
   const static uint32_t CTYPE_BYTE = 131074;
   const static uint32_t CTYPE_USHORT = 196610;

private:
   // Unique ID representing this entity within the snapshots.
   uint32_t m_id;

   // This can also be seen as a way to categorize this entity.
   uint32_t m_class_hash;

protected:
   static void _bind_methods();

public:

   void set_uid(uint32_t id) { m_id = id; }
   uint32_t get_uid() const { return m_id; }
   void set_class_hash(uint32_t chash) { m_class_hash = chash; }
   uint32_t get_class_hash() const { return m_class_hash; }


   virtual void apply_state(Node* to_node) {}

   void _init() {}

   kehSnapEntityBase(uint32_t id = 0, uint32_t chash = 0);
};


#endif
