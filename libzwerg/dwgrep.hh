/*
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of dwgrep.

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

#ifndef _DWGREP_H_
#define _DWGREP_H_

#include <memory>
#include <vector>

struct vocabulary;
std::unique_ptr <vocabulary> dwgrep_vocabulary_core ();

class dwgrep_expr
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  class result;

  explicit dwgrep_expr (std::string const &str);
  ~dwgrep_expr ();

  result query ();
};

class dwgrep_expr::result
{
  friend class dwgrep_expr;
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

  explicit result (std::unique_ptr <pimpl> p);

public:
  ~result ();
  result (result &&that);
  result (result const &that) = delete;

  class iterator;
  iterator begin ();
  iterator end ();
};

class dwgrep_expr::result::iterator
{
  friend class dwgrep_expr::result;
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

  iterator (std::unique_ptr <pimpl> p);

public:
  iterator ();
  iterator (iterator const &other);
  ~iterator ();

  std::vector <int> operator* () const;

  iterator operator++ ();
  iterator operator++ (int);

  bool operator== (iterator that);
  bool operator!= (iterator that);
};

#endif /* _DWGREP_H_ */
