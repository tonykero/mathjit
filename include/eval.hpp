#pragma once

#include <unordered_map>

#include "ast.hpp"

namespace client {
    namespace ast {
        template<typename T = double>
        struct eval : public visitor<T> {
            protected:
                std::unordered_map<char, T>    vars_map;
            public:
            eval(const decltype(vars_map)& _vars_map) : vars_map(_vars_map) {}

            T operator()(nil)      const { return 0;}
            T operator()(double n) const { return n;}
            T operator()(char var) const {
                return vars_map.at(var);
            }
            T operator()(un_op op) const {
                T rhs = boost::apply_visitor(*this, op._operand);
                switch(op.op) {
                    case '+': return +rhs;
                    case '-': return -rhs;
                }
                return 0;
            }
            T operator()(un_op op, T state) const {
                T rhs = boost::apply_visitor(*this, op._operand);
                switch(op.op) {
                    case '+': return state+rhs;
                    case '-': return state-rhs;
                    case '*': return state*rhs;
                    case '/': return state/rhs;
                    case '^': return std::pow(state,rhs);
                }
                return 0;
            }
            T operator()(expr e) const {
                T state = boost::apply_visitor(*this, e.first);
                for(const un_op& op : e.ops) {
                    state = (*this)(op, state);
                }
                return state;
            }
        };
    }
}