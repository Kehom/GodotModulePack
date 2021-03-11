/**
 *
 *
 */

#include "register_types.h"

#include "core/class_db.h"
#include "core/engine.h"
#include "encdecbuffer.h"
#include "quantize.h"

static kehQuantize* keh_quantize = NULL;

void register_kehgeneral_types()
{
   ClassDB::register_class<kehEncDecBuffer>();
   ClassDB::register_class<kehQuantize>();

   keh_quantize = memnew(kehQuantize);
   Engine::get_singleton()->add_singleton(Engine::Singleton("kehQuantize", kehQuantize::get_singleton()));
}

void unregister_kehgeneral_types()
{
   if (keh_quantize)
   {
      memdelete(keh_quantize);
      keh_quantize = NULL;
   }
}
