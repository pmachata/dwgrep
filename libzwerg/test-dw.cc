#include <gtest/gtest.h>

#include "builtin.hh"
#include "builtin-dw.hh"
#include "init.hh"
#include "value-dw.hh"
#include "stack.hh"
#include "parser.hh"
#include "op.hh"

std::string
test_file (std::string name)
{
  extern std::string g_test_case_directory;
  return g_test_case_directory + "/" + name;
}

struct ZwTest
  : public testing::Test
{
  std::unique_ptr <vocabulary> builtins;

  void
  SetUp () override final
  {
    builtins = std::make_unique <vocabulary>
      (*dwgrep_vocabulary_core (), *dwgrep_vocabulary_dw ());
  }
};

namespace
{
  std::unique_ptr <value_dwarf>
  dw (std::string fn, doneness d)
  {
    return std::make_unique <value_dwarf> (test_file (fn), 0, d);
  }
}

TEST (DwValueTest, dwarf_sanity)
{
  auto cooked = dw ("empty", doneness::cooked);
  ASSERT_TRUE (cooked->is_cooked ());
  ASSERT_FALSE (cooked->is_raw ());

  auto raw = dw ("empty", doneness::raw);
  ASSERT_TRUE (raw->is_raw ());
  ASSERT_FALSE (raw->is_cooked ());
}

namespace
{
  std::unique_ptr <stack>
  stack_with_value (std::unique_ptr <value> v)
  {
    auto stk = std::make_unique <stack> ();
    stk->push (std::move (v));
    return stk;
  }

  std::vector <std::unique_ptr <stack>>
  run_query (vocabulary &voc,
	     std::unique_ptr <stack> stk, std::string q)
  {
    std::shared_ptr <op> op = parse_query (voc, q)
      .build_exec (std::make_shared <op_origin> (std::move (stk)));

    std::vector <std::unique_ptr <stack>> yielded;
    while (auto r = op->next ())
      yielded.push_back (std::move (r));

    return yielded;
  }

  // A query on a stack with sole value, which is a Dwarf.
  std::vector <std::unique_ptr <stack>>
  run_dwquery (vocabulary &voc, std::string fn, std::string q)
  {
    return run_query (voc, stack_with_value (dw (fn, doneness::cooked)), q);
  }

#define SOLE_YIELDED_VALUE(TYPE, YIELDED)				\
  ({									\
    std::vector <std::unique_ptr <stack>> &_yielded = (YIELDED);	\
    									\
    ASSERT_EQ (1, yielded.size ())					\
      << "One result expected.";					\
									\
    ASSERT_EQ (1, yielded[0]->size ())					\
      << "Stack with one value expected.";				\
									\
    auto produced = value::as <TYPE> (&yielded[0]->get (0));		\
    ASSERT_TRUE (produced != nullptr);					\
									\
    *produced;								\
  })
}

TEST_F (ZwTest, words_dwarf_raw_cooked)
{
  {
    auto yielded = run_query
      (*builtins, stack_with_value (dw ("empty", doneness::cooked)),
       "raw");

    auto produced = SOLE_YIELDED_VALUE (value_dwarf, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_query
      (*builtins, stack_with_value (dw ("empty", doneness::raw)),
       "cooked");

    auto produced = SOLE_YIELDED_VALUE (value_dwarf, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }
}

TEST_F (ZwTest, words_cu_raw_cooked)
{
  {
    auto yielded = run_dwquery (*builtins, "empty", "unit");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "unit raw");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit cooked");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "entry raw unit");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }
}

TEST_F (ZwTest, words_die_raw_cooked)
{
  {
    auto yielded = run_dwquery (*builtins, "empty", "entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "entry raw");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw entry cooked");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "twocus", "raw unit root");
    ASSERT_EQ (2, yielded.size ());
    for (auto &stk: yielded)
      {
	ASSERT_EQ (1, stk->size ());
	auto produced = value::as <value_die> (&stk->get (0));
	ASSERT_TRUE (produced != nullptr);
	ASSERT_TRUE (produced->is_raw ());
      }
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }
}

TEST_F (ZwTest, cooked_attribute_assertion_integrates_attributes)
{
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) ?AT_name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) !AT_name").size ());
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) @AT_name").size ());

  // But it shouldn't blindly integrate everything.
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) ?AT_declaration").size ());
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) !AT_declaration").size ());

  // And shouldn't integrate in raw mode either.
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw ?AT_name").size ());
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw !AT_name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw @AT_name").size ());
}

TEST_F (ZwTest, name_on_raw_cooked_die)
{
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw name").size ());
}
