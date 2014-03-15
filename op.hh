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

class op_alt;
class op_capture;
class op_transform;
class op_protect;
class op_close; //+, *, ?
class op_const;
class op_str;
class op_format;
class op_atval;
class op_f_add;
class op_f_sub;
class op_f_mul;
class op_f_div;
class op_f_mod;
class op_f_parent;
class op_f_child;
class op_f_attribute;
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

enum class pred_result
  {
    no = false,
    yes = true,
    fail,
  };

inline pred_result
operator! (pred_result other)
{
  switch (other)
    {
    case pred_result::no:
      return pred_result::yes;
    case pred_result::yes:
      return pred_result::no;
    case pred_result::fail:
      return pred_result::fail;
    }
  abort ();
}

inline pred_result
operator&& (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) && bool (b) ? pred_result::yes : pred_result::no;
}

inline pred_result
operator|| (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) || bool (b) ? pred_result::yes : pred_result::no;
}

class pred
{
public:
  virtual pred_result result (std::shared_ptr <Dwarf> &dw,
			      stack const &stk) const = 0;
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
  result (std::shared_ptr <Dwarf> &dw, stack const &stk) const override
  {
    return ! m_a->result (dw, stk);
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
  result (std::shared_ptr <Dwarf> &dw, stack const &stk) const override
  {
    return m_a->result (dw, stk) && m_b->result (dw, stk);
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
  result (std::shared_ptr <Dwarf> &dw, stack const &stk) const override
  {
    return m_a->result (dw, stk) || m_b->result (dw, stk);
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
			      stack const &stk) const override;
};

// Mainly useful as a placeholder predicate for those OP's where we
// couldn't promote anything.
class pred_true
  : public pred
{
public:
  virtual pred_result
  result (std::shared_ptr <Dwarf> &dw, stack const &stk) const override
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
			      stack const &stk) const override;
};

class pred_eq;
class pred_ne;
class pred_gt;
class pred_ge;
class pred_lt;
class pred_le;
class pred_find;
class pred_match;
class pred_empty;
class pred_root;

#endif /* _OP_H_ */
