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


#ifndef _KEHNETWORK_PLAYERNODE_H
#define _KEHNETWORK_PLAYERNODE_H 1

#include "scene/main/node.h"

#include "inputcache.h"
#include "functoid.h"
#include "customproperty.h"

class kehInputInfo;
class kehInputData;
class kehEncDecBuffer;
class kehPingInfo;

class kehPlayerNode : public Node
{
   GDCLASS(kehPlayerNode, Node);
private:
   // The network ID
   uint32_t m_net_id;

   // This flag indicates if the player node is local or not. This is mostly relevant only on the server
   bool m_is_local;

   // Server will only send snapshot data to this client when this flag is set to true
   bool m_is_ready;

   // If this is set to false, then input polling will be disabled (obviously this is only relevant for
   // local player)
   bool m_input_enabled;

   // These vectors will be used to cache mouse data through the _input() function.
   // Obviously those will only be used on the local machine
   Vector2 m_mrelative;
   Vector2 m_mspeed;

   // Hold input information.
   kehInputInfo* m_input_info;

   // The input cache - it will work differently depending if this node is for local player or not and if
   // if this is server or not.
   kehInputCache m_input_cache;

   // Used to encode and decode input data
   Ref<kehEncDecBuffer> m_encdec;

   // This will be obtained from ProjectSettings. If true, then measured ping values will be broadcast
   // from server to all connected clients.
   bool m_broadcast_ping;

   // Created only on servers for remote players, this will control the entire "ping/pong" loop.
   Ref<kehPingInfo> m_ping;


   // Hold the custom properties of this player
   Map<String, Ref<kehCustomProperty>> m_custom_data;
   // And cache how many dirty properties have been changed and not replicated yet
   uint32_t m_custom_prop_dirty_count;


   kehFunctoid<void(uint32_t, float)> m_ping_signaler;
   kehFunctoid<void(uint32_t, const String&, const Variant&)> m_custom_prop_signaler;
   kehFunctoid<void(const String&, const Variant&)> m_custom_prop_broadcast_requester;

private:
   // Will be internally called only if this node is belonging to the local player.
   Ref<kehInputData> poll_input();

   // This function is meant to be run only on servers, called by clients. This is where input data will be
   // received and decoded to be added into internal cache
   void server_receive_input(const PoolByteArray& encoded);

   // When the interval timer expires, a function will be called and that function will remote call this, which
   // is meant to be run only on client machines. When this function is executed, it will remote call the "server_pong",
   // which is basically where the time will be measured
   void client_ping(uint32_t sig, float last);

   void server_pong(uint32_t sig);

   // If the broadcast ping option is enabled then the server will call this function on each client in
   // order to give the measure ping value and allow other clients to display somewhere the player's
   // latency values.
   void client_ping_broadcast(float value);


protected:
   void _notification(int what);

   void _input(const Ref<InputEvent>& evt);

   static void _bind_methods();

public:
   void set_ping_signaler(const kehFunctoid<void(uint32_t, float)>& functoid) { m_ping_signaler = functoid; }
   void set_cprop_signaler(const kehFunctoid<void(uint32_t, const String&, const Variant&)>& functoid) { m_custom_prop_signaler = functoid; }
   void set_cprop_broadcaster(const kehFunctoid<void(const String&, const Variant&)>& functoid) { m_custom_prop_broadcast_requester = functoid; }
   

   uint32_t get_id() const { return m_net_id; }
   void set_id(uint32_t i);

   bool is_ready() const { return m_is_ready; }
   void set_is_ready(bool ready) { m_is_ready = ready; }

   void set_input_enabled(bool enabled) { m_input_enabled = enabled; }

   void reset_data();

   // Obtain input data. If running on the local machine the state will be polled. If on a
   // client (still local machine) then the data will be sent to the server.
   // If on server (but not local) the data will be retrieved from cache/buffer.
   Ref<kehInputData> get_input(uint32_t snapsig);

   // Must be called only on client machines belonging to the local player. All of the cached
   // input data will be encoded and sent to the server.
   void dispatch_input_data();

   // Get the signature of the last input data used on this machine
   uint32_t get_last_input_signature() const { return m_input_cache.get_last_sig(); }

   // Given the snapshot signature, return the input signature that was used. THis will be valid only on servers
   // on a node corresponding to a client
   uint32_t get_used_input_in_snap(uint32_t snap_sig) const;


   // Retrieve the signature of the last acknowledged snapshot
   uint32_t get_last_acked_snap_sig() const { return m_input_cache.get_last_ack_snap(); }

   // Retrieve the amount of non acknowledged snapshots, which will be used by the server to determine
   // if full snapshot data must be sent or not
   uint32_t get_non_acked_snap_count() const { return m_input_cache.get_non_acked_scount(); }

   // Tells if there is any non acknowledged snapshot that didn't use any input from the client
   // corresponding to this node. This is another condition that will be used to determine which data
   // will be setn to this client
   bool has_snap_with_no_input() const { return m_input_cache.has_no_input(); }

   // Meant to be run on clients but not called remotely. It removes from the cache all the
   // input objects older and equal to the specified signature
   void client_acknowledge_input(uint32_t isig);

   // This function is meant to be run on servers but not called remotely. Basically when a client receives
   // snapshot data, an answer must be given specifying the signature of the newest received. With this, internal
   // clenaup can be performed and then later only the relevant data can be sent to the client
   void server_acknowledge_snapshot(uint32_t sig) { m_input_cache.acknowledge(sig); }

   /// "ping" system.
   // This must be called only on servers, which will initialize the entire "ping/pong loop"
   void start_ping();



   /// Custom property system
   // Add/register a custom property into this player node. Note that a new kehCustomProperty will be created
   void add_custom_property(const String& pname, const Ref<kehCustomProperty>& prop);

   // Returns true if there is any dirty custom property that needs replication
   bool has_dirty_custom_prop() const { return m_custom_prop_dirty_count > 0; }

   // Exposed to scripting, allows one to set a custom property value
   void set_custom_property(const String& pname, const Variant& value);

   // This is meant to set custom property but using remote calls. This should be automatically called based
   // on the replication setting. One thing to note is that this will be called using the reliable channel.
   void remote_set_custom_property(const String& pname, const Variant& value);

   // Get custom property. If it's not found, return the "defval" instead
   Variant get_custom_property(const String& pname, const Variant& defval = NULL) const;

   // Encode the "dirty" supported custom properties into the given EncDecBuffer. If a non supported property is
   // found then it will be directly sent with the check_replication() function.
   // The props argument here is the map holding the list of registered custom properties holding their initial,
   // values, which are then used to determine the expected value type.
   // Returns true if at least one of the dirty properties is supported by the EncDecBuffer
   bool encode_custom_props(Ref<kehEncDecBuffer>& into, const Map<String, Ref<kehCustomProperty>>& props, bool is_authority, bool force_nd);

   void decode_custom_props(Ref<kehEncDecBuffer>& from, const Map<String, Ref<kehCustomProperty>>& props, bool is_authority);

   // This is used to check a property replication mode and immediately send it through the network outside
   // of the "packed custom properties". This is meant to be used when a custom property is not supported by
   // the EncDecBuffer system OR if the current value type mismatches the initial type.
   void check_replication(const String& pname, Ref<kehCustomProperty>& prop, bool is_authority);



   

   kehPlayerNode(uint32_t pid = 1, bool is_local = true, kehInputInfo* input_info = NULL);
   ~kehPlayerNode();
};


#endif
