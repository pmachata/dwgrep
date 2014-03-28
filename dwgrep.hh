#ifndef _DWGREP_H_
#define _DWGREP_H_

#include <memory>
#include <vector>

// A problem object represents a graph that we want to explore, and
// any associated caches.
struct problem
{
  typedef std::shared_ptr <problem> ptr;
};

class dwgrep_expr_t
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  class result;

  explicit dwgrep_expr_t (std::string const &str);
  ~dwgrep_expr_t ();

  result query (std::shared_ptr <problem> p);
};

class dwgrep_expr_t::result
{
  friend class dwgrep_expr_t;
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

  explicit result (std::unique_ptr <pimpl> p);

public:
  ~result ();
  result (result &&that);
  result (result const &that) = delete;

  class iterator;
  iterator begin ();
  iterator end ();
};

class dwgrep_expr_t::result::iterator
{
  friend class dwgrep_expr_t::result;
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

  iterator (std::unique_ptr <pimpl> p);

public:
  iterator ();
  iterator (iterator const &other);
  ~iterator ();

  std::vector <int> operator* () const;

  iterator operator++ ();
  iterator operator++ (int);

  bool operator== (iterator that);
  bool operator!= (iterator that);
};

#endif /* _DWGREP_H_ */
