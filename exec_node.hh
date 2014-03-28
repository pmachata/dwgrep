#ifndef _PIPE_H_
#define _PIPE_H_

#include <memory>
#include <vector>
#include <array>
#include <cassert>
#include <ucontext.h>

// Exec nodes represent the structure of expression at runtime.  Each
// exec node is one coroutine.  Exec nodes are interconnected with
// queues that manage interchange of data between the coroutines.
class exec_node;

// Queues are buffers between exec nodes.  Queue nodes hold queues.
class que_node;

// The whole computation forms a pipeline of exec_node's alternating
// with que_node's.  The data flows in one direction only, from
// upstream to downstream.  Each coroutine keeps pulling from the
// input queue, and after processing pushes to the corresponding
// output queue.  When input queue is empty, control is switched
// upstream to get more data.  When output queue is full, control is
// switched downstream to process the accumulated data.  Tho whole
// scheme is actually a bit more complicated, but this is the gist.

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

  void
  clear ()
  {
    while (! empty ())
      pop_front ();
  }
};

class que_node
{
  que m_que;
  std::weak_ptr <exec_node> m_upstream;
  std::shared_ptr <exec_node> m_downstream;

public:
  que_node (std::shared_ptr <exec_node> upstream,
	    std::shared_ptr <exec_node> downstream);

  void switch_upstream (ucontext &cur);
  void switch_downstream (ucontext &cur);
  valfile::ptr pop_front ();
  void push_back (valfile::ptr vf);
  bool empty () const;
  bool full () const;
  void clear ();
  std::shared_ptr <exec_node> get_downstream ();
};

class exec_node
{
  friend class exec_node_query;
  static size_t const stack_size = 4 * 4096;

  std::weak_ptr <que_node> m_in_que;
  std::shared_ptr <que_node> m_out_que;

  ucontext m_uc;
  int m_vg_stack_id;
  volatile bool m_quitting;

private:
  // Coroutine start-up.  Used internally by this class only.
  static void start (exec_node *self);	// ucontext callback
  void call_operate ();			// operate wrapper

private:
  exec_node (ucontext *link);	// switch to LINK after coroutine ends

  void mark_chain_quitting ();	// schedule shutdown
  bool quitting () const;	// whether shutdown has been scheduled
  class x_quit {};		// thrown by push to shut down
  void switch_downstream ();	// for round-robin on shutdown
  void cleanup ();		// to release memory after shutdown

  void yield (ucontext &other);	// give up control, switch to OTHER

protected:
  // Subclass interface.  next() reads next value from the upstream
  // queue, push() pushes a value to the downstream queue.  Each of
  // those can result in a context switch.  next() returns nullptr if
  // a computation should end, push() instead raises x_quit.
  bool request_data ();
  valfile::ptr next ();
  void push (valfile::ptr vf);
  exec_node ();

public:
  virtual ~exec_node ();
  virtual void operate () = 0;
  virtual std::string name () const = 0;

  void assign_in (std::weak_ptr <que_node> q);
  void assign_out (std::shared_ptr <que_node> q);

  // Transfer context from CUR to this task.
  void activate (ucontext &cur);
};

#endif /* _PIPE_H_ */
