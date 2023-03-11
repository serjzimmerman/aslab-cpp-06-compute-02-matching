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
#include <cstdint>
#include <iostream>
#include <string_view>

namespace ac {

class automaton {
private:
  using identifier_type = int;

  static constexpr identifier_type k_input_state = 0;
  static constexpr identifier_type k_none_state = -1;

  struct node_attributes {
    std::string name;
    identifier_type failure_link = k_none_state, output_link = k_none_state;
  };

  using graph_type = graphs::basic_directed_graph<identifier_type, node_attributes, char>;

private:
  std::unordered_map<std::string, identifier_type> m_name_to_key_map;
  graph_type m_graph;

private:
  auto create_node(std::string_view view) {
    const identifier_type key = m_graph.vertices();
    auto result = m_graph.insert({key, node_attributes{std::string{view}}});

    assert(result && "[Debug]: broken automaton::create_node()");

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
  automaton() {
    m_graph.insert({k_input_state, node_attributes{}});
    m_name_to_key_map[""] = k_input_state;
  }

  void add_pattern(std::string view) {
    auto curr_node = m_graph.find(k_input_state);
    std::string curr_string;

    for (const auto &c : view) {
      curr_string += c;
      auto &&adj_list = curr_node->second;

      auto start = adj_list.begin();
      for (auto finish = adj_list.end(); start != finish; ++start) {
        if (start->edge == c) break;
      }

      if (start != adj_list.end()) {
        curr_node = m_graph.find(start->key);
        continue;
      }

      auto new_node = create_node(curr_string);
      m_graph.create_link(curr_node->first, new_node->first, c);
      curr_node = new_node;
      m_name_to_key_map.insert({curr_string, new_node->first});
    }
  }

  void build_links() {
    auto compute_visit = [&](identifier_type key) -> void {
      // Step 1. Check if node is the root
      if (key == k_input_state) return;

      // Step 2. Check if the node is one step away from the root.
      auto &wa_attributes = get_attributes(key);
      std::string_view name = wa_attributes.name;
      if (name.length() == 1) {
        wa_attributes.failure_link = k_input_state;
        return;
      }

      const char a = name.back();
      name = name.substr(0, name.length() - 1);

      identifier_type w = m_name_to_key_map.at(std::string{name});
      auto w_attributes = get_attributes(w);

      identifier_type x = w_attributes.failure_link;

      while (true) {
        assert(x != k_none_state);

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
    };

    graphs::breadth_first_search(m_graph, k_input_state, [compute_visit](auto &&node) -> void {
      compute_visit(node->key);
    });
  }

  friend struct dumper;

private:
  struct dumper {
    // 1. Automaton to dump
    const automaton &at;

    // 2. Print related info
    std::ostream &os;

  private:
    std::ostream &begin_subgraph(unsigned rank) {
      os << "subgraph rank_" << rank << " {\n";
      return os << "rank=same;\n";
    };

    std::ostream &end_block() {
      os << "\n}\n\n";
      return os;
    }

    std::ostream &begin_digraph() {
      os << "digraph {\n";
      return os << "graph [outputorder=nodesfirst];\n\n";
    }

    void print_declare_node(identifier_type node) { os << node << "; "; }

    template <typename T>
    void print_bind_node(
        identifier_type parent, identifier_type child, T &&label, std::string_view color = "black",
        std::string_view attributes = ""
    ) {
      os << parent << " -> " << child << " [label = \"" << std::forward<T>(label) << "\", color = \"" << color << "\"";
      if (attributes.length()) os << (attributes.empty() ? "" : ", ") << attributes << "";
      os << "];\n";
    }

  public:
    dumper(const automaton &p_at, std::ostream &p_os) : at{p_at}, os{p_os} {}

    void dump() {
      begin_digraph();

      graphs::breadth_first_search(at.m_graph, automaton::k_input_state, [&, rank = -1](auto &&node) mutable -> void {
        std::string_view name = node->attr.name;
        const identifier_type key = node->key;
        const auto signed_length = static_cast<int>(name.length());
        assert(signed_length >= rank);

        if (signed_length == 0) {
          rank = 0;
          begin_subgraph(rank);
          print_declare_node(key);
          return;
        }

        else if (signed_length == rank) {
          print_declare_node(key);
          return;
        }

        rank = signed_length;
        end_block();
        begin_subgraph(rank);
        print_declare_node(key);
      });

      end_block();

      for (const auto &v : at.m_graph) {
        for (const auto &e : v.second) {
          print_bind_node(v.second->key, e.key, e.edge);
        }
        if (auto failure = v.second->attr.failure_link; failure == k_none_state || failure == k_input_state) continue;
        print_bind_node(v.second->key, v.second->attr.failure_link, "", "red", "constraint=false");
      }

      end_block();
    }
  };

public:
  void dump(std::ostream &os = std::cout) const {
    dumper dot_dumper = {*this, os};
    dot_dumper.dump();
  }
};

} // namespace ac