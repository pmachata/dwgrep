// <utility> -*- C++ -*-

// Copyright (C) 2001-2014 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

// N.B.: This was taken from GCC 4.9.

#include <utility>

#ifndef STD_UTILITY_HH
#define STD_UTILITY_HH

#if __cplusplus <= 201103L
namespace std
{
  /// Class template integer_sequence
  template<typename _Tp, _Tp... _Idx>
    struct integer_sequence
    {
      typedef _Tp value_type;
      static constexpr size_t size() { return sizeof...(_Idx); }
    };

  template<typename _Tp, _Tp _Num,
	   typename _ISeq = typename _Build_index_tuple<_Num>::__type>
    struct _Make_integer_sequence;

  template<typename _Tp, _Tp _Num,  size_t... _Idx>
    struct _Make_integer_sequence<_Tp, _Num, _Index_tuple<_Idx...>>
    {
      static_assert( _Num >= 0,
		     "Cannot make integer sequence of negative length" );

      typedef integer_sequence<_Tp, static_cast<_Tp>(_Idx)...> __type;
    };

  /// Alias template make_integer_sequence
  template<typename _Tp, _Tp _Num>
    using make_integer_sequence
      = typename _Make_integer_sequence<_Tp, _Num>::__type;

  /// Alias template index_sequence
  template<size_t... _Idx>
    using index_sequence = integer_sequence<size_t, _Idx...>;

  /// Alias template make_index_sequence
  template<size_t _Num>
    using make_index_sequence = make_integer_sequence<size_t, _Num>;

  /// Alias template index_sequence_for
  template<typename... _Types>
    using index_sequence_for = make_index_sequence<sizeof...(_Types)>;

} // namespace
#endif

#endif//STD_UTILITY_HH
