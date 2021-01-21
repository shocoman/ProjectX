//
// Created by shoco on 10/22/2020.
//

#ifndef TAC_PARSER_BASIC_BLOCK_H
#define TAC_PARSER_BASIC_BLOCK_H

#include <string>
#include <optional>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <memory>

#include "quadruple.hpp"


struct BasicBlock {
    int id;
    std::string node_name;
    std::optional<std::string> lbl_name;
    std::vector<Quad> quads;
    std::optional<std::string> jumps_to;
    std::set<BasicBlock *> successors;
    std::set<BasicBlock *> predecessors;

    int phi_functions = 0;
    std::string get_name() const {
        return "BB " + std::to_string(id);
    }

    std::string fmt() const {
        std::string out;
        for (auto &q : quads)
            out += q.fmt() + "\n";
        return out;
    }

    void add_successor(BasicBlock *s) {
        successors.emplace(s);
        s->predecessors.emplace(this);
    }

    void remove_successors() {
        for (auto &s: successors)
            s->predecessors.erase(this);
        successors.clear();
    }

    void remove_predecessors() {
        for (auto &s: predecessors)
            s->successors.erase(this);
        predecessors.clear();
    }

    bool allows_fallthrough() {
        return quads.back().type != Quad::Type::Goto
               && quads.back().type != Quad::Type::Return;
    }


    bool has_phi_function(std::string name) {
        for (int i = 0; i < phi_functions; ++i) {
            if (quads[i].type == Quad::Type::PhiNode && quads[i].dest.value().name == name)
                return true;
        }
        return false;
    }

    Quad& get_phi_function(std::string name) {
        for (int i = 0; i < phi_functions; ++i) {
            if (quads[i].type == Quad::Type::PhiNode && quads[i].dest.value().name == name)
                return quads[i];
        }
    }

    void add_phi_function(std::string lname, const std::vector<std::string>& rnames) {
        Quad phi({}, {}, Quad::Type::PhiNode, Dest(lname, {}, Dest::Type::Var));
        std::vector<Operand> ops;
        for (auto &n : rnames) {
            ops.emplace_back(n);
        }
        phi.ops = ops;

        quads.insert(quads.begin() + phi_functions, phi);
        phi_functions++;
    }
};


using BasicBlocks = std::vector<std::unique_ptr<BasicBlock>>;
using ID2Block = std::map<int, BasicBlock *>;
using ID2IDOM = std::map<int, int>; // id to immediate dominator
using ID2DOM = std::map<int, std::set<int>>; // id to dominators
using ID2DF = std::map<int, std::set<int>>; // id to dominance frontier

#endif //TAC_PARSER_BASIC_BLOCK_H
