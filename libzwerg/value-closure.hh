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
#include "rendezvous.hh"
#include "value.hh"

class tree;
class frame;
class scope;

class value_closure
  : public value
{
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  std::vector <std::unique_ptr <value>> m_env;
  rendezvous m_rdv;

public:
  static value_type const vtype;

  value_closure (std::shared_ptr <op_origin> origin,
		 std::shared_ptr <op> op,
		 std::vector <std::unique_ptr <value>> env,
		 rendezvous rdv, size_t pos);

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;

  op_origin &get_origin () const
  { return *m_origin; }

  op &get_op () const
  { return *m_op; }

  value_closure *rdv_exchange (value_closure *nv)
  { return m_rdv.exchange (nv); }

  value &get_env (unsigned id)
  { return *m_env[id]; }
};

#endif /* _VALUE_CLOSURE_H_ */
