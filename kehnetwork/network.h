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

#ifndef _KEHNETWORK_NETWORK_H
#define _KEHNETWORK_NETWORK_H 1

#include "scene/main/node.h"

#include "eventinfo.h"

class kehSnapshotData;
class kehPlayerData;
class kehInputData;
class kehSnapshot;
class kehSnapEntityBase;
class kehUpdateControl;

class FuncRef;



class kehNetwork : public Node
{
   GDCLASS(kehNetwork, Node);
   typedef void(*fptr)();
private:
   enum BackMode
   {
      BM_ENet,
      BM_WebSocket,

      BM_Invalid,
   };

   static kehNetwork* s_singleton;
   fptr m_on_enter_tree;
   bool m_initialized;

   // Besides holding snapshot history, takes care of encoding and decoding replication data.
   Ref<kehSnapshotData> m_snapshot_data;
   // Keep player nodes
   Ref<kehPlayerData> m_player_data;
   // This handles the update cycle
   kehUpdateControl* m_update_control;

   // As part of the system, the server will request credentials from a connecting client. Since the credentials
   // themselves can change from project to project, a function will be called by the server (and must be run there)
   // when credentials from the client arrive. That function will get the network ID of the client as well as
   // credentials dictionary that have arrived and based on that information it must return true to accept the
   // connection and false to reject it. Said function will be pointed by this FuncRef and if it's invalid the
   // connection will be automatically accepted.
   Ref<FuncRef> m_credential_checker;

   // Cache snapshot entity types (their hash numbers). Since entity type list is not meant to change
   // during the game execution, this cache is very useful to save some CPU usage during the updates
   PoolVector<uint32_t> m_entity_type;

   // Hold registered replicated event types as well as their handlers
   Map<uint16_t, kehEventInfo> m_event_info;

   // The next ones will be obtained from ProjectSettings
   uint32_t m_compression;
   uint32_t m_backmode;             // ENet, WebSocket...
   bool m_broadcast_ping_value;
   uint32_t m_full_snap_threshold;
   uint32_t m_max_history_size;
   uint32_t m_max_client_history_size;


   // Only relevant on clients and will automatically change based on the calls to the notify_ready() and
   // notify_not_ready() functions. Basically when this is false, incoming snapshots will be ignored.
   bool m_is_ready;

// Although not exactly necessary, this (extra private) visually helps separating variables from functions, which makes
// code reading a lot easier.
private:
   // This is a helper function mostly to print warning messages in case the requested mode (ENet, Websocket...) is
   // not present in the current Godot build.
   void check_backmode();

   void on_root_completed();
   void on_root_shutdown();

   void on_player_connected(uint32_t id);
   void on_player_disconnected(uint32_t id);

   void on_connection_failed();
   void on_disconnected();


   void clear_netpeer();
   // A little helper that will perform some cleanup
   void handle_disconnection();


   /// Remote functions - that is, those that will be called by other machines in the network.
   void all_register_player(uint32_t pid);
   void all_unregister_player(uint32_t pid);

   // In this function the sender is part of the argument because it corresponds to the original
   // message sender. When the server broadcasts the message, obtaining this value from get_rpc_sender_id()
   // will not be correct if message originally came from a client.
   void all_chat_message(uint32_t sender, const String& msg, bool broadcast);

   // Called by the server and must run on client. Basically server will notify the client that a connection
   // attempt was accepted through this call.
   void client_join_accepted();
   // Server will call this to tell a client that a connection was rejected
   void client_join_rejected(const String& reason);

   // Server will call this to disconnect remote players
   void client_kicked(const String& reason);

   // The kick system has to work on a different way when using WebSockets. The problem is that the RPC telling
   // about the kick always arrive after the client has already dealt with the disconnection. But since this function
   // is called before the actual disconnection, use this to indicate the "kick"
   void client_on_websocket_close_request(int code, const String& reason);

   // This will be called locally on clients to handle incoming decoded snapshot data.
   void handle_snapshot(const Ref<kehSnapshot>& snapshot);

   // Server will call this to dispatch full snapshot data to the client.
   void client_receive_full_snapshot(const PoolByteArray& encoded);

   // Server will call this to dispatch delta snapshot data to the client.
   void client_receive_delta_snapshot(const PoolByteArray& encoded);

   //  Server will call this when dispatching events to the client
   void client_receive_net_event(const PoolByteArray& encoded);

   // Server will call this to request client to send credentials
   void client_request_credentials();


   // This helper function is used mostly to change the player node ready state
   void set_ready_state(uint32_t pid, bool ready);

   // Client will call this to notify the server that it is ready to receive snapshot data
   void server_client_is_ready();
   // Client will call this to notify the server that it is not ready to receive snapshot data anymore
   void server_client_not_ready();

   // This is called by the client in order to acknowledge that snapshot data has been received and processed.
   void server_acknowledge_snapshot(uint32_t sig);

   
   // This function is meant to be called by clients and only run on the server. It should broadcast
   // the specified property to all connected clients skipping the one that called it. This function will only
   // deal with properties that were not sent in a packet (supported by EncDecBuffer)
   void server_broadcast_custom_prop(const String& pname, const Variant& value);

   // Client will call this when sending credentials to the server
   void server_receive_credentials(const Dictionary& cred);

   // Custom properties that are supported by the EncDecbuffer will use this function to perform the synchronization.
   // Basically when this is called there is incoming data. On the server the properties must first be decoded and
   // applied ot the node corresponding to the remote player. Then the data must be encoded again but excluding any
   // property that is set to "ServerOnly"
   void all_receive_custom_prop_batch(const PoolByteArray& encoded);


   // When a snapshot is being finished, this function will be called in order to synchronize custom player properties
   void on_check_custom_properties();
   // This will be called when snapshot is finished and must perform the encoding to send to connected players.
   void on_snapshot_finished(Ref<kehSnapshot>& snap);
   // Called as part of the "snapshot finished" process. This must send accumulated events to the connected clients
   void on_dispatch_events(const PoolVector<kehNetEvent>& event);


   /// "Signaler" functions
   void ping_signaler(uint32_t pid, float ping);
   void custom_prop_signaler(uint32_t pid, const String& pname, const Variant& val);
   void custom_prop_broadcast_requester(const String& pname, const Variant& val);

protected:
   void _notification(int what);
   static void _bind_methods();

public:
   static kehNetwork* get_singleton() { return s_singleton; }

   // Necessary to initialize the networking system.
   void initialize();


   // Used to reset the internal system. Useful when restarting the game without quitting it.
   void reset_system();

   // Returns true if this machine has authority, which is true if single player or if this is
   // server/host
   bool has_authority() const;

   // Returns true if this is single player
   bool is_single_player() const;

   // Obtain the network ID of the local player
   uint32_t get_local_id() const;

   // Return true if the given network ID corresponds to the local player
   bool is_id_local(uint32_t pid) const;

   // Returns true if a connection attempt is currently going on.
   bool is_connecting() const;

   // Enable/disable any kind of extra processing (including input) for the local player node. Be very
   // careful with this call because it will not make any kind of check if the local player actually
   // corresponds to the server or not. This should be called only when it's absolutely sure the instance
   // of the game is a dedicated server meaning that the local player node will never be used as actual
   // player.
   void set_dedicated_server_mode(bool enable_dedicated);

   // Retrieve snapshot data
   Ref<kehSnapshotData> get_snapshot_data() const;
   // Retrieve player data
   Ref<kehPlayerData> get_player_data() const;


   /// Input related
   void register_action(const String& action, bool is_analog);
   void register_custom_action(const String& action, bool is_analog);
   void register_custom_input_vec2(const String& vec_name);
   void register_custom_input_vec3(const String& vec_name);

   void set_action_enabled(const String& action, bool enabled);
   void set_local_input_enabled(bool enabled);
   void set_use_mouse_relative(bool use);
   void set_use_mouse_speed(bool use);

   void reset_input();

   // Obtain input data for the specified player. If local, the data will be polled first.
   // If for a client, data wil be retrieved from input cache if running on server. Otherwise
   // an invalid reference will be returned.
   Ref<kehInputData> get_input(uint32_t pid) const;


   /// Server
   void create_server(uint32_t port, const String& sname, uint32_t max_players);
   void close_server(const String& message = "Server is closing");

   void kick_player(uint32_t id, const String& reason);


   /// Client
   void join_server(const String& IP, uint32_t port);
   void disconnect_from_server();


   void notify_ready();


   /// Snapshot system
   // Initialize a snapshot for current frame
   void init_snapshot();

   // Obtain the signature of the snapshot that is currently being built
   uint32_t get_snap_building_signature() const;

   // Create an instance of the given Snap Entity script. This will take care of setting unique
   // ID and class hash
   Ref<kehSnapEntityBase> create_snap_entity(const Ref<Script>& snap_script, uint32_t uid, uint32_t class_hash) const;

   // Add the given snapshot entity into the snapshot currently being built.
   void snapshot_entity(const Ref<kehSnapEntityBase>& entity);

   // This is used to correct the entity data within a snapshot, which will be located based on
   // the provided input data.
   void correct_in_snapshot(const Ref<kehSnapEntityBase>& entity, const Ref<kehInputData>& input);


   /// Replicated event system
   // Register an event type
   void register_event_type(uint16_t code, const Array& param_types);

   // Attach an event handler into the given event type
   void attach_event_handler(uint16_t code, const Object* obj, const String& fname);

   // Accumulate an event to be sent to the clients. This will do nothing if not on authority machine
   void send_event(uint16_t code, const Array& params);

   /// Chat system
   // Send a chat message. If the second argument is set to 0, then the message will be broadcast, otherwise
   // it will be sent to the specified peer ID
   void send_chat_message(const String& msg, uint32_t send_to = 0);


   /// Credential system
   void set_credential_checker(const Ref<FuncRef>& fref);
   Ref<FuncRef> get_credential_checker() const;

   // Client code used to dispatch credentials to the server. Credentials must be send within a Dictionary
   void dispatch_credentials(const Dictionary& cred);



   kehNetwork(fptr on_entered_tree = NULL);
   ~kehNetwork();
};


#endif
