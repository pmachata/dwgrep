#ifndef _TREE_H_
#define _TREE_H_

#include <vector>
#include <string>
#include "cst.hh"

// These constants describe how a tree is allowed to be constructed.
// It's mostly present to make sure we don't inadvertently construct
// e.g. a CONST tree with no associated constant.
//
// NULLARY, UNARY, BINARY -- a tree that's constructed over zero, one
// or two sub-trees.  Often that's the only number of sub-trees that
// makes sense for given tree type, but CAT and ALT can hold arbitrary
// number of sub-trees, they are just constructed from two.  FORMAT is
// NULLARY, but holds any number of sub-trees as well.
//
// STR, CONST -- A tree that holds a constant, has no children.
enum class tree_arity_v
  {
    NULLARY,
    UNARY,
    BINARY,
    STR,
    CONST,
  };

// CAT -- A node for holding concatenation (X Y Z).
// ALT -- A node for holding alternation (X, Y, Z).
// CAPTURE -- For holding [X].
//
// TRANSFORM -- For holding NUM/X.  The first child is a constant
// representing application depth, the second is the X.
//
// PROTECT -- For holding +X.
//
// NOP -- For holding a no-op that comes up in "%s" and (,X).
//
// CLOSE_STAR, CLOSE_PLUS, MAYBE -- For holding X*, X+, X?.  X is the
// only child of these nodes.
//
// ASSERT -- All assertions (such as ?some_tag) are modeled using an
// ASSERT node, whose only child is a predicate node expressing the
// asserted condition (such as PRED_TAG).  Negative assertions are
// modeled using PRED_NOT.  In particular, IF is expanded into (!empty
// drop), ELSE into (?empty drop).
//
// PRED_SUBX_ALL, PRED_SUBX_ANY -- For holding ?all{X} and ?{X}.
// !all{X} and !{} are modeled using PRED_NOT.
//
// CONST -- For holding named constants and integer literals.
//
// STR -- For holding string literals.  Actual string literals are
// translated to FORMAT.
//
// FORMAT -- The literal parts of the format string are stored as
// children of type STR.  Other computation is stored as children of
// other types.
//
// F_ADD, F_SUB, etc. -- For holding corresponding function words.
//
// SEL_UNIVERSE, SEL_SECTION, SEL_UNIT -- Likewise.  But note that
// SEL_UNIVERSE does NOT actually POP.  Plain "universe" is translated
// as "SHF_DROP SEL_UNIVERSE", and "+universe" is translated as mere
// "SEL_UNIVERSE".
//
// SHF_SWAP, SWP_DUP -- Stack shuffling words.

#define TREE_TYPES				\
  TREE_TYPE (CAT, BINARY)			\
  TREE_TYPE (ALT, BINARY)			\
  TREE_TYPE (CAPTURE, UNARY)			\
  TREE_TYPE (TRANSFORM, BINARY)			\
  TREE_TYPE (PROTECT, UNARY)			\
  TREE_TYPE (NOP, NULLARY)			\
  TREE_TYPE (CLOSE_PLUS, UNARY)			\
  TREE_TYPE (CLOSE_STAR, UNARY)			\
  TREE_TYPE (MAYBE, UNARY)			\
  TREE_TYPE (ASSERT, UNARY)			\
  TREE_TYPE (PRED_AT, CONST)			\
  TREE_TYPE (PRED_TAG, CONST)			\
  TREE_TYPE (PRED_EQ, NULLARY)			\
  TREE_TYPE (PRED_NE, NULLARY)			\
  TREE_TYPE (PRED_GT, NULLARY)			\
  TREE_TYPE (PRED_GE, NULLARY)			\
  TREE_TYPE (PRED_LT, NULLARY)			\
  TREE_TYPE (PRED_LE, NULLARY)			\
  TREE_TYPE (PRED_FIND, NULLARY)		\
  TREE_TYPE (PRED_MATCH, NULLARY)		\
  TREE_TYPE (PRED_EMPTY, NULLARY)		\
  TREE_TYPE (PRED_ROOT, NULLARY)		\
  TREE_TYPE (PRED_NOT, UNARY)			\
  TREE_TYPE (PRED_SUBX_ALL, UNARY)		\
  TREE_TYPE (PRED_SUBX_ANY, UNARY)		\
  TREE_TYPE (CONST, CONST)			\
  TREE_TYPE (STR, STR)				\
  TREE_TYPE (FORMAT, NULLARY)			\
  TREE_TYPE (F_ATVAL, CONST)			\
  TREE_TYPE (F_ADD, NULLARY)			\
  TREE_TYPE (F_SUB, NULLARY)			\
  TREE_TYPE (F_MUL, NULLARY)			\
  TREE_TYPE (F_DIV, NULLARY)			\
  TREE_TYPE (F_MOD, NULLARY)			\
  TREE_TYPE (F_PARENT, NULLARY)			\
  TREE_TYPE (F_CHILD, NULLARY)			\
  TREE_TYPE (F_ATTRIBUTE, NULLARY)		\
  TREE_TYPE (F_PREV, NULLARY)			\
  TREE_TYPE (F_NEXT, NULLARY)			\
  TREE_TYPE (F_TYPE, NULLARY)			\
  TREE_TYPE (F_OFFSET, NULLARY)			\
  TREE_TYPE (F_NAME, NULLARY)			\
  TREE_TYPE (F_TAG, NULLARY)			\
  TREE_TYPE (F_FORM, NULLARY)			\
  TREE_TYPE (F_VALUE, NULLARY)			\
  TREE_TYPE (F_POS, NULLARY)			\
  TREE_TYPE (F_COUNT, NULLARY)			\
  TREE_TYPE (F_EACH, NULLARY)			\
  TREE_TYPE (SEL_UNIVERSE, NULLARY)		\
  TREE_TYPE (SEL_SECTION, NULLARY)		\
  TREE_TYPE (SEL_UNIT, NULLARY)			\
  TREE_TYPE (SHF_SWAP, NULLARY)			\
  TREE_TYPE (SHF_DUP, NULLARY)			\
  TREE_TYPE (SHF_OVER, NULLARY)			\
  TREE_TYPE (SHF_ROT, NULLARY)			\
  TREE_TYPE (SHF_DROP, NULLARY)			\

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

// This is for communication between lexical and syntactic analyzers
// and the rest of the world.  It uses naked pointers all over the
// place, as in %union that lexer and parser use, we can't hold
// full-fledged classes.
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
  create_str (std::string s)
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

  // Creates either a CAT or an ALT node.
  //
  // It is smart in that it transforms CAT's of CAT's into one
  // overarching CAT, and appends or prepends other nodes to an
  // existing CAT if possible.  It also knows to ignore a nullptr
  // tree.
  template <tree_type TT>
  static tree *
  create_cat (tree *t1, tree *t2)
  {
    bool cat1 = t1 != nullptr && t1->m_tt == TT;
    bool cat2 = t2 != nullptr && t2->m_tt == TT;

    if (cat1 && cat2)
      {
	t1->append_cat (t2);
	return t1;
      }
    else if (cat1 && t2 != nullptr)
      {
	t1->take_child (t2);
	return t1;
      }
    else if (cat2 && t1 != nullptr)
      {
	t2->take_child_front (t1);
	return t2;
      }

    if (t1 == nullptr)
      {
	assert (t2 != nullptr);
	return t2;
      }
    else if (t2 == nullptr)
      return t1;
    else
      return tree::create_binary <TT> (t1, t2);
  }

  static tree *create_neg (tree *t1);
  static tree *create_assert (tree *t1);
  static tree *create_protect (tree *t1);

  // push_back (*T) and delete T.
  void take_child (tree *t);

  // push_front (*T) and delete T.
  void take_child_front (tree *t);

  // Takes a tree T, which is a CAT or an ALT, and appends all
  // children therein.  It then deletes T.
  void append_cat (tree *t);

  void
  push_back (tree t)
  {
    m_children.push_back (t);
  }

  void dump (std::ostream &o) const;
  void check () const;
};

#endif /* _TREE_H_ */
