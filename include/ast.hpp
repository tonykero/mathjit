#pragma once

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>

namespace x3 = boost::spirit::x3;

namespace mathjit
{
    namespace ast {

        struct nil {};
        struct expr;
        struct un_op;
        struct operand;

        struct operand : x3::variant<nil,
                                    double,
                                    char,
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

        template<typename T>
        struct visitor {
            virtual T operator()(nil)        const = 0;
            virtual T operator()(double n)   const = 0;
            virtual T operator()(char var)   const = 0;
            virtual T operator()(un_op op)   const = 0;
            virtual T operator()(expr e)     const = 0;
        };
        struct printer : public visitor<void> {
            void operator()(nil)        const {}
            void operator()(double n)   const { std::cout << n;}
            void operator()(char var)   const { std::cout << var;}
            void operator()(un_op op)   const {
                boost::apply_visitor(*this, op._operand);
                std::cout << op.op;
            }
            void operator()(expr e)     const {
                boost::apply_visitor(*this, e.first);
                for(const un_op& op : e.ops) {
                    (*this)(op);
                }
            }
        };
    }
}

BOOST_FUSION_ADAPT_STRUCT(mathjit::ast::expr, first, ops);
BOOST_FUSION_ADAPT_STRUCT(mathjit::ast::un_op, op, _operand);