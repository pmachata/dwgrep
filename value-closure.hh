#ifndef _VALUE_CLOSURE_H_
#define _VALUE_CLOSURE_H_

#include "value.hh"

class tree;
class frame;
class scope;

class value_closure
  : public value
{
  std::unique_ptr <tree> m_t;
  dwgrep_graph::sptr m_q;
  std::shared_ptr <scope> m_scope;
  std::shared_ptr <frame> m_frame;

public:
  static value_type const vtype;

  value_closure (tree const &t, dwgrep_graph::sptr q,
		 std::shared_ptr <scope> scope, std::shared_ptr <frame> frame,
		 size_t pos);
  value_closure (value_closure const &that);
  ~value_closure();

  tree const &get_tree () const
  { return *m_t; }

  dwgrep_graph::sptr get_graph () const
  { return m_q; }

  std::shared_ptr <scope> get_scope () const
  { return m_scope; }

  std::shared_ptr <frame> get_frame () const
  { return m_frame; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

#endif /* _VALUE_CLOSURE_H_ */
