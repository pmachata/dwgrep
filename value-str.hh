#ifndef _VALUE_STR_H_
#define _VALUE_STR_H_

#include <vector>
#include "value.hh"
#include "op.hh"

class value_str
  : public value
{
  std::string m_str;

public:
  static value_type const vtype;

  value_str (std::string &&str, size_t pos)
    : value {vtype, pos}
    , m_str {std::move (str)}
  {}

  std::string const &get_string () const
  { return m_str; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};

struct op_add_str
  : public stub_op
{
  using stub_op::stub_op;
  valfile::uptr next () override;
};

struct op_length_str
  : public stub_op
{
  using stub_op::stub_op;
  valfile::uptr next () override;
};

struct op_elem_str
  : public inner_op
{
  struct state;
  std::unique_ptr <state> m_state;

  op_elem_str (std::shared_ptr <op> upstream);
  ~op_elem_str ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

#endif /* _VALUE_STR_H_ */
