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

#ifndef _KEHNETWORK_SNAPSHOT_H
#define _KEHNETWORK_SNAPSHOT_H 1

#include "core/reference.h"

class kehSnapEntityBase;

class kehSnapshot : public Reference
{
public:
   struct EntityCollection
   {
      // Container used mostly for querying
      Map<uint32_t, Ref<kehSnapEntityBase>> uid_to_entity;
      // Container used for iteration
      PoolVector<Ref<kehSnapEntityBase>> entity_array;

      EntityCollection() {}
   };

   // Typedef that should help with obtaining iterators outside of the class
   typedef Map<uint32_t, EntityCollection> entity_data_t;

private:
   // Can also be called frame or even timestamp. In any case, this is just an
   // incrementing number to help identify the snapshots
   uint32_t m_signature;

   // Signature of the input data used when generating this snapshot. On the authority
   // machine this may not make much difference but is used on clients to compare with
   // incoming snapshot data.
   uint32_t m_inputsig;

   // Map from entity type into inner double container
   entity_data_t m_entity_data;

private:
   //bool get_entity_index(entity_array_t::Element* arr_el, uint32_t uid, uint32_t& out);
   bool get_entity_index(const PoolVector<Ref<kehSnapEntityBase>>& arr, uint32_t uid, uint32_t& out) const;

public:
   bool has_type(uint32_t ehash) const;
   void add_type(uint32_t nhash);

   uint32_t get_entity_count(uint32_t nhash) const;

   void add_entity(uint32_t nhash, const Ref<kehSnapEntityBase>& entity);

   void remove_entity(uint32_t nhash, uint32_t uid);

   // Given an entity type, obtain the list of unique IDs, filling the provided Set
   void get_entity_uids(uint32_t ehash, Set<uint32_t>& out) const;

   //void get_entity_uids(uint32_t ehash, Vector<uint32_t>& out) const;

   Ref<kehSnapEntityBase> get_entity(uint32_t nhash, uint32_t uid) const;

   uint32_t get_signature() const { return m_signature; }
   void set_input_sig(uint32_t s) { m_inputsig = s; }
   uint32_t get_input_sig() const { return m_inputsig; }

   void build_tracker(Map<uint32_t, Set<uint32_t>>& output) const;

   // Retrieve the EntityCollection element iterator from the outer container. This will give access to both inner containers
   const entity_data_t::Element* get_entity_collection(uint32_t ehash) const { return m_entity_data.find(ehash); }


   kehSnapshot(uint32_t sig, uint32_t isig = 0) : m_signature(sig), m_inputsig(isig) {}
};


#endif
