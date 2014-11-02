#include <stdexcept>
#include <cassert>
#include "int.hh"

namespace
{
  template <class F>
  bool
  catch_domain_error (char const *s, F f)
  {
    try
      {
	f ();
	return false;
      }
    catch (std::domain_error &e)
      {
	assert (std::string (e.what ()).find (s) == 0);
	return true;
      }
  }
}

#define assert_exception(S, F)			\
  assert (catch_domain_error (S, [] () { F; }))

int
main(int argc, char *argv[])
{
  assert (mpz_class (UINT64_MAX) == mpz_class (UINT64_MAX));
  assert (mpz_class (UINT64_MAX) != mpz_class (-1));
  assert (mpz_class (-1) == mpz_class (-1));
  assert (mpz_class (0, signedness::sign) == mpz_class (0, signedness::unsign));
  assert (mpz_class (-2) < mpz_class (-1));
  assert (mpz_class (-2) < mpz_class (0U));
  assert (mpz_class (-2) < mpz_class (1U));

  assert (mpz_class (UINT64_MAX) == UINT64_MAX);
  assert (mpz_class (UINT64_MAX) != -1);
  assert (mpz_class (-1) == -1);
  assert (mpz_class (-2) < -1);
  assert (mpz_class (-2) < 0U);
  assert (mpz_class (-2) < 1U);
  assert (mpz_class (-2) < 0);


  assert (mpz_class (10U) + 15U == 25);
  assert (mpz_class (10U) + 5U == 15);
  assert (mpz_class (10U) + 15 == 25);
  assert (mpz_class (10U) + 5 == 15);
  assert (mpz_class (10U) + -15 == -5);
  assert (mpz_class (10U) + -5 == 5);

  assert (mpz_class (10) + 15U == 25);
  assert (mpz_class (10) + 5U == 15);
  assert (mpz_class (10) + 15 == 25);
  assert (mpz_class (10) + 5 == 15);
  assert (mpz_class (10) + -15 == -5);
  assert (mpz_class (10) + -5 == 5);

  assert (mpz_class (-10) + 15U == 5);
  assert (mpz_class (-10) + 5U == -5);
  assert (mpz_class (-10) + 15 == 5);
  assert (mpz_class (-10) + 5 == -5);
  assert (mpz_class (-10) + -15 == -25);
  assert (mpz_class (-10) + -5 == -15);


  assert (mpz_class (10U) - 15U == -5);
  assert (mpz_class (10U) - 5U == 5U);
  assert (mpz_class (10U) - 15 == -5);
  assert (mpz_class (10U) - 5 == 5U);
  assert (mpz_class (10U) - -15 == 25);
  assert (mpz_class (10U) - -5 == 15U);

  assert (mpz_class (10) - 15U == -5);
  assert (mpz_class (10) - 5U == 5U);
  assert (mpz_class (10) - 15 == -5);
  assert (mpz_class (10) - 5 == 5);
  assert (mpz_class (10) - -15 == 25);
  assert (mpz_class (10) - -5 == 15);

  assert (mpz_class (-10) - 15U == -25);
  assert (mpz_class (-10) - 5U == -15);
  assert (mpz_class (-10) - 15 == -25);
  assert (mpz_class (-10) - 5 == -15);
  assert (mpz_class (-10) - -15 == 5);
  assert (mpz_class (-10) - -5 == -5);


  assert (mpz_class (10U) * 15U == 150);
  assert (mpz_class (10U) * 15 == 150);
  assert (mpz_class (10U) * -15 == -150);
  assert (mpz_class (10U) * 0U == 0);
  assert (mpz_class (10U) * 0 == 0);
  assert (mpz_class (0U) * 10U == 0);
  assert (mpz_class (0) * 10U == 0);

  assert (mpz_class (10) * 15U == 150);
  assert (mpz_class (10) * 15 == 150);
  assert (mpz_class (10) * -15 == -150);
  assert (mpz_class (10) * 0U == 0);
  assert (mpz_class (10) * 0 == 0);
  assert (mpz_class (0U) * 10 == 0);
  assert (mpz_class (0) * 10 == 0);

  assert (mpz_class (-10) * 15U == -150);
  assert (mpz_class (-10) * 15 == -150);
  assert (mpz_class (-10) * -15 == 150);
  assert (mpz_class (-10) * 0U == 0);
  assert (mpz_class (-10) * 0 == 0);

  assert_exception ("division", mpz_class (1u) / 0);
  assert_exception ("division", mpz_class (1) / 0);
  assert_exception ("division", mpz_class (-1) / 0);
  assert_exception ("division", mpz_class (1u) / 0u);
  assert_exception ("division", mpz_class (1) / 0u);
  assert_exception ("division", mpz_class (-1) / 0u);

  assert (mpz_class (10U) / 2U == 5);
  assert (mpz_class (10U) / 2 == 5);
  assert (mpz_class (10U) / -2 == -5);
  assert (mpz_class (10) / 2U == 5);
  assert (mpz_class (10) / 2 == 5);
  assert (mpz_class (10) / -2 == -5);
  assert (mpz_class (-10) / 2U == -5);
  assert (mpz_class (-10) / 2 == -5);
  assert (mpz_class (-10) / -2 == 5);

  assert (mpz_class (10U) / 3U == 3);
  assert (mpz_class (10U) / 3 == 3);
  assert (mpz_class (10U) / -3 == -4);
  assert (mpz_class (10) / 3U == 3);
  assert (mpz_class (10) / 3 == 3);
  assert (mpz_class (10) / -3 == -4);
  assert (mpz_class (-10) / 3U == -4);
  assert (mpz_class (-10) / 3 == -4);
  assert (mpz_class (-10) / -3 == 3);

  assert (mpz_class (10U) % 2U == 0);
  assert (mpz_class (10U) % 2 == 0);
  assert (mpz_class (10U) % -2 == 0);
  assert (mpz_class (10) % 2U == 0);
  assert (mpz_class (10) % 2 == 0);
  assert (mpz_class (10) % -2 == 0);
  assert (mpz_class (-10) % 2U == 0);
  assert (mpz_class (-10) % 2 == 0);
  assert (mpz_class (-10) % -2 == 0);

  assert (mpz_class (10U) % 3U == 1);
  assert (mpz_class (10U) % 3 == 1);
  assert (mpz_class (10U) % -3 == -2);
  assert (mpz_class (10) % 3U == 1);
  assert (mpz_class (10) % 3 == 1);
  assert (mpz_class (10) % -3 == -2);
  assert (mpz_class (-10) % 3U == 2);
  assert (mpz_class (-10) % 3 == 2);
  assert (mpz_class (-10) % -3 == -1);

  // INT64_MIN is an even number, so the following should hold.
  assert (mpz_class (INT64_MIN / 2) + (INT64_MIN / 2) == INT64_MIN);

  assert (mpz_class (INT64_MAX) + INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (INT64_MAX) + (uint64_t) INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class ((uint64_t) INT64_MAX) + INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (INT64_MIN) + 1U == INT64_MIN + 1);
  assert (mpz_class (INT64_MIN) + 1 == INT64_MIN + 1);
  assert (mpz_class (1U) + INT64_MIN == INT64_MIN + 1);
  assert (mpz_class (1) + INT64_MIN == INT64_MIN + 1);

  assert (mpz_class (10U) - INT64_MIN == (uint64_t) INT64_MAX + 11);
  assert (mpz_class (INT64_MIN) - -10 == INT64_MIN + 10);

  assert (mpz_class (1) * INT64_MIN == INT64_MIN);
  assert (mpz_class (1U) * INT64_MIN == INT64_MIN);
  assert (mpz_class (-1) * INT64_MIN == (uint64_t) INT64_MAX + 1);
  assert (mpz_class (INT64_MIN) * 1 == INT64_MIN);
  assert (mpz_class (INT64_MIN) * 1U == INT64_MIN);
  assert (mpz_class (INT64_MIN) * -1 == (uint64_t) INT64_MAX + 1);
  assert (mpz_class (2) * INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (2U) * INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (INT64_MAX) * 2 == UINT64_MAX - 1);
  assert (mpz_class (INT64_MAX) * 2U == UINT64_MAX - 1);
  assert_exception ("overflow", mpz_class (UINT64_MAX) * -1);
  assert_exception ("overflow", mpz_class ((uint64_t) INT64_MAX + 2) * -1);

  assert (mpz_class (INT64_MIN) / -1 == (uint64_t) INT64_MAX + 1);
  assert (mpz_class (INT64_MAX) / -1 == INT64_MIN + 1);
  assert_exception ("overflow", mpz_class (UINT64_MAX) / -1);
  assert_exception ("overflow", mpz_class (UINT64_MAX) / -2);

  return 0;
}
