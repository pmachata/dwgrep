#ifndef _DWGREP_H_
#define _DWGREP_H_

#include <memory>

class dwgrep_expr_t
{
  std::unique_ptr <expr_node> m_expr;

public:
  class result;

  dwgrep_expr_t ();
  result query (std::shared_ptr <problem> p);
};

class exec_node_query;

class dwgrep_expr_t::result
{
  std::shared_ptr <exec_node_query> m_q;

public:
  explicit result (std::shared_ptr <exec_node_query> q);
  ~result ();

  class iterator;
  iterator begin ();
  iterator end ();
};

class dwgrep_expr_t::result::iterator
{
  friend class dwgrep_expr_t::result;
  std::shared_ptr <exec_node_query> m_q;
  std::shared_ptr <valfile> m_vf;

  iterator (std::shared_ptr <exec_node_query> q);

  bool done () const;
  void fetch ();

public:
  iterator ();
  iterator (iterator const &other);

  std::shared_ptr <valfile> operator* () const;

  iterator operator++ ();
  iterator operator++ (int);

  bool operator== (iterator that);
  bool operator!= (iterator that);
};

#endif /* _DWGREP_H_ */
