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


#include "snapshot.h"
#include "snapentity.h"


bool kehSnapshot::has_type(uint32_t ehash) const
{
   return m_entity_data.has(ehash);
}

void kehSnapshot::add_type(uint32_t nhash)
{
   m_entity_data[nhash] = EntityCollection();
}

uint32_t kehSnapshot::get_entity_count(uint32_t nhash) const
{
   const entity_data_t::Element* e = m_entity_data.find(nhash);
   ERR_FAIL_COND_V_MSG(!e, 0, vformat("Trying to obtain entity count for a type (%d) that is not registered within the snapshot.", nhash));
   return e->value().entity_array.size();
}

void kehSnapshot::add_entity(uint32_t nhash, const Ref<kehSnapEntityBase>& entity)
{
   entity_data_t::Element* e = m_entity_data.find(nhash);
   ERR_FAIL_COND_MSG(!e, vformat("Trying to add an entity of type (%d) that is not registered within the snapshot.", nhash));

   // First check if the entity already exists. The thing is, if so, most likely the reference is different
   // and must be updated within the internal containers.
   Map<uint32_t, Ref<kehSnapEntityBase>>::Element* el = e->value().uid_to_entity.find(entity->get_uid());
   if (el)
   {
      // Entity already exists. Must update both containers
      el->value() = entity;
      // The array is a little bit more complicated as the entity's correct index must be found
      uint32_t i = 0;
      if (get_entity_index(e->value().entity_array, entity->get_uid(), i))
      {
         e->value().entity_array.set(i, entity);
      }
      else
      {
         // This should not happen! But, should this error out?
      }
   }
   else
   {
      // This is indeed a new entity, so add into both inner containers.
      e->value().entity_array.push_back(entity);
      e->value().uid_to_entity[entity->get_uid()] = entity;
   }
}

void kehSnapshot::remove_entity(uint32_t nhash, uint32_t uid)
{
   entity_data_t::Element* e = m_entity_data.find(nhash);
   ERR_FAIL_COND_MSG(!e, vformat("Trying to remove entity of a type (%d) that is not registered within the snapshot.", nhash));
   if (e->value().uid_to_entity.erase(uid))
   {
      // If here, something was deleted from the inner map. This means the entity must exist on the array.
      uint32_t i = 0;
      if (get_entity_index(e->value().entity_array, uid, i))
      {
         e->value().entity_array.remove(i);
      }
   }
}


/*
void kehSnapshot::get_entity_uids(uint32_t ehash, Vector<uint32_t>& out) const
{
   const entity_data_t::Element* e = m_entity_data.find(ehash);
   if (e)
   {
      const uint32_t ecount = e->value().entity_array.size();
      for (uint32_t i = 0; i < ecount; i++)
      {
         out.push_back(e->value().entity_array[i]->get_uid());
      }
   }
}*/

void kehSnapshot::get_entity_uids(uint32_t ehash, Set<uint32_t>& out) const
{
   out.clear();
   const entity_data_t::Element* e = m_entity_data.find(ehash);
   if (e)
   {
      const uint32_t ecount = e->value().entity_array.size();
      for (uint32_t i = 0; i < ecount; i++)
      {
         out.insert(e->value().entity_array[i]->get_uid());
      }
   }
}


Ref<kehSnapEntityBase> kehSnapshot::get_entity(uint32_t nhash, uint32_t uid) const
{
   const entity_data_t::Element* e = m_entity_data.find(nhash);
   ERR_FAIL_COND_V_MSG(!e, NULL, vformat("Trying to retrieve entity of a type (%d) that is not registered within the snapshot.", nhash));
   const Map<uint32_t, Ref<kehSnapEntityBase>>::Element* entel = e->value().uid_to_entity.find(uid);

   return entel ? entel->value() : NULL;
}


void kehSnapshot::build_tracker(Map<uint32_t, Set<uint32_t>>& output) const
{
   for (const entity_data_t::Element* ecol = m_entity_data.front(); ecol; ecol = ecol->next())
   {
      const uint32_t ecount = ecol->value().entity_array.size();

      output[ecol->key()] = Set<uint32_t>();
      Map<uint32_t, Set<uint32_t>>::Element* s = output.find(ecol->key());

      for (uint32_t i = 0; i < ecount; i++)
      {
         s->value().insert(ecol->value().entity_array[i]->get_uid());
      }
   }
}


bool kehSnapshot::get_entity_index(const PoolVector<Ref<kehSnapEntityBase>>& arr, uint32_t uid, uint32_t& out) const
{
   const uint32_t ecount = arr.size();
   for (uint32_t i = 0; i < ecount; i++)
   {
      if (arr[i]->get_uid() == uid)
      {
         out = i;
         return true;
      }
   }
   return false;
}
