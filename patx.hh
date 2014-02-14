#ifndef _PATX_H_
#define _PATX_H_

#include <vector>
#include <memory>
#include "wset.hh"

class patx
{
public:
  virtual std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws) = 0;
};

class patx_group
  : public patx
{
  std::vector <std::shared_ptr <patx> > m_exprs;

public:
  void append (std::shared_ptr <patx> expr);
  virtual std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws);
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
  virtual std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws);
  virtual bool match (value const &val) = 0;
};


class patx_nodep_any
  : public patx_nodep
{
  virtual bool match (value const &val);
};


class patx_nodep_tag
  : public patx_nodep
{
  int const m_tag;
public:
  explicit patx_nodep_tag (int tag);
  virtual bool match (value const &val);
};


class patx_nodep_hasattr
  : public patx_nodep
{
  int const m_attr;
public:
  explicit patx_nodep_hasattr (int attr);
  virtual bool match (value const &val);
};


class patx_nodep_not
  : public patx_nodep
{
  std::shared_ptr <patx_nodep> m_op;
public:
  explicit patx_nodep_not (std::shared_ptr <patx_nodep> op);
  virtual bool match (value const &val);
};


class patx_nodep_and
  : public patx_nodep
{
  std::shared_ptr <patx_nodep> m_lhs;
  std::shared_ptr <patx_nodep> m_rhs;

public:
  patx_nodep_and (std::shared_ptr <patx_nodep> lhs,
		  std::shared_ptr <patx_nodep> rhs);

  virtual bool match (value const &val);
};


class patx_nodep_or
  : public patx_nodep
{
  std::shared_ptr <patx_nodep> m_lhs;
  std::shared_ptr <patx_nodep> m_rhs;

public:
  patx_nodep_or (std::shared_ptr <patx_nodep> lhs,
		 std::shared_ptr <patx_nodep> rhs);

  virtual bool match (value const &val);
};


class patx_edgep_child
  : public patx
{
public:
  virtual std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws);
};


class patx_edgep_attr
  : public patx
{
  int const m_attr;
public:
  explicit patx_edgep_attr (int attr);
  virtual std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws);
};

#endif /* _PATX_H_ */
