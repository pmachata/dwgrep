#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <exception>
#include <memory>
#include <system_error>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <sstream>

#include <dwarf.h>
#include <libdw.h>
#include <libelf.h>
#include <boost/format.hpp>

#include "iterators.hh"

namespace
{
  void
  throw_libdw ()
  {
    int dwerr = dwarf_errno ();
    assert (dwerr != 0);
    throw std::runtime_error (dwarf_errmsg (dwerr));
  }

  void
  throw_libelf ()
  {
    int elferr = elf_errno ();
    assert (elferr != 0);
    throw std::runtime_error (elf_errmsg (elferr));
  }

  void
  throw_system ()
  {
    throw std::runtime_error
      (std::error_code (errno, std::system_category ()).message ());
  }
}

class value
{
public:
  virtual ~value () {}
  virtual std::string format () const = 0;
};

class dieref
{
  const Dwarf_Off m_offset;
public:
  explicit dieref (Dwarf_Off offset) : m_offset (offset) {}
  explicit dieref (Dwarf_Die *die) : m_offset (dwarf_dieoffset (die)) {}

  Dwarf_Die
  die (std::shared_ptr <Dwarf> &dw) const
  {
    Dwarf_Die die;
    if (dwarf_offdie (&*dw, m_offset, &die) == nullptr)
      throw_libdw ();
    return die;
  }

  Dwarf_Off
  offset () const
  {
    return m_offset;
  }
};

// XXX Currently this is just a path.  Possibly we can extend to a
// full subgraph eventually.  We don't therefore track focus at all.
class subgraph
  : public value
{
  mutable std::shared_ptr <Dwarf> m_dw;
  std::vector <dieref> m_dies;

public:
  explicit subgraph (std::shared_ptr <Dwarf> dw)
    : m_dw (dw)
  {}

  void
  add (dieref const &d)
  {
    m_dies.push_back (d);
  }

  Dwarf_Die
  focus () const
  {
    assert (m_dies.size () > 0);
    return m_dies.back ().die (m_dw);
  }

  virtual std::string
  format () const
  {
    std::stringstream ss;
    ss << "{" << std::hex;
    for (auto const &dr: m_dies)
      ss << dr.offset () << ", ";
    ss << "}";
    return ss.str ();
  }
};

class ivalue
  : public value
{
  long m_val;

public:
  virtual std::string
  format () const
  {
    return str (boost::format ("%d") % m_val);
  }
};

class fvalue
  : public value
{
  double m_val;

public:
  virtual std::string
  format () const
  {
    return str (boost::format ("%f") % m_val);
  }
};

class svalue
  : public value
{
  std::string m_str;

public:
  virtual std::string
  format () const
  {
    return m_str;
  }
};

class sequence
  : public value
{
  std::vector <std::unique_ptr <value> > m_seq;

public:
  virtual std::string
  format () const
  {
    std::stringstream ss;
    ss << "(";
    for (auto const &val: m_seq)
      ss << val->format () << ", ";
    ss << ")";
    return ss.str ();
  }
};

// ----

class wset
{
  std::vector <std::unique_ptr <value> > m_values;

public:
  static wset
  initial (std::shared_ptr <Dwarf> &dw)
  {
    wset ws;
    for (all_dies_iterator it {&*dw}; it != all_dies_iterator::end (); ++it)
      {
	Dwarf_Die *die = *it;
	auto val = std::unique_ptr <subgraph> (new subgraph (dw));
	val->add (dieref (die));
	ws.add (std::move (val));
      }
    return ws;
  }

  wset () {}

  void
  add (std::unique_ptr <value> &&val)
  {
    m_values.emplace_back (std::move (val));
  }

  size_t
  size () const
  {
    return m_values.size ();
  }

  std::vector <std::unique_ptr <value> >::iterator
  begin () { return std::begin (m_values); }

  std::vector <std::unique_ptr <value> >::iterator
  end () { return std::end (m_values); }

  std::vector <std::unique_ptr <value> >::const_iterator
  cbegin () const { return m_values.cbegin (); }

  std::vector <std::unique_ptr <value> >::const_iterator
  cend () const { return m_values.cend (); }
};

// ----

class patx
{
public:
  virtual std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &ws) = 0;
};

class patx_colon
  : public patx
{
  // Expressions in the colon.
  std::vector <std::shared_ptr <patx> > m_colon;

public:
  void
  append (std::shared_ptr <patx> expr)
  {
    m_colon.push_back (expr);
  }

  virtual std::unique_ptr <wset>
  evaluate (std::unique_ptr <wset> &ws)
  {
    std::unique_ptr <wset> tmp (std::move (ws));
    for (auto &pat: m_colon)
      tmp = pat->evaluate (tmp);

    return std::move (tmp);
  }
};

// Regex-like ?, {a,b}, *, +.  + and * have m_max of SIZE_MAX, which
// will realistically never get exhausted.
class patx_repeat
  : public patx
{
  size_t m_min;
  size_t m_max;
};

class patx_nodep
  : public patx
{
public:
  // Node predicate keeps only those values in working set that match
  // the predicate.
  virtual std::unique_ptr <wset>
  evaluate (std::unique_ptr <wset> &ws)
  {
    auto ret = std::unique_ptr <wset> (new wset ());
    for (auto &emt: *ws)
      if (match (*emt))
	ret->add (std::move (emt));
    return std::move (ret);
  }

  virtual bool match (value const &val) = 0;
};

class patx_nodep_any
  : public patx_nodep
{
  virtual bool
  match (value const &val)
  {
    return true;
  }
};

class patx_nodep_tag
  : public patx_nodep
{
  int const m_tag;
public:
  explicit patx_nodep_tag (int tag) : m_tag (tag) {}

  virtual bool
  match (value const &val)
  {
    if (auto g = dynamic_cast <subgraph const *> (&val))
      {
	Dwarf_Die die = g->focus ();
	std::cout << "  ($tag == " << std::dec << m_tag << ") on "
		  << std::hex << dwarf_dieoffset (&die)
		  << ", which is " << std::dec << dwarf_tag (&die) << std::endl;
	return dwarf_tag (&die) == m_tag;
      }
    else
      return false;
  }
};

class patx_nodep_hasattr
  : public patx_nodep
{
  int const m_attr;
public:
  explicit patx_nodep_hasattr (int attr) : m_attr (attr) {}

  virtual bool
  match (value const &val)
  {
    assert (! "patx_nodep_hasattr::match");
    abort ();
  }
};

class patx_nodep_not
  : public patx_nodep
{
  std::shared_ptr <patx_nodep> m_op;
public:
  explicit patx_nodep_not (std::shared_ptr <patx_nodep> op) : m_op (op) {}

  virtual bool
  match (value const &val)
  {
    return ! m_op->match (val);
  }
};

class patx_nodep_and
  : public patx_nodep
{
  std::shared_ptr <patx_nodep> m_lhs;
  std::shared_ptr <patx_nodep> m_rhs;
public:
  patx_nodep_and (std::shared_ptr <patx_nodep> lhs,
		  std::shared_ptr <patx_nodep> rhs)
    : m_lhs (lhs), m_rhs (rhs)
  {}

  virtual bool
  match (value const &val)
  {
    return m_lhs->match (val) && m_rhs->match (val);
  }
};

class patx_edgep
  : public patx
{
  // Edge predicate extends subgraphs in working set by following
  // matching edges.
  virtual std::unique_ptr <wset>
  evaluate (std::unique_ptr <wset> &ws)
  {
    std::cerr << "patx_edgep not implemented" << std::endl;
    return std::move (std::move (ws));
  }

  // XXX how to represent an edge for matching?
};

class patx_edgep_child
  : public patx_edgep
{
public:
};

std::shared_ptr <Dwarf>
open_dwarf (char const *fn)
{
  int fd = open (fn, O_RDONLY);
  if (fd == -1)
    throw_system ();

  Dwarf *dw = dwarf_begin (fd, DWARF_C_READ);
  if (dw == nullptr)
    throw_libdw ();

  return std::shared_ptr <Dwarf> (dw, dwarf_end);
}

int
main(int argc, char *argv[])
{
  elf_version (EV_CURRENT);

  auto x0 = std::make_shared <patx_colon> ();
  {
    {
      auto x1 = std::make_shared <patx_nodep_tag> (DW_TAG_subprogram);
      /*
      auto x2 = std::make_shared <patx_nodep_hasattr> (DW_AT_declaration);
      auto x3 = std::make_shared <patx_nodep_not> (x2);
      auto x4 = std::make_shared <patx_nodep_and> (x1, x3);
      x0->append (x4);
      */
      x0->append (x1);
    }
    /*
    x0->append (std::make_shared <patx_edgep_child> ());
    x0->append (std::make_shared <patx_nodep_tag> (DW_TAG_formal_parameter));
    */
  }

  assert (argc == 2);
  auto dw = open_dwarf (argv[1]);

  auto ws = std::unique_ptr <wset> (new wset (wset::initial (dw)));
  ws = x0->evaluate (ws);

  std::cout << "Result set has " << ws->size () << " elements." << std::endl;
  for (auto const &val: *ws)
    std::cout << "  " << val->format () << std::endl;

  /*
  something
    .match (tag (DW_TAG_subprogram) && ! hasattr (DW_AT_declaration))
    .match (child_edge)
    .match (tag (DW_TAG_formal_parameter);
  */

  return 0;
}
