/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _Iterator_h_
#define _Iterator_h_ 1

#include "util/template.h"

/** @addtogroup kernelutilities
 * @{ */

/** General iterator for structures that provide functions for the next and previous structure
 *  in the datastructure and a "value" member. This template provides a bidirectional, a constant
 *  bidirectional, a reverse bidirectional and a constant reverse bidirectional iterator.
 *\brief An iterator applicable for many data structures
 *\param[in] originalT the original element type of the iterator
 *\param[in] Struct the datastructure that provides functions for the next/previous datastructure
 *                  and a "value" member
 *\param[in] previous pointer to the member function used to iterate forward
 *\param[in] next pointer to the member function used to iterate backwards
 *\param[in] T the real element type of the iterator */
template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)() = &Struct::previous,
         Struct *(Struct::*FunctionNext)() = &Struct::next,
         typename T = originalT>
class Iterator
{
  /** All iterators must be friend in order to allow casts between some iterator types */
  template<typename _originalT,
           class _Struct,
           _Struct *(_Struct::*_FunctionPrev)(),
           _Struct *(_Struct::*_FunctionNext)(),
           typename _T>
  friend class Iterator;

  /** The assignment operator is extern */
  template<typename _originalT,
           class _Struct,
           _Struct *(_Struct::*_FunctionPrev)(),
           _Struct *(_Struct::*_FunctionNext)(),
           typename _T1,
           typename _T2>
  friend bool operator == (const Iterator<_originalT, _Struct, _FunctionPrev, _FunctionNext, _T1> &x1,
                           const Iterator<_originalT, _Struct, _FunctionPrev, _FunctionNext, _T2> &x2);

public:
  /** Type of the constant bidirectional iterator */
  typedef Iterator<originalT, Struct, FunctionPrev, FunctionNext, T const>     Const;
  /** Type of the reverse iterator */
  typedef Iterator<originalT, Struct, FunctionNext, FunctionPrev, T>           Reverse;
  /** Type of the constant reverse iterator */
  typedef Iterator<originalT, Struct, FunctionNext, FunctionPrev, T const>     ConstReverse;

  /** The default constructor constructs an invalid/unusable iterator */
  Iterator()
    : m_Node() {}
  /** The copy-constructor
   *\param[in] Iterator the reference object */
  Iterator(const Iterator &x)
    : m_Node(x.m_Node) {}
  /** The constructor
   *\param[in] Iterator the reference object */
  template<typename T2>
  Iterator(const Iterator<originalT, Struct, FunctionPrev, FunctionNext, T2> &x)
    : m_Node(x.m_Node) {}
  /** Constructor from a pointer to an instance of the data structure
   *\param[in] Node pointer to an instance of the data structure */
  Iterator(Struct *Node)
    : m_Node(Node) {}
  /** The destructor does nothing */
  ~Iterator() {}

  /** The assignment operator
   *\param[in] Iterator the reference object */
  Iterator &operator = (const Iterator &x) {
    m_Node = x.m_Node;
    return *this;
  }
  /** Preincrement operator */
  Iterator &operator ++ () {
    m_Node = (m_Node->*FunctionNext)();
    return *this;
  }
  /** Predecrement operator */
  Iterator &operator -- () {
    m_Node = (m_Node->*FunctionPrev)();
    return *this;
  }
  /** Dereference operator yields the element value */
  T &operator *() {
    // Verify that we actually have a valid node
    if(m_Node)
      return m_Node->value;
    else {
      static T ret = 0;
      return ret;
    }
  }
  /** Dereference operator yields the element value */
  T &operator ->() {
    return m_Node->value;
  }

  /** Conversion Operator to a constant iterator */
  operator Const () {
    return Const(m_Node);
  }

  /** Get the Node */
  Struct *__getNode() {
    return m_Node;
  }

protected:
  /** Pointer to the instance of the data structure or 0 */
  Struct *m_Node;
};

/** General iterator for structures that provide functions for the next and previous structure
 *  in the datastructure and a "value" member. This template provides a bidirectional, a constant
 *  bidirectional, a reverse bidirectional and a constant reverse bidirectional iterator.
 *  This specific Tree-style implementation provides access to a key and value stored in the Struct
 *\brief An iterator applicable for many data structures
 *\param[in] originalT the original element type of the iterator
 *\param[in] Struct the datastructure that provides functions for the next/previous datastructure
 *                  and a "value" member
 *\param[in] previous pointer to the member function used to iterate forward
 *\param[in] next pointer to the member function used to iterate backwards
 *\param[in] T the real element type of the iterator */
template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)() = &Struct::previous,
         Struct *(Struct::*FunctionNext)() = &Struct::next,
         typename K = originalT,
         typename T = originalT>
class TreeIterator
{
  /** All iterators must be friend in order to allow casts between some iterator types */
  template<typename _originalT,
           class _Struct,
           _Struct *(_Struct::*_FunctionPrev)(),
           _Struct *(_Struct::*_FunctionNext)(),
           typename _K,
           typename _T>
  friend class TreeIterator;

  /** The assignment operator is extern */
  template<typename _originalT,
           class _Struct,
           _Struct *(_Struct::*_FunctionPrev)(),
           _Struct *(_Struct::*_FunctionNext)(),
           typename _K1,
           typename _T1,
           typename _K2,
           typename _T2>
  friend bool operator == (const TreeIterator<_originalT, _Struct, _FunctionPrev, _FunctionNext, _K1, _T1> &x1,
                           const TreeIterator<_originalT, _Struct, _FunctionPrev, _FunctionNext, _K2, _T2> &x2);

public:
  /** Type of the constant bidirectional iterator */
  typedef TreeIterator<originalT, Struct, FunctionPrev, FunctionNext, K, T const>     Const;
  /** Type of the reverse iterator */
  typedef TreeIterator<originalT, Struct, FunctionNext, FunctionPrev, K, T>           Reverse;
  /** Type of the constant reverse iterator */
  typedef TreeIterator<originalT, Struct, FunctionNext, FunctionPrev, K, T const>     ConstReverse;

  /** The default constructor constructs an invalid/unusable iterator */
  TreeIterator()
    : m_Node() {}
  /** The copy-constructor
   *\param[in] TreeIterator the reference object */
  TreeIterator(const TreeIterator &x)
    : m_Node(x.m_Node) {}
  /** The constructor
   *\param[in] TreeIterator the reference object */
  template<typename K2, typename T2>
  TreeIterator(const TreeIterator<originalT, Struct, FunctionPrev, FunctionNext, K2, T2> &x)
    : m_Node(x.m_Node) {}
  /** Constructor from a pointer to an instance of the data structure
   *\param[in] Node pointer to an instance of the data structure */
  TreeIterator(Struct *Node)
    : m_Node(Node) {}
  /** The destructor does nothing */
  ~TreeIterator() {}

  /** The assignment operator
   *\param[in] TreeIterator the reference object */
  TreeIterator &operator = (const TreeIterator &x) {
    m_Node = x.m_Node;
    return *this;
  }
  /** Preincrement operator */
  TreeIterator &operator ++ () {
    m_Node = (m_Node->*FunctionNext)();
    return *this;
  }
  /** Predecrement operator */
  TreeIterator &operator -- () {
    m_Node = (m_Node->*FunctionPrev)();
    return *this;
  }
  /** Dereference operator yields the element value */
  T operator *() {
    return m_Node->value;
  }
  /** Dereference operator yields the element value */
  T operator ->() {
    return m_Node->value;
  }

  /** Conversion Operator to a constant iterator */
  operator Const () {
    return Const(m_Node);
  }

  /** Get the Node */
  Struct *__getNode() {
    return m_Node;
  }

  K key() {
    if(m_Node) {
      if(m_Node->value)
        return m_Node->value->key;
    }
    return 0;
  }

  T value() {
    if(m_Node) {
      if(m_Node->value)
        return m_Node->value->element;
    }
    return 0;
  }

protected:
  /** Pointer to the instance of the data structure or 0 */
  Struct *m_Node;
};

/** Comparison operator for the Iterator class
 *\param[in] x1 the first operand
 *\param[in] x2 the second operand
 *\return true, if the two iterator point to the same object, false otherwise */
template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)(),
         Struct *(Struct::*FunctionNext)(),
         typename T1,
         typename T2>
bool operator == (const Iterator<originalT, Struct, FunctionPrev, FunctionNext, T1> &x1,
                  const Iterator<originalT, Struct, FunctionPrev, FunctionNext, T2> &x2)
{
  if (x1.m_Node != x2.m_Node)return false;
  return true;
}

/** Comparison operator for the TreeIterator class
 *\param[in] x1 the first operand
 *\param[in] x2 the second operand
 *\return true, if the two iterator point to the same object, false otherwise */
template<typename originalT,
         class Struct,
         Struct *(Struct::*FunctionPrev)(),
         Struct *(Struct::*FunctionNext)(),
         typename K1,
         typename T1,
         typename K2,
         typename T2>
bool operator == (const TreeIterator<originalT, Struct, FunctionPrev, FunctionNext, K1, T1> &x1,
                  const TreeIterator<originalT, Struct, FunctionPrev, FunctionNext, K2, T2> &x2)
{
  if(!x2.m_Node) {
    if(!x1.m_Node)
      return false;
    else if(x1.m_Node->value)
      return false;
    return true;
  }
  return (x1.m_Node->value == x2.m_Node->value);
}

/** @} */

#endif /* _Iterator_h_ */
