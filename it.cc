#include <memory>
#include <vector>
#include <iostream>
#include <cassert>
#include <algorithm>

#include "make_unique.hh"

struct valfile
  : public std::vector <int>
{
  typedef std::unique_ptr <valfile> ptr;
};

class prod
{
public:
  virtual valfile::ptr next () = 0;
};

class prod_init
  : public prod
{
  bool m_done;
public:
  prod_init ()
    : m_done (false)
  {}

  valfile::ptr
  next () override
  {
    if (m_done)
      return nullptr;
    m_done = true;
    return std::make_unique <valfile> ();
  }
};

class prod_unary
  : public prod
{
protected:
  std::shared_ptr <prod> m_prod;

public:
  prod_unary (std::shared_ptr <prod> prod)
    : m_prod (prod)
  {}
};

struct prod_uni
  : public prod_unary
{
  valfile::ptr m_vf;
  int i;

public:
  prod_uni (std::shared_ptr <prod> prod)
    : prod_unary (prod)
    , i (0)
  {}

  valfile::ptr
  next () override
  {
    while (true)
      {
	if (m_vf == nullptr)
	  m_vf = m_prod->next ();
	if (m_vf == nullptr)
	  return nullptr;
	if (i < 20)
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->push_back (i++);
	    return ret;
	  }
	i = 0;
	m_vf = nullptr;
      }
  }
};

struct prod_dup
  : public prod_unary
{
  using prod_unary::prod_unary;

  valfile::ptr
  next () override
  {
    if (auto vf = m_prod->next ())
      {
	assert (vf->size () > 0);
	vf->push_back (vf->back ());
	return vf;
      }
    return nullptr;
  }
};

struct prod_mul
  : public prod_unary
{
  using prod_unary::prod_unary;

  valfile::ptr
  next () override
  {
    if (auto vf = m_prod->next ())
      {
	assert (vf->size () >= 2);
	int a = vf->back (); vf->pop_back ();
	int b = vf->back (); vf->pop_back ();
	vf->push_back (a * b);
	return vf;
      }
    return nullptr;
  }
};

struct prod_add
  : public prod_unary
{
  using prod_unary::prod_unary;

  valfile::ptr
  next () override
  {
    if (auto vf = m_prod->next ())
      {
	assert (vf->size () >= 2);
	int a = vf->back (); vf->pop_back ();
	int b = vf->back (); vf->pop_back ();
	vf->push_back (a + b);
	return vf;
      }
    return nullptr;
  }
};

struct prod_odd
  : public prod_unary
{
  using prod_unary::prod_unary;

  valfile::ptr
  next () override
  {
    while (auto vf = m_prod->next ())
      if (vf->back () % 2 != 0)
	return vf;
    return nullptr;
  }
};

struct prod_even
  : public prod_unary
{
  using prod_unary::prod_unary;

  valfile::ptr
  next () override
  {
    while (auto vf = m_prod->next ())
      if (vf->back () % 2 == 0)
	return vf;
    return nullptr;
  }
};

// Tine is placed at the beginning of each fork expression.  These
// tines together share a common file, which next() takes data from
// (each vector elements belongs to one tine of the overall fork).
//
// Tine yields null if the file slot corresponding to its branch has
// already been fetched.  It won't refill the file unless all other
// tines have fetched as well.  Tine and merge need to cooperate to
// make sure nullptr's don't get propagated unless there's really
// nothing left.  Merge knows that the source is drained, if all its
// branches yield nullptr.
class prod_tine
  : public prod_unary
{
  std::shared_ptr <std::vector <valfile::ptr> > m_file;
  size_t m_branch_id;

public:
  prod_tine (std::shared_ptr <prod> prod,
	     std::shared_ptr <std::vector <valfile::ptr> > file,
	     size_t branch_id)
    : prod_unary (prod)
    , m_file (file)
    , m_branch_id (branch_id)
  {
    assert (m_branch_id < m_file->size ());
  }

  valfile::ptr
  next () override
  {
    if (std::all_of (m_file->begin (), m_file->end (),
		     [] (valfile::ptr const &ptr) { return ptr == nullptr; }))
      {
	if (auto vf = m_prod->next ())
	  for (auto &ptr: *m_file)
	    ptr = std::make_unique <valfile> (*vf);
	else
	  return nullptr;
      }

    return std::move ((*m_file)[m_branch_id]);
  }
};

class prod_merge
  : public prod
{
  typedef std::vector <std::shared_ptr <prod> > prodvect_t;
  prodvect_t m_prods;
  prodvect_t::iterator m_it;

public:
  prod_merge (std::vector <std::shared_ptr <prod> > prods)
    : m_prods (prods)
    , m_it (m_prods.begin ())
  {}

  valfile::ptr
  next () override
  {
    if (m_it == m_prods.end ())
      return nullptr;

    for (size_t i = 0; i < m_prods.size (); ++i)
      {
	if (auto ret = (*m_it)->next ())
	  return ret;
	if (++m_it == m_prods.end ())
	  m_it = m_prods.begin ();
      }
    m_it = m_prods.end ();
    return nullptr;
  }
};

int
main(int argc, char *argv[])
{
  auto In = std::make_shared <prod_init> ();
  auto Un = std::make_shared <prod_uni> (In);
  auto Du = std::make_shared <prod_dup> (Un);

  auto f = std::make_shared <std::vector <valfile::ptr> > (2);
  auto Ti1 = std::make_shared <prod_tine> (Du, f, 0);
  auto Ti2 = std::make_shared <prod_tine> (Du, f, 1);

  auto Od = std::make_shared <prod_odd> (Ti1);
  auto Mu = std::make_shared <prod_mul> (Od);

  auto Ev = std::make_shared <prod_even> (Ti2);
  auto Ad = std::make_shared <prod_add> (Ev);

  std::vector <std::shared_ptr <prod> > v = {Mu, Ad};
  auto Mg = std::make_shared <prod_merge> (v);

  while (auto vf = Mg->next ())
    {
      std::cout << "[";
      for (auto v: *vf)
	std::cout << " " << v;
      std::cout << " ]\n";
    }

  return 0;
}
