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

#include "snapshotdata.h"
#include "snapshot.h"
#include "entityinfo.h"
#include "snapentity.h"
#include "nodespawner.h"

#include "../kehgeneral/encdecbuffer.h"


#include "scene/main/node.h"
#include "core/project_settings.h"
#include "core/io/resource_loader.h"



void kehSnapshotData::register_entity_types()
{
   ProjectSettings* psettings = ProjectSettings::get_singleton();
   Array clist;

   if (psettings->has_setting("_global_script_classes"))
   {
      clist = psettings->get("_global_script_classes");
   }

   const bool pdebug = GLOBAL_GET("keh_modules/network/general/print_debug_info");

   for (int i = 0; i < clist.size() ; i++)
   {
      Dictionary entry = clist[i];

      if (entry.has("base") && entry["base"] == "kehSnapEntityBase")
      {
         String cname(entry["class"]);
         String rpath(entry["path"]);

         // EntityInfo is a typedef, Ref<kehEntityInfo>
         EntityInfo info = memnew(kehEntityInfo);
         String err = info->check(cname, rpath);

         if (err.empty())
         {
            if (pdebug)
            {
               print_line(vformat("Registering snapshot object type '%s' with hash %d", cname, info->get_name_hash()));
            }

            // The actual registration
            m_entity_info[info->get_name_hash()] = info;

            // This will help obtain the necessary data given the class' resource
            m_entity_name[info->get_resource()] = ResInfo(cname, info->get_name_hash());
         }
         else
         {
            WARN_PRINTS(vformat("Skipping registration of class '%s' (%d). Reason: %s", cname, info->get_name_hash(), err))
         }
      }
   }
}


void kehSnapshotData::get_entity_types(PoolVector<uint32_t>& out_keys) const
{
   for (const Map<uint32_t, EntityInfo>::Element* e = m_entity_info.front(); e; e = e->next())
   {
      out_keys.append(e->key());
   }
}


void kehSnapshotData::register_spawner(const Ref<Script>& eclass, uint32_t chash, const Ref<kehNetNodeSpawner>& spawner, Node* parent, const Ref<FuncRef>& esetup)
{
   if (esetup.is_valid())
   {
      ERR_FAIL_COND_MSG(!esetup->is_valid(), "Registering a Node spawner, an extra setup FuncRef was given but it's invalid.");
   }

   const Map<Ref<Script>, ResInfo>::Element* ename = m_entity_name.find(eclass);
   ERR_FAIL_COND_MSG(!ename, vformat("Trying to register spawner associated with snapshot entity class defined in '%s', which is not registered.", eclass->get_path()));

   Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ename->value().hash);
   einfo->value()->register_spawner(chash, spawner, parent, esetup);
}

Node* kehSnapshotData::spawn_node(const Ref<Script>& eclass, uint32_t uid, uint32_t chash)
{
   Node* ret = NULL;

   const Map<Ref<Script>, ResInfo>::Element* ename = m_entity_name.find(eclass);
   if (ename)
   {
      Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ename->value().hash);
      ret = einfo->value()->spawn_node(uid, chash);
   }

   return ret;
}

Node* kehSnapshotData::get_game_node(uint32_t uid, const Ref<Script>& eclass) const
{
   Node* ret = NULL;

   const Map<Ref<Script>, ResInfo>::Element* ename = m_entity_name.find(eclass);
   if (ename)
   {
      const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ename->value().hash);
      ret = einfo->value()->get_game_node(uid);
   }

   return ret;
}

uint32_t kehSnapshotData::get_prediction_count(uint32_t uid, const Ref<Script>& eclass) const
{
   uint32_t ret = 0;

   const Map<Ref<Script>, ResInfo>::Element* ename = m_entity_name.find(eclass);
   if (ename)
   {
      const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ename->value().hash);
      ret = einfo->value()->get_pred_count(uid);
   }

   return ret;
}

void kehSnapshotData::despawn_node(const Ref<Script>& eclass, uint32_t uid)
{
   const Map<Ref<Script>, ResInfo>::Element* ename = m_entity_name.find(eclass);
   if (ename)
   {
      Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ename->value().hash);
      einfo->value()->despawn_node(uid);
   }
}


void kehSnapshotData::add_pre_spawned_node(const Ref<Script>& eclass, uint32_t uid, Node* node)
{
   const Map<Ref<Script>, ResInfo>::Element* ename = m_entity_name.find(eclass);
   if (ename)
   {
      Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ename->value().hash);
      einfo->value()->add_pre_spawned(uid, node);
   }
}


void kehSnapshotData::add_to_history(const Ref<kehSnapshot>& snapshot)
{
   m_history.push_back(snapshot);
   m_ssig_to_snap[snapshot->get_signature()] = snapshot;
   m_isig_to_snap[snapshot->get_input_sig()] = snapshot;
}

void kehSnapshotData::check_history_size(uint32_t maxsize, bool has_authority)
{
   int32_t popped = 0;

   while (m_history.size() > maxsize)
   {
      m_ssig_to_snap.erase(m_history[0]->get_signature());
      m_isig_to_snap.erase(m_history[0]->get_input_sig());
      m_history.remove(0);
      popped++;
   }

   if (has_authority)
   {
      update_prediction_count(1 - popped);
   }
}


Ref<kehSnapshot> kehSnapshotData::get_snapshot(uint32_t signature) const
{
   const Map<uint32_t, Ref<kehSnapshot>>::Element* e = m_ssig_to_snap.find(signature);
   return (e ? e->value() : NULL);
}

Ref<kehSnapshot> kehSnapshotData::get_snapshot_by_input(uint32_t isig) const
{
   const Map<uint32_t, Ref<kehSnapshot>>::Element* e = m_isig_to_snap.find(isig);
   return (e ? e->value() : NULL);
}


void kehSnapshotData::reset()
{
   for (Map<uint32_t, EntityInfo>::Element* e = m_entity_info.front(); e; e = e->next())
   {
      e->value()->clear_nodes();
   }

   m_server_state = Ref<kehSnapshot>(NULL);
   m_history.resize(0);
   m_ssig_to_snap.clear();
   m_isig_to_snap.clear();
}


void kehSnapshotData::client_check_snapshot(const Ref<kehSnapshot>& snapshot)
{
   // This function is meant to be run on clients but not called remotely. The objective here is to take the
   // provided snapshot, which contains server data, locate the internal corresponding snapshot and chompare them.
   // Any differences are to be considered as errors in the client's prediction and must be corrected using
   // the server data (the incoming snapshot object).
   // Corresponding snapshot means, primarily, the snapshot with the same input signatures. However, it's
   // possible the server will send snapshot without any client's input. This will always be the case at the
   // very beginning of the client's session, when there is not server data to initiate the local simulation.
   // Still, if there is enough data loss, during the synchronization then the server will have to send snapshot
   // data without any client's input.
   // That said, the overall tasks here:
   // - Snapshot contains input -> locate the snapshot in the history with the same signature and use that one to
   //   compare, removing any older (or equal) from the history.
   // - Snapshot does not contain input -> take the last snapshot in the history to use for comparison and don't
   //   remove anything from the history.
   // During the comparison, any difference must be corrected by applying the server state into all snapshots in
   // the local snapshot history container.
   // On errors the ideal is to locally re-simulate the game using cached input data just so no input is missed
   // on small errors. Since this is not possible with Godot in a generic way then just apply the corrected
   // state into the corresponding nodes and hope the interpolation will make things look "less glitchy".
   Ref<kehSnapshot> local;
   int32_t popcount = 0;
   const uint32_t isig = snapshot->get_input_sig();

   if (isig > 0)
   {
      Ref<kehSnapshot> finding_snap;
      while (m_history.size() > 0 && m_history[0]->get_input_sig() <= isig)
      {
         finding_snap = m_history[0];
         m_ssig_to_snap.erase(finding_snap->get_signature());
         m_isig_to_snap.erase(finding_snap->get_input_sig());
         m_history.remove(0);
         popcount++;
      }

      if (finding_snap.is_valid())
      {
         if (finding_snap->get_input_sig() != isig)
         {
            // This should not occur
            update_prediction_count(-popcount);
            return;
         }
         local = finding_snap;
      }
   }
   else
   {
      if (m_history.size() > 0)
      {
         local = m_history[m_history.size() - 1];
      }
   }

   if (!local.is_valid())
   {
      update_prediction_count(-popcount);
      // This should not occur...
      return;
   }

   m_server_state = snapshot;

   for (Map<uint32_t, EntityInfo>::Element* ehash = m_entity_info.front(); ehash; ehash = ehash->next())
   {
      EntityInfo einfo = ehash->value();

      // TODO: Remove this from release builds, keeping only on debug/editor builds.
      ERR_FAIL_COND_MSG(!local->has_type(ehash->key()) || !snapshot->has_type(ehash->key()), "Entity type must exist on both ends.");

      // Obtain list of entities in the local snapshot. This will be used to track ones that are locally
      // present but not in the server data.
      Set<uint32_t> local_entity;
      local->get_entity_uids(ehash->key(), local_entity);

      // Iterate through entities of the server snapshot
      const kehSnapshot::entity_data_t::Element* ecol = snapshot->get_entity_collection(ehash->key());

      for (uint32_t i = 0; i < ecol->value().entity_array.size(); i++)
      {
         Ref<kehSnapEntityBase> rentity = ecol->value().entity_array[i];
         Ref<kehSnapEntityBase> lentity = local->get_entity(ehash->key(), rentity->get_uid());
         Node* node = NULL;

         if (rentity.is_valid() && lentity.is_valid())
         {
            // Entity exists on both ends. First update the local entity array because it's meant to
            // hold entities that are present only in the local machine (client)
            local_entity.erase(rentity->get_uid());

            // Check if there is any difference
            const uint32_t cmask = einfo->calculate_change_mask(rentity, lentity);

            if (cmask > 0)
            {
               // There is at least one property with different values. This means it must be corrected.
               // For now just obtain the corresponding node. At the end of this function if the variable
               // is valid the apply_state() will be called.
               node = einfo->get_game_node(rentity->get_uid());
            }
         }
         else
         {
            // Entity exists only on the server data. If necessary spawn the game node.
            Node* n = einfo->get_game_node(rentity->get_uid());
            if (!n)
            {
               node = einfo->spawn_node(rentity->get_uid(), rentity->get_class_hash());
            }
         }

         if (node)
         {
            // If here then it's necessary to apply the server state into the node
            rentity->call("apply_state", node);

            // Propagate the new data into every snapshot in the local history.
            for (uint32_t i = 0; i < m_history.size(); i++)
            {
               m_history[i]->add_entity(ehash->key(), einfo->clone_entity(rentity));
            }
         }
      }

      // Check entities that are in the local snapshot but not in the remote (server) one.
      // The local ones must be removed from the game
      for (Set<uint32_t>::Element* e = local_entity.front(); e; e = e->next())
      {
         despawn_node(einfo->get_resource(), e->get());
      }
   }

   // All entities have been verified. Update the prediction count
   update_prediction_count(-popcount);
}


void kehSnapshotData::encode_full(const Ref<kehSnapshot>& snapshot, Ref<kehEncDecBuffer>& into, uint32_t input_sig) const
{
   // Encode the signature of the snapshot
   into->write_uint(snapshot->get_signature());

   // Encode input signature
   into->write_uint(input_sig);

   for (const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.front(); einfo; einfo = einfo->next())
   {
      const kehSnapshot::entity_data_t::Element* ecol = snapshot->get_entity_collection(einfo->key());
      if (!ecol)
      {
         // This should not happen. However, should this error out?
         return;
      }

      // Obtain entity count. Don't encode anything for this type if count is 0.
      const uint32_t ecount = ecol->value().entity_array.size();
      if (ecount == 0)
         continue;
      
      // Ok, there is at least one entity in the snapshot. Encode the type hash ID
      into->write_uint(einfo->key());
      // Then the entity count
      into->write_uint(ecount);

      // Now iterate through all entities within the array
      for (uint32_t i = 0; i < ecount; i++)
      {
         einfo->value()->encode_full_entity(ecol->value().entity_array[i], into);
      }
   }

}

Ref<kehSnapshot> kehSnapshotData::decode_full(Ref<kehEncDecBuffer> from) const
{
   // Decode the signature of the snapshot
   const uint32_t sig = from->read_uint();
   // Decode input signature
   const uint32_t isig = from->read_uint();

   if (isig > 0 && m_history.size() > 0 && isig < m_history[0]->get_input_sig())
   {
      // The input signature of the incoming data is older than the oldest snapshot within local history.
      // Because of that, ignore the received snapshot
      return NULL;
   }

   Ref<kehSnapshot> ret = memnew(kehSnapshot(sig, isig));

   // The snapshot checking algorithm requires that each entity type has its entry within the snapshot data.
   for (const Map<uint32_t, EntityInfo>::Element* e = m_entity_info.front(); e; e = e->next())
   {
      ret->add_type(e->key());
   }

   while (from->has_read_data())
   {
      // Read the entity type ID
      const uint32_t ehash = from->read_uint();

      const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ehash);

      if (!einfo)
      {
         // FIXME: put an error message
         return NULL;
      }

      // Take number of entities of this type
      const uint32_t count = from->read_uint();

      // Decode the entities of this type
      for (uint32_t i = 0; i < count; i++)
      {
         Ref<kehSnapEntityBase> entity = einfo->value()->decode_full_entity(from);
         if (entity.is_valid())
         {
            ret->add_entity(ehash, entity);
         }
      }
   }


   return ret;
}

void kehSnapshotData::encode_delta(const Ref<kehSnapshot>& snap, const Ref<kehSnapshot>& oldsnap, Ref<kehEncDecBuffer>& into, uint32_t isig) const
{
   // Scan oldsnap comparing to snap. Encode only the changes. Removed entities must be explicitly marked
   // with a changed mask = 0.
   // Scanning will iterate through entities in the snap object. The same entity will be retrieved from oldsnap.
   // If not found then assume iterate entity is new. A list must be used to keep track of entities that are in
   // the older snapshot but not in the newer one, indicating removed game objects.

   // Write snapshot signature
   into->write_uint(snap->get_signature());

   // Then the input signature
   into->write_uint(isig);

   // Encode a flag indicating if there is any change at all in this snapshot. Assume there isn't
   into->write_bool(false);
   
   // But not for the actual flag here. It's easier to change this to true
   bool has_data = false;

   // During entity scanning, entries from this container will be removed. After the loop any remaining
   // entries are entities removed from the game
   Map<uint32_t, Set<uint32_t>> tracker;
   oldsnap->build_tracker(tracker);

   // Iterate through valid entity types
   for (const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.front(); einfo; einfo = einfo->next())
   {
      // Obtain easy access to the entity collections
      const kehSnapshot::entity_data_t::Element* necol = snap->get_entity_collection(einfo->key());
      const kehSnapshot::entity_data_t::Element* oecol = oldsnap->get_entity_collection(einfo->key());

      // Get entity count in the recent snapshot
      const uint32_t necount = necol->value().entity_array.size();
      // Get entity count in the old snapshot
      const uint32_t oecount = oecol->value().entity_array.size();

      // Skip this entity type if both quantities are 0
      if (necount == 0 && oecount == 0)
         continue;
      
      // At least one of the snapshots contains entities of this type. Assume every single one has been changed
      uint32_t ccount = necount;

      // Postponing encoding of type has and change count to a moment where it is sure there is at least one
      // changed entity of this type. Originally tried to do things normally and remove the relevant bytes from
      // the buffer array but it didn't work very well.
      // This flag is used to tell if the type hash and change count have been encded or not, just to prevent
      // multiple encodings of this data
      bool written_type_header = false;

      // Get writing position of the entity count as most likely it will be updated (rewritten)
      const uint32_t countpos = into->get_current_size() + 4;

      // Iterate through the entities of this type.
      for (uint32_t i = 0; i < necount; i++)
      {
         // Retrieve the new state of the entity.
         Ref<kehSnapEntityBase> enew = necol->value().entity_array[i];

         // Retrieve the old state of the entity - obviously if it exists.
         const Map<uint32_t, Ref<kehSnapEntityBase>>::Element* eoldit = oecol->value().uid_to_entity.find(enew->get_uid());
         Ref<kehSnapEntityBase> eold = eoldit ? eoldit->value() : NULL;

         // Assume the entity is new
         uint32_t cmask = einfo->value()->get_full_change_mask();

         if (enew.is_valid() && eold.is_valid())
         {
            // The entity exist on both snapshots so it's not new. Calculate the "real" change mask.
            cmask = einfo->value()->calculate_change_mask(eold, enew);

            // Remove this from the tracker so it isn't considered as a removed entity
            tracker[einfo->key()].erase(enew->get_uid());
         }

         if (cmask != 0)
         {
            if (!written_type_header)
            {
               // Write the entity type hash ID
               into->write_uint(einfo->key());

               // Write the change counter
               into->write_uint(ccount);

               // Prevent rewriting of this information
               written_type_header = true;
            }

            // This entity requires encoding. Do so
            einfo->value()->encode_delta_entity(enew->get_uid(), enew, cmask, into);
            has_data = true;
         }
         else
         {
            // The entity was not changed. Update the change counter
            ccount--;
         }
      }

      // Check th tracker for entities that are in the old snapshot but not in the new one.
      // In other words, entities that were removed from the game world.
      // Those must be encoded with a change mask set to 0, which will indicate "remove entities"
      // when decoding the data.
      for (Set<uint32_t>::Element* uid = tracker[einfo->key()].front(); uid; uid = uid->next())
      {
         if (!written_type_header)
         {
            into->write_uint(einfo->key());
            into->write_uint(ccount);
            written_type_header = true;
         }

         einfo->value()->encode_delta_entity(uid->get(), NULL, 0, into);
         ccount++;
      }

      if (ccount > 0)
      {
         into->rewrite_uint(ccount, countpos);
      }
   }

   // Everything iterated through. Check if there is anything encoded at all
   if (has_data)
      into->rewrite_bool(true, 8);
}


Ref<kehSnapshot> kehSnapshotData::decode_delta(Ref<kehEncDecBuffer>& from) const
{
   // Decode snapshot signature
   const uint32_t snapsig = from->read_uint();
   // Input signature
   const uint32_t isig = from->read_uint();

   if (isig > 0 && m_history.size() > 0 && isig < m_history[0]->get_input_sig())
   {
      // The input signature of the incoming data is older than the oldest snapshot within local history.
      // Because of that, ignore the received snapshot
      return NULL;
   }

   Ref<kehSnapshot> ret = memnew(kehSnapshot(snapsig, isig));

   // The snapshot checking algorithm requires that each entity type has its entry within the snapshot data.
   for (const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.front(); einfo; einfo = einfo->next())
   {
      ret->add_type(einfo->key());
   }

   // Read the "has_data" flag
   const bool has_data = from->read_bool();

   // This will be used to track unchanged entities. Basically, when an entity is decoded the corresponding
   // entry will be removed from this data. After that, remaining entries here are indicating entities that
   // didn't change and must be copied from m_server_state into the new snapshot.
   Map<uint32_t, Set<uint32_t>> tracker;
   m_server_state->build_tracker(tracker);

   if (has_data)
   {
      while (from->has_read_data())
      {
         const uint32_t ehash = from->read_uint();
         const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ehash);

         if (!einfo)
         {
            print_error(vformat("While decoding delta snapshot data, got an entity type hash %d which doesn't map to any valid registered entity type.", ehash));
            return NULL;
         }

         // Take number of encoded entities of this type
         const uint32_t count = from->read_uint();

         // Decode those entities
         for (uint32_t i = 0; i < count; i++)
         {
            uint32_t cmask = 0;
            Ref<kehSnapEntityBase> nent = einfo->value()->decode_delta_entity(from, cmask);

            Ref<kehSnapEntityBase> oldent = m_server_state->get_entity(ehash, nent->get_uid());

            if (oldent.is_valid())
            {
               // The entity exists in the old state. Check if it's not marked for removal
               if (cmask > 0)
               {
                  // It isn't so "match" the delta to make the data correct (that is, take unchanged)
                  // data from the old state and apply into the new one.
                  einfo->value()->match_delta(nent, oldent, cmask);

                  // Add the changed entity into the decoded snapshot
                  ret->add_entity(ehash, nent);
               }

               // This is a changed entity so remove it from the tracker
               tracker[ehash].erase(nent->get_uid());
            }
            else
            {
               // Entity is not in the old state. Add the decoded data into the return value in the hopes
               // it is holding the entire correct data (this can be checked by comparing the cmask through).
               // Change mask can be 0 in this case, when the acknowledgement still didn't arrive there, when
               // server dispatched a new data set.
               if (cmask > 0)
               {
                  ret->add_entity(ehash, nent);
               }
            }
         }
      }
   }

   // Check the tracker
   for (Map<uint32_t, Set<uint32_t>>::Element* ehash = tracker.front(); ehash; ehash = ehash->next())
   {
      const Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.find(ehash->key());

      for (Set<uint32_t>::Element* uid = ehash->value().front(); uid; uid = uid->next())
      {
         Ref<kehSnapEntityBase> entity = m_server_state->get_entity(ehash->key(), uid->get());

         ret->add_entity(ehash->key(), einfo->value()->clone_entity(entity));
      }
   }


   return ret;
}



uint32_t kehSnapshotData::get_ehash(const Ref<Script>& script) const
{
   uint32_t ret = 0;
   const Map<Ref<Script>, ResInfo>::Element* e = m_entity_name.find(script);

   if (e)
   {
      ret = e->value().hash;
   }
   else
   {
      // TODO: print warning indicating that it was not possible to retrieve required data based on its script.
   }

   return ret;
}


Ref<kehSnapEntityBase> kehSnapshotData::instantiate_snap_entity(const Ref<Script>& snap_entity, uint32_t uid, uint32_t chash) const
{
   Ref<kehSnapEntityBase> ret;

   const Map<Ref<Script>, ResInfo>::Element* e = m_entity_name.find(snap_entity);

   if (e)
   {
      EntityInfo einfo = m_entity_info[e->value().hash];
      ret = einfo->create_instance(uid, chash);
   }

   return ret;
}


String kehSnapshotData::get_comparer_data(const Ref<Script>& eclass) const
{
   const Map<Ref<Script>, ResInfo>::Element* e = m_entity_name.find(eclass);
   if (e)
   {
      EntityInfo einfo = m_entity_info[e->value().hash];
      return einfo->get_comp_data();
   }
   
   // TODO: make this a message to make it clear that the specified script is not registered.
   return "";
}



void kehSnapshotData::update_prediction_count(int32_t delta)
{
   for (Map<uint32_t, EntityInfo>::Element* einfo = m_entity_info.front(); einfo; einfo = einfo->next())
   {
      einfo->value()->update_pred_count(delta);
   }
}



void kehSnapshotData::_notification(int what)
{
   switch (what)
   {
      case NOTIFICATION_PREDELETE:
      {
         m_entity_info.clear();
         kehPropComparer::cleanup();
      }
   }
}



void kehSnapshotData::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("register_spawner", "eclass", "chash", "spawner", "parent", "esetup"), &kehSnapshotData::register_spawner);
   ClassDB::bind_method(D_METHOD("spawn_node", "eclass", "uid", "chash"), &kehSnapshotData::spawn_node);
   ClassDB::bind_method(D_METHOD("get_game_node", "uid", "eclass"), &kehSnapshotData::get_game_node);
   ClassDB::bind_method(D_METHOD("get_prediction_count", "uid", "eclass"), &kehSnapshotData::get_prediction_count);
   ClassDB::bind_method(D_METHOD("despawn_node", "eclass", "uid"), &kehSnapshotData::despawn_node);
   ClassDB::bind_method(D_METHOD("add_pre_spawned_node", "eclass", "uid", "node"), &kehSnapshotData::add_pre_spawned_node);

   ClassDB::bind_method(D_METHOD("get_comparer_data", "eclass"), &kehSnapshotData::get_comparer_data);
}

kehSnapshotData::kehSnapshotData()
{
   register_entity_types();
}

kehSnapshotData::~kehSnapshotData()
{
}
