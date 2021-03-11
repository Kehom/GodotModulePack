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


#include "network.h"
#include "../kehgeneral/encdecbuffer.h"

#include "core/script_language.h"
#include "scene/main/viewport.h"
#include "core/func_ref.h"

#include "inputdata.h"
#include "networknode.h"
#include "playerdata.h"
#include "playernode.h"
#include "snapentity.h"
#include "snapshotdata.h"
#include "updtcontrol.h"



//#ifdef MODULE_ENET_ENABLED
   #include "modules/enet/networked_multiplayer_enet.h"
//#endif
//#ifdef MODULE_WEBSOCKET_ENABLED
   #include "modules/websocket/websocket_client.h"
   #include "modules/websocket/websocket_server.h"
   /*
   #ifdef JAVASCRIPT_ENABLED
      #include "modules/websocket/emws_client.h"
      #include "modules/websocket/emws_server.h"
   #else
      #include "modules/websocket/wsl_client.h"
      #include "modules/websocket/wsl_server.h"
   #endif*/
//#endif

kehNetwork* kehNetwork::s_singleton = NULL;


void kehNetwork::initialize()
{
   if (m_initialized)
   {
      return;
   }

   SceneTree* st = SceneTree::get_singleton();
   Viewport* root = st->get_root();

   if (!root)
   {
      ERR_FAIL_COND_MSG(!root, "Could not retrieve root of the tree while initializing the Network singleton node.");
      return;
   }

   // Get the project settings
   m_compression = GLOBAL_GET("keh_modules/network/generatel/compression");
   m_backmode = GLOBAL_GET("keh_modules/network/general/mode");
   m_broadcast_ping_value = GLOBAL_GET("keh_modules/network/general/broadcast_measured_ping");
   m_full_snap_threshold = GLOBAL_GET("keh_modules/network/snapshot/full_threshold");
   m_max_history_size = GLOBAL_GET("keh_modules/network/snapshot/max_history");
   m_max_client_history_size = GLOBAL_GET("keh_modules/network/snapshot/max_client_history");

   if (m_max_history_size < m_full_snap_threshold + 1)
   {
      WARN_PRINT(vformat("The desired max snapshot history (%d) is smaller than the full snapshot threshold, so setting max history to %d", m_max_history_size, m_full_snap_threshold + 1));
      m_max_history_size = m_full_snap_threshold + 1;
   }

   check_backmode();


   // Those are used to perform initialization and cleanup
   // NOTE: If tree is already completed this code will not work as intended. However I couldn't find
   // a way to detect if the tree is being initialized or already complete.
   root->connect("ready", this, "_root_completed");
   root->connect("tree_exiting", this, "_root_shutdown");

   // Events related to the networking API
   st->connect("network_peer_connected",this , "_on_player_connected");
   st->connect("network_peer_disconnected", this, "_on_player_disconnected");
   st->connect("connection_failed", this, "_on_connection_failed");
   st->connect("server_disconnected", this, "_on_disconnected");

   // Setup the remote functions
   rpc_config("_all_register_player", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_all_unregister_player", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_all_chat_message", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_all_receive_custom_prop_batch", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   
   rpc_config("_client_join_accepted", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_client_join_rejected", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_client_kicked", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_client_receive_full_snapshot", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_client_receive_delta_snapshot", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_client_receive_net_event", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_client_request_credentials", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);

   rpc_config("_server_client_is_ready", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_server_client_not_ready", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_server_acknowledge_snapshot", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_server_broadcast_custom_prop", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
   rpc_config("_server_receive_credentials", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);


   m_snapshot_data = Ref<kehSnapshotData>(memnew(kehSnapshotData));
   m_player_data = Ref<kehPlayerData>(memnew(kehPlayerData));
   m_update_control = memnew(kehUpdateControl);

   m_update_control->set_custom_prop_checker(kehUpdateControl::CustomPropCheckT(this, &kehNetwork::on_check_custom_properties));
   m_update_control->set_snapshot_finished(kehUpdateControl::SnapshotFinishedT(this, &kehNetwork::on_snapshot_finished));
   m_update_control->set_event_dispatcher(kehUpdateControl::EventDispatcherT(this, &kehNetwork::on_dispatch_events));

   m_snapshot_data->get_entity_types(m_entity_type);

   m_player_data->set_ping_signaler(kehFunctoid<void(uint32_t, float)>(this, &kehNetwork::ping_signaler));
   m_player_data->set_cprop_signaler(kehFunctoid<void(uint32_t, const String&, const Variant&)>(this, &kehNetwork::custom_prop_signaler));

   add_child(m_player_data->create_local_player());


   m_initialized = true;
}


void kehNetwork::reset_system()
{
   m_update_control->reset();
   m_snapshot_data->reset();
   m_player_data->get_local_player()->reset_data();

   // Reset incrementing IDs. Well, should this system even exist?

   // Clear the event handlers
   for (Map<uint16_t, kehEventInfo>::Element* einfo = m_event_info.front(); einfo; einfo = einfo->next())
   {
      einfo->value().clear_handlers();
   }
}


bool kehNetwork::has_authority() const
{
   SceneTree* st = get_tree();
   return (!st || !st->has_network_peer() || st->is_network_server());
}

bool kehNetwork::is_single_player() const
{
   SceneTree* st = get_tree();
   return (!st || !st->has_network_peer());
}


uint32_t kehNetwork::get_local_id() const
{
   return m_player_data.is_valid() ? m_player_data->get_local_player()->get_id() : 1;
}

bool kehNetwork::is_id_local(uint32_t pid) const
{
   return m_player_data.is_valid() ? m_player_data->get_local_player()->get_id() == pid : pid == 1;
}


bool kehNetwork::is_connecting() const
{
   SceneTree* st = get_tree();
   if (!st)
   {
      return false;
   }

   bool ret = false;
   Ref<NetworkedMultiplayerPeer> net = st->get_network_peer();
   if (net.is_valid())
   {
      ret = net->get_connection_status() == NetworkedMultiplayerPeer::ConnectionStatus::CONNECTION_CONNECTING;
   }
   return ret;
}


void kehNetwork::set_dedicated_server_mode(bool enable_dedicated)
{
   // Although the node is currently not directly using any processing besides Input, explicitly disabling
   // the others should not be harmful. Actually, it may help in case a future update adds some of those
   // processing into the player node
   kehPlayerNode* node = m_player_data->get_local_player();
   node->set_physics_process(!enable_dedicated);
   node->set_physics_process_internal(!enable_dedicated);
   node->set_process(!enable_dedicated);
   node->set_process_input(!enable_dedicated);
   node->set_process_internal(!enable_dedicated);
   node->set_process_unhandled_input(!enable_dedicated);
   node->set_process_unhandled_key_input(!enable_dedicated);
}



Ref<kehSnapshotData> kehNetwork::get_snapshot_data() const
{
   return m_snapshot_data;
}

Ref<kehPlayerData> kehNetwork::get_player_data() const
{
   return m_player_data;
}


void kehNetwork::register_action(const String& action, bool is_analog)
{
   InputMap* im = InputMap::get_singleton();
   ERR_FAIL_COND_MSG(!im, "Input map object is invalid, can't register action within network system.");

   if (im->has_action(action))
   {
      // Only register this action if there is an input map for it.
      m_player_data->register_action(action, is_analog);
   }
   else
   {
      WARN_PRINT(vformat("Trying to register '%s' input action but it's not mapped. Please check Project Settings -> Input Map tab.", action));
   }
}

void kehNetwork::register_custom_action(const String& action, bool is_analog)
{
   m_player_data->register_custom_action(action, is_analog);
}

void kehNetwork::register_custom_input_vec2(const String& vec_name)
{
   m_player_data->register_custom_vec2(vec_name);
}

void kehNetwork::register_custom_input_vec3(const String& vec_name)
{
   m_player_data->register_custom_vec3(vec_name);
}

void kehNetwork::set_action_enabled(const String& action, bool enabled)
{
   m_player_data->set_action_enabled(action, enabled);
}

void kehNetwork::set_local_input_enabled(bool enabled)
{
   m_player_data->get_local_player()->set_input_enabled(enabled);
}

void kehNetwork::set_use_mouse_relative(bool use)
{
   m_player_data->set_use_mouse_relative(use);
}

void kehNetwork::set_use_mouse_speed(bool use)
{
   m_player_data->set_use_mouse_speed(use);
}

void kehNetwork::reset_input()
{
   m_player_data->reset_input();
}


Ref<kehInputData> kehNetwork::get_input(uint32_t pid) const
{
   const bool is_authority = has_authority();

   if (!is_id_local(pid) && !is_authority)
   {
      return NULL;
   }

   kehPlayerNode* pnode = m_player_data->get_pnode(pid);

   return (pnode ? pnode->get_input(m_update_control->get_signature()) : NULL);
}


void kehNetwork::create_server(uint32_t port, const String& sname, uint32_t max_players)
{
   ERR_FAIL_COND_MSG(m_backmode == BM_Invalid, "Cannot create a server with invalid connection mode.");

   SceneTree* st = SceneTree::get_singleton();
   if (st->has_network_peer())
   {
      // Should this give a warning or something?
      return;
   }

   Ref<NetworkedMultiplayerPeer> netpeer;

   switch (m_backmode)
   {
      // NOTE: bellow the #ifdef are necessary in order to avoid errors in case the modules are not present
      case BM_ENet:
      {
#ifdef MODULE_ENET_ENABLED
         Ref<NetworkedMultiplayerENet> net(memnew(NetworkedMultiplayerENet));
         net->set_compression_mode((NetworkedMultiplayerENet::CompressionMode)m_compression);

         if (net->create_server(port, max_players) != OK)
         {
            emit_signal("server_creation_failed");
         }
         else
         {
            netpeer = net;
         }
      
#endif
      } break;

      case BM_WebSocket:
      {
#ifdef MODULE_WEBSOCKET_ENABLED
         // Not directly using the memnew because WebSocketServer is actually an abstract class. Depending on
         // the JAVASCRIPT_ENABLED macro the actual class will be created when using the ::create() static function
         Ref<WebSocketServer> net(WebSocketServer::create());

         if (net->listen(port, Vector<String>(), true) != OK)
         {
            emit_signal("server_creation_failed");
         }
         else
         {
            netpeer = net;
            set_process(true);
         }
#endif
      } break;
   }

   if (netpeer.is_valid())
   {
      // Assign the network peer into the scene tree
      st->set_network_peer(netpeer);

      // Server has been created so emit the signal indicating this fact
      emit_signal("server_created");

      // This machine is hosting the game. Probably this is not necessary, but ensuring local player's
      // data is holding the correct network ID (1) may avoid some bugs.
      m_player_data->get_local_player()->set_id(1);

      // TODO: build server info
   }
}



void kehNetwork::close_server(const String& message)
{
   SceneTree* st = SceneTree::get_singleton();
   if (!st->has_network_peer())
      return;

   Ref<NetworkedMultiplayerPeer> netp = st->get_network_peer();
   if (netp->get_connection_status() != NetworkedMultiplayerPeer::ConnectionStatus::CONNECTION_DISCONNECTED)
   {
      if (!st->is_network_server())
         return;
      
      // Must kick connected players. Normal iteration through the player list can't be done because at each
      // one a disconnect_peer() signal will be caleed, which will remove that player node from the list. So,
      // first retrieve the list of keys (network IDs) then iterate through that list in order to kick everyone.
      PoolVector<uint32_t> pids;
      m_player_data->get_remote_player_list(pids);
      const uint32_t psize = pids.size();
      for (uint32_t i = 0; i < psize; i++)
      {
         kick_player(pids[i], message);
      }
   }

   // This is necessary if running in websocket mode, but there is absolutely no problem in doing this in
   // ENet mode.
   set_process(false);

   // Cleanup the network object through a deferred call just to ensure all remote calls get processed.
   // Directly using the scene tree object to defer the call is not working so relaying to an internal function
   call_deferred("_clear_netpeer");
}


void kehNetwork::kick_player(uint32_t id, const String& reason)
{
   if (!is_inside_tree())
   {
      return;
   }

   if (!has_authority())
   {
      // This can only be run by server
      return;
   }

   switch (m_backmode)
   {
      case BM_ENet:
      {
#ifdef MODULE_ENET_ENABLED
         // On ENet first remote call the function that will give the reason to the kicked player
         rpc_id(id, "_client_kicked", reason);

         // Then remove the player
         Ref<NetworkedMultiplayerENet> peer = get_tree()->get_network_peer();
         if (peer.is_valid())
         {
            peer->disconnect_peer(id);
         }
#endif
      } break;


      case BM_WebSocket:
      {
#ifdef MODULE_WEBSOCKET_ENABLED
         // When in websocket, first notifying the client through RPC will not work as it will arrive after
         // the client disconnects. However, the client will still get an extra signal, the "server_close_request"
         // before, meaning that it will be used to give the "reason".
         Ref<WebSocketServer> net = get_tree()->get_network_peer();
         net->disconnect_peer(id, 1000, reason);
#endif
      } break;
   }

   // Ensure internal remote player container is properly cleaned
   all_unregister_player(id);
}


void kehNetwork::join_server(const String& IP, uint32_t port)
{
   ERR_FAIL_COND_MSG(m_backmode == BM_Invalid, "Trying to join a server with an invalid connection mode.");

   SceneTree* st = SceneTree::get_singleton();
   if (st->has_network_peer())
   {
      // TODO: proper handling of this situation.
      return;
   }

   Ref<NetworkedMultiplayerPeer> netpeer;;

   switch (m_backmode)
   {
      case BM_ENet:
      {
#ifdef MODULE_ENET_ENABLED
         Ref<NetworkedMultiplayerENet> enet(memnew(NetworkedMultiplayerENet));
         enet->set_compression_mode((NetworkedMultiplayerENet::CompressionMode)m_compression);

         if (enet->create_client(IP, port) != OK)
         {
            emit_signal("join_fail");
         }
         else
         {
            netpeer = enet;
         }
#endif
      } break;

      case BM_WebSocket:
      {
#ifdef MODULE_WEBSOCKET_ENABLED
         Ref<WebSocketClient> net(WebSocketClient::create());

         // Must listen to this signal because it will arrive before the actual disconnection.
         net->connect("server_close_request", this, "_client_on_websocket_close_request");

         // Websocket connections require a "w://" at the beginning of the address
         const String url(vformat("ws://%s:%d", IP, port));

         if (net->connect_to_url(url, Vector<String>(), true) != OK)
         {
            emit_signal("join_fail");
         }
         else
         {
            netpeer = net;
         }
#endif
      } break;
   }
   

   if (netpeer.is_valid())
   {
      st->set_network_peer(netpeer);

      // At this point it does not necessarily mean that the connection is successful, only that
      // the attempt is now going on. In other words, there isn't much else to do here besides
      // waiting for the signal indicating either a success or failure.
   }
}


void kehNetwork::disconnect_from_server()
{
   SceneTree* st = get_tree();

   if (!st->has_network_peer())
      return;

   // Perform cleanup.
   handle_disconnection();
}


void kehNetwork::notify_ready()
{
   if (!has_authority())
   {
      m_is_ready = true;
      rpc_id(1, "_server_client_is_ready");
   }
}



void kehNetwork::init_snapshot()
{
   m_update_control->start(m_entity_type);
}


uint32_t kehNetwork::get_snap_building_signature() const
{
   return m_update_control->get_signature();
}


Ref<kehSnapEntityBase> kehNetwork::create_snap_entity(const Ref<Script>& snap_script, uint32_t uid, uint32_t class_hash) const
{
   return m_snapshot_data->instantiate_snap_entity(snap_script, uid, class_hash);
}


void kehNetwork::snapshot_entity(const Ref<kehSnapEntityBase>& entity)
{
   ERR_FAIL_COND_MSG(!m_update_control->is_building(), "Trying to add entity into invalid snapshot. Did you call kehNetwork.init_snapshot()?");

   Ref<Script> script = entity->get_script();
   const uint32_t ehash = m_snapshot_data->get_ehash(script);

   if (ehash > 0)
   {
      m_update_control->add_to_snapshot(ehash, entity);
   }
}

void kehNetwork::correct_in_snapshot(const Ref<kehSnapEntityBase>& entity, const Ref<kehInputData>& input)
{
   Ref<kehSnapshot> snap = m_snapshot_data->get_snapshot_by_input(input->get_signature());
   if (snap.is_valid())
   {
      const uint32_t ehash = m_snapshot_data->get_ehash(entity->get_script());
      snap->add_entity(ehash, entity);
   }
}


void kehNetwork::register_event_type(uint16_t code, const Array& param_types)
{
   if (!kehEventInfo::check_types(param_types))
   {
      WARN_PRINT(vformat("While registering event code '%d', an unsupported argument type was given.", code));
      return;
   }

   m_event_info[code] = kehEventInfo(code, param_types);
}

void kehNetwork::attach_event_handler(uint16_t code, const Object* obj, const String& fname)
{
   // TODO: remove this fail condition from non editor release builds
   ERR_FAIL_COND_MSG(!m_event_info.has(code), vformat("Trying to attach an event handler to a non registered event type (%d)", code));
   
   // These two must remain even on non editor builds
   ERR_FAIL_COND_MSG(!obj, "While attaching event handler, the given object must be valid.");
   ERR_FAIL_COND_MSG(!obj->has_method(fname), vformat("While attaching event handler, the function (%s) must exist on the given object.", fname));

   m_event_info[code].attach_handler(obj, fname);
}

void kehNetwork::send_event(uint16_t code, const Array& params)
{
   // TODO: remove this fail condition from non editor release builds
   ERR_FAIL_COND_MSG(!m_event_info.has(code), vformat("Tryint to send an event (%d) that is not registered.", code));

   // Only authority can replicate events
   if (!has_authority())
      return;
   
   m_update_control->push_event(code, params);
}


void kehNetwork::send_chat_message(const String& msg, uint32_t send_to)
{
   // The "sender" here corresponds to the local machine, so obtain the unique ID
   const uint32_t sender = SceneTree::get_singleton()->get_network_unique_id();
   if (send_to != 0)
   {
      rpc_id(send_to, "_all_chat_message", sender, msg, false);
   }
   else
   {
      // The message is meant to be broadcast.
      if (sender != 1)
      {
         // Not sending from server, so request the server to do the broadcasting.
         rpc_id(1, "_all_chat_message", sender, msg, true);
      }
      else
      {
         // Sending from the server, so perform the broadcasting.
         all_chat_message(1, msg, true);
      }
   }
}


void kehNetwork::set_credential_checker(const Ref<FuncRef>& fref)
{
   m_credential_checker = fref;
}

Ref<FuncRef> kehNetwork::get_credential_checker() const
{
   return m_credential_checker;
}

void kehNetwork::dispatch_credentials(const Dictionary& cred)
{
   // This function must be run only on clients and is irrelevant on servers.
   if (has_authority())
      return;
   
   // Send the dictionary to the server
   rpc_id(1, "_server_receive_credentials", cred);
}



void kehNetwork::check_backmode()
{
#ifdef MODULE_ENET_ENABLED
   const bool has_enet = true;
#else
   const bool has_enet = false;
#endif

#ifdef MODULE_WEBSOCKET_ENABLED
   const bool has_websocket = true;
#else
   const bool has_websocket = false;
#endif

   switch (m_backmode)
   {
      case BM_ENet:
      {
         if (!has_enet)
         {
            print_error("Requesting to use ENet but the module is not present on current Build.");
            m_backmode = BM_Invalid;
         }
      } break;

      case BM_WebSocket:
      {
         if (!has_websocket)
         {
            print_error("Requesting to use WebSocket but the module is not present on current build.");
            m_backmode = BM_Invalid;
         }
      } break;
   }
}


void kehNetwork::on_root_completed()
{
   if (!m_on_enter_tree)
   {
      return;
   }

   SceneTree::get_singleton()->get_root()->add_child((Node*)this);
   m_on_enter_tree();


}

void kehNetwork::on_root_shutdown()
{
   if (!m_initialized)
   {
      return;
   }

   // If not in multiplayer or if this is not server, nothing (hopefully) bad will happen.
   close_server();

   if (m_update_control)
      memdelete(m_update_control);
   
   // m_snapshot_data is set as Ref<>, so just clearing the internal pointer should be enough from here
   m_snapshot_data = Ref<kehSnapshotData>();
   // The same thing is valid for the m_player_data
   m_player_data = Ref<kehPlayerData>();

   m_update_control = NULL;
}


void kehNetwork::on_player_connected(uint32_t id)
{
   if (get_tree()->is_network_server())
   {
      if (m_credential_checker.is_valid() && m_credential_checker->is_valid())
      {
         // Credential checker has been set so ask client to send credentials.
         rpc_id(id, "_request_credentials");
      }
      else
      {
         // Credential checker is not set so assume this feature is not desired.
         // Automatically accept the new player.
         rpc_id(id, "_client_join_accepted");
      }
   }
}

void kehNetwork::on_player_disconnected(uint32_t id)
{
   if (get_tree()->is_network_server())
   {
      // Unregister the player from server's list
      all_unregister_player(id);

      // And from everyone else
      if (m_backmode == BM_WebSocket)
      {
         // In websocket mode must defer the call otherwise there is chance of servers not correctly
         // synchronize the RPC. Somtimes the server tries to send the remote call to the player that
         // already left too, which somehow causes every client to ignore the RPC call.
         call_deferred("rpc", "_all_unregister_player", id);
      }
      else
      {
         rpc("_all_unregister_player", id);
      }
   }
}

void kehNetwork::on_connection_failed()
{
   if (m_backmode == BM_WebSocket && is_processing())
   {
      // If here then connection was previously established but it was lost. Problem is, for some reason when
      // server calls disconnect_peer() the signal "server_close_request" is given as documented but instead
      // of the "connection_close" being given later, the connection failed comes in. Returning from here and
      // continuing to poll data (as described in the documentation) will not work so emit the "disconnected"
      // signal from here instead.
      emit_signal("disconnected");
   }
   else
   {
      // Hopefully the previous check is enough to cover every situation in which code will reach this function
      // but is not exactly a "connection failed" event.
      emit_signal("join_fail");
   }

   handle_disconnection();
}

void kehNetwork::on_disconnected()
{
   // INform outside code about this
   emit_signal("disconnected");

   // Perform some cleanup
   handle_disconnection();
}


void kehNetwork::clear_netpeer()
{
   // As explained, this function is just to relay a deferred call to the scene tree, because directly
   // doing so is not working.
   get_tree()->set_network_peer(NULL);
}

void kehNetwork::handle_disconnection()
{
   // Well, since disconnected, it's sure there are no remote players, so clear the list.
   m_player_data->clear_remote();

   // Ensure local player is holding ID corresponding to authority (1) as now this machine is "alone"
   // This call will also take care of correctly updating the Node name within the tree.
   m_player_data->get_local_player()->set_id(1);

   // It doesn't hurt to call this even on ENet mode
   set_process(false);

   // Network peer must be reset within the scene tree.
   // If not deferring the call then there will an error (object being destroyed while emitting a signal).
   // However, directly deferring the call within the scene tree is not working, so doing that through
   // an internal function (clear_netpeer) which is exposed as _clear_netpeer so it's not exposed to
   // scripting languages.
   call_deferred("_clear_netpeer");
}

void kehNetwork::all_register_player(uint32_t pid)
{
   // This function should be called only when actually running in multiplayer, so not checking if the tree
   // has a multiplayer API assigned to it
   const bool is_server = SceneTree::get_singleton()->is_network_server();

   // Create the player node - this will *not* register the player within the container yet.
   kehPlayerNode* player = m_player_data->create_player(pid);

   // Add to the tree
   add_child(player);

   if (is_server)
   {
      Ref<kehEncDecBuffer> edec = m_update_control->get_enc_dec();

      // Start the ping/pong loop
      player->start_ping();

      // Server server's custom properties that are meant to be broadcast to the new player
      if (m_player_data->get_local_player()->encode_custom_props(edec, m_player_data->get_custom_props(), true, true))
      {
         rpc_id(pid, "_all_receive_custom_prop_batch", edec->get_buffer());
      }

      for (Map<uint32_t, kehPlayerNode*>::Element* p = m_player_data->get_remote_iterator(); p; p = p->next())
      {
         // Send currently iterated player to the one who just joined
         rpc_id(pid, "_all_register_player", p->value()->get_id());

         // Send new player to currently iterated one
         rpc_id(p->value()->get_id(), "_all_register_player", pid);

         // If there are custom properties meant to be broadcast, take the data of current iterated player and
         // send to the new one.
         if (p->value()->encode_custom_props(edec, m_player_data->get_custom_props(), true, true))
         {
            rpc_id(pid, "_all_receive_custom_prop_batch", edec->get_buffer());
         }

         // Custom property data of the new player will be sent to the server only when it begins the
         // snapshot cycle, so there is no point in trying to send that to the other players at this moment.
      }
   }

   // Perform the actual registration of the node within the internal container
   m_player_data->add_remote(player);

   // Outside code might need to do something whe a new player is registered.
   emit_signal("player_added", pid);
}


void kehNetwork::all_unregister_player(uint32_t pid)
{
   kehPlayerNode* pnode = m_player_data->get_remote_player(pid);
   if (pnode)
   {
      // Outside code might need to do something when a player leaves the game.
      emit_signal("player_removed", pid);

      // Make the node for removal
      if (!pnode->is_queued_for_deletion())
      {
         pnode->queue_delete();
      }

      // Remove the reference to the node from the internal container.
      m_player_data->remove_remote_player(pid);
   }
}


void kehNetwork::all_chat_message(uint32_t sender, const String& msg, bool broadcast)
{
   if (has_authority())
   {
      if (broadcast)
      {
         // This message is meant to be sent to everyone. Iterate through every connected player.
         // However, skip the sender, which should handle the message locally
         for (Map<uint32_t, kehPlayerNode*>::Element* p = m_player_data->get_remote_iterator(); p; p = p->next())
         {
            if (sender != p->value()->get_id())
            {
               rpc_id(p->value()->get_id(), "_all_chat_message", sender, msg, broadcast);
            }
         }
      }

      // Check if sender is the server's player, in which case return from here otherwise the
      // message may be handled twice. In a way, "skip the sender"
      if (sender == 1)
         return;
   }

   // "Everyone code". Just emit a signal indicating that a new chat message has arrived.
   emit_signal("chat_message_received", msg, sender);
}



void kehNetwork::client_join_accepted()
{
   // Enable processsing - which will poll network data if in WebSocket mode
   if (m_backmode == BM_WebSocket)
      set_process(true);

   // Must emit signal indicating this event.
   emit_signal("join_accepted");

   // This machine has joined a server, so must update its local network ID
   const uint32_t nid = get_tree()->get_network_unique_id();
   // This should also update the node name
   m_player_data->get_local_player()->set_id(nid);

   // Create a node representing the host
   all_register_player(1);

   // Request the server to register this player within everyon's list. With this remote call the server
   // will also send other players this way.
   rpc_id(1, "_all_register_player", nid);
}

void kehNetwork::client_join_rejected(const String& reason)
{
   // Just notify about this. Further cleanup will be done from the on_disconnected
   emit_signal("join_rejected", reason);
}


void kehNetwork::client_kicked(const String& reason)
{
   // Just tell outside code about this
   emit_signal("kicked", reason);
}


void kehNetwork::client_on_websocket_close_request(int code, const String& reason)
{
   emit_signal("kicked", reason);
}


void kehNetwork::handle_snapshot(const Ref<kehSnapshot>& snapshot)
{
   // Acknowledge to the server the received snapshot
   rpc_unreliable_id(1, "_server_acknowledge_snapshot", snapshot->get_signature());

   // Check this snapshot comparing to the predicted one. This function also updates
   // the internal m_server_state property, which must match the most recent received data.
   m_snapshot_data->client_check_snapshot(snapshot);

   // The snapshot may contain input signature, which serves as an acknowledgement from
   // the server about that input data. So perform clearing of internal input cache so
   // this data (and older) doesn't get sent to the server again
   if (snapshot->get_input_sig() > 0)
   {
      m_player_data->get_local_player()->client_acknowledge_input(snapshot->get_input_sig());
   }
}

void kehNetwork::client_receive_full_snapshot(const PoolByteArray& encoded)
{
   ERR_FAIL_COND_MSG(has_authority(), "Receiving full snapshot data cannot be done on server.");
   if (!m_is_ready)
   {
      // Ignore this snapshot if not ready
      return;
   }

   Ref<kehEncDecBuffer> encdec = m_update_control->get_enc_dec();
   encdec->set_buffer(encoded);
   Ref<kehSnapshot> decoded = m_snapshot_data->decode_full(encdec);
   if (decoded.is_valid())
   {
      handle_snapshot(decoded);
   }
}

void kehNetwork::client_receive_delta_snapshot(const PoolByteArray& encoded)
{
   ERR_FAIL_COND_MSG(has_authority(), "Receiving delta snapshot data cannot be done on server.");
   if (!m_is_ready)
   {
      // Ignore this snapshot if not ready
      return;
   }

   Ref<kehEncDecBuffer> encdec = m_update_control->get_enc_dec();
   encdec->set_buffer(encoded);

   Ref<kehSnapshot> decoded = m_snapshot_data->decode_delta(encdec);
   if (decoded.is_valid())
   {
      handle_snapshot(decoded);
   }
}



void kehNetwork::client_receive_net_event(const PoolByteArray& encoded)
{
   // Authority does not need to do anything here.
   if (has_authority())
      return;
   
   Ref<kehEncDecBuffer> edec = m_update_control->get_enc_dec();
   edec->set_buffer(encoded);

   // Decode number of events
   const uint16_t evtcount = edec->read_ushort();

   // Process each encoded event
   for (uint16_t i = 0; i < evtcount; i++)
   {
      // Read the event code
      const uint16_t code = edec->read_ushort();

      // Take the event info so the parameters can be decoded and the handlers called once finished
      const Map<uint16_t, kehEventInfo>::Element* einfo = m_event_info.find(code);

      if (!einfo)
      {
         // This should not occur
         print_error(vformat("While decoding network event, found an event code (%d) that is not registered.", code));
         return;
      }

      einfo->value().decode(edec);
   }
}


void kehNetwork::client_request_credentials()
{
   // The server is requesting credentials. Since this changes from project to project, emit a signal so
   // game specific code can deal with this
   emit_signal("credentials_requested");
}



void kehNetwork::set_ready_state(uint32_t pid, bool ready)
{
   ERR_FAIL_COND_MSG(!has_authority(), "Trying to set ready state on a player node when there is no authority to do so.");

   kehPlayerNode* pn = m_player_data->get_remote_player(pid);
   if (pn)
   {
      pn->set_is_ready(ready);
   }
}

void kehNetwork::server_client_is_ready()
{
   if (!has_authority())
   {
      return;
   }
   set_ready_state(SceneTree::get_singleton()->get_rpc_sender_id(), true);
}

void kehNetwork::server_client_not_ready()
{
   if (!has_authority())
   {
      return;
   }
   set_ready_state(SceneTree::get_singleton()->get_rpc_sender_id(), false);
}

void kehNetwork::server_acknowledge_snapshot(uint32_t sig)
{
   if (!has_authority())
   {
      return;
   }

   uint32_t pid = SceneTree::get_singleton()->get_rpc_sender_id();
   kehPlayerNode* player = m_player_data->get_remote_player(pid);
   if (player)
   {
      player->server_acknowledge_snapshot(sig);
   }
}


void kehNetwork::server_broadcast_custom_prop(const String& pname, const Variant& value)
{
   if (!has_authority())
      return;
   
   // Obtain the ID of the player requesting the broadcast
   const uint32_t caller = SceneTree::get_singleton()->get_rpc_sender_id();

   // Obtain the player node
   kehPlayerNode* pnode = m_player_data->get_remote_player(caller);

   if (pnode)
   {
      // First set the property locally - on the server
      pnode->remote_set_custom_property(pname, value);

      // Then broadcast to every other player
      for (Map<uint32_t, kehPlayerNode*>::Element* p = m_player_data->get_remote_iterator(); p; p = p->next())
      {
         if (pnode != p->value())
         {
            pnode->rpc_id(p->key(), "_remote_set_custom_property", pname, value);
         }
      }
   }
}


void kehNetwork::server_receive_credentials(const Dictionary& cred)
{
   if (!has_authority())
      return;
   
   const uint32_t pid = get_tree()->get_rpc_sender_id();

   // If the credential checker Funcref is invalid then automatically accept the new player
   // Otherwise only if the referenced function returns "" (empty string)
   Array args;
   args.push_back(pid);
   args.push_back(cred);
   const String reason = ((m_credential_checker.is_valid() && m_credential_checker->is_valid()) ? m_credential_checker->call_funcv(args) : "");
   if (reason.length() == 0)
   {
      rpc_id(pid, "on_join_accepted");
   }
   else
   {
      rpc_id(pid, "on_join_rejected");
      // Rejected player should not remain connected
      kick_player(pid, reason);
   }
}



void kehNetwork::all_receive_custom_prop_batch(const PoolByteArray& encoded)
{
   Ref<kehEncDecBuffer> edec = m_update_control->get_enc_dec();
   edec->set_buffer(encoded);

   const uint32_t belong_to = edec->read_uint();
   kehPlayerNode* pnode = m_player_data->get_pnode(belong_to);
   if (!pnode)
      return;
   
   const bool authority = has_authority();
   pnode->decode_custom_props(edec, m_player_data->get_custom_props(), authority);

   if (authority)
   {
      if (pnode->encode_custom_props(edec, m_player_data->get_custom_props(), authority, false))
      {
         // If here there is encoded data that must be broadcast to clients
         const uint32_t caller = SceneTree::get_singleton()->get_rpc_sender_id();

         // Send the encoded data to remote players, skipping the caller
         for (Map<uint32_t, kehPlayerNode*>::Element* p = m_player_data->get_remote_iterator(); p; p = p->next())
         {
            if (caller != p->key())
            {
               rpc_id(p->key(), "_all_receive_custom_prop_batch", edec->get_buffer());
            }
         }
      }
   }
}



void kehNetwork::on_check_custom_properties()
{
   kehPlayerNode* pn = m_player_data->get_local_player();
   if (!pn->has_dirty_custom_prop())
      return;
   
   const bool authority = has_authority();
   Ref<kehEncDecBuffer> edec = m_update_control->get_enc_dec();

   if (pn->encode_custom_props(edec, m_player_data->get_custom_props(), authority, false))
   {
      // There is at least one encoded custom property. Send the data
      if (authority)
      {
         // If here the data is meant to be broadcast to all clients
         for (Map<uint32_t, kehPlayerNode*>::Element* pit = m_player_data->get_remote_iterator(); pit; pit = pit->next())
         {
            rpc_id(pit->key(), "_all_receive_custom_prop_batch", edec->get_buffer());
         }
      }
      else
      {
         // This is a client and the data is encoded. Send to the server
         rpc_id(1, "_all_receive_custom_prop_batch", edec->get_buffer());
      }
   }
}

void kehNetwork::on_snapshot_finished(Ref<kehSnapshot>& snap)
{
   // This function is automatically called whenever the snapshot is actually finished.
   // This function is meant to iterate through connected players and send them snapshot
   // when necessary. The server must "decide" when it's necessary to send full snapshot
   // data or delta snapshots. The "rules" are as follow:
   // 1 - When simulating, if the player in question does not contain input data then it
   //     will trigger a "full snapshot flag"
   // 2 - Amount of non acknowledged snapshots reaches a certain threshold will trigger a
   //     "full snapshot flag"
   // 3 - If the "full snapshot flag" is not triggered, then send delta snapshot.

   kehPlayerNode* lplayer = m_player_data->get_local_player();

   // Attach the input signature into the finished snapshot. It's irrelevant on servers but
   // doing so is not a problem anyways
   snap->set_input_sig(lplayer->get_last_input_signature());
   // And add to the history container.
   m_snapshot_data->add_to_history(snap);

   if (has_authority())
   {
      m_snapshot_data->check_history_size(m_max_history_size, true);
   }
   else
   {
      m_snapshot_data->check_history_size(m_max_client_history_size, false);

      // Dispatch input data to the server
      m_player_data->get_local_player()->dispatch_input_data();

      // Clients don't have anything else to do here, so bail
      return;
   }

   PoolVector<kehPlayerNode*> remote_players;
   m_player_data->fill_remote_player_node(remote_players);
   const uint32_t psize = remote_players.size();
   for (uint32_t i = 0; i < psize; i++)
   {
      kehPlayerNode* player = remote_players[i];
      if (!player->is_ready())
      {
         // The iterated player is not ready, so just move into the next one
         continue;
      }

      // Assume delta snapshot will be encoded
      bool send_full = false;

      // Obtain the list of non acknowledged snapshots for this client, including the corresponding input signatures
      uint32_t non_ack_count = player->get_non_acked_snap_count();

      // First check - if number of non acknowledged snapshots is too big, send full data.
      if (non_ack_count > m_full_snap_threshold)
      {
         send_full = true;
      }

      // This will be used as reference snapshot to build deltas
      Ref<kehSnapshot> refsnap;

      // Second check - the necessary reference snapshot does not exist.
      if (!send_full)
      {
         const uint32_t snapsig = player->get_last_acked_snap_sig();
         refsnap = m_snapshot_data->get_snapshot(snapsig);
         if (!refsnap.is_valid())
            send_full = true;
      }

      // NOTE: a third check was planned: if client contains snapshots without input, send full data. However there is a case where client
      // may not have input data but not caused by data loss, which is basically when client opens a menu or something and stops polling
      // input device. Should some check like this still happen? It's still possible the non acknowledge count check may be enough to consider
      // data loss and send full snapshot data.

      Ref<kehEncDecBuffer> encdec = m_update_control->get_enc_dec();
      // ensure the byte buffer is empty
      encdec->set_buffer(PoolByteArray());

      // During the simulation a player input was used. Retrieve the signature of that data, which must be attached into the encoded data.
      const uint32_t isig = player->get_used_input_in_snap(snap->get_signature());

      if (send_full)
      {
         m_snapshot_data->encode_full(snap, encdec, isig);
         rpc_unreliable_id(player->get_id(), "_client_receive_full_snapshot", encdec->get_buffer());
      }
      else
      {
         m_snapshot_data->encode_delta(snap, refsnap, encdec, isig);
         rpc_unreliable_id(player->get_id(), "_client_receive_delta_snapshot", encdec->get_buffer());
      }
   }
}

void kehNetwork::on_dispatch_events(const PoolVector<kehNetEvent>& event)
{
   // Note something here: even if there are no remote players, some part of the function still need to run.
   // The thing is, it will call the event handlers for each accumulated event, which should run on the local
   // machine. However, encoding will only occur if not in single player and if there is at least one connected
   // player

   // Only authority can send events. And if there are no events, no need to continue here
   if (!has_authority() || event.size() == 0)
      return;
   
   Ref<kehEncDecBuffer> edec = m_update_control->get_enc_dec();

   // Only encode something if there are remote players
   if (m_player_data->get_player_count() > 1)
   {
      // Initialize the EncDecBuffer
      edec->set_buffer(PoolByteArray());

      // Then write number of events. 16 bits should be more than enough
      edec->write_ushort(event.size());
   }

   // Iterate through the events
   for (uint32_t i = 0; i < event.size(); i++)
   {
      const Map<uint16_t, kehEventInfo>::Element* einfo = m_event_info.find(event[i].type);
      if (!einfo)
      {
         print_error(vformat("While preparing to encode events, found an event code (%d) that is not registered. Aborting.", event[i].type));
         return;
      }

      // Again, only encode something if there is at least one remote player
      if (m_player_data->get_player_count() > 1)
      {
         // Write event type code
         edec->write_ushort(event[i].type);

         // Now the parameters
         einfo->value().encode(edec, event[i].params);
      }

      // Call attached event handlers for this event. This allows the server to act on this event
      einfo->value().call_handlers(event[i].params);
   }

   // All events processed. If any connected player, dispatch the events.
   if (m_player_data->get_player_count() > 1)
      rpc("_client_receive_net_event", edec->get_buffer());
}



void kehNetwork::ping_signaler(uint32_t pid, float ping)
{
   emit_signal("ping_updated", pid, ping);
}

void kehNetwork::custom_prop_signaler(uint32_t pid, const String& pname, const Variant& val)
{
   emit_signal("custom_property_changed", pid, pname, val);
}

void kehNetwork::custom_prop_broadcast_requester(const String& pname, const Variant& val)
{
   if (has_authority())
      return;
   
   rpc_id(1, "_server_broadcast_custom_prop", pname, val);
}



void kehNetwork::_notification(int what)
{
   switch (what)
   {
      case NOTIFICATION_READY:
      {
         // By default disable processing.
         set_process(false);
      } break;

      case NOTIFICATION_PROCESS:
      {
         // Polling is necessary for WebSockets to work and emit signals. That said, processing will be enabled
         // only when necessary - that is, creating/joining WebSocket server
         get_tree()->get_network_peer()->poll();
      } break;
   }
}


void kehNetwork::_bind_methods()
{
   // Bind some functions that will not be exposed to scripting. Those are necessary so RPC
   // and event listeners can work.
   ClassDB::bind_method(D_METHOD("_root_completed"), &kehNetwork::on_root_completed);
   ClassDB::bind_method(D_METHOD("_root_shutdown"), &kehNetwork::on_root_shutdown);

   ClassDB::bind_method(D_METHOD("_on_player_connected", "id"), &kehNetwork::on_player_connected);
   ClassDB::bind_method(D_METHOD("_on_player_disconnected", "id"), &kehNetwork::on_player_disconnected);
   ClassDB::bind_method(D_METHOD("_on_connection_failed"), &kehNetwork::on_connection_failed);
   ClassDB::bind_method(D_METHOD("_on_disconnected"), &kehNetwork::on_disconnected);

   ClassDB::bind_method(D_METHOD("_clear_netpeer"), &kehNetwork::clear_netpeer);


   ClassDB::bind_method(D_METHOD("_all_register_player", "pid"), &kehNetwork::all_register_player);
   ClassDB::bind_method(D_METHOD("_all_unregister_player", "pid"), &kehNetwork::all_unregister_player);
   ClassDB::bind_method(D_METHOD("_all_chat_message", "sender", "msg", "broadcast"), &kehNetwork::all_chat_message);
   ClassDB::bind_method(D_METHOD("_all_receive_custom_prop_batch", "encoded"), &kehNetwork::all_receive_custom_prop_batch);
   
   ClassDB::bind_method(D_METHOD("_client_join_accepted"), &kehNetwork::client_join_accepted);
   ClassDB::bind_method(D_METHOD("_client_join_rejected", "reason"), &kehNetwork::client_join_rejected);
   ClassDB::bind_method(D_METHOD("_client_kicked", "reason"), &kehNetwork::client_kicked);
   ClassDB::bind_method(D_METHOD("_client_on_websocket_close_request", "code", "reason"), &kehNetwork::client_on_websocket_close_request);
   ClassDB::bind_method(D_METHOD("_client_receive_full_snapshot", "encoded"), &kehNetwork::client_receive_full_snapshot);
   ClassDB::bind_method(D_METHOD("_client_receive_delta_snapshot", "encoded"), &kehNetwork::client_receive_delta_snapshot);
   ClassDB::bind_method(D_METHOD("_client_receive_net_event", "encoded"), &kehNetwork::client_receive_net_event);
   ClassDB::bind_method(D_METHOD("_client_request_credentials"), &kehNetwork::client_request_credentials);

   ClassDB::bind_method(D_METHOD("_server_client_is_ready"), &kehNetwork::server_client_is_ready);
   ClassDB::bind_method(D_METHOD("_server_client_not_ready"), &kehNetwork::server_client_not_ready);
   ClassDB::bind_method(D_METHOD("_server_acknowledge_snapshot", "sig"), &kehNetwork::server_acknowledge_snapshot);
   ClassDB::bind_method(D_METHOD("_server_broadcast_custom_prop", "pname", "value"), &kehNetwork::server_broadcast_custom_prop);
   ClassDB::bind_method(D_METHOD("_server_receive_credentials", "cred"), &kehNetwork::server_receive_credentials);

   // Bind functions that will be exposed to scripting
   ClassDB::bind_method(D_METHOD("initialize"), &kehNetwork::initialize);

   ClassDB::bind_method(D_METHOD("reset_system"), &kehNetwork::reset_system);
   ClassDB::bind_method(D_METHOD("has_authority"), &kehNetwork::has_authority);
   ClassDB::bind_method(D_METHOD("is_single_player"), &kehNetwork::is_single_player);
   ClassDB::bind_method(D_METHOD("get_local_id"), &kehNetwork::get_local_id);
   ClassDB::bind_method(D_METHOD("is_id_local", "pid"), &kehNetwork::is_id_local);
   ClassDB::bind_method(D_METHOD("is_connecting"), &kehNetwork::is_connecting);
   ClassDB::bind_method(D_METHOD("set_dedicated_server_mode", "enable_dedicated"), &kehNetwork::set_dedicated_server_mode);

   ClassDB::bind_method(D_METHOD("get_snapshot_data"), &kehNetwork::get_snapshot_data);
   ClassDB::bind_method(D_METHOD("get_player_data"), &kehNetwork::get_player_data);

   ClassDB::bind_method(D_METHOD("register_action", "action", "is_analog"), &kehNetwork::register_action);
   ClassDB::bind_method(D_METHOD("register_custom_action", "action", "is_analog"), &kehNetwork::register_custom_action);
   ClassDB::bind_method(D_METHOD("register_custom_input_vec2", "vec_name"), &kehNetwork::register_custom_input_vec2);
   ClassDB::bind_method(D_METHOD("register_custom_input_vec3", "vec_name"), &kehNetwork::register_custom_input_vec3);

   ClassDB::bind_method(D_METHOD("set_action_enabled", "action", "enabled"), &kehNetwork::set_action_enabled);
   ClassDB::bind_method(D_METHOD("set_use_mouse_relative", "use"), &kehNetwork::set_use_mouse_relative);
   ClassDB::bind_method(D_METHOD("set_use_mouse_speed", "use"), &kehNetwork::set_use_mouse_speed);

   ClassDB::bind_method(D_METHOD("reset_input"), &kehNetwork::reset_input);
   ClassDB::bind_method(D_METHOD("get_input", "pid"), &kehNetwork::get_input);

   ClassDB::bind_method(D_METHOD("create_server", "port", "server_name", "max_players"), &kehNetwork::create_server);
   ClassDB::bind_method(D_METHOD("close_server", "message"), &kehNetwork::close_server, DEFVAL(String("Server is closing")));
   ClassDB::bind_method(D_METHOD("kick_player", "id", "reason"), &kehNetwork::kick_player);

   ClassDB::bind_method(D_METHOD("join_server", "ip", "port"), &kehNetwork::join_server);
   ClassDB::bind_method(D_METHOD("disconnect_from_server"), &kehNetwork::disconnect_from_server);
   ClassDB::bind_method(D_METHOD("notify_ready"), &kehNetwork::notify_ready);

   ClassDB::bind_method(D_METHOD("init_snapshot"), &kehNetwork::init_snapshot);
   ClassDB::bind_method(D_METHOD("get_snap_building_signature"), &kehNetwork::get_snap_building_signature);
   ClassDB::bind_method(D_METHOD("create_snap_entity", "snap_entity_class", "uid", "class_hash"), &kehNetwork::create_snap_entity);
   ClassDB::bind_method(D_METHOD("snapshot_entity", "entity"), &kehNetwork::snapshot_entity);
   ClassDB::bind_method(D_METHOD("correct_in_snapshot", "entity", "input"), &kehNetwork::correct_in_snapshot);

   ClassDB::bind_method(D_METHOD("register_event_type", "code", "param_types"), &kehNetwork::register_event_type);
   ClassDB::bind_method(D_METHOD("attach_event_handler", "code", "obj", "funcname"), &kehNetwork::attach_event_handler);
   ClassDB::bind_method(D_METHOD("send_event", "code", "params"), &kehNetwork::send_event);

   ClassDB::bind_method(D_METHOD("send_chat_message", "msg", "send_to"), &kehNetwork::send_chat_message, DEFVAL(0));

   ClassDB::bind_method(D_METHOD("set_credential_checker", "fref"), &kehNetwork::set_credential_checker);
   ClassDB::bind_method(D_METHOD("get_credential_checker"), &kehNetwork::get_credential_checker);
   ClassDB::bind_method(D_METHOD("dispatch_credentials", "cred"), &kehNetwork::dispatch_credentials);


   // Add properties
   ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "snapshot_data", PROPERTY_HINT_RESOURCE_TYPE, "kehSnapshotData", NULL), "", "get_snapshot_data");
   ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "player_data", PROPERTY_HINT_RESOURCE_TYPE, "kehPlayerData", NULL), "", "get_player_data");

   ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "credential_checker", PROPERTY_HINT_RESOURCE_TYPE, "FuncRef", NULL), "set_credential_checker", "get_credential_checker");

   // Register the signals.
   ADD_SIGNAL(MethodInfo("server_created"));
   ADD_SIGNAL(MethodInfo("server_creation_failed"));

   ADD_SIGNAL(MethodInfo("player_added", PropertyInfo(Variant::INT, "id")));
   ADD_SIGNAL(MethodInfo("player_removed", PropertyInfo(Variant::INT, "id")));

   // If the credential system is being used this signal will be emitted only on client machines
   // trying to join the server, indicating that the server is requesting the credential data.
   ADD_SIGNAL(MethodInfo("credentials_requested"));

   ADD_SIGNAL(MethodInfo("join_fail"));
   ADD_SIGNAL(MethodInfo("join_accepted"));
   ADD_SIGNAL(MethodInfo("join_rejected", PropertyInfo(Variant::STRING, "reason")));

   ADD_SIGNAL(MethodInfo("disconnected"));

   ADD_SIGNAL(MethodInfo("ping_updated", PropertyInfo(Variant::INT, "pid"), PropertyInfo(Variant::REAL, "value")));

   ADD_SIGNAL(MethodInfo("kicked", PropertyInfo(Variant::STRING, "reason")));

   ADD_SIGNAL(MethodInfo("chat_message_received", PropertyInfo(Variant::STRING, "msg"), PropertyInfo(Variant::INT, "sender")));

   ADD_SIGNAL(MethodInfo("custom_property_changed", PropertyInfo(Variant::INT, "pid"), PropertyInfo(Variant::STRING, "pname"), PropertyInfo(Variant::NIL, "value")));
}


kehNetwork::kehNetwork(fptr on_entered_tree) :
   m_on_enter_tree(on_entered_tree),
   m_initialized(false)
{
   s_singleton = this;

   set_name("__kehNetwork");

   m_update_control = NULL;

   m_compression = NetworkedMultiplayerENet::CompressionMode::COMPRESS_RANGE_CODER;
   m_broadcast_ping_value = true;
   m_full_snap_threshold = 12;
   m_max_history_size = 120;
   m_max_client_history_size = 60;

   m_is_ready = false;

}

kehNetwork::~kehNetwork()
{

}