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

#include "snapentity.h"



void kehSnapEntityBase::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("get_id"), &kehSnapEntityBase::get_uid);
   ClassDB::bind_method(D_METHOD("get_class_hash"), &kehSnapEntityBase::get_class_hash);

   BIND_VMETHOD(MethodInfo("apply_state", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "Node")));
   
   

   ADD_PROPERTY(PropertyInfo(Variant::INT, "id", PROPERTY_HINT_NONE, "", 0), "", "get_id");
   ADD_PROPERTY(PropertyInfo(Variant::INT, "class_hash", PROPERTY_HINT_NONE, "", 0), "", "get_class_hash");
}


kehSnapEntityBase::kehSnapEntityBase(uint32_t id, uint32_t chash)
   : m_id(id), m_class_hash(chash)
{
   
}