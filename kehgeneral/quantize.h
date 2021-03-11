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

#ifndef _KEHGENERAL_QUANTIZE_H
#define _KEHGENERAL_QUANTIZE_H 1

#include "core/reference.h"


class kehQuantize : public Reference
{
   GDCLASS(kehQuantize, Reference);

private:
   const float ROTATION_BOUNDS = 0.707107f;

   // "Define" some masks to help pack/unpack quantized rotation quaternions into/from integers
   const uint32_t MASK_A_9BIT = 511;              // 511 == 111111111   (9 bits per component)
   const uint32_t MASK_B_9BIT = 511 << 9;
   const uint32_t MASK_C_9BIT = 511 << 18;
   const uint32_t MASK_INDEX_9BIT = 3 << 27;      // 3 = 11
   const uint32_t MASK_SIGNAL_9BIT = 1 << 30;

   const uint32_t MASK_A_10BIT = 1023;            // 1023 = 1111111111    (10 bit per component)
   const uint32_t MASK_B_10BIT = 1023 << 10;
   const uint32_t MASK_C_10BIT = 1023 << 20;
   const uint32_t MASK_INDEX_10BIT = 3 << 30;

   const uint32_t MASK_A_15BIT = 32767;
   const uint32_t MASK_B_15BIT = 32767 << 15;
   // Component C in this case is packed into a secondary integer, but having a dedicated const
   // for it may help reduce confusion when reading the code
   const uint32_t MASK_C_15BIT = 32767;
   const uint32_t MASK_INDEX_15BIT = 3 << 30;
   // There is "one bit left", which will be used for the quaternion signal
   const uint32_t MASK_SIGNAL_15BIT = 1 << 15;

   static kehQuantize* s_singleton;

protected:
   static void _bind_methods();

public:
   // Dictionaries use Variants to store data. Retrieving them can be somewhat problematic when
   // bit masking is desired. To that end this internal struct will be used to perform most of
   // the packing/unpacking of rotation quaternions. The methods exposed to GDScript will deal
   // with dictionaries but will internally use the struct versions.
   struct uquat_data
   {
      uint32_t a;
      uint32_t b;
      uint32_t c;
      uint8_t index;
      int8_t signal;

      uquat_data(uint32_t _a, uint32_t _b, uint32_t _c, uint8_t i, int8_t s = 1) :
         a(_a), b(_b), c(_c), index(i), signal(s) {}
   };

   // Quantize a unit float (range [0..1]) into an integer of the specified number of bits.
   uint32_t quantize_unit_float(float value, int num_bits);

   // Restore a floating point from encoded integer. It assumes the original float is in the
   // [0..1] range, that is, was encoded with the quantize_unit_float(). Also, the number of
   // bits must match.
   float restore_unit_float(uint32_t quantized, int num_bits);

   // Quantize a float in an arbitrary range defined by [minval..maxval].
   uint32_t quantize_float(float value, float minval, float maxval, int num_bits);

   // Restore a float from a quantized value defined by [minval..maxval] range.
   // Number of bits must match the one used to quantize the float.
   float restore_float(uint32_t quantized, float minval, float maxval, int num_bits);

   // Compress the given rotation quaternion (or unit Quat) using the specified number of bits
   // per component using the smallest three method.
   // NOTE: this one is not meant to be exposed to GDScript as it uses internal struct
   uquat_data compress_rotation_quat(const Quat& q, int num_bits);

   // Compress the given rotation quaternion (or unit Quat) using the specified number of bits
   // per component using the smallest three method. The returned dictionary contains 5 fields:
   // a, b, c -> the smallest three quantized components
   // index -> the index [0..3] of the dropped (largest) component
   // sig -> the signal of the dropped component (1 if positive, -1 if negative)
   // NOTE: Signal is not exactly necessary, but is provided just so if there is any desire to
   // encode it somewhere, it can be used.
   // This is meant to be exposed to GDScript but internally uses compres_rotation_quat()
   Dictionary _compress_rotation_quat(const Quat& q, int num_bits);

   // Restore a rotation quaternion
   Quat restore_rotation_quat(const uquat_data& quant, int num_bits);

   // Restore a rotation quaternion. The quantized values must be given in a dictionary with the
   // same format of the one returned by the compress_rotation_quat() function.
   Quat _restore_rotation_quat(const Dictionary& quant, int num_bits);


   // Compress the given quaternion using 9 bits per component. In this wrapper function, the
   // data is packed into a single integer value. Because there is still some "room" (only 29
   // bits of the 32 are used), the original signal of the quaternion is also stored.
   uint32_t compress_rquat_9bits(const Quat& q);

   // Restores a quaternion previously quantized using compress_rquat_9bits.
   Quat restore_rquat_9bits(uint32_t compressed);


   // Compress the given quaternion using 10 bits per component. In this wrapper function, the
   // data is packed into a single integer value. Because all bits are used for the data, the
   // original signal may be lost and the quaternion may be "flipped". For rotations this is not
   // a problem as q and -q represent the exact same orientation.
   uint32_t compress_rquat_10bits(const Quat& q);

   // Restores a quaternion previously quantized using compress_rquat_10bits. Remember the signal
   // may be flipped in this case.
   Quat restore_rquat_10bits(uint32_t compressed);


   // Compress the given quaternion using 15 bits per component. In this wrapper function, the
   // data is packed into two integer values, returned in a PoolIntArray. In memory this will still
   // use the full range of the integer values, but the second entry in the array can safely discard
   // 16 bits, which is basically the desired usage when sending data through network. Note that in
   // this case, using 32 bit + 16 bit leaves room for a single bit, which is used to encode the
   // original quaternion signal.
   PoolIntArray compress_rquat_15bits(const Quat& q);

   // Restores a quaternion previously quantized with compress_rquat_15bits. The input must be
   // integers within the PoolIntArray of the compression function, in the same order for the arguments.
   Quat restore_rquat_15bits(uint32_t pack0, uint16_t pack1);


   static kehQuantize* get_singleton();

   kehQuantize();
};


#endif
