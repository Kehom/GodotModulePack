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

#ifndef _KEHNETWORK_CUSTOMPROPERTY_H
#define _KEHNETWORK_CUSTOMPROPERTY_H 1

#include "core/reference.h"

class kehEncDecBuffer;

class kehCustomProperty : public Reference
{
   GDCLASS(kehCustomProperty, Reference);
public:
   // Properties through this system can be marked for automatic replication. This enum
   // configures how that will work
   enum ReplicationMode
   {
      None,              // No replication of this property
      ServerOnly,        // If a property is changed in a client machine, it will be sent only to the server
      ServerBroadcast,   // Property value will be broadcast to every player through the server
   };

private:
   // A custom property can be of "any type", so using variant here to hold the value
   Variant m_value;

   // The replication method/mode for this custom property
   ReplicationMode m_replicate;

   // This flag tells if the custom property must be synchronized or not. Normally this will be set after
   // changing the "value" of the property
   bool m_dirty;

protected:
   static void _bind_methods();

public:

   // Encode this property into the given EncDecBuffer, provided the expected type is supported.
   // If force_encode is true then the value will be encoded even if the dirty state is false.
   bool encode_to(Ref<kehEncDecBuffer>& into, const String& pname, uint32_t expected_type, bool force_encode);

   bool decode_from(Ref<kehEncDecBuffer>& from, uint32_t expected_type, bool make_dirty);

   void set_value(const Variant& v);

   Variant get_value() const { return m_value; }
   ReplicationMode get_mode() const { return m_replicate; }

   bool is_dirty() const { return m_dirty; }
   void clear_dirt() { m_dirty = false; }

   kehCustomProperty(const Variant& initial = NULL, ReplicationMode mode = ReplicationMode::ServerOnly);
};

VARIANT_ENUM_CAST(kehCustomProperty::ReplicationMode);


#endif
