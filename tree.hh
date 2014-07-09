#ifndef _TREE_H_
#define _TREE_H_

#include <vector>
#include <string>
#include <iosfwd>
#include <boost/optional.hpp>

#include "constant.hh"
#include "valfile.hh"
#include "builtin.hh"

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
//
// SCOPE -- A tree that holds one sub-tree and a scope.
enum class tree_arity_v
  {
    NULLARY,
    UNARY,
    BINARY,
    TERNARY,
    STR,
    CST,
    SCOPE,
    BUILTIN,
  };

// CAT -- A node for holding concatenation (X Y Z).
// ALT -- A node for holding alternation (X, Y, Z).
// CAPTURE -- For holding [X].
// OR -- For holding first-match alternation (X || Y || Z)
//
// TRANSFORM -- For holding NUM/X.  The first child is a constant
// representing application depth, the second is the X.
//
// NOP -- For holding a no-op that comes up in "%s" and (,X).
//
// CLOSE_STAR -- For holding X*.  X is the only child.  X+ is emulated
// as (CAT X X*); X? is emulated as (X,).
//
// ASSERT -- All assertions (such as ?some_tag) are modeled using an
// ASSERT node, whose only child is a predicate node expressing the
// asserted condition (such as PRED_TAG).  Negative assertions are
// modeled using PRED_NOT.  In particular, IF is expanded into (!empty
// drop), ELSE into (?empty drop).
//
// PRED_SUBX_ANY -- For holding ?(X).  !() is modeled using PRED_NOT.
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
// F_CAST -- For domain casting.  The argument is a constant, whose
// domain determines what domain to cast to.

#define TREE_TYPES				\
  TREE_TYPE (CAT, BINARY)			\
  TREE_TYPE (ALT, BINARY)			\
  TREE_TYPE (CAPTURE, UNARY)			\
  TREE_TYPE (IFELSE, TERNARY)			\
  TREE_TYPE (OR, BINARY)			\
  TREE_TYPE (SCOPE, SCOPE)			\
  TREE_TYPE (BLOCK, UNARY)			\
  TREE_TYPE (BIND, STR)				\
  TREE_TYPE (READ, STR)				\
  TREE_TYPE (EMPTY_LIST, NULLARY)		\
  TREE_TYPE (TRANSFORM, BINARY)			\
  TREE_TYPE (NOP, NULLARY)			\
  TREE_TYPE (CLOSE_STAR, UNARY)			\
  TREE_TYPE (ASSERT, UNARY)			\
  TREE_TYPE (PRED_AT, CST)			\
  TREE_TYPE (PRED_TAG, CST)			\
  TREE_TYPE (PRED_FIND, NULLARY)		\
  TREE_TYPE (PRED_MATCH, NULLARY)		\
  TREE_TYPE (PRED_EMPTY, NULLARY)		\
  TREE_TYPE (PRED_ROOT, NULLARY)		\
  TREE_TYPE (PRED_AND, BINARY)			\
  TREE_TYPE (PRED_OR, BINARY)			\
  TREE_TYPE (PRED_NOT, UNARY)			\
  TREE_TYPE (PRED_SUBX_ANY, UNARY)		\
  TREE_TYPE (PRED_LAST, NULLARY)		\
  TREE_TYPE (CONST, CST)			\
  TREE_TYPE (STR, STR)				\
  TREE_TYPE (FORMAT, NULLARY)			\
  TREE_TYPE (F_PARENT, NULLARY)			\
  TREE_TYPE (F_CHILD, NULLARY)			\
  TREE_TYPE (F_ATTRIBUTE, NULLARY)		\
  TREE_TYPE (F_ATTR_NAMED, CST)			\
  TREE_TYPE (F_PREV, NULLARY)			\
  TREE_TYPE (F_NEXT, NULLARY)			\
  TREE_TYPE (F_TYPE, NULLARY)			\
  TREE_TYPE (F_OFFSET, NULLARY)			\
  TREE_TYPE (F_NAME, NULLARY)			\
  TREE_TYPE (F_TAG, NULLARY)			\
  TREE_TYPE (F_FORM, NULLARY)			\
  TREE_TYPE (F_VALUE, NULLARY)			\
  TREE_TYPE (F_POS, NULLARY)			\
  TREE_TYPE (F_ELEM, NULLARY)			\
  TREE_TYPE (F_LENGTH, NULLARY)			\
  TREE_TYPE (F_CAST, CST)			\
  TREE_TYPE (F_APPLY, NULLARY)			\
  TREE_TYPE (F_DEBUG, NULLARY)			\
  TREE_TYPE (SEL_UNIVERSE, NULLARY)		\
  TREE_TYPE (SEL_WINFO, NULLARY)		\
  TREE_TYPE (SEL_SECTION, NULLARY)		\
  TREE_TYPE (SEL_UNIT, NULLARY)			\
  TREE_TYPE (F_BUILTIN, BUILTIN)

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

class op;
class pred;
class scope;

// This is for communication between lexical and syntactic analyzers
// and the rest of the world.  It uses naked pointers all over the
// place, as in the %union that the lexer and parser use, we can't
// hold smart pointers.
struct tree
{
  std::unique_ptr <std::string> m_str;
  std::unique_ptr <constant> m_cst;
  std::shared_ptr <scope> m_scope;
  builtin const *m_builtin;

public:
  tree_type m_tt;
  std::vector <tree> m_children;

  tree ();
  explicit tree (tree_type tt);
  tree (tree const &other);

  tree (tree_type tt, std::string const &str);
  tree (tree_type tt, constant const &cst);
  tree (tree_type tt, std::shared_ptr <scope> scope);

  tree &operator= (tree other);
  void swap (tree &other);

  tree &child (size_t idx);
  tree const &child (size_t idx) const;
  tree_type tt () const { return m_tt; }

  std::string &str () const;
  constant &cst () const;
  std::shared_ptr <scope> scp () const;

  void push_child (tree const &t);

  bool operator< (tree const &that) const;

  // === Build interface ===
  //
  // The following methods are implemented in build.cc.  They are for
  // translation of trees to actual execution nodes.

  // Remove unnecessary operations--some stack shuffling can be
  // eliminated and protect nodes.
  // XXX this should actually be hidden behind build_exec or what not.
  void simplify ();

  // This should build an op node corresponding to this expression.
  //
  // Not every expression node needs to have an associated op, some
  // nodes will only install some plumbing (in particular, a CAT node
  // would only create a series of nested op's).  UPSTREAM should be
  // nullptr if this is the toplevel-most expression, otherwise it
  // should be a valid op that the op produced by this node feeds off.
  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope = {}) const;

  // Produce program suitable for interpretation.
  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const;

  // === Parser interface ===
  //
  // The following methods are implemented in tree_cr.hh and
  // tree_cr.cc.  They are meant as parser interface, otherwise value
  // semantics should be preferred.

  template <tree_type TT> static tree *create_nullary ();
  template <tree_type TT> static tree *create_unary (tree *op);
  template <tree_type TT> static tree *create_binary (tree *lhs, tree *rhs);
  template <tree_type TT> static tree *create_ternary (tree *op1, tree *op2,
						       tree *op3);

  template <tree_type TT> static tree *create_str (std::string s);
  template <tree_type TT> static tree *create_const (constant c);
  template <tree_type TT> static tree *create_cat (tree *t1, tree *t2);

  static tree *create_builtin (builtin const *b);

  static tree *create_neg (tree *t1);
  static tree *create_assert (tree *t1);

  static tree promote_scopes (tree t, std::shared_ptr <scope> parent = {});

  // push_back (*T) and delete T.
  void take_child (tree *t);

  // push_front (*T) and delete T.
  void take_child_front (tree *t);

  // Takes a tree T, which is a CAT or an ALT, and appends all
  // children therein.  It then deletes T.
  void take_cat (tree *t);

  friend std::ostream &operator<< (std::ostream &o, tree const &t);
};

std::ostream &operator<< (std::ostream &o, tree const &t);

#endif /* _TREE_H_ */
