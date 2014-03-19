#ifndef _OP_H_
#define _OP_H_

#include <memory>
#include <cassert>
#include "value.hh"

class stack
{
  std::unique_ptr <std::unique_ptr <value> []> m_slots;

public:
  explicit stack (size_t sz)
    : m_slots { new std::unique_ptr <value> [sz] }
  {}
  ~stack () = default;

  stack (stack const &that) = delete;
  stack (stack &&that) = delete;

  // Create a new stack by cloning this one.  This one doesn't know
  // how many slots it holds, so that's communicated by SZ.
  std::unique_ptr <stack>
  clone (size_t sz) const
  {
    std::unique_ptr <stack> ret { new stack { sz } };
    for (size_t i = 0; i < sz; ++i)
      if (m_slots[i] != nullptr)
	ret->set_slot (i, std::move (m_slots[i]->clone ()));

    return std::move (ret);
  }

  value const &
  get_slot (size_t i) const
  {
    auto const &vp = m_slots[i];
    assert (vp.get () != nullptr);
    return *vp;
  }

  void
  set_slot (size_t i, std::unique_ptr <value> v)
  {
    assert (v.get () != nullptr);
    m_slots[i].swap (v);
  }

  void
  invalidate_slot (size_t i)
  {
    auto &vp = m_slots[i];
    assert (vp.get () != nullptr);
    vp.release ();
  }
};

class yielder
{
public:
  virtual std::unique_ptr <stack> yield (stack const &stk, size_t stksz) = 0;
  virtual ~yielder () {}
};

// Class pred is for holding predicates.  These don't alter the
// computations at all.
class pred;

// Class op is for computation-altering operations.
class op
{
public:
  virtual ~op () {}
  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const = 0;
};

class op_sel_universe
  : public op
{
  size_t m_idx;
public:
  explicit op_sel_universe (size_t idx) : m_idx (idx) {}
  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_cat
  : public op
{
  std::unique_ptr <op> m_a;
  std::unique_ptr <op> m_b;

public:
  op_cat (std::unique_ptr <op> a, std::unique_ptr <op> b)
    : m_a (std::move (a))
    , m_b (std::move (b))
  {}

  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_nop
  : public op
{
public:
  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_assert
  : public op
{
  std::unique_ptr <pred> m_pred;

public:
  explicit op_assert (std::unique_ptr <pred> p)
    : m_pred (std::move (p))
  {}

  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_f_atval
  : public op
{
  int m_name;
  size_t m_idx;

public:
  op_f_atval (int name, size_t idx)
    : m_name (name)
    , m_idx (idx)
  {}

  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_format
  : public op
{
  std::string m_str;
  size_t m_idx;

public:
  op_format (std::string lit, size_t idx)
    : m_str (lit)
    , m_idx (idx)
  {}

  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_drop
  : public op
{
  size_t m_idx;

public:
  explicit op_drop (size_t idx)
    : m_idx (idx)
  {}

  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_const
  : public op
{
  std::unique_ptr <value> m_val;
  size_t m_idx;

public:
  op_const (std::unique_ptr <value> val, size_t idx)
    : m_val (std::move (val))
    , m_idx (idx)
  {}

  virtual std::unique_ptr <yielder> get_yielder
	(std::shared_ptr <Dwarf> &dw) const override;
};

class op_alt;
class op_capture;
class op_transform;
class op_protect;
class op_close; //+, *, ?
class op_const;
class op_str;
class op_atval;
class op_f_add;
class op_f_sub;
class op_f_mul;
class op_f_div;
class op_f_mod;
class op_f_parent;
class op_f_child;
class op_f_prev;
class op_f_next;
class op_f_type;
class op_f_offset;
class op_f_name;
class op_f_tag;
class op_f_form;
class op_f_value;
class op_f_pos;
class op_f_count;
class op_f_each;
class op_sel_section;
class op_sel_unit;

class pred
{
public:
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const = 0;
};

class pred_not
  : public pred
{
  std::unique_ptr <pred> m_a;

public:
  explicit pred_not (std::unique_ptr <pred> a)
    : m_a { std::move (a) }
  {}

  virtual pred_result
  result (std::shared_ptr <Dwarf> &dw,
	  stack const &stk, size_t stksz) const override
  {
    return ! m_a->result (dw, stk, stksz);
  }
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

  virtual pred_result
  result (std::shared_ptr <Dwarf> &dw,
	  stack const &stk, size_t stksz) const override
  {
    return m_a->result (dw, stk, stksz) && m_b->result (dw, stk, stksz);
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

  virtual pred_result
  result (std::shared_ptr <Dwarf> &dw,
	  stack const &stk, size_t stksz) const override
  {
    return m_a->result (dw, stk, stksz) || m_b->result (dw, stk, stksz);
  }
};

class pred_at
  : public pred
{
  int m_atname;
  size_t m_idx;

public:
  explicit pred_at (int atname, size_t idx)
    : m_atname (atname)
    , m_idx (idx)
  {}

  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

// Mainly useful as a placeholder predicate for those OP's where we
// couldn't promote anything.
class pred_true
  : public pred
{
public:
  virtual pred_result
  result (std::shared_ptr <Dwarf> &dw,
	  stack const &stk, size_t stksz) const override
  {
    return pred_result (true);
  }
};

class pred_tag
  : public pred
{
  int m_tag;
  size_t m_idx;

public:
  pred_tag (int tag, size_t idx)
    : m_tag (tag)
    , m_idx (idx)
  {}

  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

class pred_binary
  : public pred
{
  size_t m_idx_a;
  size_t m_idx_b;

public:
  pred_binary (size_t idx_a, size_t idx_b)
    : m_idx_a (idx_a)
    , m_idx_b (idx_b)
  {}

  value const &get_a (stack const &stk) const;
  value const &get_b (stack const &stk) const;
};

class pred_unary
  : public pred
{
  size_t m_idx_a;

public:
  explicit pred_unary (size_t idx_a)
    : m_idx_a (idx_a)
  {}

  value const &get_a (stack const &stk) const;
};

class pred_eq
  : public pred_binary
{
public:
  using pred_binary::pred_binary;
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

class pred_lt
  : public pred_binary
{
public:
  using pred_binary::pred_binary;
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

class pred_gt
  : public pred_binary
{
public:
  using pred_binary::pred_binary;
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

class pred_root
  : public pred_unary
{
public:
  using pred_unary::pred_unary;
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

class pred_subx_any
  : public pred
{
  std::unique_ptr <op> m_op;

public:
  explicit pred_subx_any (std::unique_ptr <op> op)
    : m_op (std::move (op))
  {}
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk, size_t stksz) const override;
};

class pred_find;
class pred_match;
class pred_empty;

#endif /* _OP_H_ */
