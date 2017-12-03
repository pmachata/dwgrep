/*
   Copyright (C) 2017 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#ifndef _VALUE_CLOSURE_H_
#define _VALUE_CLOSURE_H_

#include "op.hh"
#include "value.hh"
#include "layout.hh"

class value_closure
  : public value
{
  layout m_op_layout;
  layout::loc m_rdv_ll;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  std::vector <std::unique_ptr <value>> m_env;

public:
  static value_type const vtype;

  value_closure (layout op_layout, layout::loc rdv_ll,
		 std::shared_ptr <op_origin> origin,
		 std::shared_ptr <op> op,
		 std::vector <std::unique_ptr <value>> env,
		 size_t pos);

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;

  layout const &get_layout () const
  { return m_op_layout; }

  op_origin &get_origin () const
  { return *m_origin; }

  op &get_op () const
  { return *m_op; }

  value &get_env (unsigned id)
  { return *m_env[id]; }

  layout::loc get_rdv_ll () const
  { return m_rdv_ll; }
};

#endif /* _VALUE_CLOSURE_H_ */
