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

#ifndef _OP_H_
#define _OP_H_

#include <memory>
#include <cassert>

#include "dwgrep.hh"
#include "stack.hh"
#include "pred_result.hh"
#include "tree.hh"

// Subclasses of class op represent computations.  An op node is
// typically constructed such that it directly feeds from another op
// node, called upstream (see tree::build_exec).
class op
{
public:
  virtual ~op () {}

  // Produce next value.
  virtual stack::uptr next () = 0;
  virtual void reset () = 0;
  virtual std::string name () const = 0;
};

struct value_producer
{
  virtual ~value_producer () {}

  // Produce next value.
  virtual std::unique_ptr <value> next () = 0;
};

struct value_producer_cat
  : public value_producer
{
  std::vector <std::unique_ptr <value_producer>> m_vprs;
  size_t m_i;

  value_producer_cat ()
    : m_i {0}
  {}

  template <class... Ts>
  value_producer_cat (std::unique_ptr <value_producer> vpr1,
		      std::unique_ptr <Ts>... vprs)
    : value_producer_cat {std::move (vprs)...}
  {
    m_vprs.insert (m_vprs.begin (), std::move (vpr1));
  }

  std::unique_ptr <value> next () override;
};

// An op that's not an origin has an upstream.
class inner_op
  : public op
{
protected:
  std::shared_ptr <op> m_upstream;

public:
  inner_op (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  void reset () override
  { m_upstream->reset (); }
};

// Class pred is for holding predicates.  These don't alter the
// computations at all.
class pred
{
public:
  virtual pred_result result (stack &stk) = 0;
  virtual std::string name () const = 0;
  virtual void reset () = 0;
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
// Several operation use origin to handle sub-expressions
// (e.g. op_capture and pred_subx_any).
class op_origin
  : public op
{
  stack::uptr m_stk;
  bool m_reset;

public:
  explicit op_origin (stack::uptr s)
    : m_stk (std::move (s))
    , m_reset (false)
  {}

  void set_next (stack::uptr s);

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;
};

struct stub_op
  : public inner_op
{
  stub_op (std::shared_ptr <op> upstream)
    : inner_op {upstream}
  {}

  std::string name () const override final { return "stub"; }
};

struct stub_pred
  : public pred
{
  std::string name () const override final { return "stub"; }
  void reset () override final {}
};

class op_nop
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_nop (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  stack::uptr next () override;
  std::string name () const override;

  void reset () override
  { m_upstream->reset (); }
};

class op_assert
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::unique_ptr <pred> m_pred;

public:
  op_assert (std::shared_ptr <op> upstream, std::unique_ptr <pred> p)
    : m_upstream {upstream}
    , m_pred {std::move (p)}
  {}

  stack::uptr next () override;
  std::string name () const override;

  void reset () override
  { m_upstream->reset (); }
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
  virtual std::pair <stack::uptr, std::string> next () = 0;
  virtual void reset () = 0;
};

// The formatting starts here.  This uses a pattern similar to
// pred_subx_any, where op_format knows about the origin, passes it a
// value that it gets, and then lets it percolate through the chain of
// stringers.
class stringer_origin
  : public stringer
{
  stack::uptr m_stk;
  bool m_reset;

public:
  stringer_origin ()
    : m_reset (false)
  {}

  void set_next (stack::uptr s);

  std::pair <stack::uptr, std::string> next () override;
  void reset () override;
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
    : m_upstream (std::move (upstream))
    , m_str (str)
  {}

  std::pair <stack::uptr, std::string> next () override;
  void reset () override;
};

// A stringer for operational parts (%s, %(%)) of the format string.
class stringer_op
  : public stringer
{
  std::shared_ptr <stringer> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  std::string m_str;
  bool m_have;

public:
  stringer_op (std::shared_ptr <stringer> upstream,
	       std::shared_ptr <op_origin> origin,
	       std::shared_ptr <op> op)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
    , m_have {false}
  {}

  std::pair <stack::uptr, std::string> next () override;
  void reset () override;
};

// A top-level format-string node.
class op_format
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_format (std::shared_ptr <op> upstream,
	     std::shared_ptr <stringer_origin> origin,
	     std::shared_ptr <stringer> stringer);

  ~op_format ();

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_const
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::unique_ptr <value> m_value;

public:
  op_const (std::shared_ptr <op> upstream, std::unique_ptr <value> &&value)
    : m_upstream {upstream}
    , m_value {std::move (value)}
  {}

  stack::uptr next () override;
  std::string name () const override;

  void reset () override
  { m_upstream->reset (); }
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
//          +- [ Tine 1 ] <-     [ B ]      <-+
//  [ A ] <-+- [ Tine 2 ] <- [ C ] <- [ D ] <-+- [ Merge ] <- [ F ]
//          +- [ Tine 3 ] <-     [ E ]      <-+
class op_tine
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <std::vector <stack::uptr>> m_file;
  std::shared_ptr <bool> m_done;
  size_t m_branch_id;

public:
  op_tine (std::shared_ptr <op> upstream,
	   std::shared_ptr <std::vector <stack::uptr>> file,
	   std::shared_ptr <bool> done,
	   size_t branch_id)
    : m_upstream {upstream}
    , m_file {file}
    , m_done {done}
    , m_branch_id {branch_id}
  {
    assert (m_branch_id < m_file->size ());
  }

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_merge
  : public op
{
public:
  typedef std::vector <std::shared_ptr <op> > opvec_t;

private:
  opvec_t m_ops;
  opvec_t::iterator m_it;
  std::shared_ptr <bool> m_done;

public:
  op_merge (opvec_t ops, std::shared_ptr <bool> done)
    : m_ops (ops)
    , m_it (m_ops.begin ())
    , m_done (done)
  {
    *m_done = false;
  }

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_or
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::vector <std::pair <std::shared_ptr <op_origin>,
			  std::shared_ptr <op>>> m_branches;
  decltype (m_branches)::const_iterator m_branch_it;

  void reset_me ();

public:
  op_or (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
    , m_branch_it {m_branches.end ()}
  {}

  void
  add_branch (std::shared_ptr <op_origin> origin,
	      std::shared_ptr <op> op)
  {
    assert (m_branch_it == m_branches.end ());
    m_branches.push_back (std::make_pair (origin, op));
    m_branch_it = m_branches.end ();
  }

  void reset () override;
  stack::uptr next () override;
  std::string name () const override;
};

class op_capture
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;

public:
  op_capture (std::shared_ptr <op> upstream,
	      std::shared_ptr <op_origin> origin,
	      std::shared_ptr <op> op)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
  {}

  void reset () override;
  stack::uptr next () override;
  std::string name () const override;
};

class op_tr_closure
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_tr_closure (std::shared_ptr <op> upstream,
		 std::shared_ptr <op_origin> origin,
		 std::shared_ptr <op> op);

  ~op_tr_closure ();

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_subx
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_subx (std::shared_ptr <op> upstream,
	   std::shared_ptr <op_origin> origin,
	   std::shared_ptr <op> op,
	   size_t keep);

  ~op_subx ();

  stack::uptr next () override;
  std::string name () const override;
  void reset () override;
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
};

class op_scope
  : public op
{
  struct pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_scope (std::shared_ptr <op> upstream,
	    std::shared_ptr <op_origin> origin,
	    std::shared_ptr <op> op,
	    size_t num_vars);
  ~op_scope ();

  stack::uptr next () override;
  void reset () override;
  std::string name () const override;
};

class op_bind
  : public op
{
  std::shared_ptr <op> m_upstream;
  size_t m_depth;
  var_id m_index;

public:
  op_bind (std::shared_ptr <op> upstream, size_t depth, var_id index)
    : m_upstream {upstream}
    , m_depth {depth}
    , m_index {index}
  {}

  stack::uptr next () override;
  void reset () override;
  std::string name () const override;
};

class op_read
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_read (std::shared_ptr <op> upstream, size_t depth, var_id index);
  ~op_read ();

  stack::uptr next () override;
  void reset () override;
  std::string name () const override;
};

// Push to TOS a value_closure.
class op_lex_closure
  : public op
{
  std::shared_ptr <op> m_upstream;
  tree m_t;

public:
  op_lex_closure (std::shared_ptr <op> upstream, tree t)
    : m_upstream {upstream}
    , m_t {t}
  {}

  void reset () override;
  stack::uptr next () override;
  std::string name () const override;
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
};


class pred_not
  : public pred
{
  std::unique_ptr <pred> m_a;

public:
  explicit pred_not (std::unique_ptr <pred> a)
    : m_a {std::move (a)}
  {}

  pred_result result (stack &stk) override;
  std::string name () const override;

  void reset () override
  { m_a->reset (); }
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

  pred_result result (stack &stk) override;
  std::string name () const override;

  void reset () override
  {
    m_a->reset ();
    m_b->reset ();
  }
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

  pred_result result (stack &stk) override;
  std::string name () const override;

  void reset () override
  {
    m_a->reset ();
    m_b->reset ();
  }
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

  pred_result result (stack &stk) override;
  std::string name () const override;
  void reset () override;
};

class pred_constant
  : public pred
{
  constant m_const;

public:
  explicit pred_constant (constant cst)
    : m_const {cst}
  {}

  pred_result result (stack &stk) override;
  std::string name () const override;

  void reset () override {}
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

  pred_result result (stack &stk) override;
  std::string name () const override;
  void reset () override;
};

#endif /* _OP_H_ */
