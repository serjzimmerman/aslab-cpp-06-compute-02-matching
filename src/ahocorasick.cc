#include "popl.hpp"

#include "ahocorasick/automaton.hpp"

int main() {
  ac::automaton_builder at;
  const std::array names = {"a", "ab", "bc", "bca", "c", "caa"};
  at.add(names.begin(), names.end());
  at.build_links();
  at.dump();
}