/*
  Copyright (C) 2014, 2015 Red Hat, Inc.

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

#include "libzwerg.h"
#include "libzwerg-dw.h"

#include <string>
#include <memory>

#include "tree.hh"

struct vocabulary;

struct zw_error
{
  std::string m_message;
};

struct zw_vocabulary
{
  std::unique_ptr <vocabulary> m_voc;
};

struct zw_query
{
  tree m_query;
};

struct zw_result
{
  std::shared_ptr <op> m_op;
};

struct zw_value
{
  std::shared_ptr <value> m_value;

  zw_value (std::unique_ptr <value> value)
    : m_value {std::move (value)}
  {}
};

struct zw_stack
{
  std::vector <std::unique_ptr <zw_value>> m_values;
};
