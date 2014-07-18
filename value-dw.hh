#ifndef _VALUE_DW_H_
#define _VALUE_DW_H_

#include <elfutils/libdw.h>
#include "value.hh"

class value_die
  : public value
{
  Dwarf_Die m_die;
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_die (dwgrep_graph::sptr gr, Dwarf_Die die, size_t pos)
    : value {vtype, pos}
    , m_die (die)
    , m_gr {gr}
  {}

  value_die (value_die const &that) = default;

  Dwarf_Die &get_die ()
  { return m_die; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

class value_attr
  : public value
{
  Dwarf_Attribute m_attr;
  Dwarf_Die m_die;
  dwgrep_graph::sptr m_gr;

public:
  static value_type const vtype;

  value_attr (dwgrep_graph::sptr gr,
	      Dwarf_Attribute attr, Dwarf_Die die, size_t pos)
    : value {vtype, pos}
    , m_attr (attr)
    , m_die (die)
    , m_gr {gr}
  {}

  value_attr (value_attr const &that) = default;

  Dwarf_Attribute &get_attr ()
  { return m_attr; }

  Dwarf_Die &get_die ()
  { return m_die; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

struct value_loclist_op
  : public value
{
  int m_arity;
  constant m_atom;
  constant m_v1;
  constant m_v2;
  constant m_offset;

  static value_type const vtype;

  value_loclist_op (int arity, constant atom,
		    constant v1, constant v2, constant offset,
		    size_t pos)
    : value {vtype, pos}
    , m_arity {arity}
    , m_atom {atom}
    , m_v1 {v1}
    , m_v2 {v2}
    , m_offset {offset}
  {}

  value_loclist_op (value_loclist_op const &that) = default;

  constant &atom () { return m_atom; }
  constant &v1 () { return m_v1; }
  constant &v2 () { return m_v2; }
  constant &offset () { return m_offset; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

#endif /* _VALUE_DW_H_ */
