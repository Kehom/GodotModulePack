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

#include "entityinfo.h"
#include "snapentity.h"
#include "nodespawner.h"

#include "../kehgeneral/encdecbuffer.h"

#include "core/io/resource_loader.h"
#include "core/resource.h"
#include "scene/main/node.h"



kehEntityInfo::SpawnerData::SpawnerData() :
   spawner(NULL), parent(NULL), extra_setup(NULL) {}

kehEntityInfo::SpawnerData::SpawnerData(const Ref<kehNetNodeSpawner> s, Node* p, const Ref<FuncRef>& es) :
   spawner(s), parent(p), extra_setup(es) {}



void kehEntityInfo::register_spawner(uint32_t chash, const Ref<kehNetNodeSpawner>& spawner, Node* parent, const Ref<FuncRef>& esetup)
{
   ERR_FAIL_COND_MSG(!spawner.is_valid(), "The provided spawner is not valid.");
   ERR_FAIL_COND_MSG(!parent, "The provided parent Node is not valid.");

   m_spawner_data[chash] = SpawnerData(spawner, parent, esetup);
}



Ref<kehSnapEntityBase> kehEntityInfo::create_instance(uint32_t uid, uint32_t chash) const
{
   Ref<kehSnapEntityBase> ret;
   if (m_resource.is_valid() && m_resource->can_instance())
   {
      ret = Ref<kehSnapEntityBase>(memnew(kehSnapEntityBase(uid, chash)));

      ret->set_script(m_resource.get_ref_ptr());
   }

   return ret;
}

Ref<kehSnapEntityBase> kehEntityInfo::clone_entity(const Ref<kehSnapEntityBase>& entity) const
{
   ERR_FAIL_COND_V_MSG(m_resource != entity->get_script(), NULL, "Given object to be cloned does not match entity type descbribed by this info.");

   Ref<kehSnapEntityBase> ret = create_instance(entity->get_uid(), entity->get_class_hash());

   for (uint32_t i = 0; i < m_replicable.size(); i++)
   {
      const String name = m_replicable[i].name;
      ret->set(name, entity->get(name));
   }

   return ret;
}


uint32_t kehEntityInfo::calculate_change_mask(const Ref<kehSnapEntityBase>& e1, const Ref<kehSnapEntityBase>& e2) const
{
   // FIXME: properly check if the given entities are indeed of the same type. The get_script() is not working.
   //ERR_FAIL_COND_V_MSG(e1->get_script() == e2->get_script(), 0, "The two given entities are of different types.");

   uint32_t ret = 0;

   for (uint32_t i = 0; i < m_replicable.size(); i++)
   {
      const String pname = m_replicable[i].name;
      if (!m_replicable[i].comparer(e1->get(pname), e2->get(pname)))
         ret |= m_replicable[i].mask;
   }

   return ret;
}


void kehEntityInfo::encode_full_entity(const Ref<kehSnapEntityBase>& entity, Ref<kehEncDecBuffer>& into) const
{
   // Ensure the ID is encoded first
   into->write_uint(entity->get_uid());
   // Encode class hash if it wasn't disabled
   if (m_has_chash)
   {
      into->write_uint(entity->get_class_hash());
   }

   // Now write the rest of the properties
   for (uint32_t i = 0; i < m_replicable.size(); i++)
   {
      const ReplicableProperty& rp = m_replicable.get(i);
      if (rp.name != "id" && rp.name != "class_hash")
      {
         property_writer(rp, entity, into);
      }
   }
}


Ref<kehSnapEntityBase> kehEntityInfo::decode_full_entity(Ref<kehEncDecBuffer>& from) const
{
   // Read entity unique ID
   const uint32_t uid = from->read_uint();
   // If the class hash was not disable, read it
   const uint32_t chash = m_has_chash ? from->read_uint() : 0;

   Ref<kehSnapEntityBase> entity = create_instance(uid, chash);

   // Read (decode) the properties
   const uint32_t rsize = m_replicable.size();
   for (uint32_t i = 0; i < rsize; i++)
   {
      // UID and class hash have already been decoded, so must skip those
      ReplicableProperty rp = m_replicable.get(i);
      if (rp.name != "id" && rp.name != "class_hash")
      {
         property_reader(rp, from, entity);
      }
   }

   return entity;
}


void kehEntityInfo::encode_delta_entity(uint32_t uid, const Ref<kehSnapEntityBase>& entity, uint32_t cmask, Ref<kehEncDecBuffer>& into) const
{
   // Write entity unique ID
   into->write_uint(entity->get_uid());

   // Write change mask - using the cached amount of bits for it.
   switch (m_cmask_size)
   {
      case 1:
      {
         into->write_byte(cmask);
      } break;

      case 2:
      {
         into->write_ushort(cmask);
      } break;

      case 4:
      {
         into->write_uint(cmask);
      } break;

      default:
      {
         // TODO: Error out here
         return;
      }
   }

   // Entity ID and change mask are written. However, if change mask is 0 then there is no need to
   // iterate through replicable properties
   if (cmask == 0)
      return;
   
   // Iterate through replicable properties
   for (uint32_t i = 0; i < m_replicable.size(); i++)
   {
      // ID has already been encoded above, before the change mask, so skip it.
      // Also not skipping class hash, although it's not meant to be changed. The change mask should
      // take care of "skipping it"
      ReplicableProperty rp = m_replicable.get(i);
      if (rp.name == "id")
         continue;
      
      if (rp.mask & cmask)
      {
         // This is a changed property, so encode it
         property_writer(rp, entity, into);
      }
   }
}


Ref<kehSnapEntityBase> kehEntityInfo::decode_delta_entity(Ref<kehEncDecBuffer>& from, uint32_t& outcmask) const
{
   // Decode entity ID
   const uint32_t uid = from->read_uint();
   // Decode the change mask
   outcmask = extract_change_mask(from);

   if (outcmask == -1)
      return NULL;

   // NOTE: the returned entity is meant to contain only the changed data. The rest that does not match in the change
   // mask will be left with default values
   Ref<kehSnapEntityBase> ret = create_instance(uid, 0);

   // Avoid replicable property looping if the change mask is 0, as this entity is marked for removal and does not
   // contain any encoded data
   if (outcmask > 0)
   {
      for (uint32_t  i = 0; i < m_replicable.size(); i++)
      {
         ReplicableProperty rp = m_replicable.get(i);
         if (rp.name != "id" && rp.mask & outcmask)
         {
            property_reader(rp, from, ret);
         }
      }
   }


   return ret;
}



uint32_t kehEntityInfo::get_full_change_mask() const
{
   uint32_t ret = 0;
   switch (m_cmask_size)
   {
      case 1:
      {
         ret = 0xFF;
      } break;
      case 2:
      {
         ret = 0xFFFF;
      } break;
      case 4:
      {
         ret = 0xFFFFFFFF;
      } break;
   }
   return ret;
}


void kehEntityInfo::match_delta(Ref<kehSnapEntityBase>& changed, const Ref<kehSnapEntityBase>& source, uint32_t cmask) const
{
   for (uint32_t i = 0; i < m_replicable.size(); i++)
   {
      ReplicableProperty rp = m_replicable[i];

      // Only take from old value if the replicable property is not marked as changed
      if (!(rp.mask & cmask))
      {
         changed->set(rp.name, source->get(rp.name));
      }
   }
}


Node* kehEntityInfo::get_game_node(uint32_t uid) const
{
   const Map<uint32_t, GameEntity>::Element* e = m_entity.find(uid);
   return e ? e->value().node : NULL;
}


void kehEntityInfo::clear_nodes()
{
   for (Map<uint32_t, GameEntity>::Element* e = m_entity.front(); e; e = e->next())
   {
      Node* n = e->value().node;
      if (!n->is_queued_for_deletion())
      {
         n->queue_delete();
      }
   }

   m_entity.clear();
}

Node* kehEntityInfo::spawn_node(uint32_t uid, uint32_t chash)
{
   Node* ret;

   Map<uint32_t, SpawnerData>::Element* sdatae = m_spawner_data.find(chash);
   if (sdatae)
   {
      SpawnerData sd = sdatae->value();

      ret = sd.spawner->call("spawn");

      ret->set_meta("uid", uid);
      ret->set_meta("chash", chash);

      m_entity[uid] = GameEntity(ret, 0);
      sd.parent->add_child(ret);
      
      if (sd.extra_setup.is_valid() && sd.extra_setup->is_valid())
      {
         Array args;
         args.push_back(ret);
         sd.extra_setup->call_funcv(args);
      }
   }
   else
   {
      WARN_PRINTS(vformat("Could not retrieve spawner for entity '%s' with unique ID %d", m_namestr, uid));
   }

   return ret;
}


void kehEntityInfo::despawn_node(uint32_t uid)
{
   Map<uint32_t, GameEntity>::Element* gee = m_entity.find(uid);
   if (gee)
   {
      GameEntity ge = gee->value();

      if (!ge.node->is_queued_for_deletion())
         ge.node->queue_delete();
      
      m_entity.erase(uid);
   }
}


void kehEntityInfo::add_pre_spawned(uint32_t uid, Node* node)
{
   m_entity[uid] = GameEntity(node, 0);
}


void kehEntityInfo::update_pred_count(int32_t delta)
{
   for (Map<uint32_t, GameEntity>::Element* e = m_entity.front(); e; e = e->next())
   {
      const int32_t ccount = e->value().predcount;
      e->value().predcount = MAX(ccount + delta, 0);
   }
}

uint32_t kehEntityInfo::get_pred_count(uint32_t uid) const
{
   uint32_t ret = 0;
   const Map<uint32_t, GameEntity>::Element* ge = m_entity.find(uid);
   if (ge)
   {
      ret = ge->value().predcount;
   }

   return ret;
}



String kehEntityInfo::check(const String& cname, const String& cpath)
{
   m_name_hash = cname.hash();

   Ref<Script> res = ResourceLoader::load(cpath);

   if (!res.is_valid() || res.is_null())
   {
      return "Unable to load Script resource: " + cpath;
   }

   if (!res->can_instance())
   {
      return vformat("The Script resource '%s' was loaded, but can_instance() returned false.", cpath);
   }

   if (!res->has_method("apply_state"))
   {
      return "Method apply_state(node) is not implemented.";
   }


   // Create a dummy instance of the scripted class, which must be used in order to properly check
   // declared properties as well as their types.
   kehSnapEntityBase* dummy = memnew(kehSnapEntityBase);
   dummy->set_script(res.get_ref_ptr());

   int mask = 1;
   int min_size = 2;         // Assume class_hash is not disabled

   List<PropertyInfo> plist;
   dummy->get_property_list(&plist);
   for (List<PropertyInfo>::Element* prop = plist.front(); prop; prop = prop->next())
   {
      PropertyInfo p = prop->get();
      if (p.usage & PROPERTY_USAGE_SCRIPT_VARIABLE || p.name == "id" || p.name == "class_hash")
      {
         if (p.name == "class_hash" && dummy->has_meta("class_hash") && (int)dummy->get_meta("class_hash") == 0)
         {
            m_has_chash = false;
            min_size--;
            continue;
         }

         ReplicableProperty rprop = build_replicable_prop(p.name, p.type, mask, dummy);

         if (rprop.is_valid())
         {
            m_replicable.append(rprop);
            mask = mask << 1;
         }
      }
   }

   memdelete(dummy);
   plist.clear();

   if (m_replicable.size() <= min_size)
   {
      return "There are no defined (supported) replicable properties in class " + cname;
   }
   else
   {
      if (m_replicable.size() <= 8)
         m_cmask_size = 1;
      else if (m_replicable.size() <= 16)
         m_cmask_size = 2;
      else if (m_replicable.size() <= 32)
         m_cmask_size = 4;
      else
         return "There are more than 32 replicable properties, which is not supported by this system.";
   }

   m_resource = res;
   m_namestr = cname;

   return "";
}


String kehEntityInfo::get_comp_data() const
{
   String ret = m_namestr + "\n";
   for (uint32_t i = 0; i < m_replicable.size(); i++)
   {
      ret += vformat("- %s: %s\n", m_replicable[i].name, m_replicable[i].comparer.get_comparer_name());
   }

   return ret;
}




kehEntityInfo::ReplicableProperty kehEntityInfo::build_replicable_prop(const String& name, Variant::Type type, int mask, kehSnapEntityBase* dummy)
{
   ReplicableProperty ret;
   ret.name = name;
   String comparer_name = "";


   Variant varval;
   const bool hmeta = dummy->has_meta(name);
   if (hmeta)
   {
      varval = dummy->get_meta(name);
   }

   const int tp = ret.comparer.init(type, hmeta, varval);
   if (tp != Variant::NIL)
   {
      ret.type = tp;
      ret.mask = mask;
   }
   
   return ret;
}


void kehEntityInfo::property_writer(const ReplicableProperty& rp, const Ref<kehSnapEntityBase>& entity, Ref<kehEncDecBuffer>& into) const
{
   // First take the value from the entity. It should be a Variant
   const Variant val = entity->get(rp.name);

   switch (rp.type)
   {
      case Variant::BOOL:
      {
         into->write_bool(val);
      } break;

      case Variant::INT:
      {
         into->write_int(val);
      } break;

      case Variant::REAL:
      {
         into->write_float(val);
      } break;

      case Variant::VECTOR2:
      {
         into->write_vector2(val);
      } break;

      case Variant::RECT2:
      {
         into->write_rect2(val);
      } break;

      case Variant::QUAT:
      {
         into->write_quat(val);
      } break;

      case Variant::COLOR:
      {
         into->write_color(val);
      } break;

      case Variant::VECTOR3:
      {
         into->write_vector3(val);
      } break;

      case kehSnapEntityBase::CTYPE_UINT:
      {
         into->write_uint(val);
      } break;

      case kehSnapEntityBase::CTYPE_BYTE:
      {
         into->write_byte(val);
      } break;

      case kehSnapEntityBase::CTYPE_USHORT:
      {
         into->write_ushort(val);
      } break;

      case Variant::STRING:
      {
         into->write_string(val);
      } break;

      case Variant::POOL_BYTE_ARRAY:
      {
         const PoolByteArray ba(val);
         ERR_FAIL_COND_MSG(ba.size() > MAX_ARRAY_SIZE, vformat("Cannot encode an entity (%s) array property (%s) with more than 255 elements. Currently with %d.", m_namestr, rp.name, ba.size()));
         into->write_byte(ba.size());
         for (uint32_t i = 0; i < ba.size(); i++)
         {
            into->write_byte(ba[i]);
         }
      } break;

      case Variant::POOL_INT_ARRAY:
      {
         const PoolIntArray ia(val);
         ERR_FAIL_COND_MSG(ia.size() > MAX_ARRAY_SIZE, vformat("Cannot encode an entity (%s) array property (%s) with more than 255 elements. Currently with %d.", m_namestr, rp.name, ia.size()));
         into->write_byte(ia.size());
         for (uint32_t i = 0; i < ia.size(); i++)
         {
            into->write_int(ia[i]);
         }
      } break;

      case Variant::POOL_REAL_ARRAY:
      {
         const PoolRealArray ra(val);
         ERR_FAIL_COND_MSG(ra.size() > MAX_ARRAY_SIZE, vformat("Cannot encode an entity (%s) array property (%s) with more than 255 elements. Currently with %d.", m_namestr, rp.name, ra.size()));
         for (uint32_t i = 0; i < ra.size(); i++)
         {
            into->write_float(ra[i]);
         }
      } break;
   }
}


void kehEntityInfo::property_reader(const ReplicableProperty& rp, Ref<kehEncDecBuffer>& from, Ref<kehSnapEntityBase>& into) const
{
   switch (rp.type)
   {
      case Variant::BOOL:
      {
         into->set(rp.name, from->read_bool());
      } break;

      case Variant::INT:
      {
         into->set(rp.name, from->read_int());
      } break;

      case Variant::REAL:
      {
         into->set(rp.name, from->read_float());
      } break;

      case Variant::VECTOR2:
      {
         into->set(rp.name, from->read_vector2());
      } break;

      case Variant::RECT2:
      {
         into->set(rp.name, from->read_rect2());
      } break;

      case Variant::QUAT:
      {
         into->set(rp.name, from->read_quat());
      } break;

      case Variant::COLOR:
      {
         into->set(rp.name, from->read_color());
      } break;

      case Variant::VECTOR3:
      {
         into->set(rp.name, from->read_vector3());
      } break;

      case kehSnapEntityBase::CTYPE_UINT:
      {
         into->set(rp.name, from->read_uint());
      } break;

      case kehSnapEntityBase::CTYPE_BYTE:
      {
         into->set(rp.name, from->read_byte());
      } break;

      case kehSnapEntityBase::CTYPE_USHORT:
      {
         into->set(rp.name, from->read_ushort());
      } break;

      case Variant::STRING:
      {
         into->set(rp.name, from->read_string());
      } break;

      case Variant::POOL_BYTE_ARRAY:
      {
         const uint32_t s = from->read_byte();
         PoolByteArray a;
         for (uint32_t i = 0; i < s; i++)
         {
            a.append(from->read_byte());
         }
         into->set(rp.name, a);
      } break;

      case Variant::POOL_INT_ARRAY:
      {
         const uint32_t s = from->read_byte();
         PoolIntArray a;
         for (uint32_t i = 0; i < s; i++)
         {
            a.append(from->read_int());
         }
         into->set(rp.name, a);
      }

      case Variant::POOL_REAL_ARRAY:
      {
         const uint32_t s = from->read_byte();
         PoolRealArray a;
         for (uint32_t i = 0; i < s; i++)
         {
            a.append(from->read_float());
         }
         into->set(rp.name, a);
      } break;
   }
}


uint32_t kehEntityInfo::extract_change_mask(Ref<kehEncDecBuffer>& from) const
{
   switch (m_cmask_size)
   {
      case 1:
         return from->read_byte();
      case 2:
         return from->read_ushort();
      case 4:
         return from->read_uint();
   }
   // TODO: error out here
   return -1;
}



void kehEntityInfo::_notification(int what)
{
   if (what == NOTIFICATION_PREDELETE)
   {
      m_resource = Ref<Script>(NULL);
      
   }
}


kehEntityInfo::kehEntityInfo() :
   m_name_hash(0),
   m_resource(NULL),
   m_namestr(""),
   m_has_chash(true)
{

}


kehEntityInfo::~kehEntityInfo()
{
   m_resource = Ref<Script>(NULL);
   m_replicable.resize(0);
}