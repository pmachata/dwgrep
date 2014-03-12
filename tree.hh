#ifndef _TREE_H_
#define _TREE_H_

#include <vector>
#include <string>
#include "cst.hh"

enum class tree_arity_v
  {
    NULLARY,
    UNARY,
    BINARY,
    STR,
    CONST,
  };

#define TREE_TYPES				\
  TREE_TYPE (ARITH_ADD, NULLARY)		\
  TREE_TYPE (ARITH_DIV, NULLARY)		\
  TREE_TYPE (ARITH_MOD, NULLARY)		\
  TREE_TYPE (ARITH_MUL, NULLARY)		\
  TREE_TYPE (ARITH_SUB, NULLARY)		\
  TREE_TYPE (ASSERT, UNARY)			\
  TREE_TYPE (ATVAL, CONST)			\
  TREE_TYPE (CLOSE_PLUS, UNARY)			\
  TREE_TYPE (CLOSE_STAR, UNARY)			\
  TREE_TYPE (CONST, CONST)			\
  TREE_TYPE (EACH, NULLARY)			\
  TREE_TYPE (MAYBE, UNARY)			\
  TREE_TYPE (PIPE, BINARY)			\
  TREE_TYPE (PRED_AT, CONST)			\
  TREE_TYPE (PRED_EMPTY, NULLARY)		\
  TREE_TYPE (PRED_EQ, NULLARY)			\
  TREE_TYPE (PRED_FIND, NULLARY)		\
  TREE_TYPE (PRED_GE, NULLARY)			\
  TREE_TYPE (PRED_GT, NULLARY)			\
  TREE_TYPE (PRED_LE, NULLARY)			\
  TREE_TYPE (PRED_LT, NULLARY)			\
  TREE_TYPE (PRED_MATCH, NULLARY)		\
  TREE_TYPE (PRED_NE, NULLARY)			\
  TREE_TYPE (PRED_NOT, UNARY)			\
  TREE_TYPE (PRED_SUBX_ALL, UNARY)		\
  TREE_TYPE (PRED_SUBX_ANY, UNARY)		\
  TREE_TYPE (PRED_TAG, CONST)			\
  TREE_TYPE (SHF_DROP, NULLARY)			\
  TREE_TYPE (SHF_DUP, NULLARY)			\
  TREE_TYPE (SHF_IF, NULLARY)			\
  TREE_TYPE (SHF_OVER, NULLARY)			\
  TREE_TYPE (SHF_ROT, NULLARY)			\
  TREE_TYPE (SHF_SWAP, NULLARY)			\
  TREE_TYPE (STR, STR)				\
  TREE_TYPE (TR_CHILD, NULLARY)			\
  TREE_TYPE (TR_NEXT, NULLARY)			\
  TREE_TYPE (TR_PARENT, NULLARY)		\
  TREE_TYPE (TR_PREV, NULLARY)			\

enum class tree_type
  {
#define TREE_TYPE(ENUM, ARITY) ENUM,
    TREE_TYPES
#undef TREE_TYPE
  };

template <tree_type TT> class tree_arity;
#define TREE_TYPE(ENUM, ARITY)					\
  template <> struct tree_arity <tree_type::ENUM> {		\
    static const tree_arity_v value = tree_arity_v::ARITY;	\
  };
TREE_TYPES
#undef TREE_TYPE

struct tree
{
  tree_type m_tt;
  std::vector <tree> m_children;

  union {
    std::string *sval;
    cst *cval;
  } m_u;

  tree ();
  explicit tree (tree_type tt);
  tree (tree const &other);
  ~tree ();

  tree &operator= (tree other);
  void swap (tree &other);

  template <tree_type TT>
  static tree *
  create_nullary ()
  {
    static_assert (tree_arity <TT>::value == tree_arity_v::NULLARY,
		   "Wrong tree arity.");
    return new tree {TT};
  }

  template <tree_type TT>
  static tree *
  create_unary (tree *op)
  {
    static_assert (tree_arity <TT>::value == tree_arity_v::UNARY,
		   "Wrong tree arity.");
    auto t = new tree {TT};
    t->take_child (op);
    return t;
  }

  template <tree_type TT>
  static tree *
  create_binary (tree *lhs, tree *rhs)
  {
    static_assert (tree_arity <TT>::value == tree_arity_v::BINARY,
		   "Wrong tree arity.");
    auto t = new tree {TT};
    t->take_child (lhs);
    t->take_child (rhs);
    return t;
  }

  template <tree_type TT>
  static tree *
  create_str (const char *s)
  {
    static_assert (tree_arity <TT>::value == tree_arity_v::STR,
		   "Wrong tree arity.");
    auto t = new tree {TT};
    t->m_u.sval = new std::string {s};
    return t;
  }

  template <tree_type TT>
  static tree *
  create_const (cst c)
  {
    static_assert (tree_arity <TT>::value == tree_arity_v::CONST,
		   "Wrong tree arity.");
    auto t = new tree {TT};
    t->m_u.cval = new cst {c};
    return t;
  }

  static tree *create_pipe (tree *t1, tree *t2);
  static tree *create_neg (tree *t1);
  static tree *create_assert (tree *t1);

  void take_child (tree *t);
  void dump (std::ostream &o) const;
};

#endif /* _TREE_H_ */
