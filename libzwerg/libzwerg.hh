/*
  Copyright (C) 2015 Red Hat, Inc.

  This file is free software; you can redistribute it and/or modify
  it under the terms of either

  * the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 3 of the License, or (at
  your option) any later version

  or

  * the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at
  your option) any later version

  or both in parallel, as here.

  dwgrep is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received copies of the GNU General Public License and
  the GNU Lesser General Public License along with this program.  If
  not, see <http://www.gnu.org/licenses/>.  */

#ifndef LIBZWERG_HH_
#define LIBZWERG_HH_

#include <libzwerg.h>
#include <memory>

struct zw_deleter
{
  void
  operator() (zw_value *value)
  {
    zw_value_destroy (value);
  }

  void
  operator() (zw_vocabulary *voc)
  {
    zw_vocabulary_destroy (voc);
  }

  void
  operator() (zw_query *q)
  {
    zw_query_destroy (q);
  }

  void
  operator() (zw_stack *stk)
  {
    zw_stack_destroy (stk);
  }

  void
  operator() (zw_result *res)
  {
    zw_result_destroy (res);
  }
};

struct zw_throw_on_error
{
  zw_error *m_err;

  zw_throw_on_error ()
    : m_err {nullptr}
  {}

  operator zw_error ** ()
  {
    return &m_err;
  }

  ~zw_throw_on_error ()
  {
    if (m_err != nullptr)
      throw std::runtime_error (zw_error_message (m_err));
  }
};

#endif
