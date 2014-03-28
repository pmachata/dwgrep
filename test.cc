#include "dwgrep.hh"
#include <iostream>

int
main(int argc, char *argv[])
{
  dwgrep_expr_t expr ("uni dup mul");

  for (auto vf: expr.query (std::make_shared <problem> ()))
    {
      std::cout << "[";
      for (auto v: vf)
	std::cout << " " << v;
      std::cout << " ]\n";
    }

  return 0;
}
