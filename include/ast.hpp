#pragma once

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>

namespace x3 = boost::spirit::x3;

namespace client
{
    namespace ast {

        struct nil {};
        struct expr;
        struct un_op;
        struct operand;

        struct operand : x3::variant<nil,
                                    double,
                                    x3::forward_ast<un_op>,
                                    x3::forward_ast<expr>
                                    >
        {
            using base_type::base_type;
            using base_type::operator=;
        };

        struct expr {
            operand first;
            std::vector<un_op> ops;
            };

        struct un_op {
            char op;
            operand _operand;
        };

        struct printer {
            void operator()(nil) const      {}
            void operator()(double n) const { std::cout << n;}
            void operator()(un_op op) const {
                boost::apply_visitor(*this, op._operand);
                std::cout << op.op;
            }
            void operator()(expr e) const {
                boost::apply_visitor(*this, e.first);
                for(const un_op& op : e.ops) {
                    (*this)(op);
                }
            }
        };
    }
}

BOOST_FUSION_ADAPT_STRUCT(client::ast::expr, first, ops);
BOOST_FUSION_ADAPT_STRUCT(client::ast::un_op, op, _operand);