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

#ifndef _OP_H_
#define _OP_H_

#include <memory>
#include <cassert>
#include <iostream>

#include "stack.hh"
#include "pred_result.hh"
#include "rendezvous.hh"
#include "layout.hh"
#include "scon.hh"

// Subclasses of class op represent computations.  An op node is
// typically constructed such that it directly feeds from another op
// node, called upstream (see tree::build_exec).
class op
{
public:
  virtual ~op () {}

  // Produce next value.
  virtual stack::uptr next () { assert (!"op::next called"); abort (); }
  virtual void reset () { assert (!"op::reset called"); abort (); }
  virtual std::string name () const = 0;

  // xxx find a way to encapsulate the state_con / state_des calls in a managed
  // object.
  virtual void state_con (scon2 &sc) const = 0;
  virtual void state_des (scon2 &sc) const = 0;
  virtual stack::uptr next (scon2 &sc) const { return nullptr; }
};

template <class RT>
struct value_producer
{
  virtual ~value_producer () {}

  // Produce next value.
  virtual std::unique_ptr <RT> next () = 0;
};

template <class RT>
struct value_producer_cat
  : public value_producer <RT>
{
  std::vector <std::unique_ptr <value_producer <RT>>> m_vprs;
  size_t m_i;

  value_producer_cat ()
    : m_i {0}
  {}

  template <class... Ts>
  value_producer_cat (std::unique_ptr <value_producer <RT>> vpr1,
		      std::unique_ptr <Ts>... vprs)
    : value_producer_cat {std::move (vprs)...}
  {
    m_vprs.insert (m_vprs.begin (), std::move (vpr1));
  }

  std::unique_ptr <RT>
  next () override
  {
    for (; m_i < m_vprs.size (); ++m_i)
      if (auto v = m_vprs[m_i]->next ())
	return v;
    return nullptr;
  }
};

// An op that's not an origin has an upstream.
// xxx drop it, doesn't look it encaps useful behavior
class inner_op
  : public op
{
protected:
  std::shared_ptr <op> m_upstream;

public:
  inner_op (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  void state_con (scon2 &sc) const override
  { m_upstream->state_con (sc); }

  void state_des (scon2 &sc) const override
  { m_upstream->state_des (sc); }
};

// Class pred is for holding predicates.  These don't alter the
// computations at all.
class pred
{
public:
  virtual pred_result result (scon2 &sc, stack &stk) const = 0;
  virtual std::string name () const = 0;
  virtual ~pred () {}
};

// Origin is upstream-less node that is placed at the beginning of the
// chain of computations.  It's provided a stack from the outside by
// way of set_next.  Its only action is to send this stack down from
// next() upon request.
//
// The way origin is used is that the chain is moved to initial state
// by calling reset.  That reset eventually propagates to origin.
// set_next can be caled then.  set_next assumes that reset was
// called, and aborts if it didn't propagate all the way through
// (i.e. there was a buggy op on the way).
//
// Several operations use origin to handle sub-expressions
// (e.g. op_capture and pred_subx_any).
class op_origin
  : public op
{
  struct state;
  layout::loc m_ll;

public:
  explicit op_origin (layout &l);

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;

  void set_next (scon2 &sc, stack::uptr s) const;
};

struct stub_op
  : public inner_op
{
  using inner_op::inner_op;
  std::string name () const override final { return "stub"; }
};

struct stub_pred
  : public pred
{
  std::string name () const override final { return "stub"; }
};

struct op_nop
  : public inner_op
{
  explicit op_nop (std::shared_ptr <op> upstream)
    : inner_op {upstream}
  {}

  std::string name () const override;
  stack::uptr next (scon2 &sc) const override;
};

class op_assert
  : public inner_op
{
  std::unique_ptr <pred> m_pred;

public:
  op_assert (std::shared_ptr <op> upstream, std::unique_ptr <pred> p)
    : inner_op {upstream}
    , m_pred {std::move (p)}
  {}

  std::string name () const override;
  stack::uptr next (scon2 &sc) const override;
};

// The stringer hieararchy supports op_format, which implements
// formatting strings.  They are written similarly to op's, except
// they send along next() a work-in-progress string in addition to
// stack::ptr.  The stack::ptr is in-place mutated by the
// stringers, and when it gets all the way through, op_format takes
// whatever's left, and puts the finished string on top.  This scheme
// is similar to how pred_subx_any is written, except there we never
// mutate the passed-in stack.  But here we do, as per the language
// spec.
class stringer
{
public:
  virtual ~stringer () {}
  virtual void state_con (scon2 &sc) const = 0;
  virtual void state_des (scon2 &sc) const = 0;
  virtual std::pair <stack::uptr, std::string> next (scon2 &sc) const = 0;
};

// The formatting starts here.  This uses a pattern similar to
// pred_subx_any, where op_format knows about the origin, passes it a
// value that it gets, and then lets it percolate through the chain of
// stringers.
class stringer_origin
  : public stringer
{
  struct state;
  layout::loc m_ll;

public:
  explicit stringer_origin (layout &l);

  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  std::pair <stack::uptr, std::string> next (scon2 &sc) const override;

  void set_next (scon2 &sc, stack::uptr s);
};

// A stringer for the literal parts (non-%s, non-%(%)) of the format
// string.
class stringer_lit
  : public stringer
{
  std::shared_ptr <stringer> m_upstream;
  std::string m_str;

public:
  stringer_lit (std::shared_ptr <stringer> upstream, std::string str)
    : m_upstream {std::move (upstream)}
    , m_str {str}
  {}

  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  std::pair <stack::uptr, std::string> next (scon2 &sc) const override;
};

// A stringer for operational parts (%s, %(%)) of the format string.
class stringer_op
  : public stringer
{
  struct state;
  std::shared_ptr <stringer> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  layout::loc m_ll;

public:
  stringer_op (layout &l,
	       std::shared_ptr <stringer> upstream,
	       std::shared_ptr <op_origin> origin,
	       std::shared_ptr <op> op);

  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  std::pair <stack::uptr, std::string> next (scon2 &sc) const override;
};

// A top-level format-string node.
class op_format
  : public inner_op
{
  class state;
  std::shared_ptr <stringer_origin> m_origin;
  std::shared_ptr <stringer> m_stringer;
  layout::loc m_ll;

public:
  op_format (layout &l,
	     std::shared_ptr <op> upstream,
	     std::shared_ptr <stringer_origin> origin,
	     std::shared_ptr <stringer> stringer);

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;
};

class op_const
  : public inner_op
{
  std::unique_ptr <value> m_value;

public:
  op_const (std::shared_ptr <op> upstream, std::unique_ptr <value> &&value)
    : inner_op (upstream)
    , m_value {std::move (value)}
  {}

  std::string name () const override;
  stack::uptr next (scon2 &sc) const override;
};

// Tine is placed at the beginning of each alt expression.  These
// tines together share a vector of stacks, called a file, which
// next() takes data from (each vector element belongs to one tine of
// the overall alt).
//
// A tine yields nullptr if the file slot corresponding to its branch
// has already been fetched.  It won't refill the file unless all
// other tines have fetched as well (i.e. the file is empty).  Since
// the file is shared, it's not important which tine does the re-fill,
// they will all see the same data.
//
// Tine and merge need to cooperate to make sure nullptr's don't get
// propagated unless there's really nothing left.
//
// Thus the way the expression (A (B, C D, E) F) is constructed as:
//
//  [ A ] <------------------------------+
//  [ Tine 1 ] <-     [ B ]      <-+     |
//  [ Tine 2 ] <- [ C ] <- [ D ] <-+- [ Merge ] <- [ F ]
//  [ Tine 3 ] <-     [ E ]      <-+
class op_merge;
class op_tine
  : public op
{
  friend class op_merge;

  op_merge &m_merge;
  size_t m_branch_id;

public:
  op_tine (op_merge &merge, size_t branch_id)
    : m_merge {merge}
    , m_branch_id {branch_id}
  {}

  std::string name () const override;
  void state_con (scon2 &sc) const override {}
  void state_des (scon2 &sc) const override {}
  stack::uptr next (scon2 &sc) const override;
};

class op_merge
  : public op
{
  friend class op_tine;

public:
  typedef std::vector <std::shared_ptr <op>> opvec_t;

private:
  struct state;
  std::shared_ptr <op> m_upstream;
  layout::loc m_ll;
  opvec_t m_ops;

  state &get_state (scon2 &sc) const;
  op &get_upstream () const;

public:
  op_merge (layout &l, std::shared_ptr <op> upstream);

  void
  add_branch (std::shared_ptr <op> branch)
  {
    m_ops.push_back (branch);
  }

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;
};

class op_or
  : public inner_op
{
  struct state;
  std::vector <std::pair <std::shared_ptr <op_origin>,
			  std::shared_ptr <op>>> m_branches;
  layout::loc m_ll;

public:
  op_or (layout &l, std::shared_ptr <op> upstream);

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;

  void
  add_branch (std::shared_ptr <op_origin> origin,
	      std::shared_ptr <op> op)
  {
    m_branches.push_back (std::make_pair (origin, op));
  }
};

class op_capture
  : public inner_op
{
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;

public:
  op_capture (std::shared_ptr <op> upstream,
	      std::shared_ptr <op_origin> origin,
	      std::shared_ptr <op> op)
    : inner_op (upstream)
    , m_origin {origin}
    , m_op {op}
  {}

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;
};


enum class op_tr_closure_kind
  {
    plus, // transitive
    star, // transitive, reflective
  };

class op_tr_closure
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_tr_closure (std::shared_ptr <op> upstream,
		 std::shared_ptr <op_origin> origin,
		 std::shared_ptr <op> op,
		 op_tr_closure_kind k);

  ~op_tr_closure ();

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;

  void state_con (scon2 &sc) const override { assert (! "op_trclos"); }
  void state_des (scon2 &sc) const override { assert (! "op_trclos"); }
  stack::uptr next (scon2 &sc) const { return nullptr; }
};

class op_subx
  : public op
{
  class state;

  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  size_t m_keep;
  layout::loc m_ll;

public:
  op_subx (layout &l,
	   std::shared_ptr <op> upstream,
	   std::shared_ptr <op_origin> origin,
	   std::shared_ptr <op> op,
	   size_t keep);

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;
};

class op_f_debug
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_f_debug (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;

  void state_con (scon2 &sc) const override { assert (! "op_debug "); }
  void state_des (scon2 &sc) const override { assert (! "op_debug "); }
  stack::uptr next (scon2 &sc) const { return nullptr; }
};

class op_bind
  : public inner_op
{
  struct state;
  layout::loc m_ll;

public:
  explicit op_bind (layout &l, std::shared_ptr <op> upstream);

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;

  // xxx devirtualize
  virtual std::unique_ptr <value> current (scon2 &sc) const;
};

class op_read
  : public inner_op
{
  class state;
  op_bind &m_src;
  layout::loc m_ll;

public:
  op_read (layout &l, std::shared_ptr <op> upstream, op_bind &src);

  std::string name () const override;
  void state_con (scon2 &sc) const override;
  void state_des (scon2 &sc) const override;
  stack::uptr next (scon2 &sc) const override;
};

// ---------- closure support -----------------

// Pseudo bind plays the role of a bind for lexical blocks. It knows about the
// actual binding that it's emulating, also about the rendezvous cell where it
// reads what closure instance is currently under evaluation, and has a number
// that it uses to find current bound value inside the closure value referenced
// by the rendezvous.
class pseudo_bind
  : public op_bind
// xxx make op_bind a subclass of an ABC. Rename pseudo_bind to something else.
{
  rendezvous m_rdv;
  std::shared_ptr <op_bind> m_src;
  unsigned m_id;

public:
  pseudo_bind (layout &l, rendezvous rdv,
	       std::shared_ptr <op_bind> src, unsigned id)
    : op_bind {l, nullptr}
    , m_rdv {rdv}
    , m_src {src}
    , m_id {id}
  {}

  std::unique_ptr <value> fetch (scon2 &sc, unsigned assert_id) const;
  std::unique_ptr <value> current (scon2 &sc) const override;
};

struct op_lex_closure
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  std::vector <std::shared_ptr <pseudo_bind>> m_pseudos;
  rendezvous m_rdv;

public:
  op_lex_closure (std::shared_ptr <op> upstream,
		  std::shared_ptr <op_origin> origin,
		  std::shared_ptr <op> op,
		  std::vector <std::shared_ptr <pseudo_bind>> pseudos,
		  rendezvous rdv)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
    , m_pseudos {pseudos}
    , m_rdv {rdv}
  {}

  void reset () override;
  stack::uptr next () override;
  std::string name () const override;

  void state_con (scon2 &sc) const override { assert (! "op_lexclo"); }
  void state_des (scon2 &sc) const override { assert (! "op_lexclo"); }
  stack::uptr next (scon2 &sc) const { return nullptr; }
};

class op_ifelse
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_ifelse (std::shared_ptr <op> upstream,
	     std::shared_ptr <op_origin> cond_origin,
	     std::shared_ptr <op> cond_op,
	     std::shared_ptr <op_origin> then_origin,
	     std::shared_ptr <op> then_op,
	     std::shared_ptr <op_origin> else_origin,
	     std::shared_ptr <op> else_op);

  ~op_ifelse ();

  void reset () override;
  stack::uptr next () override;
  std::string name () const override;

  void state_con (scon2 &sc) const override { assert (! "op_ifelse"); }
  void state_des (scon2 &sc) const override { assert (! "op_ifelse"); }
  stack::uptr next (scon2 &sc) const { return nullptr; }
};


class pred_not
  : public pred
{
  std::unique_ptr <pred> m_a;

public:
  explicit pred_not (std::unique_ptr <pred> a)
    : m_a {std::move (a)}
  {}

  pred_result result (scon2 &sc, stack &stk) const override;
  std::string name () const override;
};

class pred_and
  : public pred
{
  std::unique_ptr <pred> m_a;
  std::unique_ptr <pred> m_b;

public:
  pred_and (std::unique_ptr <pred> a, std::unique_ptr <pred> b)
    : m_a {std::move (a)}
    , m_b {std::move (b)}
  {}

  pred_result result (scon2 &sc, stack &stk) const override;
  std::string name () const override;
};

class pred_or
  : public pred
{
  std::unique_ptr <pred> m_a;
  std::unique_ptr <pred> m_b;

public:
  pred_or (std::unique_ptr <pred> a, std::unique_ptr <pred> b)
    : m_a { std::move (a) }
    , m_b { std::move (b) }
  {}

  pred_result result (scon2 &sc, stack &stk) const override;
  std::string name () const override;
};

class pred_subx_any
  : public pred
{
  std::shared_ptr <op> m_op;
  std::shared_ptr <op_origin> m_origin;

public:
  pred_subx_any (std::shared_ptr <op> op,
		 std::shared_ptr <op_origin> origin)
    : m_op {op}
    , m_origin {origin}
  {}

  pred_result result (scon2 &sc, stack &stk) const override;
  std::string name () const override;
};

class pred_subx_compare
  : public pred
{
  std::shared_ptr <op> m_op1;
  std::shared_ptr <op> m_op2;
  std::shared_ptr <op_origin> m_origin;
  std::unique_ptr <pred> m_pred;

public:
  pred_subx_compare (std::shared_ptr <op> op1,
		     std::shared_ptr <op> op2,
		     std::shared_ptr <op_origin> origin,
		     std::unique_ptr <pred> pred)
    : m_op1 {op1}
    , m_op2 {op2}
    , m_origin {origin}
    , m_pred {std::move (pred)}
  {}

  pred_result result (scon2 &sc, stack &stk) const override;
  std::string name () const override;
};

class pred_pos
  : public pred
{
  size_t m_pos;

public:
  explicit pred_pos (size_t pos)
    : m_pos (pos)
  {}

  pred_result result (scon2 &sc, stack &stk) const override;
  std::string name () const override;
};

#endif /* _OP_H_ */
