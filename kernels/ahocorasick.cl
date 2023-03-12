/*
 *
 *  @kernel    ( {"name" : "aho_corasick_kernel", "entry" : "search"} )
 *  @signature ( ["cl::Buffer", "cl::Buffer", "cl_uint", "cl::Buffer"] )
 *  @macros    ( [{"type" : "unsigned char", "name": "ALPHA_SIZE"}, {"type" : "std::string", "name": "INDEX_TYPE"} ] )
 *
 */

/* Compile to llvm-ir: clang -c -S -O2 -cl-std=CL3.0 -emit-llvm -o kernel.ll kernels/ahocorasick.cl */

#ifndef INDEX_TYPE
#define INDEX_TYPE ushort
#endif

#ifndef ALPHA_SIZE
#define ALPHA_SIZE 0
#endif

typedef struct __attribute__((packed)) s_automaton_node {
  INDEX_TYPE failure_link, output_link;
  uint accepting; /* Store index of matching pattern. If == 0, then the state is not accepting */
  INDEX_TYPE children_links[ALPHA_SIZE];
} automaton_node;

void increment_found(__global uint *counts, uint index) {
  /* index != 0 is a prerequirement, index == 0 means that the node is not accepting */
  atomic_inc(&counts[index - 1]);
}

__kernel void search(
    __constant char *string, __constant automaton_node *nodes, __constant uchar *char_lookup, uchar none_link,
    __global uint *counts
) {
  INDEX_TYPE curr_index = 0;
  __constant automaton_node *curr_state = &nodes[curr_index];

  ulong pos = 0;
  for (__constant char *c_ptr = string; *c_ptr != '\0'; ++c_ptr, ++pos) {
    uchar unsigned_c = *c_ptr;
    uchar mapped = char_lookup[unsigned_c];

    /* Unknown symbol, go staight to the root */
    if (mapped == none_link) {
      curr_index = 0;
      curr_state = &nodes[curr_index];
      continue;
    }

    /* Jump through the suffix links util a suitable transition is found */
    while (curr_state->children_links[mapped] == 0 && curr_index != 0) {
      curr_index = curr_state->failure_link;
      curr_state = &nodes[curr_index];
    }

    curr_index = curr_state->children_links[mapped];
    curr_state = &nodes[curr_index];

    /* Chase output links */
    __constant automaton_node *temp_state = curr_state;
    while (temp_state->output_link) {
      temp_state = &nodes[temp_state->output_link];
      increment_found(counts, temp_state->accepting);
    }

    /* If the current state is accepting, then a pattern has been found */
    if (curr_state->accepting) {
      increment_found(counts, curr_state->accepting);
    }
  }
}