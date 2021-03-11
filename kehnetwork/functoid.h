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

#ifndef _KEHNETWORK_FUNCTOID_H
#define _KEHNETWORK_FUNCTOID_H 1

// Well, this could easily be called functor instead, but this is actually very
// very simplified to take Godot Object as the "this".
// The idea here is to use function pointers to avoid having to place signals
// all over the place and instead of the FuncRef given by Godot.
// Yes, I know there is some hacky things here...

#include "core/object.h"

template <typename>
class kehFunctoid;

// Functions without argument
template <class RT>
class kehFunctoid<RT()>
{
public:
   typedef RT(Object::*func_t)();
private:
   Object* m_obj;
   func_t m_func;

public:
   kehFunctoid() : m_obj(0), m_func(0) {}
   kehFunctoid(const kehFunctoid& f) : m_obj(f.m_obj), m_func(f.m_func) {}
   template <class C>
   kehFunctoid(C* o, RT (C::*f)()) { set(o, f); }

   bool is_valid() const { return (m_obj != NULL && m_func != NULL); }
   inline RT operator()() { return (m_obj->*m_func)(); }

   template <class C>
   void set(C* o, RT (C::*f)())
   {
      // Yes, those assigns are hacky, but they work.
      m_obj = (Object*)o;
      m_func = (func_t)f;
   }
};

// Functions with 1 argument
template <class RT, typename T1>
class kehFunctoid<RT(T1)>
{
public:
   typedef RT (Object::*func_t)(T1);
private:
   Object* m_obj;
   func_t m_func;

public:
   kehFunctoid() : m_obj(0), m_func(0) {}
   kehFunctoid(const kehFunctoid& f) : m_obj(f.m_obj), m_func(f.m_func) {}
   template <class C>
   kehFunctoid(C* o, RT (C::*f)(T1)) { set(o, f); }

   bool is_valid() const { return (m_obj != NULL && m_func != NULL); }
   inline RT operator()(T1 a1) { return (m_obj->*m_func)(a1); }
   
   template <class C>
   void set(C* o, RT (C::*f)(T1))
   {
      m_obj = (Object*)o;
      m_func = (func_t)f;
   }
};

// Functions with 2 arguments
template <class RT, typename T1, typename T2>
class kehFunctoid<RT(T1, T2)>
{
public:
   typedef RT (Object::*func_t)(T1, T2);
private:
   Object* m_obj;
   func_t m_func;

public:
   kehFunctoid() : m_obj(0), m_func(0) {}
   kehFunctoid(const kehFunctoid& f) : m_obj(f.m_obj), m_func(f.m_func) {}
   template <class C>
   kehFunctoid(C* o, RT (C::*f)(T1, T2)) { set(o, f); }

   bool is_valid() const { return (m_obj != NULL && m_func != NULL); }
   inline RT operator()(T1 a1, T2 a2) { return (m_obj->*m_func)(a1, a2); }

   template <class C>
   void set(C* o, RT (C::*f)(T1, T2))
   {
      m_obj = (Object*)o;
      m_func = (func_t)f;
   }
};


// Functions with 3 arguments
template <class RT, typename T1, typename T2, typename T3>
class kehFunctoid<RT(T1, T2, T3)>
{
public:
   typedef RT (Object::*func_t)(T1, T2, T3);
private:
   Object* m_obj;
   func_t m_func;

public:
   kehFunctoid() : m_obj(0), m_func() {}
   kehFunctoid(const kehFunctoid& f) : m_obj(f.m_obj), m_func(f.m_func) {}
   template <class C>
   kehFunctoid(C* o, RT (C::*f)(T1, T2, T3)) { set(o, f); }

   bool is_valid() const { return (m_obj != NULL && m_func != NULL); }
   inline RT operator()(T1 a1, T2 a2, T3 a3) { return (m_obj->*m_func)(a1, a2, a3); }


   template <class C>
   void set(C* o, RT (C::*f)(T1, T2, T3))
   {
      m_obj = (Object*)o;
      m_func = (func_t)f;
   }
};


#endif
