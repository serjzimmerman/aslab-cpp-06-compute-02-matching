#include "popl.hpp"

#include "ahocorasick/automaton.hpp"

int main() {
  ac::automaton at;

  at.add_pattern("ta");
  at.add_pattern("ate");
  at.add_pattern("hello");
  at.add_pattern("coats");
  at.add_pattern("so");
  at.add_pattern("test");
  at.add_pattern("hell");
  at.add_pattern("temper");
  at.add_pattern("tempest");
  at.add_pattern("temporary");
  at.add_pattern("cool");

  at.build_links();

  at.dump();
}