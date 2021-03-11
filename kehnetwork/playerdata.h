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


#ifndef _KEHNETWORK_PLAYERDATA_H
#define _KEHNETWORK_PLAYERDATA_H 1

#include "core/reference.h"
#include "inputinfo.h"
#include "functoid.h"
#include "customproperty.h"

class kehPlayerNode;


class kehPlayerData : public Reference
{
   GDCLASS(kehPlayerData, Reference);
private:
   // Hold a node belonging to itself in a different place, which will make things easier specially when handling
   // single player mode.
   kehPlayerNode* m_local_player;
   // This will hold the list of remote players. If connected to the server, host player will also be part of this list.
   Map<uint32_t, kehPlayerNode*> m_remote_player;
   // Hold information related to how input will be encoded/decoded by the system.
   kehInputInfo m_input_info;

   // Holds the registered custom properties. When a player node is created, a copy of this map must be attached into
   // that node. Note that it must be a copy and not a reference since the custom properties are potentially different
   // on each player.
   // This also have a secondary use: since it will always hold the initial value it will server to determine the
   // expected value type when encoding and decoding.
   Map<String, Ref<kehCustomProperty>> m_custom_property;


   // This will be used to hold the actual function that will emit a signal whenever a new ping value arrives.
   // The kehNetwork singleton node will setup this "functoid"
   kehFunctoid<void(uint32_t, float)> m_ping_signaler;

   kehFunctoid<void(uint32_t, const String&, const Variant&)> m_cprop_signaler;
   kehFunctoid<void(const String&, const Variant&)> m_cprop_broadcaster;

private:
   // This helper function is meant to create player node, set everything that is necessary and return the pointer.
   kehPlayerNode* create_pnode(uint32_t pid, bool local);

protected:
   void _notification(int what);

   static void _bind_methods();

public:
   // Signaler setters
   void set_ping_signaler(const kehFunctoid<void(uint32_t, float)>& functoid) { m_ping_signaler = functoid; }
   void set_cprop_signaler(const kehFunctoid<void(uint32_t, const String&, const Variant&)>& functoid) { m_cprop_signaler = functoid; }
   void set_cprop_broadcaster(const kehFunctoid<void(const String&, const Variant&)>& functoid) { m_cprop_broadcaster = functoid; }


   const Map<String, Ref<kehCustomProperty>>& get_custom_props() const { return m_custom_property; }

   kehPlayerNode* create_local_player();
   kehPlayerNode* get_local_player() const { return m_local_player; }

   // This will perform some extra setup on the node, but will not automatically add it into the internal container.
   kehPlayerNode* create_player(uint32_t id);

   // Retrieve the remote player node given its network ID. Null will be returned if there is no registered player with
   // the provided ID.
   kehPlayerNode* get_remote_player(uint32_t pid) const;

   // Fill the provided array with the list of remote registered players
   void get_remote_player_list(PoolVector<uint32_t>& outlist) const;

   // Remote the remote player from internal container given its network ID
   void remove_remote_player(uint32_t pid);

   // Add a player node to the internal remote player list
   void add_remote(kehPlayerNode* pnode);
   // Remove all nodes corresponding to remote players from the internal container (and the tree)
   void clear_remote();

   // Retrieve a player node given its network ID.
   kehPlayerNode* get_pnode(uint32_t pid) const;

   // Obtain number of registered players, including the local one
   uint32_t get_player_count() const { return m_remote_player.size() + 1; }


   // Add (register) a custom player property
   void add_custom_property(const String& pname, const Variant& default_value, kehCustomProperty::ReplicationMode mode = kehCustomProperty::ReplicationMode::ServerOnly);

   // Set a custom property within the local player node
   void set_custom_property(const String& pname, const Variant& value);

   // Retrieve the value of a custom property
   Variant get_custom_property(const String& pname, const Variant& defval = NULL) const;


   // Relay into the input info object
   void register_action(const String& action, bool is_analog);
   void register_custom_action(const String& action, bool is_analog);
   void register_custom_vec2(const String& vec);
   void register_custom_vec3(const String& vec);

   void set_action_enabled(const String& action, bool enabled);
   void set_use_mouse_relative(bool use);
   void set_use_mouse_speed(bool use);
   
   void reset_input();

   // Fill the provided array with the list of unique IDs of all registered remote players
   void fill_remote_player_list(PoolVector<uint32_t>& out) const;
   // Fill the provided array with the nodes of all registered remote players
   void fill_remote_player_node(PoolVector<kehPlayerNode*>& out) const;

   // Obtains the registered players in an array containing instances of kehPlayerNode
   // If the "include_local" is true, then the local player will be part of the array, as the very first element.
   // This is using the Array (which is of Variants) because the function is meant to be exposed to scripting
   Array get_registered_players(bool include_local = false) const;

   // Retrieve the "front iterator" of the remote player map container. This can be used to iterate over registered
   // remote players and perform tasks as necessary
   Map<uint32_t, kehPlayerNode*>::Element* get_remote_iterator() const { return m_remote_player.front(); }


   kehPlayerData();
   ~kehPlayerData();
};


#endif
