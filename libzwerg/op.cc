/*
   Copyright (C) 2017, 2018 Petr Machata
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

#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <algorithm>
#include "../extern/optional.hpp"

#include "op.hh"
#include "builtin-closure.hh"
#include "overload.hh"
#include "value-closure.hh"
#include "value-cst.hh"
#include "value-seq.hh"
#include "value-str.hh"
#include "scon.hh"

namespace
{
  void
  debug_stack (stack const &stk)
  {
    std::vector <std::reference_wrapper <value const>> stack;
    for (size_t i = 0, n = stk.size (); i < n; ++i)
      stack.push_back (stk.get (i));

    std::cerr << "<";
    std::for_each (stack.rbegin (), stack.rend (),
		   [&stk] (std::reference_wrapper <value const> v) {
		     v.get ().show ((std::cerr << ' '));
		   });
    std::cerr << " >";
  }
}

struct op_origin::state
{
  stack::uptr m_stk;
};

op_origin::op_origin (layout &l)
  : m_ll {l.reserve <state> ()}
{}

std::string
op_origin::name () const
{
  return "origin";
}

void
op_origin::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
}

void
op_origin::state_des (scon &sc) const
{
  sc.des <state> (m_ll);
}

void
op_origin::set_next (scon &sc, stack::uptr s) const
{
  state &st = sc.get <state> (m_ll);
  // Unless state_con percolated all the way here, the following line likely
  // crashes the process, because the state buffer is still poisoned.
  st.m_stk = std::move (s);
}

stack::uptr
op_origin::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  return std::move (st.m_stk);
}


stack::uptr
op_nop::next (scon &sc) const
{
  return m_upstream->next (sc);
}

std::string
op_nop::name () const
{
  return "nop";
}


stack::uptr
op_assert::next (scon &sc) const
{
  while (auto stk = m_upstream->next (sc))
    if (m_pred->result (sc, *stk) == pred_result::yes)
      return stk;
  return nullptr;
}

std::string
op_assert::name () const
{
  return std::string ("assert<") + m_pred->name () + ">";
}


struct stringer_origin::state
{
  stack::uptr m_stk;
};

stringer_origin::stringer_origin (layout &l)
  : m_ll {l.reserve <state> ()}
{}

void
stringer_origin::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
}

void
stringer_origin::state_des (scon &sc) const
{
  sc.des <state> (m_ll);
}

std::pair <stack::uptr, std::string>
stringer_origin::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  return std::make_pair (std::move (st.m_stk), "");
}

void
stringer_origin::set_next (scon &sc, stack::uptr s)
{
  state &st = sc.get <state> (m_ll);
  assert (st.m_stk == nullptr);
  st.m_stk = std::move (s);
}


void
stringer_lit::state_con (scon &sc) const
{
  m_upstream->state_con (sc);
}

void
stringer_lit::state_des (scon &sc) const
{
  m_upstream->state_des (sc);
}

std::pair <stack::uptr, std::string>
stringer_lit::next (scon &sc) const
{
  auto up = m_upstream->next (sc);
  if (up.first == nullptr)
    return std::make_pair (nullptr, "");
  up.second = m_str + up.second;
  return up;
}


struct stringer_op::state
{
  nonstd::optional<std::string> m_str;
};

stringer_op::stringer_op (layout &l,
			  std::shared_ptr <stringer> upstream,
			  std::shared_ptr <op_origin> origin,
			  std::shared_ptr <op> op)
  : m_upstream {upstream}
  , m_origin {origin}
  , m_op {op}
  , m_ll {l.reserve <state> ()}
{}

void
stringer_op::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  m_op->state_con (sc);
  m_upstream->state_con (sc);
}

void
stringer_op::state_des (scon &sc) const
{
  m_upstream->state_des (sc);
  m_op->state_des (sc);
  sc.des <state> (m_ll);
}

std::pair <stack::uptr, std::string>
stringer_op::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);

  while (true)
    {
      if (! st.m_str)
	{
	  auto up = m_upstream->next (sc);
	  if (up.first == nullptr)
	    return std::make_pair (nullptr, "");

	  m_origin->set_next (sc, std::move (up.first));
	  st.m_str.emplace (std::move (up.second));
	}

      if (auto stk = m_op->next (sc))
	{
	  std::stringstream ss;
	  (stk->pop ())->show (ss);
	  return std::make_pair (std::move (stk), ss.str () + *st.m_str);
	}

      st.m_str = nonstd::nullopt;
    }
}


struct op_format::state
{
  size_t m_pos;

  state ()
    : m_pos {0}
  {}
};

op_format::op_format (layout &l,
		      std::shared_ptr <op> upstream,
		      std::shared_ptr <stringer_origin> origin,
		      std::shared_ptr <stringer> stringer)
  : inner_op {upstream}
  , m_origin {origin}
  , m_stringer {stringer}
  , m_ll {l.reserve <state> ()}
{}

void
op_format::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  m_stringer->state_con (sc);
  inner_op::state_con (sc);
}

void
op_format::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  m_stringer->state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_format::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  while (true)
    {
      auto stk = m_stringer->next (sc);
      if (stk.first != nullptr)
	{
	  stk.first->push (std::make_unique <value_str>
			   (std::move (stk.second), st.m_pos++));
	  return std::move (stk.first);
	}

      if (auto stk = m_upstream->next (sc))
	{
	  sc.reset <state> (m_ll);
	  m_origin->set_next (sc, std::move (stk));
	}
      else
	return nullptr;
    }
}

std::string
op_format::name () const
{
  return "format";
}


stack::uptr
op_const::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      stk->push (m_value->clone ());
      return stk;
    }
  return nullptr;
}

std::string
op_const::name () const
{
  std::stringstream ss;
  ss << "const<";
  m_value->show (ss);
  ss << ">";
  return ss.str ();
}


struct op_merge::state
{
  std::vector <stack::uptr> m_file;
  size_t m_idx;
  bool m_done;

  explicit state (size_t sz)
    : m_file (sz)
    , m_idx {0}
    , m_done {false}
  {}
};

stack::uptr
op_tine::next (scon &sc) const
{
  op_merge::state &mst = m_merge.get_state (sc);

  if (mst.m_done)
    return nullptr;

  if (std::all_of (mst.m_file.begin (), mst.m_file.end (),
		   [] (stack::uptr const &ptr) { return ptr == nullptr; }))
    {
      if (auto stk = m_merge.get_upstream ().next (sc))
	for (auto &ptr: mst.m_file)
	  ptr = std::make_unique <stack> (*stk);
      else
	{
	  mst.m_done = true;
	  return nullptr;
	}
    }

  return std::move (mst.m_file[m_branch_id]);
}

std::string
op_tine::name () const
{
  std::stringstream ss;
  ss << "tine<" << m_branch_id << ">";
  return ss.str ();
}


op_merge::op_merge (layout &l, std::shared_ptr <op> upstream)
  : inner_op {upstream}
  , m_ll {l.reserve <state> ()}
{}

op_merge::state &
op_merge::get_state (scon &sc) const
{
  return sc.get <state> (m_ll);
}

op &
op_merge::get_upstream () const
{
  return *m_upstream;
}

void
op_merge::state_con (scon &sc) const
{
  sc.con <state> (m_ll, m_ops.size ());
  for (auto const &branch: m_ops)
    branch->state_con (sc);
  inner_op::state_con (sc);
}

void
op_merge::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  for (auto const &branch: m_ops)
    branch->state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_merge::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  if (st.m_done)
    return nullptr;

  while (! st.m_done)
    {
      if (auto ret = m_ops[st.m_idx]->next (sc))
	return ret;
      if (++st.m_idx == m_ops.size ())
	st.m_idx = 0;
    }

  return nullptr;
}

std::string
op_merge::name () const
{
  std::stringstream ss;
  ss << "merge<";

  bool first = true;
  for (auto const &branch: m_ops)
    {
      if (! first)
	ss << ",";
      ss << branch->name ();
      first = false;
    }

  ss << ">";
  return ss.str ();
}


struct op_or::state
{
  decltype (op_or::m_branches)::const_iterator m_branch_it;

  explicit state (decltype (op_or::m_branches) const &branches)
    : m_branch_it {branches.end ()}
  {}
};

op_or::op_or (layout &l, std::shared_ptr <op> upstream)
  : inner_op {upstream}
  , m_ll {l.reserve <state> ()}
{}

void
op_or::state_con (scon &sc) const
{
  sc.con <state> (m_ll, m_branches);
  for (auto const &branch: m_branches)
    branch.second->state_con (sc);
  inner_op::state_con (sc);
}

void
op_or::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  for (auto const &branch: m_branches)
    branch.second->state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_or::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);

  while (true)
    {
      while (st.m_branch_it == m_branches.end ())
	{
	  if (auto stk = m_upstream->next (sc))
	    for (st.m_branch_it = m_branches.begin ();
		 st.m_branch_it != m_branches.end (); ++st.m_branch_it)
	      {
		auto &origin = st.m_branch_it->first;
		origin->set_next (sc, std::make_unique <stack> (*stk));
		if (auto stk2 = st.m_branch_it->second->next (sc))
		  return stk2;
	      }
	  else
	    return nullptr;
	}

      if (auto stk2 = st.m_branch_it->second->next (sc))
	return stk2;

      sc.reset <state> (m_ll, m_branches);
    }
}

std::string
op_or::name () const
{
  std::stringstream ss;
  ss << "or<";
  bool sep = false;
  for (auto const &branch: m_branches)
    {
      if (sep)
	ss << " || ";
      sep = true;
      ss << branch.second->name ();
    }
  ss << ">";
  return ss.str ();
}


void
op_capture::state_con (scon &sc) const
{
  m_op->state_con (sc);
  inner_op::state_con (sc);
}

void
op_capture::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  m_op->state_des (sc);
}

stack::uptr
op_capture::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      m_origin->set_next (sc, std::make_unique <stack> (*stk));

      value_seq::seq_t vv;
      while (auto stk2 = m_op->next (sc))
	vv.push_back (stk2->pop ());

      stk->push (std::make_unique <value_seq> (std::move (vv), 0));
      m_op->state_des (sc);
      m_op->state_con (sc);
      return stk;
    }

  return nullptr;
}

std::string
op_capture::name () const
{
  return std::string ("capture<") + m_op->name () + ">";
}


namespace
{
  struct deref_less
  {
    template <class T>
    bool
    operator() (T const &a, T const &b)
    {
      return *a < *b;
    }
  };
}

struct op_tr_closure::state
{
  std::set <std::shared_ptr <stack>, deref_less> m_seen;
  std::vector <std::shared_ptr <stack> > m_stks;
  bool m_op_drained;

  state ()
    : m_op_drained {true}
  {}

  std::unique_ptr <stack> yield_and_cache (std::shared_ptr <stack> stk);
};

op_tr_closure::op_tr_closure (layout &l,
			      std::shared_ptr <op> upstream,
			      std::shared_ptr <op_origin> origin,
			      std::shared_ptr <op> op,
			      op_tr_closure_kind k)
  : inner_op (upstream)
  , m_origin {origin}
  , m_op {op}
  , m_is_plus {k == op_tr_closure_kind::plus}
  , m_ll {l.reserve <state> ()}
{}

void
op_tr_closure::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  m_op->state_con (sc);
  inner_op::state_con (sc);
}

void
op_tr_closure::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  m_op->state_des (sc);
  sc.des <state> (m_ll);
}

std::unique_ptr <stack>
op_tr_closure::state::yield_and_cache (std::shared_ptr <stack> stk)
{
  if (m_seen.insert (stk).second)
    {
      m_stks.push_back (stk);
      return std::make_unique <stack> (*stk);
    }
  else
    return nullptr;
}

stack::uptr
op_tr_closure::next_from_upstream (state &st, scon &sc) const
{
  // When we get a new stack from upstream, that provides a fresh
  // context, and we need to forget what we've seen so far.
  // E.g. consider the following expression:
  //
  //     $ 'entry root dup child* ?eq'
  //
  // We should see as many root-root matches as there are entries.
  // But if we fail to clear the seen-cache, we only see one.

  st.m_seen.clear ();
  return m_upstream->next (sc);
}

stack::uptr
op_tr_closure::next_from_op (state &st, scon &sc) const
{
  if (st.m_op_drained)
    return nullptr;
  if (auto ret = m_op->next (sc))
    return ret;
  st.m_op_drained = true;
  return nullptr;
}

bool
op_tr_closure::send_to_op (state &st, scon &sc,
			   std::unique_ptr <stack> stk) const
{
  if (stk == nullptr)
    return false;

  m_origin->set_next (sc, std::move (stk));
  st.m_op_drained = false;
  return true;
}

bool
op_tr_closure::send_to_op (state &st, scon &sc) const
{
  if (st.m_stks.empty ())
    return m_is_plus ? send_to_op (st, sc,
				   next_from_upstream (st, sc)) : false;

  send_to_op (st, sc, std::make_unique <stack> (*st.m_stks.back ()));
  st.m_stks.pop_back ();
  return true;
}

stack::uptr
op_tr_closure::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);

  do
    while (std::shared_ptr <stack> stk = next_from_op (st, sc))
      if (auto ret = st.yield_and_cache (stk))
	return ret;
  while (send_to_op (st, sc));

  if (! m_is_plus)
    if (std::shared_ptr <stack> stk = next_from_upstream (st, sc))
      return st.yield_and_cache (stk);

  return nullptr;
}

std::string
op_tr_closure::name () const
{
  return std::string ("close<") + m_upstream->name () + ">";
}


struct op_subx::state
{
  stack::uptr m_stk;
};

op_subx::op_subx (layout &l,
		  std::shared_ptr <op> upstream,
		  std::shared_ptr <op_origin> origin,
		  std::shared_ptr <op> op,
		  size_t keep)
  : inner_op {upstream}
  , m_origin {origin}
  , m_op {op}
  , m_keep {keep}
  , m_ll {l.reserve <state> ()}
{}

void
op_subx::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  m_op->state_con (sc);
  inner_op::state_con (sc);
}

void
op_subx::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  m_op->state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_subx::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);

  while (true)
    {
      while (st.m_stk == nullptr)
	if (st.m_stk = m_upstream->next (sc))
	  m_origin->set_next (sc, std::make_unique <stack> (*st.m_stk));
	else
	  return nullptr;

      if (auto stk = m_op->next (sc))
	{
	  auto ret = std::make_unique <stack> (*st.m_stk);
	  std::vector <std::unique_ptr <value>> kept;
	  for (size_t i = 0; i < m_keep; ++i)
	    kept.push_back (stk->pop ());
	  for (size_t i = 0; i < m_keep; ++i)
	    {
	      ret->push (std::move (kept.back ()));
	      kept.pop_back ();
	    }
	  return ret;
	}

      st.m_stk = nullptr;
    }
}

std::string
op_subx::name () const
{
  return std::string ("subx<") + m_op->name () + ">";
}


stack::uptr
op_f_debug::next (scon &sc) const
{
  while (auto stk = m_upstream->next (sc))
    {
      debug_stack (*stk);
      return stk;
    }
  return nullptr;
}

std::string
op_f_debug::name () const
{
  return "f_debug";
}


struct op_bind::state
{
  std::unique_ptr <value> m_current;
};

op_bind::op_bind (layout &l, std::shared_ptr <op> upstream)
  : inner_op {upstream}
  , m_ll {l.reserve <state> ()}
{}

std::string
op_bind::name () const
{
  std::stringstream ss;
  ss << "bind<" << this << ">";
  return ss.str ();
}

void
op_bind::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  inner_op::state_con (sc);
}

void
op_bind::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_bind::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  if (auto stk = m_upstream->next (sc))
    {
      st.m_current = stk->pop ();
      return stk;
    }
  return nullptr;
}

std::unique_ptr <value>
op_bind::current (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  return st.m_current->clone ();
}


op_read::op_read (std::shared_ptr <op> upstream, op_bind &src)
  : inner_op {upstream}
  , m_src {src}
{}

stack::uptr
op_read::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      stk->push (m_src.current (sc));
      return stk;
    }
  else
    return nullptr;
}

std::string
op_read::name () const
{
  std::stringstream ss;
  ss << "read<" << &m_src << ">";
  return ss.str ();
}


op_upread::op_upread (std::shared_ptr <op> upstream, unsigned id,
		      layout::loc rdv_ll)
  : inner_op (upstream)
  , m_id {id}
  , m_rdv_ll {rdv_ll}
{}

std::string
op_upread::name () const
{
  std::stringstream ss;
  ss << "upread<" << m_id << ">";
  return ss.str ();
}

stack::uptr
op_upread::next (scon &sc) const
{
  auto &rdv = sc.get <op_apply::rendezvous> (m_rdv_ll);
  if (auto stk = m_upstream->next (sc))
    {
      stk->push (rdv.closure.get_env (m_id).clone ());
      return stk;
    }
  else
    return nullptr;
}


op_lex_closure::op_lex_closure (std::shared_ptr <op> upstream,
				layout op_layout, layout::loc rdv_ll,
				std::shared_ptr <op_origin> origin,
				std::shared_ptr <op> op,
				size_t n_upvalues)
  : inner_op {upstream}
  , m_op_layout {op_layout}
  , m_rdv_ll {rdv_ll}
  , m_origin {origin}
  , m_op {op}
  , m_n_upvalues {n_upvalues}
{}

stack::uptr
op_lex_closure::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      // Fetch actual values of the referenced environment bindings.
      std::vector <std::unique_ptr <value>> env;
      for (size_t i = 0; i < m_n_upvalues; ++i)
	env.push_back (std::move (stk->pop ()));

      stk->push (std::make_unique <value_closure> (m_op_layout, m_rdv_ll,
						   m_origin, m_op,
						   std::move (env), 0));
      return stk;
    }

  return nullptr;
}

std::string
op_lex_closure::name () const
{
  return "lex_closure";
}


struct op_ifelse::state
{
  nonstd::optional <scon_guard> m_sg;
};

op_ifelse::op_ifelse (layout &l,
		      std::shared_ptr <op> upstream,
		      std::shared_ptr <op_origin> cond_origin,
		      std::shared_ptr <op> cond_op,
		      std::shared_ptr <op_origin> then_origin,
		      std::shared_ptr <op> then_op,
		      std::shared_ptr <op_origin> else_origin,
		      std::shared_ptr <op> else_op)
  : inner_op (upstream)
  , m_cond_origin {cond_origin}
  , m_cond_op {cond_op}
  , m_then_origin {then_origin}
  , m_then_op {then_op}
  , m_else_origin {else_origin}
  , m_else_op {else_op}
  , m_ll {l.reserve <state> ()}
{}

void
op_ifelse::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  inner_op::state_con (sc);
}

void
op_ifelse::state_des (scon &sc) const
{
  inner_op::state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_ifelse::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);

  while (true)
    {
      // Condition and then and else branches share state space. So in the
      // following, make sure that the condition state space is only used after
      // the then/else one is destroyed, and vice versa, that the then/else is
      // not initialized before the condition stops running.
      if (st.m_sg == nonstd::nullopt)
	{
	  if (auto stk = m_upstream->next (sc))
	    {
	      stack::uptr cond_stk;
	      {
		scon_guard sg {sc, *m_cond_op};
		m_cond_origin->set_next (sc, std::make_unique <stack> (*stk));
		cond_stk = sg.next ();
	      }

	      if (cond_stk != nullptr)
		{
		  st.m_sg.emplace (std::ref (sc), *m_then_op);
		  m_then_origin->set_next (sc, std::move (stk));
		}
	      else
		{
		  st.m_sg.emplace (std::ref (sc), *m_else_op);
		  m_else_origin->set_next (sc, std::move (stk));
		}
	    }
	  else
	    return nullptr;
	}

      assert (st.m_sg);
      if (auto stk = st.m_sg->next ())
	return stk;

      st.m_sg = nonstd::nullopt;
    }
}

std::string
op_ifelse::name () const
{
  return "ifelse";
}

stack::uptr
op_drop_below::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      auto tos = stk->pop ();
      stk->drop (m_drop);
      stk->push (std::move (tos));
      return stk;
    }
  return nullptr;
}

std::string
op_drop_below::name () const
{
  std::stringstream ss;
  ss << "drop_below<" << m_drop << ">";
  return ss.str ();
}


pred_result
pred_not::result (scon &sc, stack &stk) const
{
  return ! m_a->result (sc, stk);
}

std::string
pred_not::name () const
{
  return std::string ("not<") + m_a->name () + ">";
}


pred_result
pred_and::result (scon &sc, stack &stk) const
{
  return m_a->result (sc, stk) && m_b->result (sc, stk);
}

std::string
pred_and::name () const
{
  return std::string ("and<") + m_a->name () + "><" + m_b->name () + ">";
}


pred_result
pred_or::result (scon &sc, stack &stk) const
{
  return m_a->result (sc, stk) || m_b->result (sc, stk);
}

std::string
pred_or::name () const
{
  return std::string ("or<") + m_a->name () + "><" + m_b->name () + ">";
}

pred_result
pred_subx_any::result (scon &sc, stack &stk) const
{
  scon_guard sg {sc, *m_op};
  m_origin->set_next (sc, std::make_unique <stack> (stk));
  if (m_op->next (sc) != nullptr)
    return pred_result::yes;
  else
    return pred_result::no;
}

std::string
pred_subx_any::name () const
{
  return std::string ("pred_subx_any<") + m_op->name () + ">";
}


pred_result
pred_pos::result (scon &sc, stack &stk) const
{
    auto const &value = stk.top ();
    return value.get_pos () == m_pos ? pred_result::yes : pred_result::no;
}

std::string
pred_pos::name () const
{
  std::stringstream ss;
  ss << "pred_pos<" << m_pos << ">";
  return ss.str ();
}
