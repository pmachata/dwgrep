#include "dwgrep.hh"
#include <iostream>

int
main(int argc, char *argv[])
{
  dwgrep_expr expr ("uni dup mul");

  for (auto vf: expr.query (dwgrep_graph ()))
    {
      std::cout << "[";
      for (auto v: vf)
	std::cout << " " << v;
      std::cout << " ]\n";
    }

  return 0;
}
