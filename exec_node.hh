#ifndef _EXEC_NODE_H_
#define _EXEC_NODE_H_

#include <memory>
#include <vector>

struct valfile
  : public std::vector <int>
{
  typedef std::unique_ptr <valfile> ptr;
};

class exec_node
{
public:
  virtual ~exec_node () {}
  virtual valfile::ptr next () = 0;
  virtual std::string name () const = 0;
};

class exec_node_unary
  : public exec_node
{
protected:
  std::shared_ptr <exec_node> m_exec;

public:
  exec_node_unary (std::shared_ptr <exec_node> exec)
    : m_exec (exec)
  {}
};

#endif /* _EXEC_NODE_H_ */
