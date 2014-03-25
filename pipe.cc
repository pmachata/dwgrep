#include <sys/mman.h>
#include <array>
#include <cassert>
#include <vector>
#include <memory>
#include <ucontext.h>
#include <system_error>
#include <iostream>

#include <valgrind/valgrind.h>

#include "make_unique.hh"

namespace
{
  inline void
  throw_system ()
  {
    throw std::runtime_error
      (std::error_code (errno, std::system_category ()).message ());
  }
}

struct valfile
  : public std::vector <int>
{
  typedef std::unique_ptr <valfile> ptr;
};

class op;
class leaf_op;

class que
{
  static unsigned const N = 64;
  std::array <valfile::ptr, N> m_que;
  unsigned m_head;
  unsigned m_tail;

  leaf_op &m_left;
  leaf_op &m_right;

  static unsigned
  wrap (unsigned i)
  {
    return i % N;
  }

public:
  que (leaf_op &left, leaf_op &right);

  bool
  empty () const
  {
    return m_head == m_tail;
  }

  bool
  full () const
  {
    return wrap (m_head + 1) == m_tail;
  }

  void
  push_back (valfile::ptr vf)
  {
    assert (! full ());
    m_que[m_head] = std::move (vf);
    m_head = wrap (m_head + 1);
  }

  valfile::ptr
  pop_front ()
  {
    assert (! empty ());
    unsigned idx = m_tail;
    m_tail = wrap (m_tail + 1);
    return std::move (m_que[idx]);
  }

  void switch_right (ucontext &cur);
  void switch_left (ucontext &cur);
};

class in_que
{
  que &m_que;

public:
  in_que (que &q)
    : m_que (q)
  {}

  valfile::ptr
  pop_front ()
  {
    return m_que.pop_front ();
  }

  bool
  empty () const
  {
    return m_que.empty ();
  }

  void
  switch_left (ucontext &cur)
  {
    m_que.switch_left (cur);
  }
};

class out_que
{
  que &m_que;

public:
  out_que (que &q)
    : m_que (q)
  {}

  void
  push_back (valfile::ptr vf)
  {
    m_que.push_back (std::move (vf));
  }

  bool
  empty () const
  {
    return m_que.empty ();
  }

  bool
  full () const
  {
    return m_que.full ();
  }

  void
  switch_right (ucontext &cur)
  {
    m_que.switch_right (cur);
  }
};

struct query
{
};

class op
{
public:
  virtual ~op () {}

  virtual leaf_op &leftmost () = 0;
  virtual leaf_op &rightmost () = 0;
};

class leaf_op
  : public op
{
  static size_t const stack_size = 4 * 4096;
  std::unique_ptr <in_que> m_in_que;
  std::unique_ptr <out_que> m_out_que;
  std::unique_ptr <ucontext> m_uc;
  int m_vg_stack_id;

  static void
  call_operate (leaf_op *self)
  {
    std::cout << "passing NULL query object!!\n";
    self->operate (* (query *) nullptr);
  }

protected:
  valfile::ptr
  next ()
  {
    while (m_in_que->empty () && ! m_out_que->empty ())
      m_out_que->switch_right (*m_uc);

    if (m_in_que->empty ())
      m_in_que->switch_left (*m_uc);

    if (m_in_que->empty ())
      {
	std::cout << "in que still empty.  Done?\n";
	return nullptr;
      }

    //std::cout << name () << " pops\n";
    return m_in_que->pop_front ();
  }

  void
  push (valfile::ptr vf)
  {
    while (m_out_que->full ())
      m_out_que->switch_right (*m_uc);

    //std::cout << name () << " pushes\n";
    m_out_que->push_back (std::move (vf));
  }

public:
  leaf_op ()
  {
  }

  ~leaf_op ()
  {
    if (m_uc != nullptr)
      {
	VALGRIND_STACK_DEREGISTER (m_vg_stack_id);
	munmap (m_uc->uc_stack.ss_sp, stack_size);
      }
  }

  virtual void operate (query const &q) = 0;
  virtual std::string name () const = 0;

  leaf_op &leftmost () override { return *this; }
  leaf_op &rightmost () override { return *this; }

  void
  assign_in (que &q)
  {
    m_in_que = std::make_unique <in_que> (q);
  }

  void
  assign_out (que &q)
  {
    m_out_que = std::make_unique <out_que> (q);
  }

  void
  activate (ucontext &other)
  {
    if (m_uc == nullptr)
      {
	m_uc = std::make_unique <ucontext> ();
	if (getcontext (&*m_uc) == -1)
	  throw_system ();

	void *stack = mmap (0, stack_size, PROT_READ|PROT_WRITE,
			    MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);

	if (stack == MAP_FAILED)
	  throw_system ();

	m_vg_stack_id = VALGRIND_STACK_REGISTER
	  (stack, static_cast <char *> (stack) + stack_size);
	m_uc->uc_stack.ss_sp = stack;
	m_uc->uc_stack.ss_size = stack_size;
	m_uc->uc_stack.ss_flags = 0;

	makecontext (&*m_uc, (void (*)()) call_operate, 1, this);
      }

    //std::cout << "activate " << name () << std::endl;
    swapcontext (&other, &*m_uc);
  }

protected:
  void
  quit (ucontext &main)
  {
    assert (m_uc != nullptr);
    swapcontext (&*m_uc, &main);
  }
};

que::que (leaf_op &left, leaf_op &right)
  : m_head (0)
  , m_tail (0)
  , m_left (left)
  , m_right (right)
{
  m_left.assign_out (*this);
  m_right.assign_in (*this);
}

void
que::switch_right (ucontext &cur)
{
  //std::cout << "switching right" << std::endl;
  m_right.activate (cur);
}

void
que::switch_left (ucontext &cur)
{
  //std::cout << "switching left" << std::endl;
  m_left.activate (cur);
}

class op_cat
  : public op
{
  std::unique_ptr <op> m_left;
  std::unique_ptr <op> m_right;
  que m_que;

public:
  op_cat (std::unique_ptr <op> left, std::unique_ptr <op> right)
    : m_left (std::move (left))
    , m_right (std::move (right))
    , m_que (m_left->rightmost (), m_right->leftmost ())
  {}

  leaf_op &
  leftmost () override
  {
    return m_left->leftmost ();
  }

  leaf_op &
  rightmost () override
  {
    return m_right->rightmost ();
  }
};

struct op_uni
  : public leaf_op
{
  void
  operate (query const &q) override
  {
    while (valfile::ptr vf = next ())
      {
	for (int i = 0; i < 25000; ++i)
	  {
	    auto vf2 = std::make_unique <valfile> (*vf);
	    vf2->push_back (i);
	    push (std::move (vf2));
	  }
      }
  }

  std::string
  name () const override
  {
    return "uni";
  }
};

struct op_dup
  : public leaf_op
{
  void
  operate (query const &q) override
  {
    while (valfile::ptr vf = next ())
      {
	assert (! vf->empty ());
	vf->push_back (vf->back ());
	push (std::move (vf));
      }
  }

  std::string
  name () const override
  {
    return "dup";
  }
};

struct op_mul
  : public leaf_op
{
  void
  operate (query const &q) override
  {
    while (valfile::ptr vf = next ())
      {
	assert (vf->size () >= 2);
	int a = vf->back (); vf->pop_back ();
	int b = vf->back (); vf->pop_back ();
	vf->push_back (a * b);
	push (std::move (vf));
      }
  }

  std::string
  name () const override
  {
    return "mul";
  }
};

class op_query
  : public leaf_op
{
  std::unique_ptr <op> m_op;
  que m_first;
  que m_last;
  ucontext m_main_ctx;

public:
  explicit op_query (std::unique_ptr <op> op)
    : m_op (std::move (op))
    , m_first (*this, m_op->leftmost ())
    , m_last (m_op->rightmost (), *this)
  {
    if (getcontext (&m_main_ctx) == -1)
      throw_system ();
  }

  void
  operate (query const &q) override
  {
    push (std::make_unique <valfile> ());

    while (valfile::ptr vf = next ())
      {
	std::cout << "[";
	for (auto v: *vf)
	  std::cout << " " << v;
	std::cout << " ]\n";
      }

    // next () is written in such a way that if the input que is
    // empty, but there's stuff in the output que, we switch right
    // (downstream).  So we only get here if the first guy in pipeline
    // got out of what he can produce, which signifies the end of
    // query.
    quit (m_main_ctx);
  }

  std::string
  name () const override
  {
    return "query";
  }

  void
  run ()
  {
    activate (m_main_ctx);
  }
};

int main(int argc, char *argv[])
{
  auto U = std::make_unique <op_uni> ();
  auto D = std::make_unique <op_dup> ();
  auto M = std::make_unique <op_mul> ();

  auto C2 = std::make_unique <op_cat> (std::move (D), std::move (M));
  auto C1 = std::make_unique <op_cat> (std::move (U), std::move (C2));

  auto Q = std::make_unique <op_query> (std::move (C1));

  Q->run ();
  std::cout << "done\n";
  //Q->operate (q);

  return 0;
}
