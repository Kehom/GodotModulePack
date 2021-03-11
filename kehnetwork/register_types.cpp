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

#include "register_types.h"

#include "core/class_db.h"
#include "core/engine.h"
#include "core/os/os.h"

#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"

#include "customproperty.h"
#include "playernode.h"
#include "playerdata.h"
#include "snapentity.h"
#include "inputdata.h"
#include "snapshotdata.h"
#include "network.h"
#include "nodespawner.h"

// This is the network singleton
static kehNetwork* keh_network = NULL;
// When the network node get inside the tree, there will be no need to delete it from here. However if
// it does not get into the tree, this cleanup will be necessary. So, whenever the node get into the tree
// a function will be called to toggle this flag.
// This "hacky" method is the only way I have found to *not* get error messages when exiting the editor/game.
// If there is a better way, please tell me!
static bool got_on_tree = false;

// This will be used mostly to push the settings to the end of the list within the ProjectSettings window.
// Almost sure this is not entirely necessary but doing so mostly to distinguish from "vanilla" Godot
static uint32_t setting_order = ProjectSettings::NO_BUILTIN_ORDER_BASE;

// This function will be assigned as a function pointer within the singleton node and will be called as
// soon as the node enters the scene tree.
void on_enter_tree()
{
   got_on_tree = true;
}


// Yes, there is the GLOBAL_DEF that can be used to create new settings. But while trying to debug
// some weird behavior, this function was created... now it will stay here.
void create_psetting(const String& sname, const Variant& dval, Variant::Type type = Variant::NIL, PropertyHint phint = PROPERTY_HINT_NONE, const String& hint = "")
{
   ProjectSettings* ps = ProjectSettings::get_singleton();
   if (!ps->has_setting(sname))
   {
      ps->set(sname, dval);
   }

   ps->set_initial_value(sname, dval);
   ps->set_builtin_order(sname);

   if (type != Variant::NIL)
   {
      ps->set_custom_property_info(sname, PropertyInfo(type, sname, phint, hint));
   }

   ps->set_order(sname, setting_order++);
}


void register_kehnetwork_types()
{
   // Register the classes
   ClassDB::register_class<kehCustomProperty>();
   ClassDB::register_class<kehInputData>();
   ClassDB::register_virtual_class<kehNetNodeSpawner>();
   ClassDB::register_class<kehNetDefaultSpawner>();
   ClassDB::register_class<kehPlayerNode>();
   ClassDB::register_class<kehPlayerData>();
   ClassDB::register_virtual_class<kehSnapEntityBase>();
   ClassDB::register_class<kehSnapshotData>();
   ClassDB::register_class<kehNetwork>();

   // Register network singleton
   keh_network = memnew(kehNetwork(&on_enter_tree));

   Engine::get_singleton()->add_singleton(Engine::Singleton("kehNetwork", kehNetwork::get_singleton()));

   // The settings related to the networking
   ProjectSettings* ps = ProjectSettings::get_singleton();
   if (ps)
   {
      create_psetting("keh_modules/network/general/print_debug_info", false);
      create_psetting("keh_modules/network/generatel/compression", 1, Variant::INT, PROPERTY_HINT_ENUM, "None, Rangecoder, FastLZ, ZLib, ZSTD");
      create_psetting("keh_modules/network/general/mode", 0, Variant::INT, PROPERTY_HINT_ENUM, "ENet, WebSocket");
      create_psetting("keh_modules/network/general/broadcast_measured_ping", true);

      create_psetting("keh_modules/network/snapshot/max_history", 120);
      create_psetting("keh_modules/network/snapshot/max_client_history", 60);
      create_psetting("keh_modules/network/snapshot/full_threshold", 12);

      create_psetting("keh_modules/network/input/use_mouse_relative", false);
      create_psetting("keh_modules/network/input/use_mouse_speed", false);
      create_psetting("keh_modules/network/input/quantize_analog_data", false);
   }
}

void unregister_kehnetwork_types()
{
   if (!got_on_tree)
   {
      memdelete(keh_network);
   }
   

   // Nullifying the pointers is probably not necessary, but...
   keh_network = NULL;
}
