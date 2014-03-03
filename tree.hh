#include <vector>
#include <string>

enum class tree_arity_v
  {
    NULLARY,
    UNARY,
    BINARY,
    STR,
    INT,
  };

#define TREE_TYPES				\
  TREE_TYPE (SUBX_A_ANY, UNARY)			\
  TREE_TYPE (SUBX_A_NONE, UNARY)		\
  TREE_TYPE (SUBX_A_ALL, UNARY)			\
  TREE_TYPE (CLOSE_PLUS, UNARY)			\
  TREE_TYPE (CLOSE_STAR, UNARY)			\
  TREE_TYPE (MAYBE, UNARY)			\
  TREE_TYPE (CONSTVAL, STR)			\
  TREE_TYPE (PROPREF, STR)			\
  TREE_TYPE (ATVAL, STR)			\
  TREE_TYPE (ATREF, STR)			\
  TREE_TYPE (ATVAL_TRAVERSAL, STR)		\
  TREE_TYPE (TAG_A_YES, STR)			\
  TREE_TYPE (TAG_A_NO, STR)			\
  TREE_TYPE (AT_A_YES, STR)			\
  TREE_TYPE (AT_A_NO, STR)			\
  TREE_TYPE (INT, INT)				\
  TREE_TYPE (PIPE, BINARY)			\
  TREE_TYPE (STR, STR)				\
  TREE_TYPE (AR_ADD, NULLARY)			\
  TREE_TYPE (AR_SUB, NULLARY)			\
  TREE_TYPE (AR_MUL, NULLARY)			\
  TREE_TYPE (AR_DIV, NULLARY)			\
  TREE_TYPE (AR_MOD, NULLARY)			\
  TREE_TYPE (CMP_EQ, NULLARY)			\
  TREE_TYPE (CMP_NE, NULLARY)			\
  TREE_TYPE (CMP_GT, NULLARY)			\
  TREE_TYPE (CMP_GE, NULLARY)			\
  TREE_TYPE (CMP_LT, NULLARY)			\
  TREE_TYPE (CMP_LE, NULLARY)			\
  TREE_TYPE (CMP_MATCH, NULLARY)		\
  TREE_TYPE (SHF_SWAP, NULLARY)			\
  TREE_TYPE (SHF_DUP, NULLARY)			\
  TREE_TYPE (SHF_OVER, NULLARY)			\
  TREE_TYPE (SHF_ROT, NULLARY)			\
  TREE_TYPE (SHF_DROP, NULLARY)			\
  TREE_TYPE (SPREAD, NULLARY)			\
  TREE_TYPE (TR_CHILD, NULLARY)			\
  TREE_TYPE (TR_PARENT, NULLARY)		\
  TREE_TYPE (TR_NEXT, NULLARY)			\
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
    long int ival;
    std::string *sval;
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
  create_int (long v)
  {
    static_assert (tree_arity <TT>::value == tree_arity_v::INT,
		   "Wrong tree arity.");
    auto t = new tree {TT};
    t->m_u.ival = v;
    return t;
  }

  static tree *create_pipe (tree *t1, tree *t2);

  void take_child (tree *t);
  void dump (std::ostream &o) const;
};
