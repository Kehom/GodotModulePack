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

#ifndef _KEHNETWORK_PROPCOMPARER_H
#define _KEHNETWORK_PROPCOMPARER_H 1

#include "core/reference.h"

// This class basically hides the actual comparer, which require templates. It also
// sort of manage the various instances of the internal comparers in a way to avoid
// the instantiation of multiple ones that are meant to do the exactly same task.
class kehPropComparer
{
public:
   struct ComparerProxy : public Reference
   {
      virtual bool compare(const Variant& var1, const Variant& var2) const = 0;
      virtual String get_comp_name() const { return ""; }
   };


private:
   // The actual comparer. See top of propcomparer.cpp for implementations of the inner class.
   Ref<ComparerProxy> m_comparer;

   // To avoid creating multiple instances of the comparers that are meant to do the
   // exact same comparison type, this map will hold the created instances based on
   // a "name".
   static Map<String, Ref<ComparerProxy> > s_comp_collection;

   // Based on a few settings, build the name of comparer that uses tolerance, either custom or automatic
   String get_comp_name(const String& prefix, float tol) const;


public:

   bool is_valid() const { return m_comparer.is_valid(); }

   inline bool operator()(const Variant& var1, const Variant& var2) const { return m_comparer->compare(var1, var2); }

   inline String get_comparer_name() const { return m_comparer->get_comp_name(); }

   // tolerance will be ignored if approx is false.
   int init(Variant::Type type, bool has_meta, const Variant& metaval);

   // Because the comaparer are held in a static container, it's necessary to manually perform
   // some sort of cleanup. This function is meant to perform that
   static void cleanup();

};


#endif

