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


#include "networknode.h"
/*
#include "playerdata.h"
#include "playernode.h"



void kehNetNode::set_local_player_id(uint32_t id)
{
   m_player_data->get_local_player()->set_id(id);
}

void kehNetNode::clear_remote_players()
{

}



void kehNetNode::all_register_player(uint32_t pid)
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
      // TODO: start the ping/pong loop

      // Code is running on host/server. Synchronize custom properties of the local player with the new one


      // Distribute data accross players.
      ///********** HOW TO ITERATE LIST OF PLAYERS FROM HERE?
   }

   // Perform the actual registration of the node within the internal container
   m_player_data->add_remote(player);

   // Outside code might need to do something whe a new player is registered. Use the signaler functor so the
   // singleton can properly emit the signal.
   if (m_player_added.is_valid())
   {
      m_player_added(pid);
   }
}

void kehNetNode::all_unregister_player(uint32_t pid)
{
   
}


void kehNetNode::client_join_accepted()
{
   // TODO: If in websocket, enable the processing

   // Must emit signal indicating this event. However this signal must be given by the singleton, not this
   // node.
   if (m_join_accepted.is_valid())
   {
      // This should call the correct function that will perform the signal emitting.
      m_join_accepted();
   }

   // Since this client has joined the server, must update the local player node to the newly assigned
   // network ID.
   const uint32_t nid = get_tree()->get_network_unique_id();
   // The "set_id()" should properly rename the node.
   m_player_data->get_local_player()->set_id(nid);

   // Create a node for the host player.
   all_register_player(1);

   // Request the server to register the new player within everyone's list. With this remote call the server
   // will also send already connected player data to this peer
   rpc_id(1, "_all_register_player", nid);
}



void kehNetNode::_notification(int what)
{
   switch(what)
   {
      case NOTIFICATION_ENTER_TREE:
      {
         // The node code into the tree, so there is no need for manually cleaning up this.
         m_need_manual_cleanup = false;

      } break;

      case NOTIFICATION_READY:
      {
         // Only enable processing when in websocket mode.
         set_process(false);


         rpc_config("_all_register_player", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
         rpc_config("_client_join_accepted", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);

      } break;

      case NOTIFICATION_PROCESS:
      {

      } break;
   }

}


void kehNetNode::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("_all_register_player", "pid"), &kehNetNode::all_register_player);
   ClassDB::bind_method(D_METHOD("_client_join_accepted"), &kehNetNode::client_join_accepted);


}


kehNetNode::kehNetNode() :
   m_need_manual_cleanup(true)
{
   m_player_data = memnew(kehPlayerData);
   
   add_child((Node*)m_player_data->get_local_player());
}

kehNetNode::~kehNetNode()
{
   if (m_player_data)
   {
      memdelete(m_player_data);
   }
   m_player_data = NULL;
}
*/