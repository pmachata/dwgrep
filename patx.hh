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
  std::vector <std::unique_ptr <patx> > m_exprs;

public:
  void append (std::unique_ptr <patx> &&expr);
  std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws) override;
};


// Regex-like ?, {a,b}, *, +.  + and * have m_max of SIZE_MAX, which
// will realistically never get exhausted.
class patx_repeat
  : public patx
{
  std::unique_ptr <patx> m_patx;
  size_t m_min;
  size_t m_max;

public:
  patx_repeat (std::unique_ptr <patx> &&patx, size_t min, size_t max);
  std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws) override;
};


class patx_nodep
  : public patx
{
public:
  // Node predicate keeps only those values in working set that match
  // the predicate.
  std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws) final override;
  virtual bool match (value const &val) = 0;
};


class patx_nodep_any
  : public patx_nodep
{
  bool match (value const &val) override;
};


class patx_nodep_tag
  : public patx_nodep
{
  int const m_tag;
public:
  explicit patx_nodep_tag (int tag);
  bool match (value const &val) override;
};


class patx_nodep_hasattr
  : public patx_nodep
{
  int const m_attr;
public:
  explicit patx_nodep_hasattr (int attr);
  bool match (value const &val) override;
};


class patx_nodep_not
  : public patx_nodep
{
  std::unique_ptr <patx_nodep> m_op;
public:
  explicit patx_nodep_not (std::unique_ptr <patx_nodep> &&op);
  bool match (value const &val) override;
};


class patx_nodep_and
  : public patx_nodep
{
  std::unique_ptr <patx_nodep> m_lhs;
  std::unique_ptr <patx_nodep> m_rhs;

public:
  patx_nodep_and (std::unique_ptr <patx_nodep> &&lhs,
		  std::unique_ptr <patx_nodep> &&rhs);

  bool match (value const &val) override;
};


class patx_nodep_or
  : public patx_nodep
{
  std::unique_ptr <patx_nodep> m_lhs;
  std::unique_ptr <patx_nodep> m_rhs;

public:
  patx_nodep_or (std::unique_ptr <patx_nodep> &&lhs,
		 std::unique_ptr <patx_nodep> &&rhs);

  bool match (value const &val) override;
};


class patx_edgep_child
  : public patx
{
public:
  std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws) override;
};


class patx_edgep_attr
  : public patx
{
  int const m_attr;
public:
  explicit patx_edgep_attr (int attr);
  std::unique_ptr <wset> evaluate (std::unique_ptr <wset> &&ws) override;
};

#endif /* _PATX_H_ */
