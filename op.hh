#ifndef _OP_H_
#define _OP_H_

#include <memory>
#include <cassert>

//#include "value.hh"
#include "make_unique.hh"
#include "dwgrep.hh"
#include "valfile.hh"
#include "pred_result.hh"

// Subclasses of class op represent computations.  An op node is
// typically constructed such that it directly feeds from another op
// node, called upstream (see tree::build_exec).
class op
{
public:
  virtual ~op () {}

  // Produce next value.
  virtual valfile::uptr next () = 0;
  virtual void reset () = 0;
  virtual std::string name () const = 0;
};

// Class pred is for holding predicates.  These don't alter the
// computations at all.
class pred;

// Origin is upstream-less node that is placed at the beginning of the
// chain of computations.  It's provided a valfile from the outside by
// way of set_next.  Its only action is to send this valfile down from
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
  valfile::uptr m_vf;
  bool m_reset;

public:
  explicit op_origin (valfile::uptr vf)
    : m_vf (std::move (vf))
    , m_reset (false)
  {}

  void set_next (valfile::uptr vf);

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_sel_winfo
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_sel_winfo (std::shared_ptr <op> upstream, dwgrep_graph::sptr q);
  ~op_sel_winfo ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_sel_unit
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_sel_unit (std::shared_ptr <op> upstream,
	       dwgrep_graph::sptr q);
  ~op_sel_unit ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_f_child
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_f_child (std::shared_ptr <op> upstream,
	       dwgrep_graph::sptr gr);
  ~op_f_child ();
  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_f_attr
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_f_attr (std::shared_ptr <op> upstream,
	     dwgrep_graph::sptr gr);
  ~op_f_attr ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_nop
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_nop (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
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

  valfile::uptr next () override;
  std::string name () const override;

  void reset () override
  { m_upstream->reset (); }
};

class op_f_value
  : public op
{
  std::shared_ptr <op> m_upstream;
  dwgrep_graph::sptr m_gr;

public:
  op_f_value (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
    : m_upstream {upstream}
    , m_gr {gr}
  {}

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_f_cast
  : public op
{
  std::shared_ptr <op> m_upstream;
  constant_dom const *m_dom;

public:
  op_f_cast (std::shared_ptr <op> upstream, constant_dom const *dom)
    : m_upstream {upstream}
    , m_dom {dom}
  {}

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class dwop_f
  : public op
{
  std::shared_ptr <op> m_upstream;

protected:
  dwgrep_graph::sptr m_g;

public:
  dwop_f (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
    : m_upstream {upstream}
    , m_g {gr}
  {}

  valfile::uptr next () override final;

  void reset () override final
  { m_upstream->reset (); }

  virtual std::string name () const override = 0;

  virtual bool operate (valfile &vf, Dwarf_Die &die)
  { return false; }

  virtual bool operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die)
  { return false; }
};

class op_f_attr_named
  : public dwop_f
{
  int m_name;

public:
  op_f_attr_named (std::shared_ptr <op> upstream, dwgrep_graph::sptr g,
		   int name)
    : dwop_f {upstream, g}
    , m_name {name}
  {}

  std::string name () const override;
  bool operate (valfile &vf, Dwarf_Die &die) override;
};

class op_f_offset
  : public dwop_f
{
public:
  using dwop_f::dwop_f;

  std::string name () const override;
  bool operate (valfile &vf, Dwarf_Die &die) override;
};

class op_f_name
  : public dwop_f
{
public:
  using dwop_f::dwop_f;

  std::string name () const override;
  bool operate (valfile &vf, Dwarf_Die &die) override;
  bool operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die) override;
};

class op_f_tag
  : public dwop_f
{
public:
  using dwop_f::dwop_f;

  std::string name () const override;
  bool operate (valfile &vf, Dwarf_Die &die) override;
};

class op_f_form
  : public dwop_f
{
public:
  using dwop_f::dwop_f;

  std::string name () const override;
  bool operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die) override;
};

class op_f_parent
  : public dwop_f
{
public:
  using dwop_f::dwop_f;

  std::string name () const override;
  bool operate (valfile &vf, Dwarf_Die &die) override;
  bool operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die) override;
};

// The stringer hieararchy supports op_format, which implements
// formatting strings.  They are written similarly to op's, except
// they send along next() a work-in-progress string in addition to
// valfile::ptr.  The valfile::ptr is in-place mutated by the
// stringers, and when it gets all the way through, op_format takes
// whatever's left, and puts the finished string on top.  This scheme
// is similar to how pred_subx_any is written, except there we never
// mutate the passed-in valfile.  But here we do, as per the language
// spec.
class stringer
{
public:
  virtual std::pair <valfile::uptr, std::string> next () = 0;
  virtual void reset () = 0;
};

// The formatting starts here.  This uses a pattern similar to
// pred_subx_any, where op_format knows about the origin, passes it a
// value that it gets, and then lets it percolate through the chain of
// stringers.
class stringer_origin
  : public stringer
{
  valfile::uptr m_vf;
  bool m_reset;

public:
  stringer_origin ()
    : m_reset (false)
  {}

  void set_next (valfile::uptr vf);

  std::pair <valfile::uptr, std::string> next () override;
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

  std::pair <valfile::uptr, std::string> next () override;
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

  std::pair <valfile::uptr, std::string> next () override;
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

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_drop
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_drop (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_dup
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_dup (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_swap
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_swap (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
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

  valfile::uptr next () override;
  std::string name () const override;

  void reset () override
  { m_upstream->reset (); }
};

// Tine is placed at the beginning of each alt expression.  These
// tines together share a vector of valfiles, called a file, which
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
  std::shared_ptr <std::vector <valfile::uptr> > m_file;
  std::shared_ptr <bool> m_done;
  size_t m_branch_id;

public:
  op_tine (std::shared_ptr <op> upstream,
	   std::shared_ptr <std::vector <valfile::uptr> > file,
	   std::shared_ptr <bool> done,
	   size_t branch_id)
    : m_upstream {upstream}
    , m_file {file}
    , m_done {done}
    , m_branch_id {branch_id}
  {
    assert (m_branch_id < m_file->size ());
  }

  valfile::uptr next () override;
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

  valfile::uptr next () override;
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
  valfile::uptr next () override;
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
  valfile::uptr next () override;
  std::string name () const override;
};

class op_f_each
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  explicit op_f_each (std::shared_ptr <op> upstream);
  ~op_f_each ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_f_length
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_f_length (std::shared_ptr <op> upstream)
    : m_upstream (upstream)
  {}

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class op_f_type
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_f_type (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
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

  valfile::uptr next () override;
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
	   std::shared_ptr <op> op);

  ~op_subx ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;

};

class op_f_add
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit op_f_add (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
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

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

class simple_op
  : public op
{
  std::shared_ptr <op> m_upstream;

public:
  explicit simple_op (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  valfile::uptr next () override;
  void reset () override;

  virtual std::unique_ptr <value> operate (value const &v) const = 0;
};

class op_f_pos
  : public simple_op
{
public:
  using simple_op::simple_op;

  std::unique_ptr <value> operate (value const &v) const override;
  std::string name () const override;
};

// Pop DEPTH slots, perform OP, and for each produced stack, push
// those slots back and yield that stack.
class op_transform
  : public op
{
  struct pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_transform (std::shared_ptr <op> upstream,
		std::shared_ptr <op_origin> origin,
		std::shared_ptr <op> op,
		unsigned depth);
  ~op_transform ();

  valfile::uptr next () override;
  void reset () override;
  std::string name () const override;
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

  valfile::uptr next () override;
  void reset () override;
  std::string name () const override;
};

class op_bind
  : public op
{
protected:
  std::shared_ptr <op> m_upstream;
  size_t m_depth;
  var_id m_index;

public:
  op_bind (std::shared_ptr <op> upstream, size_t depth, var_id index)
    : m_upstream {upstream}
    , m_depth {depth}
    , m_index {index}
  {}

  valfile::uptr next () override;
  void reset () override;
  std::string name () const override;
};

class op_read
  : public op_bind
{
public:
  using op_bind::op_bind;

  valfile::uptr next () override;
  std::string name () const override;
};

// Push to TOS a value_closure defined from given m_origin, m_op and
// current top scope.
class op_lex_closure
  : public op
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;

public:
  op_lex_closure (std::shared_ptr <op> upstream,
		  std::shared_ptr <op_origin> origin,
		  std::shared_ptr <op> op)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
  {}

  void reset () override;
  valfile::uptr next () override;
  std::string name () const override;
};

// Pop closure, execute it.
class op_apply
  : public op
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  op_apply (std::shared_ptr <op> upstream);
  ~op_apply ();

  void reset () override;
  valfile::uptr next () override;
  std::string name () const override;
};

class op_f_sub;
class op_f_mul;
class op_f_div;
class op_f_mod;
class op_f_prev;
class op_f_next;
class op_f_form;
class op_sel_section;

class pred
{
public:
  virtual pred_result result (valfile &vf) = 0;
  virtual std::string name () const = 0;
  virtual void reset () = 0;
};

class pred_not
  : public pred
{
  std::unique_ptr <pred> m_a;

public:
  explicit pred_not (std::unique_ptr <pred> a)
    : m_a {std::move (a)}
  {}

  pred_result result (valfile &vf) override;
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
    : m_a { std::move (a) }
    , m_b { std::move (b) }
  {}

  pred_result result (valfile &vf) override;
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

  pred_result result (valfile &vf) override;
  std::string name () const override;

  void reset () override
  {
    m_a->reset ();
    m_b->reset ();
  }
};

class pred_at
  : public pred
{
  unsigned m_atname;

public:
  explicit pred_at (unsigned atname)
    : m_atname (atname)
  {}

  pred_result result (valfile &vf) override;
  std::string name () const override;
  void reset () override {}
};

class pred_tag
  : public pred
{
  int m_tag;

public:
  explicit pred_tag (int tag)
    : m_tag (tag)
  {}

  pred_result result (valfile &vf) override;
  std::string name () const override;
  void reset () override {}
};

class pred_binary
  : public pred
{
public:
  pred_binary () {}
  void reset () override {}
};

class pred_eq
  : public pred_binary
{
public:
  using pred_binary::pred_binary;

  pred_result result (valfile &vf) override;
  std::string name () const override;
};

class pred_lt
  : public pred_binary
{
public:
  using pred_binary::pred_binary;

  pred_result result (valfile &vf) override;
  std::string name () const override;
};

class pred_gt
  : public pred_binary
{
public:
  using pred_binary::pred_binary;

  pred_result result (valfile &vf) override;
  std::string name () const override;
};

class pred_root
  : public pred
{
  dwgrep_graph::sptr m_g;

public:
  explicit pred_root (dwgrep_graph::sptr g);

  pred_result result (valfile &vf) override;
  std::string name () const override;
  void reset () override {}
};

class pred_subx_any
  : public pred
{
  std::shared_ptr <op> m_op;
  std::shared_ptr <op_origin> m_origin;

public:
  pred_subx_any (std::shared_ptr <op> op,
		 std::shared_ptr <op_origin> origin)
    : m_op (op)
    , m_origin (origin)
  {}

  pred_result result (valfile &vf) override;
  std::string name () const override;
  void reset () override;
};

class pred_empty
  : public pred
{
public:
  pred_empty () {}

  pred_result result (valfile &vf) override;
  std::string name () const override;
  void reset () override {}
};

class pred_match
  : public pred_binary
{
public:
    using pred_binary::pred_binary;

    pred_result result (valfile &vf) override;
    std::string name () const override;
};

class pred_find;


#endif /* _OP_H_ */
