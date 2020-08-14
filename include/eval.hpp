#pragma once

#include "ast.hpp"

namespace client {
    namespace ast {
        struct eval {
            double operator()(nil) const      { return 0;}
            double operator()(double n) const { return n;}
            double operator()(un_op op) const {
                double rhs = boost::apply_visitor(*this, op._operand);
                switch(op.op) {
                    case '+': return +rhs;
                    case '-': return -rhs;
                }
                return 0;
            }
            double operator()(un_op op, double state) const {
                double rhs = boost::apply_visitor(*this, op._operand);
                switch(op.op) {
                    case '+': return state+rhs;
                    case '-': return state-rhs;
                    case '*': return state*rhs;
                    case '/': return state/rhs;
                    case '^': return std::pow(state,rhs);
                }
                return 0;
            }
            double operator()(expr e) const {
                double state = boost::apply_visitor(*this, e.first);
                for(const un_op& op : e.ops) {
                    state = (*this)(op, state);
                }
                return state;
            }
        };
    }
}