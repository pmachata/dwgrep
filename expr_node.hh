#ifndef _EXPR_NODE_H_
#define _EXPR_NODE_H_

#include <utility>
#include <memory>
#include "dwgrep.hh"

class exec_node;

// Expression classes represent the structure of expression.  The
// output of a compiled expression is a tree of expr objects.
//
// An expression can be instantiated for a particular dwgrep graph.
// That means a data-flow structure of exec_node's is created for it,
// and the query is executed over the given graph.  This design allows
// the user to compile a dwgrep expression without necessarily running
// it, and to avoid redundant parsing when a bunch of graphs is to be
// explored with a single expression.
class expr_node
{
public:
  virtual ~expr_node () {}

  // This should build an exec_node corresponding to this expr_node.
  // Not every expr_node needs to have an associated exec_node, some
  // nodes will only install some plumbing.
  virtual std::shared_ptr <exec_node>
  build_exec (std::shared_ptr <exec_node> upstream,
	      dwgrep_graph::ptr q) = 0;
};

class query_expr
{
  std::unique_ptr <expr_node> m_expr;
public:
  explicit query_expr (std::unique_ptr <expr_node> expr);

  std::shared_ptr <exec_node> build_exec (dwgrep_graph::ptr q);
};

class expr_node_cat
  : public expr_node
{
  std::unique_ptr <expr_node> m_left;
  std::unique_ptr <expr_node> m_right;

public:
  expr_node_cat (std::unique_ptr <expr_node> left,
		 std::unique_ptr <expr_node> right);

  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

class expr_node_alt
  : public expr_node
{
  std::vector <std::unique_ptr <expr_node> > m_branches;

public:
  expr_node_alt (std::vector <std::unique_ptr <expr_node> > branches);

  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

struct expr_node_uni
  : public expr_node
{
  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

struct expr_node_dup
  : public expr_node
{
  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

struct expr_node_mul
  : public expr_node
{
  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

struct expr_node_add
  : public expr_node
{
  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

enum class pred_result
  {
    no = false,
    yes = true,
    fail,
  };

inline pred_result
operator! (pred_result other)
{
  switch (other)
    {
    case pred_result::no:
      return pred_result::yes;
    case pred_result::yes:
      return pred_result::no;
    case pred_result::fail:
      return pred_result::fail;
    }
  abort ();
}

inline pred_result
operator&& (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) && bool (b) ? pred_result::yes : pred_result::no;
}

inline pred_result
operator|| (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) || bool (b) ? pred_result::yes : pred_result::no;
}

struct pred
{
  pred_result result (valfile::ptr vf) const;
};

class expr_node_assert
  : public expr_node
{
  std::shared_ptr <pred> m_pred;

public:
  std::shared_ptr <exec_node> build_exec (std::shared_ptr <exec_node> upstream,
					  dwgrep_graph::ptr q) override;
};

struct pred_odd
  : public pred
{
  pred_result result (valfile::ptr vf) const;
  
};
#endif /* _EXPR_NODE_H_ */
