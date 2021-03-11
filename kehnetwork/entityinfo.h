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

#ifndef _KEHNETWORK_ENTITYINFO_H
#define _KEHNETWORK_ENTITYINFO_H 1

#include "core/reference.h"
#include "core/script_language.h"
#include "core/func_ref.h"

#include "propcomparer.h"


class kehSnapEntityBase;
class kehNetNodeSpawner;

class kehEncDecBuffer;


class kehEntityInfo : public Reference
{
private:
   const int MAX_ARRAY_SIZE = 0xFF;

   struct ReplicableProperty
   {
      String name;
      int type;
      int mask;
      kehPropComparer comparer;

      bool is_valid() const { return type != 0 && mask != 0 && comparer.is_valid(); }
      bool compare(const Variant& v1, const Variant& v2) const { return comparer(v1, v2); }

      ReplicableProperty() : type(0), mask(0) {}
   };

   struct SpawnerData
   {
      Ref<kehNetNodeSpawner> spawner;
      Node* parent;
      // Not using "functoid" because this is meant to call script functions and all the binding for that case
      // is already done with FuncRef.
      Ref<FuncRef> extra_setup;

      SpawnerData();
      SpawnerData(const Ref<kehNetNodeSpawner> s, Node* p, const Ref<FuncRef>& es);
   };

   // This inner "class" is meant to keep track of game nodes and any other extra data associate with that
   // specific node
   struct GameEntity
   {
      Node* node;
      // Useful only on clients, keep track of how many frames were used during prediction for this entity.
      // This can be used later to re-simulate entities that don't require any input data when correction
      // is applied.
      uint16_t predcount;

      GameEntity(Node* n = NULL, uint16_t pc = 0) : 
         node(n), predcount(pc) {}
   };

   // Entity type name is hashed into this
   uint32_t m_name_hash;
   // Resource used to create instances of entities described by this EntityInfo
   Ref<Script> m_resource;
   // List of properties that can be replicated
   PoolVector<ReplicableProperty> m_replicable;
   // Class name of the entity. Used mostly for debugging
   String m_namestr;
   // Snapshot entities may disable class_hash and this info is cached here
   bool m_has_chash;

   // When encoding delta snapshot, the change mask has to be encoded before the entity itself. This variable
   // holds how many bytes (1, 2 or 4) are used for this information within the raw data for this entity type.
   // This sort of limit the number of properties per entity to only 30/31 (ID takes one spot and if not disabled)
   // class_hash takes another).
   uint8_t m_cmask_size;

   // Holds all spanwed entities. This will allow the network system to keep track of all entities that require
   // replication within the snapshots. Map key is entity Unique ID.
   Map<uint32_t, GameEntity> m_entity;

   // Key is the class hash. This sort of force the creation of multiple spawners even if the actual class_hash
   // only points to inner properties of the spawned node. This design decision greatly simplifies the automatic
   // snapshot system.
   Map<uint32_t, SpawnerData> m_spawner_data;

private:
   // Builds an instance of the inner "class" ReplicableProperty
   ReplicableProperty build_replicable_prop(const String& name, Variant::Type type, int mask, kehSnapEntityBase* dummy);

   // Given an instance of ReplicableProperty, writes the corresponding entity property into the given EncDecBuffer
   void property_writer(const ReplicableProperty& rp, const Ref<kehSnapEntityBase>& entity, Ref<kehEncDecBuffer>& into) const;

   // Based on the given instance of ReplicableProperty, reads a property from the byte buffer into an instance of
   // kehSnapEntityBase object.
   void property_reader(const ReplicableProperty& rp, Ref<kehEncDecBuffer>& from, Ref<kehSnapEntityBase>& into) const;

   // Helper function to extract the change mask from given EncDecBuffer. 
   uint32_t extract_change_mask(Ref<kehEncDecBuffer>& from) const;

protected:
   void _notification(int what);


public:
   uint32_t get_name_hash() const { return m_name_hash; }
   Ref<Script> get_resource() const { return m_resource; }
   

   void register_spawner(uint32_t chash, const Ref<kehNetNodeSpawner>& spawner, Node* parent, const Ref<FuncRef>& esetup = Ref<FuncRef>());

   // Create instance of the entity described by this info
   Ref<kehSnapEntityBase> create_instance(uint32_t uid, uint32_t chash) const;
   // Create a clone of the given entity, as long as it matches this info
   Ref<kehSnapEntityBase> clone_entity(const Ref<kehSnapEntityBase>& entity) const;

   uint32_t get_change_mask_size() const { return m_cmask_size; }

   uint32_t calculate_change_mask(const Ref<kehSnapEntityBase>& e1, const Ref<kehSnapEntityBase>& e2) const;

   // Encode full entity data into the given EncDecBuffer
   void encode_full_entity(const Ref<kehSnapEntityBase>& entity, Ref<kehEncDecBuffer>& into) const;

   // Decode full entity data from the given EncDecBuffer
   Ref<kehSnapEntityBase> decode_full_entity(Ref<kehEncDecBuffer>& from) const;

   // Encode delta entity data into the given EncDecBuffer
   void encode_delta_entity(uint32_t uid, const Ref<kehSnapEntityBase>& entity, uint32_t cmask, Ref<kehEncDecBuffer>& into) const;

   // Decode delta entity data from the given EncDecBuffer
   Ref<kehSnapEntityBase> decode_delta_entity(Ref<kehEncDecBuffer>& from, uint32_t& outcmask) const;


   uint32_t get_full_change_mask() const;

   // Based on the given change mask this function is meant to transfer the different properties from
   // the "source" entity into the "changed" one.
   void match_delta(Ref<kehSnapEntityBase>& changed, const Ref<kehSnapEntityBase>& source, uint32_t cmask) const;


   /// Node spawning, despawning etc
   Node* get_game_node(uint32_t uid) const;

   // Perform full cleanup of the internal container that is used to manage game nodes
   void clear_nodes();

   // Spawn a new game node
   Node* spawn_node(uint32_t uid, uint32_t chash);

   // Remove a node from the game (and internal container)
   void despawn_node(uint32_t uid);

   // Game nodes created through the editor must be registered in order to be replicated
   void add_pre_spawned(uint32_t uid, Node* node);

   // Update the prediction count on all tracked entities by the given delta value
   // Basically, prediction_count += delta. If delta is negative, note that the count will never go
   // bellow 0.
   void update_pred_count(int32_t delta);

   // Get prediction count for the given game node ID
   uint32_t get_pred_count(uint32_t uid) const;


   String check(const String& cname, const String& cpath);

   String get_comp_data() const;


   kehEntityInfo();
   ~kehEntityInfo();
};

#endif

