#include "popl.hpp"

#include "ahocorasick/automaton.hpp"

int main() {
  ac::automaton_builder at;
  const std::array names = {"rac", "barak", "ab"};
  at.add(names.begin(), names.end());
  at.build_links();

  auto searcher = at.make_cpu_searcher();
  auto res = searcher.search("abracadabra");

  for (const auto &v : res.patterns) {
    std::cout << v.first << ": ";
    for (const auto &p : v.second) {
      std::cout << p << " ";
    }
    std::cout << "\n";
  }
}