#ifndef _EXPR_NODE_H_
#define _EXPR_NODE_H_

#include <utility>
#include <memory>
#include "dwgrep.hh"

class exec_node;

// Expression classes represent the structure of expression.  The
// output of a compiled expression is a tree of expr objects.  An
// expression can be instantiated for a particular problem.
class expr_node
{
public:
  virtual ~expr_node () {}

  // This should build an exec_node corresponding to this expr_node.
  // Not every expr_node needs to have an associated exec_node, some
  // nodes will only do an interconnect.  It returns a pair of
  // endpoints.
  virtual std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
  build_exec (problem::ptr q) = 0;
};

template <class Exec>
std::pair <std::shared_ptr <exec_node>, std::shared_ptr <exec_node> >
build_leaf_exec ()
{
  auto ret = std::make_shared <Exec> ();
  return std::make_pair (ret, ret);
}

class expr_node_cat
  : public expr_node
{
  std::unique_ptr <expr_node> m_left;
  std::unique_ptr <expr_node> m_right;

public:
  expr_node_cat (std::unique_ptr <expr_node> left,
		 std::unique_ptr <expr_node> right);

  std::pair <std::shared_ptr <exec_node>,
	     std::shared_ptr <exec_node> > build_exec (problem::ptr q) override;
};

struct expr_node_uni
  : public expr_node
{
  std::pair <std::shared_ptr <exec_node>,
	     std::shared_ptr <exec_node> > build_exec (problem::ptr q) override;
};

struct expr_node_dup
  : public expr_node
{
  std::pair <std::shared_ptr <exec_node>,
	     std::shared_ptr <exec_node> > build_exec (problem::ptr q) override;
};

struct expr_node_mul
  : public expr_node
{
  std::pair <std::shared_ptr <exec_node>,
	     std::shared_ptr <exec_node> > build_exec (problem::ptr q) override;
};

#endif /* _EXPR_NODE_H_ */
