#ifndef _DWGREP_H_
#define _DWGREP_H_

#include <memory>
#include <vector>
#include <map>

#include <elfutils/libdw.h> // XXX

struct parent_cache
{
  static Dwarf_Off const none_off = (Dwarf_Off) -1;

  enum unit_type
    {
      UT_INFO,
    };

private:
  struct unit_key
  {
    unit_type ut;
    Dwarf_Off off;

    bool
    operator< (unit_key other) const
    {
      return ut < other.ut || (ut == other.ut && off < other.off);
    }
  };

  typedef std::vector <std::pair <Dwarf_Off, Dwarf_Off> > unit_cache_t;
  typedef std::map <unit_key, unit_cache_t> cache_t;

  std::shared_ptr <Dwarf> m_dw;
  cache_t m_cache;

  void recursively_populate_unit (unit_cache_t &uc, Dwarf_Die die,
				  Dwarf_Off paroff);

  unit_cache_t populate_unit (Dwarf_Die die);

public:
  explicit parent_cache (std::shared_ptr <Dwarf> dw)
    : m_dw (dw)
  {}

  Dwarf_Off find (Dwarf_Die die);
};

// A dwgrep_graph object represents a graph that we want to explore,
// and any associated caches.
struct dwgrep_graph
{
  typedef std::shared_ptr <dwgrep_graph> sptr;
  std::shared_ptr <Dwarf> dwarf;
  parent_cache parcache;

  dwgrep_graph (std::shared_ptr <Dwarf> d)
    : dwarf (d)
    , parcache (d)
  {}
};

class dwgrep_expr
{
  class pimpl;
  std::unique_ptr <pimpl> m_pimpl;

public:
  class result;

  explicit dwgrep_expr (std::string const &str);
  ~dwgrep_expr ();

  result query (std::shared_ptr <dwgrep_graph> p);
  result query (dwgrep_graph p);
};

class dwgrep_expr::result
{
  friend class dwgrep_expr;
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

class dwgrep_expr::result::iterator
{
  friend class dwgrep_expr::result;
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
