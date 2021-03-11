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


#include "playernode.h"
#include "inputinfo.h"
#include "inputdata.h"
#include "pinginfo.h"

#include "../kehgeneral/encdecbuffer.h"

#include "core/os/input.h"




void kehPlayerNode::set_id(uint32_t pid)
{
   m_net_id = pid;
   set_name("player_" + String::num_int64(pid));
}


void kehPlayerNode::reset_data()
{
   m_input_cache.reset();
}


Ref<kehInputData> kehPlayerNode::get_input(uint32_t snapsig)
{
   const bool is_authority = !get_tree()->has_network_peer() || get_tree()->is_network_server();

   ERR_FAIL_COND_V_MSG(!(m_is_local || is_authority), NULL, "kehPlayerNode::get_input() should be called only by the authority or local machine.");

   Ref<kehInputData> ret;
   if (m_is_local)
   {
      ret = poll_input();

      if (!is_authority)
      {
         // Local machine but on a client. This means the input data must be sent to the server
         // First cache it
         m_input_cache.cache_local_input(ret);

         // Input will be sent to the server when the snapshot is finished. This gives some chance for
         // any custom input to be correctly set before dispatching the data.
      }
   }
   else
   {
      // In theory if here it's authority machine as the ERR_FAIL above should break things if not on
      // local machine and not on authority. Still, checking here for extra confirmation
      if (!is_authority)
         return ret;
      
      // Running on server but requiring data for a client. Must retrieve the data from the input cache.
      ret = m_input_cache.get_client_input();

      if (!ret.is_valid() || ret.is_null())
      {
         // An "empty" input object is required. The internal containers still hold the mapped input entries
         ret = m_input_info->make_empty();
      }
      
      // Associate the used input with the snapshot signature. This will be needed later
      m_input_cache.associate(snapsig, ret->get_signature());
   }

   return ret;
}


void kehPlayerNode::dispatch_input_data()
{
   SceneTree* st = SceneTree::get_singleton();
   ERR_FAIL_COND_MSG(!st->has_network_peer() && !st->is_network_server(), "Dispatching input data can only be done on client.");
   ERR_FAIL_COND_MSG(!m_is_local, "Dispatching input data can only be done on node belonging to local player.");

   // NOTE: should amount of input data be checked and do nothing if 0?

   // Prepare the EncDecBuffer to encode input data
   m_encdec->set_buffer(PoolByteArray());

   const uint32_t csize = m_input_cache.get_cache_size();

   // Encode buffer size (that is, how many input objects are there). Using two bytes should
   // give plenty of packet loss time
   m_encdec->write_ushort(csize);

   for (uint32_t i = 0; i < csize; i++)
   {
      Ref<kehInputData> idata = m_input_cache.get_input_data(i);
      m_input_info->encode_to(m_encdec, idata);
   }

   // Send the encoded data to the server - this should go directly to the correct player node
   // within the server
   rpc_unreliable_id(1, "_server_receive_input", m_encdec->get_buffer());
}


uint32_t kehPlayerNode::get_used_input_in_snap(uint32_t snap_sig) const
{
   SceneTree* st = SceneTree::get_singleton();
   ERR_FAIL_COND_V_MSG(!st->has_network_peer() && st->is_network_server(), 0, "Retrieving the used input signature within a snapshot is only valid on server.");
   return m_input_cache.get_used_input_in_snap(snap_sig);
}


void kehPlayerNode::client_acknowledge_input(uint32_t isig)
{
   SceneTree* st = SceneTree::get_singleton();
   ERR_FAIL_COND_MSG(!st->has_network_peer() && !st->is_network_server(), "Trying to acknowledge input data but this is not a client.");

   m_input_cache.clear_older(isig);
}


void kehPlayerNode::start_ping()
{
   SceneTree* st = SceneTree::get_singleton();
   ERR_FAIL_COND_MSG(!st->has_network_peer() && !st->is_network_server(), "Starting the ping system is meant to be run only on servers.");

   m_ping = Ref<kehPingInfo>(memnew(kehPingInfo(m_net_id, this)));
}


void kehPlayerNode::add_custom_property(const String& pname, const Ref<kehCustomProperty>& prop)
{
   m_custom_data[pname] = Ref<kehCustomProperty>(memnew(kehCustomProperty(prop->get_value(), prop->get_mode())));
}


void kehPlayerNode::set_custom_property(const String& pname, const Variant& value)
{
   Map<String, Ref<kehCustomProperty>>::Element* e = m_custom_data.find(pname);
   if (e)
   {
      e->value()->set_value(value);
      if (e->value()->is_dirty())
      {
         m_custom_prop_dirty_count++;
      }
   }
   // NOTE: should this error out if the custom property does not exist?
}


void kehPlayerNode::remote_set_custom_property(const String& pname, const Variant& value)
{
   Map<String, Ref<kehCustomProperty>>::Element* prop = m_custom_data.find(pname);
   if (prop)
   {
      prop->value()->set_value(value);

      if (m_custom_prop_signaler.is_valid())
         m_custom_prop_signaler(m_net_id, pname, value);
   }
}


Variant kehPlayerNode::get_custom_property(const String& pname, const Variant& defval) const
{
   const Map<String, Ref<kehCustomProperty>>::Element* e = m_custom_data.find(pname);
   return e ? e->value()->get_value() : defval;
}


bool kehPlayerNode::encode_custom_props(Ref<kehEncDecBuffer>& into, const Map<String, Ref<kehCustomProperty>>& props, bool is_authority, bool force_nd)
{
   if (!has_dirty_custom_prop())
      return false;
   
   into->set_buffer(PoolByteArray());
   // Write ID of the player owning the custom properties
   into->write_uint(m_net_id);

   // Yes, this limits to 255 custom properties that can be encoded into a single packet.
   // This should be way more than enough. Regardless, the writing loop will end at 255 encoded properties.
   // On the next loop iteration the remaining dirty properties will be encoded (if any).
   into->write_byte(0);      // This will be rewritten.
   
   uint32_t encoded_props = 0;

   // NOTE:
   // In theory both the "reference properties" map and the "local map" should have the exact same layout, only
   // holding different values. This means that it would be possible to have two iterators (one for each map)
   // pointing to the exact same property name. For now just making the straightforward and process that is
   // sure to work. IF this hit performance then change the iteration method here.
   for (Map<String, Ref<kehCustomProperty>>::Element* cprop = m_custom_data.front(); cprop; cprop = cprop->next())
   {
      if (cprop->value()->get_mode() == kehCustomProperty::ReplicationMode::ServerOnly && is_authority)
      {
         // The property is meant to be "server only" and the code is already running on the server. Ensure the
         // property is not dirty and don't encode it.
         cprop->value()->clear_dirt();
         continue;
      }

      // Get the type of the reference value. In here relying on the fact that both maps **should** have the
      // property key, only potentially holding different property values.
      const uint32_t reftype = props.find(cprop->key())->value()->get_value().get_type();

      // And the type of the actual stored value. If they are different then push a warning telling about the problem
      // Also, leave the replication to the check_replication() function if the property is dirty
      const uint32_t stype = cprop->value()->get_value().get_type();
      
      if (reftype == stype && cprop->value()->encode_to(into, cprop->key(), stype, force_nd))
      {
         // If here the property was encoded - that means, it is of a supported type and was dirty. Update the encoded counter
         encoded_props++;
      }
      else if (cprop->value()->is_dirty() || force_nd)
      {
         if (reftype != stype)
         {
            // This extra check is mostly to output a warning telling about the fact that a custom property will be
            // replicated but its expected type does not match
            WARN_PRINT(vformat("Replication of custom property '%s' mismatched value type. Replicating the property in a different packet.", cprop->key()));            
         }

         check_replication(cprop->key(), cprop->value(), is_authority);
      }

      if (encoded_props == 255)
      {
         // Do not allow encoding go past 255 properties. Yet, with this system any property that still need to be synchronized
         // will be dispatched on the next loop iteration.
         break;
      }
   }

   if (encoded_props > 0)
   {
      // Rewrite the custom property "header" which is basically the number of encoded properties
      into->rewrite_byte(encoded_props, 4);
   }

   return encoded_props > 0;
}


void kehPlayerNode::decode_custom_props(Ref<kehEncDecBuffer>& from, const Map<String, Ref<kehCustomProperty>>& props, bool is_authority)
{
   const uint32_t ecount = from->read_byte();

   for (uint32_t i = 0; i < ecount; i++)
   {
      const String pname = from->read_string();

      const Map<String, Ref<kehCustomProperty>>::Element* pref = props.find(pname);
      if (!pref)
      {
         // Should this error out?
         return;
      }

      Map<String, Ref<kehCustomProperty>>::Element* stored = m_custom_data.find(pname);
      if (!stored)
      {
         // Should this error out?
         return;
      }

      // Decode the property, making it dirty if in server. This will allow the property to be verified
      // during the loop iteration and broadcast it if necessary.
      if (stored->value()->decode_from(from, pref->value()->get_value().get_type(), is_authority))
      {
         // Allow the "core" of the networking system to emit a signal indicating that a custom property
         // has been changed through synchronization
         if (m_custom_prop_signaler.is_valid())
            m_custom_prop_signaler(m_net_id, pname, stored->value()->get_value());
      }
      else
      {
         return;
      }
      
      if (stored->value()->is_dirty())
         m_custom_prop_dirty_count++;
   }
}


void kehPlayerNode::check_replication(const String& pname, Ref<kehCustomProperty>& prop, bool is_authority)
{
   switch (prop->get_mode())
   {
      case kehCustomProperty::ReplicationMode::ServerOnly:
      {
         // This property is meant to be given only to the server so if already there nothing
         // to be done. Otherwise, directly send the property to the server, which will be
         // automatically given to the correct player node.
         if (!is_authority)
         {
            rpc_id(1, "_remote_set_custom_property", pname, prop->get_value());
         }
      } break;

      case kehCustomProperty::ReplicationMode::ServerBroadcast:
      {
         // This property must be broadcast through the server. SO if running there directly use
         // the rpc() function, otherwise use the broadcast requester functoid to request the server
         // to do the broadcasting
         if (is_authority)
         {
            rpc("_remote_set_custom_property", pname, prop->get_value());
         }
         else
         {
            if (m_custom_prop_broadcast_requester.is_valid())
               m_custom_prop_broadcast_requester(pname, prop->get_value());
         }
      } break;
   }

   prop->clear_dirt();
}






Ref<kehInputData> kehPlayerNode::poll_input()
{
   ERR_FAIL_COND_V_MSG(!m_is_local, NULL, "Trying to poll input data from a node not belonging to local player.");

   Ref<kehInputData> ret = memnew(kehInputData(m_input_cache.increment_input()));

   if (m_input_info->use_mouse_relative() && m_input_enabled)
   {
      ret->set_mouse_relative(m_mrelative);
      // Must reset cached value otherwise motion will still be sent even if there is none
      m_mrelative = Vector2();
   }

   if (m_input_info->use_mouse_speed() && m_input_enabled)
   {
      ret->set_mouse_speed(m_mspeed);
      m_mspeed = Vector2();
   }

   // Iterate through analog data
   for (const Map<String, kehInputInfo::ActionInfo>::Element* e = m_input_info->get_bool_iterator(); e; e = e->next())
   {
      if (!e->value().custom && (m_input_enabled && e->value().enabled))
      {
         ret->set_pressed(e->key(), Input::get_singleton()->is_action_pressed(e->key()));
      }
      else
      {
         // Assume boolean (custom or not) is not pressed. Doing this to ensure the data is
         // present on the returned object
         ret->set_pressed(e->key(), false);
      }
   }

   // Interate through boolean data
   for (const Map<String, kehInputInfo::ActionInfo>::Element* e = m_input_info->get_analog_iterator(); e; e = e->next())
   {
      if (e->value().custom && (m_input_enabled && e->value().enabled))
      {
         ret->set_analog(e->key(), Input::get_singleton()->get_action_strength(e->key()));
      }
      else
      {
         // Assume this analog data is "neutral". Doing this to ensure the data is present
         // on the returned object
         ret->set_analog(e->key(), 0.0f);
      }
   }


   return ret;
}


void kehPlayerNode::server_receive_input(const PoolByteArray& encoded)
{
   ERR_FAIL_COND_MSG(!get_tree()->is_network_server(), "");

   m_encdec->set_buffer(encoded);

   // Decode amount of InputData objects within the encoded data
   const uint16_t count = m_encdec->read_ushort();

   // Decode each one of the objects
   for (uint16_t i = 0; i < count; i++)
   {
      Ref<kehInputData> input = m_input_info->decode_from(m_encdec);

      // If this is newer than the last input signature in the cache, add it into the buffer
      if (input->get_signature() > m_input_cache.get_last_sig())
      {
         m_input_cache.cache_remote_input(input);
      }
   }
}


void kehPlayerNode::client_ping(uint32_t sig, float last)
{
   // Answer back to the server
   rpc_unreliable_id(1, "_server_pong", sig);

   // Use the signaler so the kehNetwork singleton node can properly emit the signal indicating
   // that a new measured ping value has arrived.
   if (sig > 1 && m_ping_signaler.is_valid())
   {
      m_ping_signaler(m_net_id, last);
   }
}

void kehPlayerNode::server_pong(uint32_t sig)
{
   // Bail if not the server - this should be an error though
   if (!SceneTree::get_singleton()->is_network_server())
      return;
   
   // The RPC call arrives at the node corresponding to the player that called it.
   // This mens that the m_net_id holds teh correct network of the client
   float measured = m_ping->calculate_and_restart(sig);
   if (measured >= 0.0f)
   {
      if (m_broadcast_ping)
      {
         Vector<int> cpeers = SceneTree::get_singleton()->get_network_connected_peers();
         for (uint32_t i = 0; i < cpeers.size(); i++)
         {
            // Skip the player corresponding to the measured ping
            if (cpeers[i] != m_net_id)
            {
               rpc_unreliable_id(cpeers[i], "_client_ping_broadcast", measured);
            }
         }
      }

      // The server must get a signal with the measured value
      if (m_ping_signaler.is_valid())
      {
         m_ping_signaler(m_net_id, measured);
      }
   }
}

void kehPlayerNode::client_ping_broadcast(float value)
{
   // When this is called it will run on the player node corresponding to the correct player.
   // This means that the m_net_id is properly set for the signal that must be emitted.
   if (m_ping_signaler.is_valid())
   {
      m_ping_signaler(m_net_id, value);
   }
}




void kehPlayerNode::_notification(int what)
{
   switch (what)
   {
      case NOTIFICATION_READY:
      {
         set_process_input(m_is_local);

         rpc_config("_server_receive_input", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
         rpc_config("_client_ping", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
         rpc_config("_server_pong", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
         rpc_config("_client_ping_broadcast", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);

         rpc_config("_remote_set_custom_property", MultiplayerAPI::RPCMode::RPC_MODE_REMOTE);
      } break;

      
   }
}

void kehPlayerNode::_input(const Ref<InputEvent>& evt)
{
   // const InputEventScreenTouch *st = Object::cast_to<InputEventScreenTouch>(*p_event);
   const InputEventMouseMotion* mm = Object::cast_to<InputEventMouseMotion>(*evt);
   if (mm)
   {
      m_mrelative += mm->get_relative();
      m_mspeed = mm->get_speed();
   }
}



void kehPlayerNode::_bind_methods()
{
   // Bind non exposed (to scripting) functions
   ClassDB::bind_method(D_METHOD("_input", "event"), &kehPlayerNode::_input);
   ClassDB::bind_method(D_METHOD("_server_receive_input", "encoded"), &kehPlayerNode::server_receive_input);

   ClassDB::bind_method(D_METHOD("_client_ping", "sig", "last"), &kehPlayerNode::client_ping);
   ClassDB::bind_method(D_METHOD("_server_pong", "sig"), &kehPlayerNode::server_pong);
   ClassDB::bind_method(D_METHOD("_client_ping_broadcast", "value"), &kehPlayerNode::client_ping_broadcast);

   ClassDB::bind_method(D_METHOD("_remote_set_custom_property", "pname", "value"), &kehPlayerNode::remote_set_custom_property);

   // Bind exposed functions
   ClassDB::bind_method(D_METHOD("get_uid"), &kehPlayerNode::get_id);
   ClassDB::bind_method(D_METHOD("reset_data"), &kehPlayerNode::reset_data);

   ClassDB::bind_method(D_METHOD("set_custom_property", "pname", "value"), &kehPlayerNode::set_custom_property);
   ClassDB::bind_method(D_METHOD("get_custom_property", "pname", "defval"), &kehPlayerNode::get_custom_property, DEFVAL(NULL));
   

   ADD_PROPERTY(PropertyInfo(Variant::INT, "net_id", PROPERTY_HINT_NONE, "", 1), "", "get_uid");
}


kehPlayerNode::kehPlayerNode(uint32_t pid, bool is_local, kehInputInfo* input_info) :
   m_is_local(is_local),
   m_is_ready(false),
   m_input_info(input_info),
   m_custom_prop_dirty_count(0)
{
   // Authority by default
   set_id(pid);

   m_input_enabled = is_local;

   SceneTree* st = SceneTree::get_singleton();
   if (is_local || (st->has_network_peer() && st->is_network_server()))
   {
      // The EncDecBuffer is only relevant if this node belongs to local player or if this is
      // the server.
      m_encdec = Ref<kehEncDecBuffer>(memnew(kehEncDecBuffer));
   }
}

kehPlayerNode::~kehPlayerNode()
{

}
