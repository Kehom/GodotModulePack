/**
 *
 *
 */

#ifndef _KEHGENERAL_ENCDECBUFFER_H
#define _KEHGENERAL_ENCDECBUFFER_H 1

#include "core/reference.h"

// The kehEncDecBuffer is meant to simplify the task of encoding/decoding data into low level
// bytes (PoolByteArray). The thing is, the ideal is to remove the variant header bytes from
// properties, which incorporate 4 bytes for each one.
// This class deals with a sub-set of the types given by Godot scripting and was primarily meant
// to be used with the networking addon, but this can be useful in other scenarios, like a saving
// system, for example.
//
// Now, why this trouble? Variables in GDScript take more bytes than we normally expect. Each one
// contains an additional set of 4 bytes representing the "header", which is basically indicating
// to Godot which type is actually held in memory. Some types may even bring further overhead
// and directly using them through the network may not necessarily be the best option.
//
// Now there is one very special case there. Unfortunately  we don't have unsigned integers within
// GDScript. This brings a somewhat not so fun "limitation" to how numbers are represented.
//
// The maximum positive number that can be represented with an unsigned 32 bit integer is 4294967295.
// However, because GDScript only deals with signed numbers, the limit here would be 2147483647.
// But we can have bigger positive numbers in GDScript, only that behind the scenes Godot uses 64
// bit integers. In other words, if we directly send a hash number (32 bit), the result will be that
// 12 bytes will be used instead of just 8 (or the desired 4 bytes).
//
// This class allows "unsigned integers" to be stored in the PoolByteArray using the desired
// 4 bytes, provided the value stays within the boundary.


class kehEncDecBuffer : public Reference
{
   GDCLASS(kehEncDecBuffer, Reference);

private:
   const uint32_t MAX_UINT = 0xFFFFFFFF;

   // Bytes will be stored here.
   PoolByteArray m_buffer;

   // The reading position
   uint32_t m_rindex;



   // Generic function to append bytes into the internal buffer
   void append_bytes(const uint8_t* in, const uint32_t count);



   // Generic function to encode raw bytes into the given buffer
   void encode_bytes(const uint8_t* ptr, const uint32_t count, const int offset);


   // Read the specified number of bytes from the internal buffer. Automatically moves the reading index
   void decode_bytes(const int count, uint8_t* output);
   

protected:
   static void _bind_methods();

public:
   // If true is returned then the reading index is at a position not past the last byte of the internal PoolByteArray
   bool has_read_data() const;

   // Obtain current amount of bytes stored within the internal buffer
   int get_current_size() const;



   // Append a boolean into the buffer array
   void write_bool(bool value);
   // Rewrite a boolean at the given byte position
   void rewrite_bool(bool value, int at);
   // Read a boolean from the internal buffer. Automatically moves the reading index
   bool read_bool();

   // Append an integer into the buffer array
   void write_int(int value);
   // Rewrite an integer at the given byte position
   void rewrite_int(int value, int at);
   // Read an integer from the internal buffer. Automatically moves the reading index
   int read_int();

   // Append a floating point into the buffer array
   void write_float(float value);
   // Rewrite a floating point at the given byte position
   void rewrite_float(float value, int at);
   // Read a float from the internal buffer. Automatically moves the reading index
   float read_float();

   // Append a Vector2 into the buffer array
   void write_vector2(const Vector2& value);
   // Rewrite a Vector2 at the given byte position
   void rewrite_vector2(const Vector2& value, int at);
   // Read a vector2 from the internal buffer. Automatically moves the reading index
   Vector2 read_vector2();

   // Append a Rect2 into the buffer array
   void write_rect2(const Rect2& value);
   // Rewrite a Rect2 at the given byte position
   void rewrite_rect2(const Rect2& value, int at);
   // Read a Rect2 from the internal buffer. Automatically moves the reading index
   Rect2 read_rect2();

   // Append a Vector3 into the buffer array
   void write_vector3(const Vector3& value);
   // Rewrite a Vector3 at the given byte position
   void rewrite_vector3(const Vector3& value, int at);
   // Read a Vector3 from the internal buffer. Automatically moves the reading index
   Vector3 read_vector3();

   // Append a Quaternion into the buffer array
   void write_quat(const Quat& value);
   // Rewrite a Quaternion at the given byte position
   void rewrite_quat(const Quat& value, int at);
   // Read a Quaternion from the internal buffer. Automatically moves the reading index
   Quat read_quat();

   // Append a Color into the buffer array
   void write_color(const Color& value);
   // Rewrite a Color at the given byte position
   void rewrite_color(const Color& value, int at);
   // Read a Color from the internal buffer. Automatically moves the reading index
   Color read_color();

   // Append an unsigned 32 bits integer into the buffer array
   void write_uint(uint32_t value);
   // Rewrite an unsigned 32 bits integer at the given byte position
   void rewrite_uint(uint32_t value, int at);
   // Read an unsigned 32 bits integer from the internal buffer. Automatically moves the reading index
   uint32_t read_uint();

   // Append a single byte into the buffer array
   void write_byte(int value);
   // Rewrite a single byte at the given position
   void rewrite_byte(int value, int at);
   // Read a single byte from the internal buffer. Automatically moves the reading index
   uint8_t read_byte();

   // Append an unsigned 16 bits integer into the buffer array
   void write_ushort(int value);
   // Rewrite an unsigned 16 bits integer at the given byte position
   void rewrite_ushort(int value, int at);
   // Read an unsigned 16 bites integer from the internal buffer. Automatically moves the reading index
   uint16_t read_ushort();

   // Append a String into the buffer array.
   // Note that because strings may have different sizes rewriting them is not supported
   void write_string(const String& value);
   // Read a String from the internal buffer. Automatically moves teh reading index
   String read_string();



   /// Setters/Getters

   void set_buffer(const PoolByteArray& b);
   PoolVector<uint8_t> get_buffer() const { return m_buffer; }


   kehEncDecBuffer();
};

#endif
