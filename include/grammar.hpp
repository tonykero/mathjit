#pragma once

#include "ast.hpp"

namespace mathjit {
    namespace grammar
    {
        //TODO: match xi (factor + i)
        //TODO: Add math functions
        using x3::char_;
        using x3::double_;
        using x3::alpha;
        using x3::optional;

        const x3::rule<class expr,  ast::expr>  expr("expr");
        const x3::rule<class term,  ast::expr>  term("term");
        const x3::rule<class exp,   ast::expr>  exp("exp");
        const x3::rule<class factor, ast::operand> factor("factor");

        auto const expr_def =
            term >> *( 
                        (char_('+') >> term) |
                        (char_('-') >> term)
                    )
            ;
        auto const term_def =
            exp >> *( 
                        (char_('*') >> exp) |
                        (char_('/') >> exp)
                    )
            ;

        auto const exp_def = 
            factor >> *(
                        (char_('^') >> factor)
                    )
            ;
        
        auto const factor_def =
            double_ >> -char_('i')   |
            '(' >> expr >> ')'  |
            alpha               |
            char_('+') >> factor|
            char_('-') >> factor
            ;

        BOOST_SPIRIT_DEFINE(
            expr,
            term,
            factor,
            exp
        );

        auto calculator = expr;
    }
    using grammar::calculator;
}