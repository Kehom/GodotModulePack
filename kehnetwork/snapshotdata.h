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

#ifndef _KEHNETWORK_SNAPSHOTDATA_H
#define _KEHNETWORK_SNAPSHOTDATA_H 1

#include "core/reference.h"
#include "core/func_ref.h"


class Script;
class kehEntityInfo;
class kehSnapEntityBase;
class kehNetNodeSpawner;
class kehSnapshot;

class kehEncDecBuffer;


class kehSnapshotData : public Reference
{
   GDCLASS(kehSnapshotData, Reference);

   typedef Ref<kehEntityInfo> EntityInfo;

   // This inner "class" will be used as a helper within a map. Basically, given a script class
   // an instance of this will be held in that map. With the information in this instance it
   // is then possible to reach the EntityInfo instance within that container.
   struct ResInfo
   {
      String name;
      uint32_t hash;

      // Ideally the parametes would be required rather than optional but compilation fails
      // because maps require default constructors.
      ResInfo(const String& n = "", uint32_t h = 0) : name(n), hash(h) {}
   };

private:
   // Used to deal with "descriptions" of the entity types.
   Map<uint32_t, EntityInfo> m_entity_info;
   Map<Ref<Script>, ResInfo> m_entity_name;

   // Local snapshot history.
   PoolVector<Ref<kehSnapshot>> m_history;

   // These two maps are used for querying. One from snapshot signature into the snapshot and the
   // other from input signature into corresponding snapshot
   Map<uint32_t, Ref<kehSnapshot>> m_ssig_to_snap;
   Map<uint32_t, Ref<kehSnapshot>> m_isig_to_snap;

   // This is used only on clients. Basically this will hold the most recent snapshot data received
   // from the server. Because this is always "correct (at least at some point of the simulation)",
   // this is also used as reference to rebuild full snapshots when delta data is received.
   Ref<kehSnapshot> m_server_state;

private:
   void update_prediction_count(int32_t delta);

protected:
   void _notification(int what);

   static void _bind_methods();

public:

   // Part of the initialization. Will be called from outside but should not be exposed to scripting
   void register_entity_types();
   // Fill the given array with the entity hashed name of all registered entity types.
   void get_entity_types(PoolVector<uint32_t>& out_keys) const;

   // Register a spawner for the given SnapEntity type, which will be associated with the specified
   // class hashnumber. Optionally a function ref can be given, which will be called whenever the
   // spawner is used to spawn a new node and can be used to perform extra setup after the spawn.
   void register_spawner(const Ref<Script>& eclass, uint32_t chash, const Ref<kehNetNodeSpawner>& spawner, Node* parent, const Ref<FuncRef>& esetup = NULL);

   // Spawn a game node using a registered spawner. This must be used to spawn nodes just so the networking
   // system can track them and properly synchronize their states.
   Node* spawn_node(const Ref<Script>& eclass, uint32_t uid, uint32_t chash);

   // Retrieve a game node given its unique ID and associated snapshot entity class type
   Node* get_game_node(uint32_t uid, const Ref<Script>& eclass) const;

   // Retrieve the prediction count for the specified entity
   uint32_t get_prediction_count(uint32_t uid, const Ref<Script>& eclass) const;

   // Despawn a node from the game. It only work if the node was spawned using spawn_node() function
   void despawn_node(const Ref<Script>& eclass, uint32_t uid);

   // Register a pre-spawned node (through Editor) just so the networking system can properly track it and
   // perform the tasks to replicate it through the network.
   void add_pre_spawned_node(const Ref<Script>& eclass, uint32_t uid, Node* node);

   // Add the given snapshot into the internal history container
   void add_to_history(const Ref<kehSnapshot>& snapshot);

   // Keep the history container within the given size.
   void check_history_size(uint32_t maxsize, bool has_authority);


   // Obtain a snapshot given its signature
   Ref<kehSnapshot> get_snapshot(uint32_t signature) const;

   // Given the input signature, locate the corresponding snapshot and return it.
   Ref<kehSnapshot> get_snapshot_by_input(uint32_t isig) const;

   // Resets the snapshot data. Basically clear everything
   void reset();

   // When a client receive snapshot data, this will be used to compare to local data and perform the necessary
   // tasks to correct if necessary.
   void client_check_snapshot(const Ref<kehSnapshot>& snapshot);

   // Encode the provided snapshot into the given EncDecBuffer, "attaching" the given input signature as
   // part of the data. This function encodes the entire snapshot.
   void encode_full(const Ref<kehSnapshot>& snapshot, Ref<kehEncDecBuffer>& into, uint32_t input_sig) const;

   // Decode the (full) snapshot data from the given EncDecBuffer, returning an instance of kehSnapshot.
   Ref<kehSnapshot> decode_full(Ref<kehEncDecBuffer> from) const;

   // Take provided snapshot and compare with older one to calculate delta. Encode only the changed properties
   // into the given EncDecBuffer then send the resulting data through the network
   void encode_delta(const Ref<kehSnapshot>& snap, const Ref<kehSnapshot>& oldsnap, Ref<kehEncDecBuffer>& into, uint32_t isig) const;

   // In here the "old snapshot" is not needed because it is basically a property in this class (m_server_state)
   Ref<kehSnapshot> decode_delta(Ref<kehEncDecBuffer>& from) const;



   uint32_t get_ehash(const Ref<Script>& script) const;

   // Creates an instance of SnapEntity given its script. Unique ID and Class hash will be assigned
   Ref<kehSnapEntityBase> instantiate_snap_entity(const Ref<Script>& snap_entity, uint32_t uid, uint32_t chash) const;


   // This function is here mostly for debugging purposes. It will return a String containing detailed information
   // regarding the comparers used by each replicable property found on the given SnapEntity script.
   String get_comparer_data(const Ref<Script>& eclass) const;

   
   kehSnapshotData();
   ~kehSnapshotData();
};

#endif

