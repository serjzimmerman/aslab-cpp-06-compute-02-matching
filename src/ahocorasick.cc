#include "ahocorasick/automaton.hpp"

#include "popl.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <istream>
#include <stdexcept>

namespace {

std::string read_strn(std::istream &is, unsigned long n) {
  std::string str;
  std::copy_n(std::istream_iterator<char>{is >> std::ws >> std::noskipws}, n, std::back_inserter(str));
  is >> std::skipws;
  return str;
}

std::string read_file(const std::filesystem::path &filepath) {
  std::ifstream is;

  is.exceptions(is.exceptions() | std::ios::badbit);
  is.open(filepath);

  std::string str;
  is >> std::noskipws;
  std::copy(std::istream_iterator<char>{is}, std::istream_iterator<char>{}, std::back_inserter(str));

  return str;
}

} // namespace

int main(int argc, char **argv) {
  popl::OptionParser op("Avaliable options");
  auto help_option = op.add<popl::Switch>("h", "help", "Print this help message");
  auto icustom_option = op.add<popl::Switch>("c", "icustom", "Specify file path for the input file");
  auto verbose_option = op.add<popl::Switch>("v", "verbose", "Verbose output");
  op.parse(argc, argv);

  if (help_option->is_set()) {
    std::cout << op << "\n ";
    return EXIT_SUCCESS;
  }

  std::cin.exceptions(std::cin.exceptions() | std::ios::badbit);

  const auto get_string = []() -> std::string {
    unsigned long length;
    std::cin >> length;
    return read_strn(std::cin, length);
  };

  std::string tosearch;
  if (icustom_option->is_set()) {
    std::string filepath;
    std::getline(std::cin, filepath);
    tosearch = read_file(filepath);
  }

  else {
    tosearch = get_string();
  }

  unsigned count;
  std::cin >> count;

  std::vector<std::string> patterns;
  for (unsigned i = 0; i < count; ++i) {
    patterns.push_back(get_string());
  }

  ac::automaton_builder at;
  at.add(patterns.begin(), patterns.end());
  at.build_links();
  auto searcher = at.make_cpu_searcher();
  auto res = searcher.search(tosearch);

  const auto print_verbose_result = [](auto &&res) {
    for (const auto &v : res.patterns) {
      std::cout << v.first << ": " << v.second.size() << "\n";
    }
  };

  const auto print_result = [&](auto &&res) {
    for (unsigned i = 0; const auto &v : patterns) {
      std::cout << ++i << ": " << res.patterns.at(v).size() << "\n";
    }
  };

  if (verbose_option->is_set()) {
    print_verbose_result(res);
  } else {
    print_result(res);
  }
}