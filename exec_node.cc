#include <sys/mman.h>
#include <system_error>
#include <iostream>
#include <valgrind/valgrind.h>
#include <memory>
#include "make_unique.hh"

#include "exec_node.hh"
#include "expr_node.hh"
#include "dwgrep.hh"

namespace
{
  inline void
  throw_system ()
  {
    throw std::runtime_error
      (std::error_code (errno, std::system_category ()).message ());
  }
}

que_node::que_node (std::shared_ptr <exec_node> upstream,
		    std::shared_ptr <exec_node> downstream)
  : m_upstream ((assert (upstream != nullptr), upstream))
  , m_downstream ((assert (downstream != nullptr), downstream))
{}

void
que_node::switch_upstream (ucontext &cur)
{
  m_upstream.lock ()->activate (cur);
}

void
que_node::switch_downstream (ucontext &cur)
{
  m_downstream->activate (cur);
}

valfile::ptr
que_node::pop_front ()
{
  return m_que.pop_front ();
}

void
que_node::push_back (valfile::ptr vf)
{
  m_que.push_back (std::move (vf));
}

bool
que_node::empty () const
{
  return m_que.empty ();
}

bool
que_node::full () const
{
  return m_que.full ();
}

void
que_node::clear ()
{
  m_que.clear ();
}

std::shared_ptr <exec_node>
que_node::get_downstream ()
{
  return m_downstream;
}

void
exec_node::start (exec_node *self)
{
  self->call_operate ();
}

void
exec_node::cleanup ()
{
  std::cout << "exec_node::cleanup\n";
  // Break the circular chain of shared pointers.
  m_out_que = nullptr;
}

bool
exec_node::quitting () const
{
  return m_quitting;
}

exec_node::exec_node (ucontext *link)
  : m_uc ({})
  , m_quitting (false)
{
  if (getcontext (&m_uc) == -1)
    throw_system ();

  void *stack = mmap (0, stack_size, PROT_READ|PROT_WRITE,
		      MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);

  if (stack == MAP_FAILED)
    throw_system ();

  m_vg_stack_id = VALGRIND_STACK_REGISTER
    (stack, static_cast <char *> (stack) + stack_size);
  m_uc.uc_stack.ss_sp = stack;
  m_uc.uc_stack.ss_size = stack_size;
  m_uc.uc_stack.ss_flags = 0;
  m_uc.uc_link = link;

  makecontext (&m_uc, (void (*)()) start, 1, this);
}

exec_node::exec_node ()
  : exec_node (nullptr)
{}

exec_node::~exec_node ()
{
  VALGRIND_STACK_DEREGISTER (m_vg_stack_id);
  munmap (m_uc.uc_stack.ss_sp, stack_size);
}

void
exec_node::assign_in (std::weak_ptr <que_node> q)
{
  m_in_que = q;
}

void
exec_node::assign_out (std::shared_ptr <que_node> q)
{
  m_out_que = q;
}

void
exec_node::activate (ucontext &cur)
{
  std::cout << "activate " << name () << std::endl;
  swapcontext (&cur, &m_uc);
}

void
exec_node::yield (ucontext &other)
{
  std::cout << "yield " << name () << std::endl;
  swapcontext (&m_uc, &other);
}

class exec_node_query
  : public exec_node
{
  ucontext m_main_ctx;

public:
  exec_node_query ()
    : exec_node (&m_main_ctx)
    , m_main_ctx ({})
  {}

  valfile::ptr
  next_in_queue ()
  {
    auto inq = m_in_que.lock ();
    assert (! inq->empty ());
    return inq->pop_front ();
  }

  bool
  queue_empty () const
  {
    return m_in_que.lock ()->empty ();
  }

  bool
  done () const
  {
    return m_out_que == nullptr;
  }

  void
  quit ()
  {
    if (! quitting ())
      {
	mark_chain_quitting ();
	activate (m_main_ctx);
      }
  }

  void
  operate () override
  {
    push (std::make_unique <valfile> ());

    // The goal of query node is different from the others.  It needs
    // to prepare data in m_out_que for result iterator, but shouldn't
    // pull them out.
    while (request_data ())
      yield (m_main_ctx);

    // There are two scenarios under which we can get here.  Either
    // the caller decided to terminate the computation prematurely, in
    // which case m_quitting is true and this is part of the
    // termination.  Or next() hit an empty queue, and the chain of
    // requests ended back here, meaning nobody can produce anything
    // more, unless we put more stuff in the queue.  Which we don't.
    // In the latter case, we need to initiate the termination
    // ourselves:
    mark_chain_quitting ();
  }

  std::string
  name () const override
  {
    return "query";
  }

  void
  run ()
  {
    assert (! quitting ());
    activate (m_main_ctx);
    std::cout << "finished running\n";
  }
};

bool
exec_node::request_data ()
{
  auto inq = m_in_que.lock ();
  assert (inq != nullptr);

  while (inq->empty () && ! m_out_que->empty () && ! quitting ())
    m_out_que->switch_downstream (m_uc);

  if (inq->empty () && ! quitting ())
    inq->switch_upstream (m_uc);

  if (inq->empty ())
    // If the in que is still empty, we are done.
    return false;

  if (quitting ())
    {
      std::cout << name () << " should quit\n";
      return false;
    }

  return true;
}

valfile::ptr
exec_node::next ()
{
  if (! request_data ())
    return nullptr;

  //std::cout << name () << " pops\n";
  return m_in_que.lock ()->pop_front ();
}

void
exec_node::push (valfile::ptr vf)
{
  while (m_out_que->full () && ! quitting ())
    m_out_que->switch_downstream (m_uc);

  if (quitting ())
    {
      std::cout << name () << " should quit\n";
      throw x_quit ();
    }

  //std::cout << name () << " pushes\n";
  m_out_que->push_back (std::move (vf));
}

void
exec_node::mark_chain_quitting ()
{
  m_quitting = true;
  for (auto it = m_out_que->get_downstream ();
       &*it != this; it = it->m_out_que->get_downstream ())
    {
      std::cout << "marking " << it->name () << " for quitting\n";
      it->m_quitting = true;
    }
}

void
exec_node::switch_downstream ()
{
  assert (quitting ());
  m_out_que->switch_downstream (m_uc);
}

void
exec_node::call_operate ()
{
  try
    {
      operate ();
    }
  catch (x_quit)
    {}
  std::cout << name () << " done\n";

  // At this point, we need to go downstream through the whole
  // pipeline, which will result in nullptr being returned to all
  // the operate's along the way, thus finishing any unfinished
  // business.  In particular, any weak_ptr's that were converted to
  // shared_ptr's by lock () need to be closed again, and any
  // unique_ptr's pulled out of the queue and stored in local
  // variables need to be destroyed, so that we don't leak.
  //
  // This will eventually transfer control back here, because
  // exec_node's are connected into a circle (see the comment at
  // exec_node_query).
  switch_downstream ();

  // If we got here, all the operate's were terminated, we are
  // exec_node_query, and can clean up the whole chain of nodes, so
  // that the only thing that remains is our selves.
  assert (dynamic_cast <exec_node_query *> (this) != nullptr);
  cleanup ();
}

namespace
{
  // This class wraps the original query and becomes new entry-point
  // to the expression.  The exec_node_query that it produces ties the
  // upstream-most and downstream-most nodes of the overall
  // expression.  Thus the overall object graph forms a circle of
  // alternating exec_node's and que_node's.
  class expr_node_query
    : public expr_node
  {
    std::unique_ptr <expr_node> m_expr;

  public:
    explicit expr_node_query (std::unique_ptr <expr_node> expr)
      : m_expr (std::move (expr))
    {}

    std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
    build_exec (problem::ptr q)
    {
      auto ee = m_expr->build_exec (q);
      auto qr = std::make_shared <exec_node_query> ();
      auto lqi = std::make_shared <que_node> (qr, ee.first);
      auto lqo = std::make_shared <que_node> (ee.second, qr);

      ee.first->assign_in (lqi);
      ee.second->assign_out (lqo);

      qr->assign_in (lqo);
      qr->assign_out (lqi);

      return std::make_pair (qr, qr);
    }
  };

  // This function returns what a parser would.
  std::unique_ptr <expr_node>
  build_my_expr ()
  {
    auto U = std::make_unique <expr_node_uni> ();
    auto D = std::make_unique <expr_node_dup> ();
    auto M = std::make_unique <expr_node_mul> ();

    auto C2 = std::make_unique <expr_node_cat> (std::move (D), std::move (M));
    return std::make_unique <expr_node_cat> (std::move (U), std::move (C2));
  }

  std::unique_ptr <expr_node>
  wrap_expr (std::unique_ptr <expr_node> expr)
  {
    return std::make_unique <expr_node_query> (std::move (expr));
  }
}

dwgrep_expr_t::result::result (std::shared_ptr <exec_node_query> q)
  : m_q (q)
{}

dwgrep_expr_t::result::~result ()
{
  m_q->quit ();
}

bool
dwgrep_expr_t::result::iterator::done () const
{
  return m_q == nullptr;
}

void
dwgrep_expr_t::result::iterator::fetch ()
{
  assert (! done ());
  m_vf = nullptr;
  if (m_q->queue_empty ())
    {
      m_q->run ();

      if (m_q->done ())
	{
	  m_q = nullptr;
	  return;
	}
    }

  m_vf = std::shared_ptr <valfile> (m_q->next_in_queue ());
}

dwgrep_expr_t::result::iterator::iterator ()
{}

dwgrep_expr_t::result::iterator::iterator (std::shared_ptr <exec_node_query> q)
  : m_q (q)
{
  assert (m_q != nullptr);
  fetch ();
}

dwgrep_expr_t::result::iterator::iterator (iterator const &other)
  : m_q (other.m_q)
  , m_vf (other.m_vf)
{}

std::shared_ptr <valfile>
dwgrep_expr_t::result::iterator::operator* () const
{
  return m_vf;
}

dwgrep_expr_t::result::iterator
dwgrep_expr_t::result::iterator::operator++ ()
{
  fetch ();
  return *this;
}

dwgrep_expr_t::result::iterator
dwgrep_expr_t::result::iterator::operator++ (int)
{
  iterator ret = *this;
  ++*this;
  return ret;
}

bool
dwgrep_expr_t::result::iterator::operator== (iterator that)
{
  if (that.m_q == nullptr)
    return m_q == nullptr;
  return that.m_q == m_q
    && that.m_vf == m_vf;
}

bool
dwgrep_expr_t::result::iterator::operator!= (iterator that)
{
  return !(*this == that);
}

dwgrep_expr_t::dwgrep_expr_t ()
  : m_expr (wrap_expr (build_my_expr ()))
{}

dwgrep_expr_t::result
dwgrep_expr_t::query (std::shared_ptr <problem> p)
{
  std::pair <std::shared_ptr <exec_node>,
	     std::shared_ptr <exec_node> > endpoints = m_expr->build_exec (p);
  assert (endpoints.first != nullptr);
  assert (endpoints.first == endpoints.second);
  auto q = std::dynamic_pointer_cast <exec_node_query> (endpoints.first);
  assert (q != nullptr);
  return result (q);
}

dwgrep_expr_t::result::iterator
dwgrep_expr_t::result::begin ()
{
  return iterator (m_q);
}

dwgrep_expr_t::result::iterator
dwgrep_expr_t::result::end ()
{
  return iterator ();
}

int
main(int argc, char *argv[])
{
  dwgrep_expr_t expr;

  for (auto vf: expr.query (std::make_shared <problem> ()))
    {
      std::cout << "[";
      for (auto v: *vf)
	std::cout << " " << v;
      std::cout << " ]\n";
      break;
    }

  return 0;
}
