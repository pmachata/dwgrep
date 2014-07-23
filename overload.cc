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

#include <memory>
#include "make_unique.hh"
#include <set>

#include "overload.hh"

overload_instance::overload_instance
	(std::vector <std::tuple <value_type, builtin &>> const &stencil,
	 dwgrep_graph::sptr q, std::shared_ptr <scope> scope, unsigned arity)
  : m_arity {arity}
{
  assert (m_arity > 0);
  for (auto const &v: stencil)
    {
      auto pred = std::get <1> (v).build_pred (q, scope);

      auto origin = std::make_shared <op_origin> (nullptr);
      auto op = std::get <1> (v).build_exec (origin, q, scope);

      assert (op != nullptr || pred != nullptr);

      if (op != nullptr)
	m_execs.push_back (std::make_tuple (std::get <0> (v), origin, op));
      if (pred != nullptr)
	m_preds.push_back (std::make_tuple (std::get <0> (v),
					    std::move (pred)));
    }
}

namespace
{
  value_type
  check_vt_near_top (valfile &vf, unsigned depth)
  {
    value_type vt = vf.top ().get_type ();
    for (unsigned i = 1; i < depth; ++i)
      if (vt != vf.get (i).get_type ())
	return value_type {0};
    return vt;
  }
}

overload_instance::exec_vec::value_type *
overload_instance::find_exec (valfile &vf)
{
  value_type vt = check_vt_near_top (vf, m_arity);
  if (vt == value_type {0})
    return nullptr;

  auto it = std::find_if (m_execs.begin (), m_execs.end (),
			  [vt] (exec_vec::value_type const &ovl)
			  { return std::get <0> (ovl) == vt; });

  if (it == m_execs.end ())
    return nullptr;
  else
    return &*it;
}

overload_instance::pred_vec::value_type *
overload_instance::find_pred (valfile &vf)
{
  value_type vt = check_vt_near_top (vf, m_arity);
  if (vt == value_type {0})
    return nullptr;

  auto it = std::find_if (m_preds.begin (), m_preds.end (),
			  [vt] (pred_vec::value_type const &ovl)
			  { return std::get <0> (ovl) == vt; });

  if (it == m_preds.end ())
    return nullptr;
  else
    return &*it;
}

void
show_expects (std::string const &name, std::vector <value_type> vts)
{
  std::cerr << "Error: `" << name << "'";

  if (vts.empty ())
    {
      std::cerr << " has no registered overloads.\n";
      return;
    }

  std::cerr << " expects ";
  size_t i = 0;
  for (auto vt: vts)
    {
      if (i == 0)
	;
      else if (i == vts.size () - 1)
	std::cerr << " or ";
      else
	std::cerr << ", ";

      ++i;
      std::cerr << vt.name ();
    }
  std::cerr << " on TOS.\n";
}

void
overload_instance::show_error (std::string const &name)
{
  std::set <value_type> vts;
  for (auto const &ex: m_execs)
    vts.insert (std::get <0> (ex));
  for (auto const &pr: m_preds)
    vts.insert (std::get <0> (pr));

  return show_expects (name,
		       std::vector <value_type> (vts.begin (), vts.end ()));
}

void
overload_tab::add_overload (value_type vt, builtin &b)
{
  // If this assert fails, the likely suspect is static initialization
  // order fiasco.
  assert (vt != value_type {0});

  // Check someone didn't order overload for this type yet.
  assert (std::find_if (m_overloads.begin (), m_overloads.end (),
			[vt] (std::tuple <value_type, builtin &> const &p)
			{ return std::get <0> (p) == vt; })
	  == m_overloads.end ());

  m_overloads.push_back (std::tuple <value_type, builtin &> {vt, b});
}

overload_instance
overload_tab::instantiate (dwgrep_graph::sptr q, std::shared_ptr <scope> scope)
{
  return overload_instance {m_overloads, q, scope, m_arity};
}


struct overload_op::pimpl
{
  std::shared_ptr <op> m_upstream;
  overload_instance m_ovl_inst;
  std::shared_ptr <op> m_op;

  void
  reset_me ()
  {
    m_op = nullptr;
  }

  pimpl (std::shared_ptr <op> upstream, overload_instance ovl_inst)
    : m_upstream {upstream}
    , m_ovl_inst {ovl_inst}
    , m_op {nullptr}
  {}

  valfile::uptr
  next (op &self)
  {
    while (true)
      {
	while (m_op == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		auto ovl = m_ovl_inst.find_exec (*vf);
		if (ovl == nullptr)
		  m_ovl_inst.show_error (self.name ());
		else
		  {
		    m_op = std::get <2> (*ovl);
		    m_op->reset ();
		    std::get <1> (*ovl)->set_next (std::move (vf));
		  }
	      }
	    else
	      return nullptr;
	  }

	if (auto vf = m_op->next ())
	  return vf;

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

overload_op::overload_op (std::shared_ptr <op> upstream,
			  overload_instance ovl_inst)
  : m_pimpl {std::make_unique <pimpl> (upstream, ovl_inst)}
{}

overload_op::~overload_op ()
{}

valfile::uptr
overload_op::next ()
{
  return m_pimpl->next (*this);
}

void
overload_op::reset ()
{
  return m_pimpl->reset ();
}

pred_result
overload_pred::result (valfile &vf)
{
  auto ovl = m_ovl_inst.find_pred (vf);
  if (ovl == nullptr)
    {
      m_ovl_inst.show_error (name ());
      return pred_result::fail;
    }
  else
    return std::get <1> (*ovl)->result (vf);
}
