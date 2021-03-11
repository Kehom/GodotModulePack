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

#include "playerdata.h"

#include "playernode.h"


kehPlayerNode* kehPlayerData::create_local_player()
{
   if (!m_local_player)
   {
      m_local_player = create_pnode(1, true);
   }

   return m_local_player;
}


kehPlayerNode* kehPlayerData::create_player(uint32_t id)
{
   return create_pnode(id, false);
}


kehPlayerNode* kehPlayerData::get_remote_player(uint32_t pid) const
{
   const Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.find(pid);
   return e ? e->value() : NULL;
}

void kehPlayerData::get_remote_player_list(PoolVector<uint32_t>& outlist) const
{
   for (const Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.front(); e; e = e->next())
   {
      outlist.append(e->key());
   }
}

void kehPlayerData::remove_remote_player(uint32_t pid)
{
   m_remote_player.erase(pid);
}



void kehPlayerData::add_remote(kehPlayerNode* pnode)
{
   m_remote_player[pnode->get_id()] = pnode;
}

void kehPlayerData::clear_remote()
{
   // First queue player nodes for deletion
   for (Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.front(); e; e = e->next())
   {
      e->value()->queue_delete();
   }

   m_remote_player.clear();
}


kehPlayerNode* kehPlayerData::get_pnode(uint32_t pid) const
{
   if (m_local_player->get_id() == pid)
      return m_local_player;
   
   const Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.find(pid);
   return e ? e->value() : NULL;
}


void kehPlayerData::add_custom_property(const String& pname, const Variant& default_value, kehCustomProperty::ReplicationMode mode)
{
   Ref<kehCustomProperty> prop = Ref<kehCustomProperty>(memnew(kehCustomProperty(default_value, mode)));
   m_custom_property[pname] = prop;

   // Add this property to the local player
   m_local_player->add_custom_property(pname, prop);

   // Ensure all remote players also have this property
   for (Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.front(); e; e = e->next())
   {
      e->value()->add_custom_property(pname, prop);
   }
}


void kehPlayerData::set_custom_property(const String& pname, const Variant& value)
{
   Map<String, Ref<kehCustomProperty>>::Element* e = m_custom_property.find(pname);
   if (e)
   {
      if (e->value()->get_value().get_type() == value.get_type())
      {
         m_local_player->set_custom_property(pname, value);
      }
      else
      {
         WARN_PRINT(vformat("Trying to assign '%s' custom property but value type does not match the expected one.", pname));
      }
   }
}

Variant kehPlayerData::get_custom_property(const String& pname, const Variant& defval) const
{
   return m_local_player->get_custom_property(pname, defval);
}




void kehPlayerData::register_action(const String& action, bool is_analog)
{
   m_input_info.register_action(action, is_analog, false);
}

void kehPlayerData::register_custom_action(const String& action, bool is_analog)
{
   m_input_info.register_action(action, is_analog, true);
}

void kehPlayerData::register_custom_vec2(const String& vec)
{
   m_input_info.register_vec2(vec);
}

void kehPlayerData::register_custom_vec3(const String& vec)
{
   m_input_info.register_vec3(vec);
}

void kehPlayerData::set_action_enabled(const String& action, bool enabled)
{
   m_input_info.set_action_enabled(action, enabled);
}

void kehPlayerData::set_use_mouse_relative(bool use)
{
   m_input_info.set_use_mouse_relative(use);
}

void kehPlayerData::set_use_mouse_speed(bool use)
{
   m_input_info.set_use_mouse_speed(use);
}


void kehPlayerData::reset_input()
{
   m_input_info.reset_registrations();
}


void kehPlayerData::fill_remote_player_list(PoolVector<uint32_t>& out) const
{
   for (const Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.front(); e; e = e->next())
   {
      out.append(e->key());
   }
}

void kehPlayerData::fill_remote_player_node(PoolVector<kehPlayerNode*>& out) const
{
   for (const Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.front(); e; e = e->next())
   {
      out.append(e->value());
   }
}


Array kehPlayerData::get_registered_players(bool include_local) const
{
   Array ret;
   if (include_local)
   {
      ret.push_back(m_local_player);
   }

   for (const Map<uint32_t, kehPlayerNode*>::Element* e = m_remote_player.front(); e; e = e->next())
   {
      ret.append(e->value());
   }

   return ret;
}



kehPlayerNode* kehPlayerData::create_pnode(uint32_t pid, bool local)
{
   kehPlayerNode* ret = memnew(kehPlayerNode(pid, local, (kehInputInfo*)&m_input_info));

   // Assign the signalers
   ret->set_ping_signaler(m_ping_signaler);
   ret->set_cprop_broadcaster(m_cprop_broadcaster);
   ret->set_cprop_signaler(m_cprop_signaler);

   // Add the registered custom properties
   for (const Map<String, Ref<kehCustomProperty>>::Element* e = m_custom_property.front(); e; e = e->next())
   {
      ret->add_custom_property(e->key(), e->value());
   }
   return ret;
}


void kehPlayerData::_notification(int what)
{
   switch (what)
   {
      case NOTIFICATION_PREDELETE:
      {
         // The local player node is always a child of another node, meaning that it will be automatically
         // deleted when the parent is removed from the tree.
         // Nullifying the pointer here just for cleanup.
         m_local_player = NULL;
      } break;
   }
}



void kehPlayerData::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("get_registered_players", "include_local"), &kehPlayerData::get_registered_players, DEFVAL(false));
   ClassDB::bind_method(D_METHOD("get_player_count"), &kehPlayerData::get_player_count);

   ClassDB::bind_method(D_METHOD("get_local_player"), &kehPlayerData::get_local_player);
   ClassDB::bind_method(D_METHOD("get_remote_player", "pid"), &kehPlayerData::get_remote_player);
   ClassDB::bind_method(D_METHOD("get_pnode", "pid"), &kehPlayerData::get_pnode);

   ClassDB::bind_method(D_METHOD("add_custom_property", "pname", "default_value", "replicate"), &kehPlayerData::add_custom_property, DEFVAL(kehCustomProperty::ReplicationMode::ServerOnly));
   ClassDB::bind_method(D_METHOD("set_custom_property", "pname", "value"), &kehPlayerData::set_custom_property);
   ClassDB::bind_method(D_METHOD("get_custom_property", "pname", "defval"), &kehPlayerData::get_custom_property, DEFVAL(NULL));
}


kehPlayerData::kehPlayerData()
{
   m_local_player = NULL;
}

kehPlayerData::~kehPlayerData()
{
}

