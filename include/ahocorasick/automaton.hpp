/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <tsimmerman.ss@phystech.edu>, <alex.rom23@mail.ru> wrote this file.  As long as you
 * retain this notice you can do whatever you want with this stuff. If we meet
 * some day, and you think this stuff is worth it, you can buy us a beer in
 * return.
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "graphs/directed_graph.hpp"

#include <bit>
#include <cassert>
#include <climits>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace ac {

class cpu_searcher;

class automaton_builder {
public:
  using identifier_type = int;

  static constexpr identifier_type k_input_state = 0;
  static constexpr identifier_type k_none_state = -1;

  struct node_attributes {
    std::string name;
    identifier_type failure_link = k_none_state, output_link = k_none_state; // Output link can't point to the input.
    // Thus, it can be used to indicate that the is no output link.
    bool accepting = false;
  };

  using graph_type = graphs::basic_directed_graph<identifier_type, node_attributes, char>;

private:
  std::unordered_map<std::string, identifier_type> m_name_to_key_map;
  graph_type m_graph;

private:
  auto create_node(std::string_view view) {
    const identifier_type key = m_graph.vertices();
    auto result = m_graph.insert({key, node_attributes{std::string{view}}});
    assert(result && "[Debug]: Broken automaton::create_node()");
    return m_graph.find(key);
  }

  auto &get_attributes(identifier_type key) & {
    if (auto found = m_graph.find(key); found != m_graph.end()) {
      return found->second->attr;
    }
    assert(0 && "[Debug]: Broken automaton::get_attributes()");
  }

  auto &get_attributes(identifier_type key) const & {
    if (auto found = m_graph.find(key); found != m_graph.end()) {
      return found->second->attr;
    }
    assert(0 && "[Debug]: Broken automaton::get_attributes()");
  }

public:
  automaton_builder() {
    m_graph.insert({k_input_state, node_attributes{}});
    m_name_to_key_map[""] = k_input_state;
  }

  template <std::input_iterator It> automaton_builder(It start, It finish) { add(start, finish); }

  template <std::convertible_to<std::string_view> T>
  automaton_builder(std::initializer_list<T> list) : automaton_builder{list.begin(), list.end()} {}

  void add(std::string_view view) {
    if (view.empty()) return;

    auto curr_node = m_graph.find(k_input_state);
    std::string curr_string;

    for (auto cstart = view.cbegin(), cfinish = view.cend(); cstart != cfinish; ++cstart) {
      const char &c = *cstart;

      curr_string += c;
      auto &&adj_list = curr_node->second;

      auto start = adj_list.begin();
      for (auto finish = adj_list.end(); start != finish; ++start) {
        if (start->edge == c) break;
      }

      if (start != adj_list.end()) {
        curr_node = m_graph.find(start->key);
        if (cstart == std::prev(cfinish)) curr_node->second->attr.accepting = true;
        continue;
      }

      auto new_node = create_node(curr_string);
      m_graph.create_link(curr_node->first, new_node->first, c);
      curr_node = new_node;
      m_name_to_key_map.insert({curr_string, new_node->first});

      if (cstart == std::prev(cfinish)) curr_node->second->attr.accepting = true;
    }
  }

  template <std::input_iterator It>
    requires std::convertible_to<typename std::iterator_traits<It>::value_type, std::string_view>
  void add(It start, It finish) {
    for (; start != finish; ++start) {
      add(*start);
    }
  }

  void build_links() {
    auto compute_visit = [&](identifier_type key) -> void {
      // Step 1. Check if node is the root
      if (key == k_input_state) return;

      // Step 2. Check if the node is one step away from the root.
      auto &wa_attributes = get_attributes(key);
      const auto fill_output_link = [&]() {
        assert(wa_attributes.failure_link != k_none_state);
        auto suffix_node_attributes = get_attributes(wa_attributes.failure_link);

        if (suffix_node_attributes.accepting) {
          wa_attributes.output_link = wa_attributes.failure_link;
        } else {
          wa_attributes.output_link = suffix_node_attributes.output_link;
        }
      };

      std::string_view name = wa_attributes.name;
      if (name.length() == 1) {
        wa_attributes.failure_link = k_input_state;
        fill_output_link();
        return;
      }

      const char a = name.back();
      name = name.substr(0, name.length() - 1);

      identifier_type w = m_name_to_key_map.at(std::string{name});
      auto w_attributes = get_attributes(w);
      identifier_type x = w_attributes.failure_link;

      while (true) {
        assert(x != k_none_state && "[Debug]: Internal error in automaton::build_links()");

        auto x_attributes = get_attributes(x);
        std::string xa_name = x_attributes.name + a;

        if (auto xa = m_name_to_key_map.find(xa_name); xa != m_name_to_key_map.end()) {
          wa_attributes.failure_link = xa->second;
          break;
        }

        if (x == k_input_state) {
          wa_attributes.failure_link = k_input_state;
          break;
        }

        x = x_attributes.failure_link;
      }

      fill_output_link();
    };

    graphs::breadth_first_search(m_graph, k_input_state, [compute_visit](auto &&node) -> void {
      compute_visit(node->key);
    });

    for (auto &node : m_graph) {
      auto &attr = node.second->attr;
      if (attr.output_link == k_none_state) attr.output_link = k_input_state;
    }

    get_attributes(k_input_state).failure_link = k_input_state;

#ifndef NDEBUG
    // Verify that all node indices are positive.
    for (const auto &node : m_graph) {
      if (node.second->key == k_input_state) continue;
      const auto &attr = node.second->attr;
      assert(attr.failure_link >= 0);
      assert(attr.output_link >= 0);
    }
#endif
  }

  friend struct dumper;

private:
  struct dumper {
    // 1. Automaton to dump
    const automaton_builder &at;

    // 2. Print related info
    std::ostream &os;

  private:
    std::ostream &begin_subgraph(unsigned rank) {
      os << "subgraph rank_" << rank << " {\n";
      return os << "rank = same;\n";
    };

    std::ostream &end_block() {
      os << "\n}\n\n";
      return os;
    }

    std::ostream &begin_digraph() {
      os << "digraph {\n"
         << "rankdir = \"LR\";\n\n";
      return os << "graph [outputorder=nodesfirst];\n\n";
    }

    std::ostream &print_declare_node(identifier_type node, std::string_view attributes = "") {
      const auto name = at.get_attributes(node).name;
      os << node << " ["
         << "label = \"" << name << "\"";
      if (attributes.length()) os << (attributes.empty() ? "" : ", ") << attributes << "";
      return os << "];\n";
    }

    template <typename T>
    std::ostream &print_bind_node(
        identifier_type parent, identifier_type child, T &&label, std::string_view color = "black",
        std::string_view attributes = ""
    ) {
      os << parent << " -> " << child << " [label = \"" << std::forward<T>(label) << "\", color = \"" << color << "\"";
      if (attributes.length()) os << (attributes.empty() ? "" : ", ") << attributes << "";
      return os << "];\n";
    }

  public:
    dumper(const automaton_builder &p_at, std::ostream &p_os) : at{p_at}, os{p_os} {}

    void dump() {
      begin_digraph();

      graphs::breadth_first_search(
          at.m_graph, automaton_builder::k_input_state,
          [&, rank = -1](auto &&node) mutable -> void {
            std::string_view name = node->attr.name;
            const identifier_type key = node->key;
            const auto signed_length = static_cast<int>(name.length()), old_rank = rank;

            assert(signed_length >= rank && "[Debug]: Internal error in AC dumper");
            rank = signed_length;

            const auto declare_key = [&](auto &&key) {
              bool accepting = at.m_graph.find(key)->second->attr.accepting;
              const auto attributes = (accepting ? "shape=doublecircle" : "shape=oval");
              print_declare_node(key, attributes);
            };

            if (signed_length == 0) {
              begin_subgraph(rank);
              declare_key(key);
              return;
            }

            else if (signed_length == old_rank) {
              declare_key(key);
              return;
            }

            end_block();
            begin_subgraph(rank);
            declare_key(key);
          }
      );

      end_block();

      for (const auto &v : at.m_graph) {
        for (const auto &e : v.second) {
          print_bind_node(v.first, e.key, e.edge);
        }

        if (auto failure = v.second->attr.failure_link; !(failure == k_none_state || failure == k_input_state)) {
          print_bind_node(v.first, failure, "", "red", "constraint=false");
        }

        if (auto output = v.second->attr.output_link; output != k_input_state) {
          assert(output != k_none_state && "[Debug]: Internal error. Output link pointing to placeholder node");
          print_bind_node(v.first, output, "", "blue", "constraint=false");
        }
      }

      end_block();
    }
  };

public:
  void dump(std::ostream &os = std::cout) const {
    dumper dot_dumper = {*this, os};
    dot_dumper.dump();
  }

  cpu_searcher make_cpu_searcher() const;
};

class cpu_searcher {
public:
  static constexpr unsigned k_max_size = 1 << CHAR_BIT;
  using index_type = uint16_t;

  struct node {
    index_type failure_link, output_link;
    bool accepting;
    std::vector<index_type> children;
  };

private:
  // Patterns that are being searched for
  std::unordered_map<index_type, std::string> m_patterns;

  // Lookup array to map input characters to custom alphabet
  std::array<unsigned char, k_max_size> m_alpha_lookup;

  // Flat automaton representation
  std::vector<node> m_nodes;
  unsigned char m_none_link;

private:
  template <typename It> static auto create_lookup_table(It start, It finish, unsigned char other) {
    decltype(m_alpha_lookup) table = {};

    std::fill(table.begin(), table.end(), other);
    for (; start != finish; ++start) {
      unsigned char c = start->first; // Cast to unsigned
      table[c] = start->second;
    }

    return table;
  }

public:
  cpu_searcher(const automaton_builder::graph_type &graph) {
    std::unordered_map<char, unsigned> used_chars;

    unsigned i = 0;
    for (const auto &node : graph) {
      const auto &attr = node.second->attr;
      std::for_each(attr.name.begin(), attr.name.end(), [&](char c) {
        if (used_chars.insert({c, i}).second) ++i;
      });
    }
    m_none_link = i;

    m_alpha_lookup = create_lookup_table(used_chars.cbegin(), used_chars.cend(), used_chars.size());
    assert(graph.vertices() <= std::numeric_limits<index_type>::max() && "Automaton to large to represent in memory");

    const auto alphabet_size = used_chars.size();
    m_nodes.resize(graph.vertices());
    for (const auto &v : graph) {
      auto &attr = v.second->attr;
      auto &curr_node = m_nodes.at(v.first);

      curr_node.failure_link = attr.failure_link;
      curr_node.output_link = attr.output_link;
      curr_node.accepting = attr.accepting;

      curr_node.children.resize(alphabet_size);
      if (attr.accepting) {
        m_patterns.insert({v.first, attr.name});
      }

      for (const auto &e : v.second) {
        const auto c = static_cast<unsigned char>(e.edge);
        const auto mapped_index = m_alpha_lookup.at(c);
        curr_node.children.at(mapped_index) = e.key;
      }
    }
  }

  struct search_result {
    using occurences_type = std::vector<std::string_view::size_type>;
    std::unordered_map<std::string, occurences_type> patterns;
  };

  search_result search(std::string_view view) const {
    search_result result;

    for (const auto &v : m_patterns) {
      result.patterns.insert({v.second, {}});
    }

    const auto none_link = m_none_link;

    using match_type = std::pair<index_type, std::string_view::size_type>;
    std::vector<match_type> matches;

    index_type curr_index = 0;
    const auto *curr_state = &m_nodes.at(curr_index);

    const auto follow_output_links = [&](index_type index, auto pos) {
      const node *state = &m_nodes.at(index);
      while (state->output_link) {
        index = state->output_link;
        state = &m_nodes.at(index);
        assert(state->accepting && "[Debug]: Broken output link chain. The state should be accepting");
        matches.push_back({index, pos});
      }
    };

    std::string_view::size_type pos = 0;
    for (char signed_c : view) {
      assert(curr_state);

      ++pos;

      unsigned char c = signed_c;
      const auto mapped = m_alpha_lookup.at(c);

      if (mapped == none_link) {
        curr_index = 0;
        curr_state = &m_nodes.at(curr_index);
        continue;
      }

      while (curr_state->children.at(mapped) == 0 && curr_index != 0) { // No next link
        curr_index = curr_state->failure_link;
        curr_state = &m_nodes.at(curr_index);
      }

      curr_index = curr_state->children.at(mapped);
      curr_state = &m_nodes.at(curr_index);

      follow_output_links(curr_index, pos);
      if (curr_state->accepting) {
        matches.push_back({curr_index, pos});
      }
    }

    for (const auto &v : matches) {
      const auto &pattern = m_patterns.at(v.first);
      result.patterns.at(pattern).push_back(v.second - pattern.length());
    }

    return result;
  }
};

inline cpu_searcher automaton_builder::make_cpu_searcher() const {
  return cpu_searcher{m_graph};
}

} // namespace ac