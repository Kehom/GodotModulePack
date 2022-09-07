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

#include "propcomparer.h"
#include "snapentity.h"

typedef kehPropComparer::ComparerProxy CProxy;

Map<String, Ref<CProxy> > kehPropComparer::s_comp_collection;


// This is the "generic" comparer, which will directly use th given Variants
template <typename T, bool approx, bool custom>
struct TheComparer : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const { return var1 == var2; }
   String get_comp_name() const { return "generic"; }

   // Using memnew fails when using templates. So, making a "self factory" to avoid using
   // template arguments inside the memnew macro
   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Floating point comparer using automatic tolerance calculation, with Math::is_equal_approx()
template <>
struct TheComparer<float, true, false> : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const { return Math::is_equal_approx(float(var1), float(var2)); }
   String get_comp_name() const { return "float_auto"; }

   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Floating point comparer with custom tolerance value
template <>
struct TheComparer<float, true, true> : public CProxy
{
   TheComparer(float t) : m_tolerance(t) {}
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const float v1 = var1;
      const float v2 = var2;
      return (Math::abs(v1 - v2) < m_tolerance);
   }
   String get_comp_name() const { return vformat("float_custom_%f", m_tolerance); }

   static Ref<TheComparer> create(float t) { return memnew(TheComparer(t)); }

private:
   float m_tolerance;
};

// Vector2 with automatic tolerance, using Vector2's method to perform is_equal_approx() on each component
template <>
struct TheComparer<Vector2, true, false> : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Vector2 v1(var1);
      return v1.is_equal_approx(var2);
   }
   String get_comp_name() const { return "vector2_auto"; }

   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Vector2 with custom tolerance value
template <>
struct TheComparer<Vector2, true, true> : public CProxy
{
   TheComparer(float t) : m_tolerance(t) {}
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Vector2 v1(var1);
      const Vector2 v2(var2);
      return (Math::abs(v1.x - v2.x) < m_tolerance && Math::abs(v1.y - v2.y) < m_tolerance);
   }
   String get_comp_name() const { return vformat("vector2_custom_%f", m_tolerance); }

   static Ref<TheComparer> create(float t) { return memnew(TheComparer(t)); }

private:
   float m_tolerance;
};


// Rect2 with automatic tolerance. Again, the class offers a method to use is_equal_approx() on each component
template <>
struct TheComparer<Rect2, true, false> : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Rect2 v1(var1);
      return v1.is_equal_approx(var2);
   }

   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Rect2 with custom tolerance value
template <>
struct TheComparer<Rect2, true, true> : public CProxy
{
   TheComparer(float t) : m_tolerance(t) {}

   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Rect2 v1(var1);
      const Rect2 v2(var2);
      return (Math::abs(v1.position.x - v2.position.x) < m_tolerance &&
            Math::abs(v1.position.y - v2.position.y) < m_tolerance &&
            Math::abs(v1.size.x - v2.size.x) < m_tolerance &&
            Math::abs(v1.size.y - v2.size.y) < m_tolerance);
      return false;
   }

   static Ref<TheComparer> create(float t) { return memnew(TheComparer(t)); }
private:
   float m_tolerance;
};

// Quat with automatic tolerance value
template <>
struct TheComparer<Quat, true, false> : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Quat v1(var1);
      return v1.is_equal_approx(var2);
   }

   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Quat with custom tolerance value
template <>
struct TheComparer<Quat, true, true> : public CProxy
{
   TheComparer(float t): m_tolerance(t) {}
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Quat v1(var1);
      const Quat v2(var2);

      return (Math::abs(v1.x - v2.x) < m_tolerance &&
         Math::abs(v1.y - v2.y) < m_tolerance &&
         Math::abs(v1.z - v2.z) < m_tolerance &&
         Math::abs(v1.w - v2.w) < m_tolerance);
   }

   static Ref<TheComparer> create(float t) { return memnew(TheComparer(t)); }

private:
   float m_tolerance;
};

// Vector3 with automatic tolerance
template <>
struct TheComparer<Vector3, true, false> : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Vector3 v1(var1);
      return v1.is_equal_approx(var2);
   }

   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Vector3 with custom tolerance
template <>
struct TheComparer<Vector3, true, true> : public CProxy
{
   TheComparer(float t) : m_tolerance(t) {}
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Vector3 v1(var1);
      const Vector3 v2(var2);

      return (Math::abs(v1.x - v2.x) < m_tolerance &&
         Math::abs(v1.y - v2.y) < m_tolerance &&
         Math::abs(v1.z - v2.z) < m_tolerance);
   }

   static Ref<TheComparer> create(float t) { return memnew(TheComparer(t)); }

private:
   float m_tolerance;
};

// Color with automatic tolerance
template <>
struct TheComparer<Color, true, false> : public CProxy
{
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Color v1(var1);
      return v1.is_equal_approx(var2);
   }

   static Ref<TheComparer> create() { return memnew(TheComparer); }
};

// Color with custom tolerance
template <>
struct TheComparer<Color, true, true> : public CProxy
{
   TheComparer(float t) : m_tolerance(t) {}
   bool compare(const Variant& var1, const Variant& var2) const
   {
      const Color v1(var1);
      const Color v2(var2);

      return (Math::abs(v1.r - v2.r) < m_tolerance &&
         Math::abs(v1.g - v2.g) < m_tolerance &&
         Math::abs(v1.b - v2.b) < m_tolerance &&
         Math::abs(v1.a - v2.a) < m_tolerance);
   }

   static Ref<TheComparer> create(float t) { return memnew(TheComparer(t)); }

private:
   float m_tolerance;
};


// This is exclusively to get/create comparers that use tolerance
template <typename T>
static Ref<CProxy> get_or_create(const String& name, float tol, Map<String, Ref<CProxy>>& collection)
{
   Ref<CProxy> ret;

   const Map<String, Ref<CProxy>>::Element* e = collection.find(name);
   if (e)
   {
      ret = e->value();
   }
   else
   {
      if (tol != 0.0f)
         ret = TheComparer<T, true, true>::create(tol);
      else
         ret = TheComparer<T, true, false>::create();
      
      collection[name] = ret;
   }

   return ret;
}


int kehPropComparer::init(Variant::Type type, bool has_meta, const Variant& metaval)
{
   // Assume the property type is supported.
   int ret = (int)type;
   // Assume a generic comparer will be used.
   bool use_generic = true;

   switch (type)
   {
      case Variant::INT:
      {
         if (has_meta)
         {
            const int mval = metaval;
            switch (mval)
            {
               case kehSnapEntityBase::CTYPE_UINT:
               case kehSnapEntityBase::CTYPE_BYTE:
               case kehSnapEntityBase::CTYPE_USHORT:
                  ret = mval;
            }
         }


      } break;

      case Variant::REAL:
      {
         if (has_meta)
         {
            use_generic = false;
            const float tol = metaval;
            const String compname = get_comp_name("float", tol);
            m_comparer = get_or_create<float>(compname, tol, s_comp_collection);
         }
      } break;

      case Variant::VECTOR2:
      {
         if (has_meta)
         {
            use_generic = false;
            const float tol = metaval;
            const String compname = get_comp_name("vec2", tol);
            m_comparer = get_or_create<Vector2>(compname, tol, s_comp_collection);
         }
      } break;

      case Variant::RECT2:
      {
         if (has_meta)
         {
            use_generic = false;
            const float tol = metaval;
            String compname = get_comp_name("rect2", tol);
            m_comparer = get_or_create<Rect2>(compname, tol, s_comp_collection);
         }
      } break;

      case Variant::QUAT:
      {
         if (has_meta)
         {
            use_generic = false;
            const float tol = metaval;
            const String compname = get_comp_name("quat", tol);
            m_comparer = get_or_create<Quat>(compname, tol, s_comp_collection);
         }
      } break;

      case Variant::VECTOR3:
      {
         if (has_meta)
         {
            use_generic = false;
            const float tol = metaval;
            const String compname = get_comp_name("vec3", tol);
            m_comparer = get_or_create<Vector3>(compname, tol, s_comp_collection);
         }
      } break;

      case Variant::COLOR:
      {
         if (has_meta)
         {
            use_generic = false;
            const float tol = metaval;
            const String compname = get_comp_name("color", tol);
            m_comparer = get_or_create<Color>(compname, tol, s_comp_collection);
         }
      } break;

      // Allowing falling through here because none of those types have any special case. The generic comparer is enough
      case Variant::BOOL:
      case Variant::STRING:
      case Variant::POOL_BYTE_ARRAY:
      case Variant::POOL_INT_ARRAY:
      case Variant::POOL_REAL_ARRAY:
      {
         // Nothing to do here. The necessary values are already set. The cases here are just to prevent the "default" from
         // invalidating the return value. The actual comparer will be retrieved/created after the switch.
      } break;

      default:
      {
         // If here then non supported type
         ret = (int)Variant::NIL;
      } break;
   }

   
   if (use_generic && ret != (int)Variant::NIL)
   {
      const Map<String, Ref<ComparerProxy>>::Element* e = s_comp_collection.find("generic");
      if (e)
      {
         m_comparer = e->value();
      }
      else
      {
         m_comparer = TheComparer<Variant, false, false>::create();
         s_comp_collection["generic"] = m_comparer;
      }
   }

   return ret;
}


void kehPropComparer::cleanup()
{
   // This should be enough since all internal comparers are References
   s_comp_collection.clear();
}



String kehPropComparer::get_comp_name(const String& prefix, float tol) const
{
   return tol != 0.0f ? vformat("%s_custom_%f", prefix, tol) : vformat("%s_auto", prefix);
}
