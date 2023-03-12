/* Tiled matrix multiplication with local memory. Accepts arbitrary size matrices and TILE_SIZE
 *
 *  @kernel    ( {"name" : "aho_corasick_kernel", "entry" : "search"} )
 *  @signature ( ["cl::Buffer", "cl::Buffer", "cl_uint", "cl::Buffer"] )
 *  @macros    ( [{"type" : "unsigned char", "name": "ALPHA_SIZE"}, {"type" : "std::string", "name": "INDEX_TYPE"} ] )
 *
 */

#ifndef INDEX_TYPE
#define INDEX_TYPE ushort
#endif

#ifndef ALPHA_SIZE
#define ALPHA_SIZE 0
#endif

typedef struct __attribute__((packed)) s_automaton_node {
  INDEX_TYPE failure_link, output_link;
  INDEX_TYPE children_links[ALPHA_SIZE];
  uint accepting; // Store index of matching pattern. If == 0, then the state is not accepting
} automaton_node;

__global atomic_int x = ATOMIC_VAR_INIT(0);

__kernel void
search(__constant automaton_node *nodes, __constant uchar *char_lookup, uchar none_link, __global uint *counts) {}