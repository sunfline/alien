#include "parser/generator/clr.h"

namespace alien::parser::generator {

    std::vector<clr::item> clr_helper::clr_closure(const std::vector<clr::item>& items) {
        auto it = cache.find(items);

        if (it != cache.end()) {
            return it->second;
        }

        util::hash_vecset<clr::item, boost::hash<clr::item>> closure{items};

        for (std::size_t i = 0; i < closure.size(); ++i) {
            auto [rule, production, pos, lookahead] = closure[i];

            const rules::production& prod = rules.ruleset[rule][production];

            if (pos >= prod.symbols.size()) {
                continue;
            }

            const rules::grammar_symbol& symbol = prod.symbols[pos];

            if (symbol.type == rules::symbol_type::TERMINAL) {
                continue;
            }

            std::vector<rules::grammar_symbol> str{prod.symbols.begin(), prod.symbols.end()};
            str.push_back({rules::symbol_type::TERMINAL, lookahead});

            auto first = get_first(str, pos + 1);

            for (std::size_t j = 0; j < rules.ruleset[symbol.index].size(); ++j) {
                for (std::ptrdiff_t s : first) {
                    closure.push_back({symbol.index, j, 0, s});
                }
            }
        }

        return cache.insert({items, (std::vector<clr::item>) closure}).first->second;
    }

    std::vector<clr::item> clr_helper::clr_move(const std::vector<clr::item>& items, const rules::grammar_symbol& symbol) {
        std::vector<clr::item> moved;

        for (const clr::item& item : items) {
            auto [rule, production, pos, lookahead] = item;

            const rules::production& prod = rules.ruleset[rule][production];

            if (pos < prod.symbols.size() && prod.symbols[pos] == symbol) {
                moved.emplace_back(rule, production, pos + 1, lookahead);
            }
        }

        return moved;
    }

    parsing_table clr_generator::generate_table() {
        util::vecset<std::vector<clr::item>> states{helper.clr_closure({{0, 0, 0, -2}})};

        table.resize(1);

        for (std::size_t i = 0; i < states.size(); ++i) {
            for (const auto& item : states[i]) {
                auto [rule, production, pos, lookahead] = item;

                const rules::production& prod = rules.ruleset[rule][production];

                if (pos < prod.symbols.size()) {
                    continue;
                }

                parsing_action action{
                        action_type::REDUCE,
                        rule,
                        production
                };

                if (rule == 0) {
                    action.type = action_type::ACCEPT;
                }

                insert(i, {rules::symbol_type::TERMINAL, lookahead}, action);
            }

            for (std::ptrdiff_t j = 1; j < alphabet.non_terminals.size(); ++j) {
                auto next = helper.clr_closure(helper.clr_move(states[i], {rules::symbol_type::NON_TERMINAL, j}));

                if (next.empty()) {
                    continue;
                }

                insert(i, {rules::symbol_type::NON_TERMINAL, j}, {action_type::SHIFT, clr_transition(states, next)});
            }

            for (std::ptrdiff_t j = 0; j < alphabet.terminals.size(); ++j) {
                auto next = helper.clr_closure(helper.clr_move(states[i], {rules::symbol_type::TERMINAL, j}));

                if (next.empty()) {
                    continue;
                }

                insert(i, {rules::symbol_type::TERMINAL, j}, {action_type::SHIFT, clr_transition(states, next)});
            }
        }

        return table;
    }

    std::size_t clr_generator::clr_transition(util::vecset<std::vector<clr::item>>& states, std::vector<clr::item>& next) {
        std::size_t to_state = states.push_back(std::move(next));

        if (to_state >= table.size()) {
            table.emplace_back();
        }

        return to_state;
    }

}