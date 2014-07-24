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
	(std::vector <std::tuple <value_type,
				  std::shared_ptr <builtin>>> const &stencil,
	 dwgrep_graph::sptr q, std::shared_ptr <scope> scope, unsigned arity)
  : m_arity {arity}
{
  assert (m_arity > 0);
  for (auto const &v: stencil)
    {
      auto pred = std::get <1> (v)->build_pred (q, scope);

      auto origin = std::make_shared <op_origin> (nullptr);
      auto op = std::get <1> (v)->build_exec (origin, q, scope);

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

overload_tab::overload_tab (overload_tab const &a, overload_tab const &b)
  : overload_tab {a.m_arity}
{
  assert (a.m_arity == b.m_arity);

  std::set <value_type> vts;
  auto add_all = [this, &vts] (overload_tab const &t)
    {
      for (auto const &overload: t.m_overloads)
	{
	  value_type vt = std::get <0> (overload);
	  assert (vts.find (vt) == vts.end ());
	  vts.insert (vt);
	  add_overload (vt, std::get <1> (overload));
	}
    };

  add_all (a);
  add_all (b);
}

void
overload_tab::add_overload (value_type vt, std::shared_ptr <builtin> b)
{
  // If this assert fails, the likely suspect is static initialization
  // order fiasco.
  assert (vt != value_type {0});

  // Check someone didn't order overload for this type yet.
  assert (std::find_if (m_overloads.begin (), m_overloads.end (),
			[vt] (std::tuple <value_type,
					  std::shared_ptr <builtin>> const &p)
			{ return std::get <0> (p) == vt; })
	  == m_overloads.end ());

  m_overloads.push_back (std::make_tuple (vt, b));
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

namespace
{
  struct named_overload_op
    : public overload_op
  {
    char const *m_name;

    named_overload_op (std::shared_ptr <op> upstream,
		       overload_instance ovl_inst,
		       char const *name)
      : overload_op {upstream, ovl_inst}
      , m_name {name}
    {}

    std::string
    name () const override
    {
      return m_name;
    }
  };
}

std::shared_ptr <op>
overloaded_op_builtin::build_exec (std::shared_ptr <op> upstream,
				   dwgrep_graph::sptr q,
				   std::shared_ptr <scope> scope) const
{
  return std::make_shared <named_overload_op>
    (upstream, get_overload_tab ()->instantiate (q, scope), name ());
}

std::shared_ptr <overloaded_builtin>
overloaded_op_builtin::create_merged (std::shared_ptr <overload_tab> tab) const
{
  return std::make_shared <overloaded_op_builtin> (name (), tab);
}

namespace
{
  struct named_overload_pred
    : public overload_pred
  {
    char const *m_name;

    named_overload_pred (overload_instance ovl_inst, char const *name)
      : overload_pred {ovl_inst}
      , m_name {name}
    {}

    std::string
    name () const override
    {
      return m_name;
    }
  };
}

std::unique_ptr <pred>
overloaded_pred_builtin::build_pred (dwgrep_graph::sptr q,
				     std::shared_ptr <scope> scope) const
{
  assert (name ()[0] == '?' || name ()[0] == '!');
  bool positive = name ()[0] == '?';

  auto pred = std::make_unique <named_overload_pred>
    (get_overload_tab ()->instantiate (q, scope), name ());

  if (positive)
    return std::move (pred);
  else
    return std::make_unique <pred_not> (std::move (pred));
}

std::shared_ptr <overloaded_builtin>
overloaded_pred_builtin::create_merged (std::shared_ptr <overload_tab> tab) const
{
  return std::make_shared <overloaded_pred_builtin> (name (), tab);
}
