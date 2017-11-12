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

#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <algorithm>

#include "op.hh"
#include "builtin-closure.hh"
#include "overload.hh"
#include "value-closure.hh"
#include "value-cst.hh"
#include "value-seq.hh"
#include "value-str.hh"

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

size_t
inner_op::reserve () const
{
  return m_upstream->reserve () + m_reserve;
}

void
inner_op::state_con (void *buf) const
{
  m_upstream->state_con (buf);
}

void
inner_op::state_des (void *buf) const
{
  m_upstream->state_des (buf);
}

struct op_origin::state
{
  stack::uptr m_stk;
};

std::string
op_origin::name () const
{
  return "origin";
}

size_t
op_origin::reserve () const
{
  return sizeof (state);
}

void
op_origin::state_con (void *buf) const
{
  state *st = this_state <state> (buf);
  new (st) state {};
}

void
op_origin::state_des (void *buf) const
{
  state *st = this_state <state> (buf);
  st->~state ();
}

void
op_origin::set_next (void *buf_end, stack::uptr s) const
{
  // Origin has no upstream and therefore its state has to be the last.
  state *st = this_state <state> (buf_end) - 1;

  // M_STK serves as a canary, because unless reset() percolated all the way
  // here, what STATE points at is sill a poisoned memory. Likewise if
  // set_next() is called before the next reset(), M_STK will be nonnull.
  assert (st->m_stk == nullptr);
  st->m_stk = std::move (s);
}

stack::uptr
op_origin::next (void *buf) const
{
  state *st = this_state <state> (buf);
  return std::move (st->m_stk);
}


stack::uptr
op_nop::next ()
{
  return m_upstream->next ();
}

std::string
op_nop::name () const
{
  return "nop";
}


stack::uptr
op_assert::next ()
{
  while (auto stk = m_upstream->next ())
    if (m_pred->result (*stk) == pred_result::yes)
      return stk;
  return nullptr;
}

std::string
op_assert::name () const
{
  return std::string ("assert<") + m_pred->name () + ">";
}


void
stringer_origin::set_next (stack::uptr s)
{
  assert (m_stk == nullptr);

  // set_next should have been preceded with a reset() call that
  // should have percolated all the way here.
  assert (m_reset);
  m_reset = false;

  m_stk = std::move (s);
}

std::pair <stack::uptr, std::string>
stringer_origin::next ()
{
  return std::make_pair (std::move (m_stk), "");
}

void
stringer_origin::reset ()
{
  m_stk = nullptr;
  m_reset = true;
}

std::pair <stack::uptr, std::string>
stringer_lit::next ()
{
  auto up = m_upstream->next ();
  if (up.first == nullptr)
    return std::make_pair (nullptr, "");
  up.second = m_str + up.second;
  return up;
}

void
stringer_lit::reset ()
{
  m_upstream->reset ();
}

std::pair <stack::uptr, std::string>
stringer_op::next ()
{
#if 0
  while (true)
    {
      if (! m_have)
	{
	  auto up = m_upstream->next ();
	  if (up.first == nullptr)
	    return std::make_pair (nullptr, "");

	  m_op->reset ();
	  m_origin->set_next (std::move (up.first));
	  m_str = up.second;

	  m_have = true;
	}

      if (auto stk = m_op->next ())
	{
	  std::stringstream ss;
	  (stk->pop ())->show (ss);
	  return std::make_pair (std::move (stk), ss.str () + m_str);
	}

      m_have = false;
    }
#else
  return std::make_pair (nullptr, "");
#endif
}

void
stringer_op::reset ()
{
  m_have = false;
  m_op->reset ();
  m_upstream->reset ();
}

struct op_format::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <stringer_origin> m_origin;
  std::shared_ptr <stringer> m_stringer;
  size_t m_pos;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <stringer_origin> origin,
	 std::shared_ptr <stringer> stringer)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_stringer {stringer}
    , m_pos {0}
  {}

  void
  reset_me ()
  {
    m_stringer->reset ();
    m_pos = 0;
  }

  stack::uptr
  next ()
  {
#if 0
    while (true)
      {
	auto stk = m_stringer->next ();
	if (stk.first != nullptr)
	  {
	    stk.first->push (std::make_unique <value_str>
			     (std::move (stk.second), m_pos++));
	    return std::move (stk.first);
	  }

	if (auto stk = m_upstream->next ())
	  {
	    reset_me ();
	    m_origin->set_next (std::move (stk));
	  }
	else
	  return nullptr;
      }
#else
  return nullptr;
#endif
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_format::op_format (std::shared_ptr <op> upstream,
		      std::shared_ptr <stringer_origin> origin,
		      std::shared_ptr <stringer> stringer)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, stringer)}
{}

op_format::~op_format ()
{}

stack::uptr
op_format::next ()
{
  return m_pimpl->next ();
}

void
op_format::reset ()
{
  m_pimpl->reset ();
}

std::string
op_format::name () const
{
  return "format";
}


stack::uptr
op_const::next (void *buf) const
{
  if (auto stk = m_upstream->next (buf))
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
op_tine::next (void *buf) const
{
  state *st = op::this_state <state> (buf);
  auto pair = m_merge.get_state (m_branch_id, st);
  op_merge::state &mst = pair.first;
  void *upstream_buf = pair.second;

  if (mst.m_done)
    return nullptr;

  if (std::all_of (mst.m_file.begin (), mst.m_file.end (),
		   [] (stack::uptr const &ptr) { return ptr == nullptr; }))
    {
      if (auto stk = m_merge.get_upstream ().next (upstream_buf))
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


size_t
op_merge::branch_off (size_t branch_id) const
{
  size_t off = sizeof (struct state);
  for (size_t i = 0; i < branch_id; ++i)
    off += m_ops[i]->reserve ();
  return off;
}

std::pair <op_merge::state &, void *>
op_merge::get_state (size_t branch_id, op_tine::state *tine_state) const
{
  auto buf_end = reinterpret_cast <uint8_t *> (tine_state + 1);

  // The state for tine in BRANCH should be stored at the end of the area, so
  // ask for branch_id + 1 to include the querying branch itself in the offset
  // as well.
  size_t off = branch_off (branch_id + 1);
  uint8_t *this_buf = buf_end - off;
  op_merge::state &st = *op::this_state <op_merge::state> (this_buf);

  off = branch_off (m_ops.size ());
  void *upstream_buf = this_buf + off;

  return {st, upstream_buf};
}

op &
op_merge::get_upstream () const
{
  return *m_upstream;
}

size_t
op_merge::reserve () const
{
  size_t off = sizeof (struct state);
  for (auto const &branch: m_ops)
    off += branch->reserve ();
  return off + m_upstream->reserve ();
}

void
op_merge::state_con (void *buf) const
{
  state *st = op::this_state <state> (buf);
  new (st) state {m_ops.size ()};

  auto ptr = reinterpret_cast <uint8_t *> (op::next_state (st));
  for (std::shared_ptr <op> const &branch: m_ops)
    {
      branch->state_con (ptr);
      // xxx figure out how to avoid the repeated reserve computation here and
      // in des
      ptr += branch->reserve ();
    }

  m_upstream->state_con (ptr);
}

void
op_merge::state_des (void *buf) const
{
  state *st = op::this_state <state> (buf);

  auto ptr = reinterpret_cast <uint8_t *> (op::next_state (st));
  for (auto const &branch: m_ops)
    {
      branch->state_des (ptr);
      ptr += branch->reserve ();
    }

  m_upstream->state_des (ptr);
  st->~state ();
}

stack::uptr
op_merge::next (void *buf) const
{
  state *st = op::this_state <state> (buf);
  if (st->m_done)
    return nullptr;

  while (! st->m_done)
    {
      auto brbuf = reinterpret_cast <uint8_t *> (buf);
      brbuf += branch_off (st->m_idx);
      if (auto ret = m_ops[st->m_idx]->next (brbuf))
	return ret;
      if (++st->m_idx == m_ops.size ())
	st->m_idx = 0;
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


void
op_or::reset_me ()
{
  m_branch_it = m_branches.end ();
  for (auto const &branch: m_branches)
    branch.second->reset ();
}

void
op_or::reset ()
{
  reset_me ();
  m_upstream->reset ();
}

stack::uptr
op_or::next ()
{
#if 0
  while (true)
    {
      while (m_branch_it == m_branches.end ())
	{
	  if (auto stk = m_upstream->next ())
	    for (m_branch_it = m_branches.begin ();
		 m_branch_it != m_branches.end (); ++m_branch_it)
	      {
		m_branch_it->second->reset ();
		m_branch_it->first->set_next (std::make_unique <stack> (*stk));
		if (auto stk2 = m_branch_it->second->next ())
		  return stk2;
	      }
	  else
	    return nullptr;
	}

      if (auto stk2 = m_branch_it->second->next ())
	return stk2;

      reset_me ();
    }
#else
  return nullptr;
#endif
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


stack::uptr
op_capture::next ()
{
#if 0
  if (auto stk = m_upstream->next ())
    {
      m_op->reset ();
      m_origin->set_next (std::make_unique <stack> (*stk));

      value_seq::seq_t vv;
      while (auto stk2 = m_op->next ())
	vv.push_back (stk2->pop ());

      stk->push (std::make_unique <value_seq> (std::move (vv), 0));
      return stk;
    }

  return nullptr;
#else
  return nullptr;
#endif
}

void
op_capture::reset ()
{
  m_op->reset ();
  m_upstream->reset ();
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

struct op_tr_closure::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;

  std::set <std::shared_ptr <stack>, deref_less> m_seen;
  std::vector <std::shared_ptr <stack> > m_stks;

  bool m_is_plus;
  bool m_op_drained;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op,
	 op_tr_closure_kind k)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
    , m_is_plus {k == op_tr_closure_kind::plus}
    , m_op_drained {true}
  {}

  void
  reset_me ()
  {
    m_stks.clear ();
    m_seen.clear ();
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }

  std::unique_ptr <stack>
  yield_and_cache (std::shared_ptr <stack> stk)
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
  next_from_upstream ()
  {
    // When we get a new stack from upstream, that provides a fresh
    // context, and we need to forget what we've seen so far.
    // E.g. consider the following expression:
    //
    //     $ 'entry root dup child* ?eq'
    //
    // We should see as many root-root matches as there are entries.
    // But if we fail to clear the seen-cache, we only see one.

     m_seen.clear ();
    return m_upstream->next ();
  }

  stack::uptr
  next_from_op ()
  {
    if (m_op_drained)
      return nullptr;
    if (auto ret = m_op->next ())
      return ret;
    m_op_drained = true;
    return nullptr;
  }

  bool
  send_to_op (std::unique_ptr <stack> stk)
  {
#if 0
    if (stk == nullptr)
      return false;

    m_op->reset ();
    m_origin->set_next (std::move (stk));
    m_op_drained = false;
    return true;
#else
  return false;
#endif
  }

  bool
  send_to_op ()
  {
    if (m_stks.empty ())
      return m_is_plus ? send_to_op (next_from_upstream ()) : false;

    send_to_op (std::make_unique <stack> (*m_stks.back ()));
    m_stks.pop_back ();
    return true;
  }

  stack::uptr
  next ()
  {
    do
      while (std::shared_ptr <stack> stk = next_from_op ())
	if (auto ret = yield_and_cache (stk))
	  return ret;
    while (send_to_op ());

    if (! m_is_plus)
      if (std::shared_ptr <stack> stk = next_from_upstream ())
	return yield_and_cache (stk);

    return nullptr;
  }

  std::string
  name () const
  {
    return std::string ("close<") + m_upstream->name () + ">";
  }
};

op_tr_closure::op_tr_closure (std::shared_ptr <op> upstream,
			      std::shared_ptr <op_origin> origin,
			      std::shared_ptr <op> op,
			      op_tr_closure_kind k)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op, k)}
{}

op_tr_closure::~op_tr_closure ()
{}

stack::uptr
op_tr_closure::next ()
{
  return m_pimpl->next ();
}

void
op_tr_closure::reset ()
{
  m_pimpl->reset ();
}

std::string
op_tr_closure::name () const
{
  return m_pimpl->name ();
}


struct op_subx::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  stack::uptr m_stk;
  size_t m_keep;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op,
	 size_t keep)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
    , m_keep {keep}
  {}

  void
  reset_me ()
  {
    m_stk = nullptr;
  }

  stack::uptr
  next ()
  {
#if 0
    while (true)
      {
	while (m_stk == nullptr)
	  if (m_stk = m_upstream->next ())
	    {
	      m_op->reset ();
	      m_origin->set_next (std::make_unique <stack> (*m_stk));
	    }
	  else
	    return nullptr;

	if (auto stk = m_op->next ())
	  {
	    auto ret = std::make_unique <stack> (*m_stk);
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

	reset_me ();
      }
#else
  return nullptr;
#endif
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_subx::op_subx (std::shared_ptr <op> upstream,
		  std::shared_ptr <op_origin> origin,
		  std::shared_ptr <op> op,
		  size_t keep)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op, keep)}
{}

op_subx::~op_subx ()
{}

stack::uptr
op_subx::next ()
{
  return m_pimpl->next ();
}

void
op_subx::reset ()
{
  m_pimpl->reset ();
}

std::string
op_subx::name () const
{
  return std::string ("subx<") + m_pimpl->m_op->name () + ">";
}

stack::uptr
op_f_debug::next ()
{
  while (auto stk = m_upstream->next ())
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

void
op_f_debug::reset ()
{
  m_upstream->reset ();
}


void
op_bind::reset ()
{
  m_upstream->reset ();
  m_current = nullptr;
}

stack::uptr
op_bind::next ()
{
  if (auto stk = m_upstream->next ())
    {
      m_current = stk->pop ();
      return stk;
    }
  return nullptr;
}

std::string
op_bind::name () const
{
  return std::string ("bind");
}

std::unique_ptr <value>
op_bind::current () const
{
  return m_current->clone ();
}


struct op_read::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_bind> m_src;
  std::shared_ptr <op> m_apply;

  pimpl (std::shared_ptr <op> upstream,
         std::shared_ptr <op_bind> src)
    : m_upstream {upstream}
    , m_src {src}
  {}

  void
  reset_me ()
  {
    m_apply = nullptr;
  }

  stack::uptr
  do_next ()
  {
#if 0
    while (true)
      {
	if (m_apply == nullptr)
	  {
	    if (auto stk = m_upstream->next ())
	      {
		auto val = m_src->current ();
		bool is_closure = val->is <value_closure> ();
		stk->push (std::move (val));

		// If a referenced value is not a closure, then the
		// result is just that one value.
		if (! is_closure)
		  return stk;

		// If it's a closure, then this is a function
		// reference.  We need to execute it and fetch all the
		// values.

		auto origin = std::make_shared <op_origin> (std::move (stk));
		m_apply = std::make_shared <op_apply> (origin);
		std::cerr << m_apply.get () << " is a new apply" << std::endl;
	      }
	    else
	      return nullptr;
	  }

	assert (m_apply != nullptr);
	if (auto stk = m_apply->next ())
	  return stk;

	reset_me ();
      }
#else
  return nullptr;
#endif
  }

  stack::uptr
  next ()
  {
    std::cerr << this << " op_read next" << std::endl;
    auto ret = do_next ();
    std::cerr << this << " /op_read next" << std::endl;
    return ret;
  }

  void
  reset ()
  {
    std::cerr << this << " op_read reset" << std::endl;
    reset_me ();
    m_upstream->reset ();
    std::cerr << this << " /op_read reset" << std::endl;
  }
};

op_read::op_read (std::shared_ptr <op> upstream,
                  std::shared_ptr <op_bind> src)
  : m_pimpl {std::make_unique <pimpl> (upstream, src)}
{}

op_read::~op_read ()
{}

void
op_read::reset ()
{
  m_pimpl->reset ();
}

stack::uptr
op_read::next ()
{
  return m_pimpl->next ();
}

std::string
op_read::name () const
{
  return std::string ("read");
}


std::unique_ptr <value>
pseudo_bind::fetch (unsigned assert_id) const
{
  assert (m_id == assert_id);
  return m_src->current ();
}

std::unique_ptr <value>
pseudo_bind::current () const
{
  value_closure *closure = *m_rdv;
  assert (closure != nullptr);
  return closure->get_env (m_id).clone ();
}


void
op_lex_closure::reset ()
{
  m_upstream->reset ();
}

stack::uptr
op_lex_closure::next ()
{
  if (auto stk = m_upstream->next ())
    {
      // Fetch actual values of the referenced environment bindings.
      std::vector <std::unique_ptr <value>> env;
      for (auto const &pseudo: m_pseudos)
	env.push_back (pseudo->fetch (env.size ()));

      stk->push (std::make_unique <value_closure> (m_origin, m_op,
						   std::move (env), m_rdv, 0));
      return stk;
    }

  return nullptr;
}

std::string
op_lex_closure::name () const
{
  return "lex_closure";
}


struct op_ifelse::pimpl
{
  std::shared_ptr <op> m_upstream;

  std::shared_ptr <op_origin> m_cond_origin;
  std::shared_ptr <op> m_cond_op;

  std::shared_ptr <op_origin> m_then_origin;
  std::shared_ptr <op> m_then_op;

  std::shared_ptr <op_origin> m_else_origin;
  std::shared_ptr <op> m_else_op;

  std::shared_ptr <op_origin> m_sel_origin;
  std::shared_ptr <op> m_sel_op;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> cond_origin,
	 std::shared_ptr <op> cond_op,
	 std::shared_ptr <op_origin> then_origin,
	 std::shared_ptr <op> then_op,
	 std::shared_ptr <op_origin> else_origin,
	 std::shared_ptr <op> else_op)
    : m_upstream {upstream}
    , m_cond_origin {cond_origin}
    , m_cond_op {cond_op}
    , m_then_origin {then_origin}
    , m_then_op {then_op}
    , m_else_origin {else_origin}
    , m_else_op {else_op}
  {
    reset_me ();
  }

  void
  reset_me ()
  {
    m_sel_origin = nullptr;
    m_sel_op = nullptr;
  }

  stack::uptr
  do_next ()
  {
#if 0
    while (true)
      {
	if (m_sel_op == nullptr)
	  {
	    if (auto stk = m_upstream->next ())
	      {
		m_cond_op->reset ();
		m_cond_origin->set_next (std::make_unique <stack> (*stk));

		if (m_cond_op->next () != nullptr)
		  {
		    m_sel_origin = m_then_origin;
		    m_sel_op = m_then_op;
		  }
		else
		  {
		    m_sel_origin = m_else_origin;
		    m_sel_op = m_else_op;
		  }

		m_sel_op->reset ();
		m_sel_origin->set_next (std::move (stk));
	      }
	    else
	      return nullptr;
	  }

	if (auto stk = m_sel_op->next ())
	  return stk;

	reset_me ();
      }
#else
  return nullptr;
#endif
  }

  stack::uptr
  next ()
  {
    std::cerr << this << " op_ifelse next" << std::endl;
    auto ret = do_next ();
    std::cerr << this << " /op_ifelse next" << std::endl;
    return ret;
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_ifelse::op_ifelse (std::shared_ptr <op> upstream,
		      std::shared_ptr <op_origin> cond_origin,
		      std::shared_ptr <op> cond_op,
		      std::shared_ptr <op_origin> then_origin,
		      std::shared_ptr <op> then_op,
		      std::shared_ptr <op_origin> else_origin,
		      std::shared_ptr <op> else_op)
  : m_pimpl {std::make_unique <pimpl> (upstream, cond_origin, cond_op,
				       then_origin, then_op,
				       else_origin, else_op)}
{}

op_ifelse::~op_ifelse ()
{}

void
op_ifelse::reset ()
{
  m_pimpl->reset ();
}

stack::uptr
op_ifelse::next ()
{
  return m_pimpl->next ();
}

std::string
op_ifelse::name () const
{
  return "ifelse";
}


pred_result
pred_not::result (stack &stk)
{
  return ! m_a->result (stk);
}

std::string
pred_not::name () const
{
  return std::string ("not<") + m_a->name () + ">";
}


pred_result
pred_and::result (stack &stk)
{
  return m_a->result (stk) && m_b->result (stk);
}

std::string
pred_and::name () const
{
  return std::string ("and<") + m_a->name () + "><" + m_b->name () + ">";
}


pred_result
pred_or::result (stack &stk)
{
  return m_a->result (stk) || m_b->result (stk);
}

std::string
pred_or::name () const
{
  return std::string ("or<") + m_a->name () + "><" + m_b->name () + ">";
}

pred_result
pred_subx_any::result (stack &stk)
{
#if 0
  m_op->reset ();
  m_origin->set_next (std::make_unique <stack> (stk));
  if (m_op->next () != nullptr)
    return pred_result::yes;
  else
    return pred_result::no;
#else
  return pred_result::no;
#endif
}

std::string
pred_subx_any::name () const
{
  return std::string ("pred_subx_any<") + m_op->name () + ">";
}

void
pred_subx_any::reset ()
{
  m_op->reset ();
}


pred_result
pred_subx_compare::result (stack &stk)
{
#if 0
  m_op1->reset ();
  m_origin->set_next (std::make_unique <stack> (stk));
  while (auto stk_1 = m_op1->next ())
    {
      m_op2->reset ();
      m_origin->set_next (std::make_unique <stack> (stk));

      while (auto stk_2 = m_op2->next ())
	{
	  stk_1->push (stk_2->pop ());

	  if (m_pred->result (*stk_1) == pred_result::yes)
	    return pred_result::yes;

	  stk_1->pop ();
	}
    }

  return pred_result::no;
#else
  return pred_result::no;
#endif
}

std::string
pred_subx_compare::name () const
{
  return std::string ("pred_subx_compare<") + m_op1->name () + "><"
    + m_op2->name () + "><" + m_pred->name () + ">";
}

void
pred_subx_compare::reset ()
{
  m_op1->reset ();
  m_op2->reset ();
  m_pred->reset ();
}

pred_result
pred_pos::result (stack &stk)
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
