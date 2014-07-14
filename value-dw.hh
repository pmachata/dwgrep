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
  constant get_type_const () const override;
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
  constant get_type_const () const override;
  cmp_result cmp (value const &that) const override;
};


#endif /* _VALUE_DW_H_ */
