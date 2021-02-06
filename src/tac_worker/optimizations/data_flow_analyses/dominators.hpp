//
// Created by shoco on 2/5/2021.
//

#ifndef TAC_PARSER_DOMINATORS_HPP
#define TAC_PARSER_DOMINATORS_HPP

#include <numeric>
#include <set>
#include <map>

#include "graph_writer/graph_writer.hpp"
#include "tac_worker/structure/function.hpp"
#include "utilities.hpp"

ID2DF find_dominance_frontier(const BasicBlocks &blocks, ID2IDOM &id_to_immediate_dominator);
ID2IDOM find_immediate_dominators(Function &function);
ID2DOM find_dominators(Function &function);
void print_dominator_tree(ID2Block &id_to_block, ID2IDOM &id_to_idom);


#endif // TAC_PARSER_DOMINATORS_HPP
