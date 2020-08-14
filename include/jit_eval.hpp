#pragma once

#define ASMJIT_STATIC
#include <asmjit/asmjit.h>

#include "ast.hpp"

#include <memory>

using namespace asmjit;


namespace client {
    namespace ast {
        struct jit_eval {
            
            typedef double (*fun_type)(void);
            private:
                JitRuntime rt;
                CodeHolder code;
                std::unique_ptr<x86::Compiler> cc_ptr;

                fun_type fn;
            public:


            jit_eval() : rt(), code() {
                code.init(rt.environment());
                cc_ptr = std::make_unique<x86::Compiler>(&code);

                cc_ptr->addFunc(FuncSignatureT<double>());

            }

            ~jit_eval() {
                rt.release(fn);
            }

            void eval(expr e) {
                cc_ptr->ret((*this)(e));
                cc_ptr->endFunc();
                cc_ptr->finalize();
                
                Error err = rt.add(&fn, &code);
            }

            double compute() {
                return fn();
            }

            x86::Xmm operator()(nil) const      {
                return (*this)(0.0);
            }
            x86::Xmm operator()(double n) const {
                x86::Xmm r = cc_ptr->newXmmSd();
                cc_ptr->movsd(r, cc_ptr->newDoubleConst(ConstPool::kScopeLocal, n));
                return r;
            }
            x86::Xmm operator()(un_op op) const {
                x86::Xmm rhs = boost::apply_visitor(*this, op._operand);

                x86::Xmm ret = (*this)(0.0);
                switch(op.op) {
                    case '+':
                        cc_ptr->addsd(ret, rhs);
                        return ret;
                    case '-':
                        cc_ptr->subsd(ret, rhs);
                        return ret;
                }
                // unreachable
                return ret;
            }
            x86::Xmm operator()(un_op op, x86::Xmm state) const {
                x86::Xmm rhs = boost::apply_visitor(*this, op._operand);
                switch(op.op) {
                    case '+':
                    cc_ptr->addsd(state, rhs);
                    return state;
                    case '-':
                    cc_ptr->subsd(state, rhs);
                    return state;
                    case '*':
                    cc_ptr->mulsd(state, rhs);
                    return state;
                    case '/':
                    cc_ptr->divsd(state, rhs);
                    return state;
                    //case '^': return std::pow(state,rhs);
                }
                // unreachable
                return state;
            }
            x86::Xmm operator()(expr e) const {
                x86::Xmm state = boost::apply_visitor(*this, e.first);
                for(const un_op& op : e.ops) {
                    state = (*this)(op, state);
                }
                return state;
            }
        };
    }
}