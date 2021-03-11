/**
 *
 *
 */

#include "encdecbuffer.h"

/// TODO: better boundary check when decoding and rewriting values.

bool kehEncDecBuffer::has_read_data() const
{
   return m_rindex < m_buffer.size();
}

int kehEncDecBuffer::get_current_size() const
{
   return m_buffer.size();
}




void kehEncDecBuffer::write_bool(bool value)
{
   m_buffer.append(value);
}

void kehEncDecBuffer::rewrite_bool(bool value, int at)
{
   ERR_FAIL_COND_MSG(at >= m_buffer.size(), "Trying to rewrite boolean value at invalid buffer byte offset.");

   m_buffer.set(at, value);
}

bool kehEncDecBuffer::read_bool()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), false, "Trying to decode boolean but reading index has moved past last byte in the buffer.");
   bool ret = false;
   decode_bytes(1, (uint8_t*)&ret);

   return ret;
}


void kehEncDecBuffer::write_int(int value)
{
   append_bytes((const uint8_t*)&value, 4);
}
   
void kehEncDecBuffer::rewrite_int(int value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 4 >= m_buffer.size()), "Trying to rewrite integer value at invalid buffer byte offset.");
   encode_bytes((const uint8_t*)&value, 4, at);
}

int kehEncDecBuffer::read_int()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), 0, "Trying to decode integer but reading index has moved past last byte in the buffer.");
   int ret = 0;
   decode_bytes(4, (uint8_t*)&ret);

   return ret;
}


void kehEncDecBuffer::write_float(float value)
{
   append_bytes((const uint8_t*)&value, 4);
}

void kehEncDecBuffer::rewrite_float(float value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 4 >= m_buffer.size()), "Trying to rewrite float value at invalid buffer byte offset.");
   encode_bytes((uint8_t*)&value, 4, at);
}

float kehEncDecBuffer::read_float()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), 0.0f, "Trying to decode float but reading index has moved past last byte in the buffer.");
   float ret = 0.0f;
   decode_bytes(4, (uint8_t*)&ret);

   return ret;
}


void kehEncDecBuffer::write_vector2(const Vector2& value)
{
   float ptr[2];
   ptr[0] = (float)value.x;
   ptr[1] = (float)value.y;
   append_bytes((const uint8_t*)ptr, 8);
}

void kehEncDecBuffer::rewrite_vector2(const Vector2& value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 8 >= m_buffer.size()), "Trying to rewrite Vector2 at invalid buffer byte offset.");
   float ptr[2];
   ptr[0] = (float)value.x;
   ptr[1] = (float)value.y;
   encode_bytes((uint8_t*)ptr, 8, at);
}

Vector2 kehEncDecBuffer::read_vector2()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), Vector2(), "Trying to decode Vector2 but reading index has moved past last byte in the buffer.");
   float aux[2];
   decode_bytes(8, (uint8_t*)aux);

   return Vector2(aux[0], aux[1]);
}


void kehEncDecBuffer::write_rect2(const Rect2& value)
{
   float ptr[4];
   ptr[0] = (float)value.position.x;
   ptr[1] = (float)value.position.y;
   ptr[2] = (float)value.size.x;
   ptr[3] = (float)value.size.y;
   append_bytes((const uint8_t*)ptr, 16);
}

void kehEncDecBuffer::rewrite_rect2(const Rect2& value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 16 >= m_buffer.size()), "Trying to rewrite Rect2 at invalid buffer byte offset.");
   float ptr[4];
   ptr[0] = (float)value.position.x;
   ptr[1] = (float)value.position.y;
   ptr[2] = (float)value.size.x;
   ptr[3] = (float)value.size.y;
   encode_bytes((uint8_t*)ptr, 16, at);
}

Rect2 kehEncDecBuffer::read_rect2()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), Rect2(), "Trying to decode Rect2 but reading index has moved past last byte in the buffer.");
   float aux[4];
   decode_bytes(16, (uint8_t*)aux);

   return Rect2(aux[0], aux[1], aux[2], aux[3]);
}


void kehEncDecBuffer::write_vector3(const Vector3& value)
{
   float ptr[3];
   ptr[0] = (float)value.x;
   ptr[1] = (float)value.y;
   ptr[2] = (float)value.z;
   append_bytes((const uint8_t*)ptr, 12);
}

void kehEncDecBuffer::rewrite_vector3(const Vector3& value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 12 >= m_buffer.size()), "Trying to rewrite Vector3 at invalid buffer byte offset.");
   float ptr[3];
   ptr[0] = (float)value.x;
   ptr[1] = (float)value.y;
   ptr[2] = (float)value.z;
   encode_bytes((uint8_t*)ptr, 12, at);
}

Vector3 kehEncDecBuffer::read_vector3()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), Vector3(), "Trying to decode Vector3 but reading index has moved past last byte in the buffer.");
   float aux[3];
   decode_bytes(12, (uint8_t*)aux);

   return Vector3(aux[0], aux[1], aux[3]);
}


void kehEncDecBuffer::write_quat(const Quat& value)
{
   float ptr[4];
   ptr[0] = (float)value.x;
   ptr[1] = (float)value.y;
   ptr[2] = (float)value.z;
   ptr[3] = (float)value.w;
   append_bytes((const uint8_t*)ptr, 16);
}

void kehEncDecBuffer::rewrite_quat(const Quat& value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 16 >= m_buffer.size()), "Trying to rewrite Quat at invalid buffer byte offset.");
   float ptr[4];
   ptr[0] = (float)value.x;
   ptr[1] = (float)value.y;
   ptr[2] = (float)value.z;
   ptr[3] = (float)value.w;
   encode_bytes((uint8_t*)ptr, 16, at);
}

Quat kehEncDecBuffer::read_quat()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), Quat(), "Trying to decode Quat but reading index has moved past last byte in the buffer.");
   float aux[4];
   decode_bytes(16, (uint8_t*)aux);

   return Quat(aux[0], aux[1], aux[2], aux[3]);
}


void kehEncDecBuffer::write_color(const Color& value)
{
   float ptr[4];
   ptr[0] = (float)value.r;
   ptr[1] = (float)value.g;
   ptr[2] = (float)value.b;
   ptr[3] = (float)value.a;
   append_bytes((const uint8_t*)ptr, 16);
}

void kehEncDecBuffer::rewrite_color(const Color& value, int at)
{
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 16 >= m_buffer.size()), "Trying to rewrite Color at invalid buffer byte offset.");
   float ptr[4];
   ptr[0] = (float)value.r;
   ptr[1] = (float)value.g;
   ptr[2] = (float)value.b;
   ptr[3] = (float)value.a;
   encode_bytes((uint8_t*)ptr, 16, at);
}

Color kehEncDecBuffer::read_color()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), Color(), "Trying to decode Color but reading index has moved past last byte in the buffer.");
   float aux[4];
   decode_bytes(16, (uint8_t*)aux);

   return Color(aux[0], aux[1], aux[2], aux[3]);
}


void kehEncDecBuffer::write_uint(uint32_t value)
{
   append_bytes((const uint8_t*)&value, 4);
}

void kehEncDecBuffer::rewrite_uint(uint32_t value, int at)
{
//   ERR_FAIL_COND_MSG(value < 0 || value > MAX_UINT, "Trying to rewrite unsigned integer, but its value is outside of the supported range.");
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 4 >= m_buffer.size()), "Trying to rewrite unsigned integer at invalid buffer byte offset.");
   encode_bytes((uint8_t*)&value, 4, at);
}

uint32_t kehEncDecBuffer::read_uint()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), 0, "Trying to decode uint but reading index has moved past last byte in the buffer.");
   uint32_t ret = 0;
   decode_bytes(4, (uint8_t*)&ret);

   return ret;
}


void kehEncDecBuffer::write_byte(int value)
{
   ERR_FAIL_COND_MSG(value < 0 || value > 0xFF, "Trying to write a byte, but its value is outside of the supported range.");
   uint8_t aux = (uint8_t)value;
   m_buffer.append(aux);
}

void kehEncDecBuffer::rewrite_byte(int value, int at)
{
   ERR_FAIL_COND_MSG(value < 0 || value > 0xFF, "Trying to rewrite a byte, but its value is outside of the supported range.");
   ERR_FAIL_COND_MSG(at >= m_buffer.size(), "Trying to rewrite a byte value at invalid buffer byte offset.");
   uint8_t aux = (uint8_t)value;
   m_buffer.set(at, aux);
}

uint8_t kehEncDecBuffer::read_byte()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), 0, "Trying to decode byte but reading index has moved past last byte in the buffer.");
   uint8_t ret = 0;
   decode_bytes(1, &ret);

   return ret;
}


void kehEncDecBuffer::write_ushort(int value)
{
   ERR_FAIL_COND_MSG(value < 0 || value > 0xFFFF, "Trying to write unsigned short integer, but its value is outside of the supported range.");
   uint16_t aux = (uint16_t)value;
   append_bytes((const uint8_t*)&aux, 2);
}

void kehEncDecBuffer::rewrite_ushort(int value, int at)
{
   ERR_FAIL_COND_MSG(value < 0 || value > 0xFFFF, "Trying to rewrite unsigned short integer, but its value is outside of the supported range.");
   ERR_FAIL_COND_MSG((at >= m_buffer.size()) || (at + 2 >= m_buffer.size()), "Trying to rewrite unsigned short integer at invalid buffer byte offset.");
   uint16_t aux = (uint16_t)value;
   encode_bytes((uint8_t*)&aux, 2, at);
}

uint16_t kehEncDecBuffer::read_ushort()
{
   ERR_FAIL_COND_V_MSG(m_rindex >= m_buffer.size(), 0, "Trying to decode ushort but reading index has moved past last byte in the buffer.");
   uint16_t ret = 0;
   decode_bytes(2, (uint8_t*)&ret);

   return ret;
}


void kehEncDecBuffer::write_string(const String& value)
{
   // Ensure UTF8 encoding
   CharString utf8 = value.utf8();
   // Apparently the size includes a trailing "0"
   const uint32_t size = utf8.size() - 1;
   // Write the size of the raw data
   write_uint(size);
   // Then append the bytes
   append_bytes((const uint8_t*)utf8.get_data(), size);
}

String kehEncDecBuffer::read_string()
{
   // Read amount of bytes
   const uint32_t size = read_uint();
   // Create the buffer
   CharString utf;
   // While reincorporating the trailing "0"
   utf.resize(size + 1);
   // Decode the string from the internal array bytes
   decode_bytes(size, (uint8_t*)&utf[0]);
   utf[size] = 0;

   return String(utf);
}



/// Setters/Getters

void kehEncDecBuffer::set_buffer(const PoolByteArray& b)
{
   m_buffer = b;
   m_rindex = 0;
}



/// Private and protected sections
void kehEncDecBuffer::append_bytes(const uint8_t* in, const uint32_t count)
{
   int csize = m_buffer.size();
   m_buffer.resize(csize + count);
   encode_bytes(in, count, csize);
}

void kehEncDecBuffer::encode_bytes(const uint8_t* ptr, const uint32_t count, const int offset)
{
   PoolVector<uint8_t>::Write w = m_buffer.write();
   uint8_t* out = w.ptr() + offset;

   for (uint32_t i = 0; i < count; i++)
   {
      *out = *ptr;
      ptr++;
      out++;
   }
}

void kehEncDecBuffer::decode_bytes(const int count, uint8_t* output)
{
   PoolVector<uint8_t>::Read r = m_buffer.read();
   const uint8_t* in = r.ptr() + m_rindex;

   for (uint32_t i = 0; i < count; i++)
   {
      *output = *in;
      in++;
      output++;
   }

   m_rindex += count;
}


void kehEncDecBuffer::_bind_methods()
{
   ClassDB::bind_method(D_METHOD("has_read_data"), &kehEncDecBuffer::has_read_data);
   ClassDB::bind_method(D_METHOD("get_current_size"), &kehEncDecBuffer::get_current_size);

   ClassDB::bind_method(D_METHOD("write_bool", "value"), &kehEncDecBuffer::write_bool);
   ClassDB::bind_method(D_METHOD("rewrite_bool", "value", "offset"), &kehEncDecBuffer::rewrite_bool);
   ClassDB::bind_method(D_METHOD("read_bool"), &kehEncDecBuffer::read_bool);

   ClassDB::bind_method(D_METHOD("write_int", "value"), &kehEncDecBuffer::write_int);
   ClassDB::bind_method(D_METHOD("rewrite_int", "value", "offset"), &kehEncDecBuffer::rewrite_int);
   ClassDB::bind_method(D_METHOD("read_int"), &kehEncDecBuffer::read_int);

   ClassDB::bind_method(D_METHOD("write_float", "value"), &kehEncDecBuffer::write_float);
   ClassDB::bind_method(D_METHOD("rewrite_float", "value", "offset"), &kehEncDecBuffer::rewrite_float);
   ClassDB::bind_method(D_METHOD("read_float"), &kehEncDecBuffer::read_float);

   ClassDB::bind_method(D_METHOD("write_vector2", "value"), &kehEncDecBuffer::write_vector2);
   ClassDB::bind_method(D_METHOD("rewrite_vector2", "value", "offset"), &kehEncDecBuffer::rewrite_vector2);
   ClassDB::bind_method(D_METHOD("read_vector2"), &kehEncDecBuffer::read_vector2);

   ClassDB::bind_method(D_METHOD("write_rect2", "value"), &kehEncDecBuffer::write_rect2);
   ClassDB::bind_method(D_METHOD("rewrite_rect2", "value", "offset"), &kehEncDecBuffer::rewrite_rect2);
   ClassDB::bind_method(D_METHOD("read_rect2"), &kehEncDecBuffer::read_rect2);

   ClassDB::bind_method(D_METHOD("write_vector3", "value"), &kehEncDecBuffer::write_vector3);
   ClassDB::bind_method(D_METHOD("rewrite_vector3", "value", "offset"), &kehEncDecBuffer::rewrite_vector3);
   ClassDB::bind_method(D_METHOD("read_vector3"), &kehEncDecBuffer::read_vector3);

   ClassDB::bind_method(D_METHOD("write_quat", "value"), &kehEncDecBuffer::write_quat);
   ClassDB::bind_method(D_METHOD("rewrite_quat", "value", "offset"), &kehEncDecBuffer::rewrite_quat);
   ClassDB::bind_method(D_METHOD("read_quat"), &kehEncDecBuffer::read_quat);

   ClassDB::bind_method(D_METHOD("write_color", "value"), &kehEncDecBuffer::write_color);
   ClassDB::bind_method(D_METHOD("rewrite_color", "value", "offset"), &kehEncDecBuffer::rewrite_color);
   ClassDB::bind_method(D_METHOD("read_color"), &kehEncDecBuffer::read_color);

   ClassDB::bind_method(D_METHOD("write_uint", "value"), &kehEncDecBuffer::write_uint);
   ClassDB::bind_method(D_METHOD("rewrite_uint", "value", "offset"), &kehEncDecBuffer::rewrite_uint);
   ClassDB::bind_method(D_METHOD("read_uint"), &kehEncDecBuffer::read_uint);

   ClassDB::bind_method(D_METHOD("write_byte", "value"), &kehEncDecBuffer::write_byte);
   ClassDB::bind_method(D_METHOD("rewrite_byte", "value", "offset"), &kehEncDecBuffer::rewrite_byte);
   ClassDB::bind_method(D_METHOD("read_byte"), &kehEncDecBuffer::read_byte);

   ClassDB::bind_method(D_METHOD("write_ushort", "value"), &kehEncDecBuffer::write_ushort);
   ClassDB::bind_method(D_METHOD("rewrite_ushort", "value", "offset"), &kehEncDecBuffer::rewrite_ushort);
   ClassDB::bind_method(D_METHOD("read_ushort"), &kehEncDecBuffer::read_ushort);

   ClassDB::bind_method(D_METHOD("write_string", "value"), &kehEncDecBuffer::write_string);
   ClassDB::bind_method(D_METHOD("read_string"), &kehEncDecBuffer::read_string);


   ClassDB::bind_method(D_METHOD("set_buffer", "buffer"), &kehEncDecBuffer::set_buffer);
   ClassDB::bind_method(D_METHOD("get_buffer"), &kehEncDecBuffer::get_buffer);


   ADD_PROPERTY(PropertyInfo(Variant::POOL_BYTE_ARRAY, "buffer"), "set_buffer", "get_buffer");
}


/// Constructor and destructor
kehEncDecBuffer::kehEncDecBuffer()
{
   m_rindex = 0;
}

