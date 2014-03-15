#ifndef _VALUE_H_
#define _VALUE_H_

#include <string>
#include <memory>
#include <vector>

#include <elfutils/libdw.h>

class value
{
public:
  virtual ~value () {}
  virtual std::string format () const = 0;
  virtual std::unique_ptr <value> clone () const = 0;
};


class v_die
  : public value
{
  const Dwarf_Off m_offset;

public:
  explicit v_die (Dwarf_Off offset);
  explicit v_die (Dwarf_Die *die);

  Dwarf_Die die (std::shared_ptr <Dwarf> &dw) const;
  Dwarf_Off offset () const;

  virtual std::string format () const override;
  virtual std::unique_ptr <value> clone () const override;
};


class v_at
  : public value
{
  const Dwarf_Off m_dieoffset;
  const int m_atname;

public:
  v_at (Dwarf_Off offset, int atname);
  int atname () const;

  virtual std::string format () const override;
  virtual std::unique_ptr <value> clone () const override;
};


class v_locexpr
  : public value
{
public:
  virtual std::string format () const;
};


class v_int
  : public value
{
  long m_val;

public:
  virtual std::string format () const;
};


class v_flt
  : public value
{
  double m_val;

public:
  virtual std::string format () const;
};


class v_str
  : public value
{
  std::string m_str;

public:
  virtual std::string format () const;
};


class v_seq
  : public value
{
  std::vector <std::unique_ptr <value> > m_seq;

public:
  virtual std::string format () const;
};

#endif /* _VALUE_H_ */
