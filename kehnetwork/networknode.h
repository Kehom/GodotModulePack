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

#ifndef _KEHNETWORK_NETWORKNODE_H
#define _KEHNETWORK_NETWORKNODE_H 1

/*
#include "scene/main/node.h"
#include "functoid.h"

class kehPlayerData;
class kehPlayerNode;


class kehNetNode : public Node
{
   // Is this necessary? Nothing specific to this class will be exposed to GDScript.
   GDCLASS(kehNetNode, Node);
private:
   bool m_need_manual_cleanup;

   // Signalers - those will relay events to the singleton, which will properly emit the signal
   // so outside code can act on those events.
   kehFunctoid<void()> m_join_accepted;
   kehFunctoid<void(uint32_t)> m_player_added;

   // Player data management requires remote calls, thus will be handled by this node instead of the singleton
   kehPlayerData* m_player_data;


   /// These functions are meant to be run on both server and clients (thus marked as "all").
   void all_register_player(uint32_t pid);
   void all_unregister_player(uint32_t pid);


   /// Functions meant to be called by server and run on client.
   // Server will call this function to notify that the connection was accepted.
   void client_join_accepted();

protected:
   void _notification(int what);

   static void _bind_methods();

public:
   void set_join_accepted_signaler(const kehFunctoid<void()>& f) { m_join_accepted = f; }
   void set_player_added_signaler(const kehFunctoid<void(uint32_t)>& f) { m_player_added = f; }

   bool need_manual_cleanup() const { return m_need_manual_cleanup; }

   void set_local_player_id(uint32_t id);

   void clear_remote_players();





   kehNetNode();
   ~kehNetNode();
};
*/



#endif

