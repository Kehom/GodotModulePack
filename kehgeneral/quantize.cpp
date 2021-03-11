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

#include "quantize.h"

kehQuantize* kehQuantize::s_singleton = NULL;

kehQuantize* kehQuantize::get_singleton()
{
   return s_singleton;
}


uint32_t kehQuantize::quantize_unit_float(float value, int num_bits)
{
   // TODO: Check if the number of bits is in the correct range [1..32]

   uint32_t intervals = 1 << num_bits;
   float scaled = value * (intervals - 1.0f);
   uint32_t rounded = (uint32_t)(scaled + 0.5f);

   if (rounded > intervals - 1)
   {
      rounded -= 1;
   }

   return rounded;
}

float kehQuantize::restore_unit_float(uint32_t quantized, int num_bits)
{
   uint32_t intervals = 1 << num_bits;
   float interval_size = 1.0f / (float)(intervals - 1.0f);
   float approx_float = (float)quantized * interval_size;
   return approx_float;
}

uint32_t kehQuantize::quantize_float(float value, float minval, float maxval, int num_bits)
{
   // First convert the given value into a unit-float. Yes, this may even further cause error
   float unit = (value - minval) / (maxval - minval);
   uint32_t quantized = quantize_unit_float(unit, num_bits);
   return quantized;
}

float kehQuantize::restore_float(uint32_t quantized, float minval, float maxval, int num_bits)
{
   float unit = restore_unit_float(quantized, num_bits);
   // Convert back into the [minval..maxval] range
   float approx_float = minval + (unit * (maxval - minval));
   return approx_float;
}


// NOTE: this assumes the incoming quaternion is a rotation one, which means it's a unit quaternion (length = 1).
kehQuantize::uquat_data kehQuantize::compress_rotation_quat(const Quat& q, int num_bits)
{
   // Since it's not possible to iterate through the quaternion components through a loop, creating
   // this temporary array.
   float comps[4] = { q.x, q.y, q.z, q.w };
   uint32_t quant[3];           // Will hold the three quantized components
   int mindex = 0;              // Index of the largest component
   float mval = -1.0f;          // Value of the largest component
   float sig = 1.0f;            // Signal of the dropped component. Assume it's positive.

   // Locate the largest component, storing its absolute value as well as the index
   for (int i = 0; i < 4; i++)
   {
      float abval = abs(comps[i]);

      if (abval > mval)
      {
         mval = abval;
         mindex = i;
      }
   }

   if (comps[mindex] < 0.0f)
   {
      sig = -1.0f;
   }

   // Quantize the smallest components
   for (int i = 0, c = 0; i < 4; i++)
   {
      if (i != mindex)
      {
         float fl = comps[i] * sig;
         quant[c++] = quantize_float(fl, -ROTATION_BOUNDS, ROTATION_BOUNDS, num_bits);
      }
   }

   return uquat_data(quant[0], quant[1], quant[2], mindex, sig == 1.0f ? 1 : 0);
}


Dictionary kehQuantize::_compress_rotation_quat(const Quat& q, int num_bits)
{
   Dictionary ret;

   uquat_data quant = compress_rotation_quat(q, num_bits);

   ret["a"] = quant.a;
   ret["b"] = quant.b;
   ret["c"] = quant.c;
   ret["index"] = quant.index;
   ret["sig"] = quant.signal;

   return ret;
}


Quat kehQuantize::restore_rotation_quat(const kehQuantize::uquat_data& quant, int num_bits)
{
   // Take signal from 0=negative|1=positive (easier bit packing) to -1,+1 for easier multiplication
   float sig = quant.signal == 1 ? 1.0f : -1.0f;

   // Restore the three smallest components (a, b and c)
   float ra = restore_float(quant.a, -ROTATION_BOUNDS, ROTATION_BOUNDS, num_bits) * sig;
   float rb = restore_float(quant.b, -ROTATION_BOUNDS, ROTATION_BOUNDS, num_bits) * sig;
   float rc = restore_float(quant.c, -ROTATION_BOUNDS, ROTATION_BOUNDS, num_bits) * sig;
   // Restore the dropped component
   float dropped = sqrtf(1.0f - ra*ra - rb*rb - rc*rc) * sig;

   Quat ret = Quat();

   switch (quant.index)
   {
      case 0:
         // X was dropped
         ret = Quat(dropped, ra, rb, rc);
         break;
      
      case 1:
         // Y was dropped
         ret = Quat(ra, dropped, rb, rc);
         break;
      
      case 2:
         // Z was dropped
         ret = Quat(ra, rb, dropped, rc);
         break;
      
      case 3:
         // W was dropped
         ret = Quat(ra, rb, rc, dropped);
         break;
   }


   return ret;
}


Quat kehQuantize::_restore_rotation_quat(const Dictionary& quant, int num_bits)
{
   return restore_rotation_quat(uquat_data(quant["a"], quant["b"], quant["c"], quant["index"], quant["signal"]), num_bits);
}


uint32_t kehQuantize::compress_rquat_9bits(const Quat& q)
{
   // First compress normally
   uquat_data comp = compress_rotation_quat(q, 9);
   // Then pack the data
   return (((comp.signal << 30) & MASK_SIGNAL_9BIT) |
           ((comp.index << 27) & MASK_INDEX_9BIT) |
           ((comp.c << 18) & MASK_C_9BIT) |
           ((comp.b << 9) & MASK_B_9BIT) |
           (comp.a & MASK_A_9BIT));
}

Quat kehQuantize::restore_rquat_9bits(uint32_t compressed)
{
   // To restore, must use dictionary, so create it
   uquat_data unpacked(
      compressed & MASK_A_9BIT,
      (compressed & MASK_B_9BIT) >> 9,
      (compressed & MASK_C_9BIT) >> 18,
      (compressed & MASK_INDEX_9BIT) >> 27,
      (compressed & MASK_SIGNAL_9BIT) >> 30);

   return restore_rotation_quat(unpacked, 9);
}


uint32_t kehQuantize::compress_rquat_10bits(const Quat& q)
{
   uquat_data comp = compress_rotation_quat(q, 10);
   return (((comp.index << 30) & MASK_INDEX_10BIT) |
           ((comp.c << 20) & MASK_C_10BIT) |
           ((comp.b << 10) & MASK_B_10BIT) |
           (comp.a & MASK_A_10BIT));
}

Quat kehQuantize::restore_rquat_10bits(uint32_t compressed)
{
   uquat_data unpacked(
      compressed & MASK_A_10BIT,
      (compressed & MASK_B_10BIT) >> 10,
      (compressed & MASK_C_10BIT) >> 20,
      (compressed & MASK_INDEX_10BIT) >> 30);

   return restore_rotation_quat(unpacked, 10);
}


PoolIntArray kehQuantize::compress_rquat_15bits(const Quat& q)
{
   // Obtain the compressed data
   uquat_data c = compress_rotation_quat(q, 15);

   PoolIntArray ret;
   ret.append(((c.index << 30) & MASK_INDEX_15BIT) |
              ((c.b << 15) & MASK_B_15BIT) |
              (c.a & MASK_A_15BIT));
   
   ret.append(((c.signal << 15) & MASK_SIGNAL_15BIT) | (c.c & MASK_C_15BIT));

   return ret;
}

Quat kehQuantize::restore_rquat_15bits(uint32_t pack0, uint16_t pack1)
{
   uquat_data unpacked(
      pack0 & MASK_A_15BIT,
      (pack0 & MASK_B_15BIT) >> 15,
      pack1 & MASK_C_15BIT,
      (pack0 & MASK_INDEX_15BIT) >> 30,
      (pack1 & MASK_SIGNAL_15BIT) >> 15);

   return restore_rotation_quat(unpacked, 15);
}



void kehQuantize::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("quantize_unit_float", "value", "numbits"), &kehQuantize::quantize_unit_float);
   ClassDB::bind_method(D_METHOD("restore_unit_float", "quantized", "numbits"), &kehQuantize::restore_unit_float);
   ClassDB::bind_method(D_METHOD("quantize_float", "value", "minval", "maxval", "numbits"), &kehQuantize::quantize_float);
   ClassDB::bind_method(D_METHOD("restore_float", "quantized", "minval", "maxval", "numbits"), &kehQuantize::restore_float);

   ClassDB::bind_method(D_METHOD("compress_rotation_quat", "unit_quat", "numbits"), &kehQuantize::_compress_rotation_quat);
   ClassDB::bind_method(D_METHOD("restore_rotation_quat", "quant", "numbits"), &kehQuantize::_restore_rotation_quat);

   ClassDB::bind_method(D_METHOD("compress_rquat_9bits", "q"), &kehQuantize::compress_rquat_9bits);
   ClassDB::bind_method(D_METHOD("restore_rquat_9bits", "compressed"), &kehQuantize::restore_rquat_9bits);

   ClassDB::bind_method(D_METHOD("compress_rquat_10bits", "q"), &kehQuantize::compress_rquat_10bits);
   ClassDB::bind_method(D_METHOD("restore_rquat_10bits"), &kehQuantize::restore_rquat_10bits);

   ClassDB::bind_method(D_METHOD("compress_rquat_15bits", "q"), &kehQuantize::compress_rquat_15bits);
   ClassDB::bind_method(D_METHOD("restore_rquat_15bits", "pack0", "pack1"), &kehQuantize::restore_rquat_15bits);
}


kehQuantize::kehQuantize()
{
   s_singleton = this;
}
