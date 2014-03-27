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

// A problem object represents a graph that we want to explore, and
// any associated caches.
struct problem
{
  typedef std::shared_ptr <problem> ptr;
};

// Expression classes represent the structure of expression.  The
// output of a compiled expression is a tree of expr objects.
// Expression can be instantiated for a particular problem.
class expr_node;

// Exec nodes represent the structure of expression at runtime.  Each
// exec node is one coroutine.  Exec nodes are interconnected with
// queues that manage interchange of data between the coroutines.
class exec_node;

// Queue nodes hold que objects, and also know about the two
// expression objects that they are surrounded by.  A queue node thus
// knows how to transfer control, say, downstream, if, say, an
// upstream node asks.
class que_node;

// N.B.: All these classes are very likely internal, possibly with the
// exception of class problem.  On the outside we want a clear
// interface like, for example:
//
//   class dwgrep_expr_t {
//       std::unique_ptr <exec_node> m_exec_node;  // but pimpl'd away
//   public:
//       explicit dwgrep_expr_t (const char *str); // compile query
//       explicit dwgrep_expr_t (std::string str); // compile query
//       something run (problem::ptr p); // run expr on a given problem
//   };
//
// It's not very clear what the "something" should be.  Maybe the
// output end of the queue, so that the caller can decide whether they
// want to keep pulling.  The queue returned would have to be a
// sanitized wrapper that translates valfile's to std::vectors's.  We
// would still need a separate user-facing set of value interfaces
// that allow the caller program to inspect obtained values.

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

class que
{
  static unsigned const N = 64;
  std::array <valfile::ptr, N> m_que;
  unsigned m_head;
  unsigned m_tail;

  static unsigned
  wrap (unsigned i)
  {
    return i % N;
  }

public:
  que ()
    : m_head (0)
    , m_tail (0)
  {}

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
};

class expr_node
{
public:
  virtual ~expr_node () {}

  // This should build an exec_node corresponding to this expr_node.
  // Not every expr_node needs to have an associated exec_node, some
  // nodes will only do an interconnect.  It returns a pair of
  // leftmost and rightmost exec_node's in the pipeline.
  virtual std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) = 0;
};

class exec_node
{
  static size_t const stack_size = 4 * 4096;

  std::weak_ptr <que_node> m_in_que;
  std::shared_ptr <que_node> m_out_que;

  std::unique_ptr <ucontext> m_uc;
  int m_vg_stack_id;

  static void
  call_operate (exec_node *self)
  {
    self->operate_wrapper ();
  }

  void
  operate_wrapper ()
  {
    operate ();
    std::cout << name () << " done\n";
    switch_right ();
  }

protected:
  valfile::ptr next ();
  void push (valfile::ptr vf);

public:
  exec_node ()
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

  virtual ~exec_node ()
  {
    VALGRIND_STACK_DEREGISTER (m_vg_stack_id);
    munmap (m_uc->uc_stack.ss_sp, stack_size);
  }

  virtual void operate () = 0;
  virtual std::string name () const = 0;

  void
  assign_in (std::weak_ptr <que_node> q)
  {
    m_in_que = q;
  }

  void
  assign_out (std::shared_ptr <que_node> q)
  {
    m_out_que = q;
  }

  void
  activate (ucontext &other)
  {
    //std::cout << "activate " << name () << std::endl;
    swapcontext (&other, &*m_uc);
  }

protected:
  void
  quit (ucontext &main)
  {
    std::cout << "quitting\n";
    // Break the circular chain of shared pointers.
    m_out_que = nullptr;

    assert (m_uc != nullptr);
    swapcontext (&*m_uc, &main);
  }

  void switch_right ();
};

class que_node
{
  que m_que;
  std::weak_ptr <exec_node> m_left;
  std::shared_ptr <exec_node> m_right;

public:
  que_node (std::shared_ptr <exec_node> left, std::shared_ptr <exec_node> right)
    : m_left ((assert (left != nullptr), left))
    , m_right ((assert (right != nullptr), right))
  {}

  void
  switch_right (ucontext &cur)
  {
    m_right->activate (cur);
  }

  void
  switch_left (ucontext &cur)
  {
    m_left.lock ()->activate (cur);
  }

  valfile::ptr
  pop_front ()
  {
    return m_que.pop_front ();
  }

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
};

valfile::ptr
exec_node::next ()
{
  auto inq = m_in_que.lock ();
  assert (inq != nullptr);

  while (inq->empty () && ! m_out_que->empty ())
    m_out_que->switch_right (*m_uc);

  if (inq->empty ())
    inq->switch_left (*m_uc);

  if (inq->empty ())
    // If the in que is still empty, we are done.
    return nullptr;

  //std::cout << name () << " pops\n";
  return inq->pop_front ();
}

void
exec_node::push (valfile::ptr vf)
{
  while (m_out_que->full ())
    m_out_que->switch_right (*m_uc);

  //std::cout << name () << " pushes\n";
  m_out_que->push_back (std::move (vf));
}

void
exec_node::switch_right ()
{
  m_out_que->switch_right (*m_uc);
}

class expr_cat
  : public expr_node
{
  std::unique_ptr <expr_node> m_left;
  std::unique_ptr <expr_node> m_right;

public:
  expr_cat (std::unique_ptr <expr_node> left,
	    std::unique_ptr <expr_node> right)
    : m_left (std::move (left))
    , m_right (std::move (right))
  {}

  std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) override
  {
    auto le = m_left->build_exec (q);
    auto re = m_right->build_exec (q);
    auto qe = std::make_shared <que_node> (le.second, re.first);
    le.second->assign_out (qe);
    re.first->assign_in (qe);
    return std::make_pair (le.first, re.second);
  }
};

class expr_uni
  : public expr_node
{
  struct e
    : public exec_node
  {
    void
    operate () override
    {
      while (valfile::ptr vf = next ())
	{
	  for (int i = 0; i < 2500; ++i)
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

public:
  std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) override
  {
    auto ret = std::make_shared <e> ();
    return std::make_pair (ret, ret);
  }
};

class expr_dup
  : public expr_node
{
  struct e
    : public exec_node
  {
    void
    operate () override
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

public:
  std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) override
  {
    auto ret = std::make_shared <e> ();
    return std::make_pair (ret, ret);
  }
};

class expr_mul
  : public expr_node
{
  struct e
    : public exec_node
  {
    void
    operate () override
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

public:
  std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) override
  {
    auto ret = std::make_shared <e> ();
    return std::make_pair (ret, ret);
  }
};

// Query expression node is the top-level object that holds the whole
// query.  It produces an exec_node, and ties it to the leftmost and
// rightmost nodes of the overall expression.  Thus the overall object
// graph forms a circle of alternating exec_node's and que_node's.
class expr_query
  : public expr_node
{
public:
  class e
    : public exec_node
  {
    ucontext m_main_ctx;

  public:
    e ()
      : m_main_ctx ({})
    {
    }

    void
    operate () override
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
      // (downstream).  So we only get here if the first guy in
      // pipeline got out of what he can produce for the single value
      // that we put in, which signifies the end of query.
      //
      // At this point, we need to go downstream through the whole
      // pipeline, which will result in nullptr being returned to all
      // the operate's along the way, thus finishing any unfinished
      // business.  In particular, any weak_ptr's that were converted
      // to shared_ptr's by lock () need to be closed again, so that
      // we don't leak.
      //
      // operate is called from operate_wrapper, which calls
      // switch_right as well.  This will eventually transfer control
      // back to us, because exec_node's are connected into a circle
      // (see above comment).
      switch_right ();

      // Ok, we got back, now we can quit.
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
      if (getcontext (&m_main_ctx) == -1)
	throw_system ();
      activate (m_main_ctx);
      std::cout << "finished running\n";
    }
  };

  std::unique_ptr <expr_node> m_expr;

public:
  explicit expr_query (std::unique_ptr <expr_node> expr)
    : m_expr (std::move (expr))
  {}

  std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) override
  {
    auto ee = m_expr->build_exec (q);
    auto qr = std::make_shared <e> ();
    auto lqi = std::make_shared <que_node> (qr, ee.first);
    auto lqo = std::make_shared <que_node> (ee.second, qr);

    ee.first->assign_in (lqi);
    ee.second->assign_out (lqo);

    qr->assign_in (lqo);
    qr->assign_out (lqi);

    return std::make_pair (qr, qr);
  }
};

int
main(int argc, char *argv[])
{
  auto U = std::make_unique <expr_uni> ();
  auto D = std::make_unique <expr_dup> ();
  auto M = std::make_unique <expr_mul> ();

  auto C2 = std::make_unique <expr_cat> (std::move (D), std::move (M));
  auto C1 = std::make_unique <expr_cat> (std::move (U), std::move (C2));

  auto Q = std::make_unique <expr_query> (std::move (C1));

  auto prob = std::make_shared <problem> ();
  auto E = Q->build_exec (prob).first;
  if (auto q = dynamic_cast <expr_query::e *> (&*E))
    q->run ();

  std::cout << "over and out\n";

  return 0;
}
