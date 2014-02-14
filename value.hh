#ifndef _VALUE_H_
#define _VALUE_H_

#include <string>
#include <memory>
#include <vector>

#include <libdw.h>

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
  explicit dieref (Dwarf_Off offset);
  explicit dieref (Dwarf_Die *die);

  Dwarf_Die die (std::shared_ptr <Dwarf> &dw) const;
  Dwarf_Off offset () const;

  bool operator< (dieref const &other) const;
};


// XXX Currently this is just a path.  Possibly we can extend to a
// full subgraph eventually.  We don't therefore track focus at all,
// it's always implicitly the last node.
class subgraph
  : public value
{
  mutable std::shared_ptr <Dwarf> m_dw;
  std::vector <dieref> m_dies;

public:
  explicit subgraph (std::shared_ptr <Dwarf> dw);

  subgraph (subgraph const &other) = default;
  subgraph (subgraph &&other);

  void add (dieref const &d);
  Dwarf_Die focus () const;

  std::vector <dieref>::const_iterator begin ();
  std::vector <dieref>::const_iterator end ();

  virtual std::string format () const;
};


class ivalue
  : public value
{
  long m_val;

public:
  virtual std::string format () const;
};


class fvalue
  : public value
{
  double m_val;

public:
  virtual std::string format () const;
};


class svalue
  : public value
{
  std::string m_str;

public:
  virtual std::string format () const;
};


class sequence
  : public value
{
  std::vector <std::unique_ptr <value> > m_seq;

public:
  virtual std::string format () const;
};

#endif /* _VALUE_H_ */
