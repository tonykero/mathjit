#pragma once

#include "ast.hpp"

namespace client {
    namespace calculator_grammar
    {
        using x3::uint_;
        using x3::char_;
        using x3::double_;

        x3::rule<class expr, ast::expr> const expr("expr");
        x3::rule<class term, ast::expr> const term("term");
        x3::rule<class factor, ast::operand> const factor("factor");

        auto const expr_def =
            term >> *( 
                        (char_('+') >> term) |
                        (char_('-') >> term)
                    )
            ;

        auto const term_def =
            factor >> *( 
                        (char_('*') >> term) |
                        (char_('/') >> term) |
                        (char_('^') >> term)
                    )
            ;
        auto const factor_def =
            double_             |
            '(' >> expr >> ')'  |
            char_('+') >> factor       |
            char_('-') >> factor
            ;

        BOOST_SPIRIT_DEFINE(
            expr,
            term,
            factor
        );

        auto calculator = expr;
    }
    using calculator_grammar::calculator;
}