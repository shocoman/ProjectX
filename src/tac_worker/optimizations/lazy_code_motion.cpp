//
// Created by shoco on 2/11/2021.
//

#include "lazy_code_motion.hpp"

void lazy_code_motion(Function &f) {
    auto all_expressions = get_all_expressions(f);
    auto UpwardExposed = get_upward_exposed_and_killed_expressions(f).first;

    auto Latest = latest_expressions(f);
    auto UsedOut = used_expressions(f).second;

    // uniquely name every expression
    NewNameGenerator new_name_generator(f);
    std::map<Expression, std::string> expr_to_name;
    for (auto &expr : all_expressions)
        expr_to_name[expr] = new_name_generator.make_new_name();

    for (auto &b : f.basic_blocks) {
        if (b->quads.empty())
            continue;

        auto for_expr_placement = intersection_of_sets(std::vector{Latest.at(b->id), UsedOut.at(b->id)});

        auto latest_complement = all_expressions;
        for (auto &e : Latest.at(b->id))
            latest_complement.erase(e);
        auto for_name_replacement = intersection_of_sets(std::vector{
            UpwardExposed.at(b->id), union_of_sets(std::vector{latest_complement, UsedOut.at(b->id)})});

        for (auto &expr : all_expressions) {
            if (for_name_replacement.count(expr) > 0) {
                auto &[lhs, type, rhs] = expr;

                for (auto &q : b->quads) {
                    if (!q.is_binary() || q.dest->name == expr_to_name.at(expr))
                        continue;

                    if (expr == std::tuple(*q.get_op(0), q.type, *q.get_op(1))) {
                        q.clear_op(1);
                        q.type = Quad::Type::Assign;
                        q.ops[0] = Operand(expr_to_name.at(expr), Operand::Type::Var);

                        std::cout << "Replaced: " << q.fmt() << std::endl;
                    }
                }
            }

            if (for_expr_placement.count(expr) > 0) {
                auto &[lhs, type, rhs] = expr;
                auto new_quad = Quad(lhs, rhs, type);
                new_quad.dest = Dest(expr_to_name.at(expr), {}, Dest::Type::Var);
                // if block doesn't contain it already
                if (std::find(b->quads.begin(), b->quads.end(), new_quad) == b->quads.end())
                    b->quads.insert(b->quads.begin() + b->phi_functions, new_quad);

                std::cout << "Put: " << new_quad.fmt() << std::endl;
            }
        }
    }
}

std::pair<ID2EXPRS, ID2EXPRS> available_expressions_lazy_code_motion(Function &f) {
    auto all_expressions = get_all_expressions(f);
    auto AntIn = anticipable_expressions(f).first;
    auto KilledExprs = get_downward_exposed_and_killed_expressions(f).second;

    auto [IN, OUT] =
        data_flow_framework<Expression>(f, Flow::Forwards, Meet::Intersection, all_expressions,
                                        [&](ID2EXPRS &IN, ID2EXPRS &OUT, int id) {
                                            auto X = IN.at(id);
                                            for (auto &expr : AntIn.at(id))
                                                X.insert(expr);
                                            for (auto &expr : KilledExprs.at(id))
                                                X.erase(expr);
                                            return X;
                                        });

    return {IN, OUT};
}

ID2EXPRS earliest_expressions(ID2EXPRS &AntIn, ID2EXPRS &AvailIn) {
    auto X = AntIn;
    for (auto &[id, exprs] : AvailIn) {
        auto &x = X.at(id);
        for (auto &e : exprs)
            x.erase(e);
    }
    return X;
}

std::pair<ID2EXPRS, ID2EXPRS> postponable_expressions(Function &f) {
    auto all_expressions = get_all_expressions(f);

    auto AntIn = anticipable_expressions(f).first;
    auto AvailIn = available_expressions_lazy_code_motion(f).first;
    auto earliest_exprs = earliest_expressions(AntIn, AvailIn);

    auto id_to_ue_exprs = get_upward_exposed_and_killed_expressions(f).first;

    auto [IN, OUT] =
        data_flow_framework<Expression>(f, Flow::Forwards, Meet::Intersection, all_expressions,
                                        [&](ID2EXPRS &IN, ID2EXPRS &OUT, int id) {
                                            auto X = IN.at(id);
                                            for (auto &expr : earliest_exprs.at(id))
                                                X.insert(expr);
                                            for (auto &expr : id_to_ue_exprs.at(id))
                                                X.erase(expr);
                                            return X;
                                        });
    return {IN, OUT};
}

ID2EXPRS latest_expressions(Function &f) {
    auto all_expressions = get_all_expressions(f);

    auto AntIn = anticipable_expressions(f).first;
    auto AvailIn = available_expressions_lazy_code_motion(f).first;
    auto earliest_exprs = earliest_expressions(AntIn, AvailIn);

    auto id_to_ue_exprs = get_upward_exposed_and_killed_expressions(f).first;
    auto PostIn = postponable_expressions(f).first;

    ID2EXPRS Latest;
    for (auto &b : f.basic_blocks) {
        std::vector<std::set<Expression>> successors_exprs;
        std::transform(b->successors.begin(), b->successors.end(), std::back_inserter(successors_exprs),
                       [&](auto s) {
                           return union_of_sets(std::vector{earliest_exprs.at(s->id), PostIn.at(s->id)});
                       });

        auto complementary = all_expressions;
        for (auto &e : intersection_of_sets(successors_exprs))
            complementary.erase(e);

        Latest[b->id] = intersection_of_sets(
            std::vector{union_of_sets(std::vector{earliest_exprs[b->id], PostIn[b->id]}),
                        union_of_sets(std::vector{id_to_ue_exprs.at(b->id), complementary})});
    }

    return Latest;
}

std::pair<ID2EXPRS, ID2EXPRS> used_expressions(Function &f) {
    auto all_expressions = get_all_expressions(f);
    auto id_to_ue_exprs = get_upward_exposed_and_killed_expressions(f).first;
    auto Latest = latest_expressions(f);

    auto [IN, OUT] = data_flow_framework<Expression>(f, Flow::Backwards, Meet::Union, {},
                                                     [&](ID2EXPRS &IN, ID2EXPRS &OUT, int id) {
                                                         auto X = OUT.at(id);
                                                         for (auto &expr : id_to_ue_exprs.at(id))
                                                             X.insert(expr);
                                                         for (auto &expr : Latest.at(id))
                                                             X.erase(expr);
                                                         return X;
                                                     });
    return {IN, OUT};
}