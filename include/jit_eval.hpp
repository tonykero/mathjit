#pragma once

#define ASMJIT_STATIC
#include <asmjit/asmjit.h>

#include "ast.hpp"

#include <emmintrin.h>
#include <memory>
#include <unordered_map>

namespace asmjit {
    namespace Type {
        template<>
        struct IdOfT<__m128d> { enum : uint32_t { kTypeId = _kIdVec128Start }; };
    }
}

using namespace asmjit;


namespace mathjit {
    namespace ast {
        
        // TODO: generate standalone function
        template <typename T = double>
        struct jit_eval : public visitor<x86::Xmm> {
            
            typedef double* (*fun_type)(void);
            private:
                std::unordered_map<char, T> vars_map;

                JitRuntime rt;
                CodeHolder code;
                std::unique_ptr<x86::Compiler> cc_ptr;

                fun_type fn;

                static constexpr bool is_complex = is_complex_v<T>;
            public:
            using c_type = std::complex<double>;

            jit_eval(const decltype(vars_map)& _vars_map) : vars_map(_vars_map), rt(), code() {
                code.init(rt.environment());
                cc_ptr = std::make_unique<x86::Compiler>(&code);

                cc_ptr->addFunc(FuncSignatureT<double*>());

            }

            ~jit_eval() {
                rt.release(fn);
            }

            void eval(expr e) {
                x86::Gp ret_ptr     = cc_ptr->newInt64("ret_ptr");
                x86::Mem xmm_ret    = xmmAsMem((*this)(e));
                cc_ptr->lea(ret_ptr, xmm_ret);

                cc_ptr->ret(ret_ptr);

                cc_ptr->endFunc();
                cc_ptr->finalize();
                
                Error err = rt.add(&fn, &code);
            }

            T compute() {
                if constexpr(is_complex) {
                    double* ret = fn();
                    double real = ret[0];
                    double imag = ret[1];
                    return c_type(real, imag);
                } else {
                    return *fn();
                }
            }
            
            x86::Xmm operator()(nil) const      {
                return (*this)(0.0);
            }
            x86::Xmm operator()(double n) const {
                return doubleAsXmm(n);
            }
            x86::Xmm operator()(complex n) const      {
                
                if constexpr(is_complex)
                {
                    // interpret as complex
                    if(n.i) {
                        return complexAsXmm(c_type(0, n.imag));
                    } else {
                        return complexAsXmm(c_type(n.imag, 0));
                    }
                } else {
                    // interpret as double anyway
                    return (*this)(n.imag);
                }
            }
            x86::Xmm operator()(char var) const {
                return complexAsXmm(vars_map.at(var));
            }
            x86::Xmm operator()(un_op op) const {
                x86::Xmm rhs = boost::apply_visitor(*this, op._operand);

                x86::Xmm ret = complexAsXmm(c_type(0,0));
                switch(op.op) {
                    case '+':
                            if constexpr(is_complex){ cc_ptr->addpd(ret, rhs); }
                            else                    { cc_ptr->addsd(ret, rhs); }
                        return ret;
                    case '-':
                            if constexpr(is_complex){ cc_ptr->subpd(ret, rhs); }
                            else                    { cc_ptr->subsd(ret, rhs); }
                        return ret;
                }
                // unreachable
                return ret;
            }
            x86::Xmm operator()(un_op op, x86::Xmm state) const {
                x86::Xmm rhs = boost::apply_visitor(*this, op._operand);
                switch(op.op) {
                    case '+':
                        if constexpr(is_complex){ cc_ptr->addpd(state, rhs); }
                        else                    { cc_ptr->addsd(state, rhs); }
                    return state;
                    case '-':
                        if constexpr(is_complex){ cc_ptr->subpd(state, rhs); }
                        else                    { cc_ptr->subsd(state, rhs); }
                    return state;
                    case '*':
                    if constexpr(is_complex) {
                        complex_mul(state, rhs);
                    } else {
                        cc_ptr->mulsd(state, rhs);
                    }
                    return state;
                    case '/':
                    if constexpr(is_complex) {
                        complex_div(state, rhs);
                    } else {
                        cc_ptr->divsd(state, rhs);
                    }
                    return state;
                    case '^':
                    if constexpr(is_complex) {
                        wrap_invoke2c<0>((&std::pow), {state, state, rhs});
                    } else {
                        invoke2d(&(std::pow), {state, state, rhs});
                    }
                    return state;
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

            private:
            template<typename T, typename... Args>
            Error invoke(uint64_t fun_ptr, std::initializer_list<asmjit::BaseReg> list) const {
                InvokeNode* invokeNode;
                Error err = cc_ptr->invoke(&invokeNode, fun_ptr, FuncSignatureT<T, Args...>());
                uint32_t index = 0;
                for(const asmjit::BaseReg* it = list.begin(); it != list.end(); it++) {
                    if(it == list.begin()) { invokeNode->setRet(0, *it); }
                    else
                    {
                        invokeNode->setArg(index, *it);
                        index++;
                    }
                }

                return err;
            }
            Error invoke2d(double(*fun_ptr)(double, double), std::initializer_list<asmjit::BaseReg> list) const {
                uint64_t fun_ptr_int = reinterpret_cast<uint64_t>(fun_ptr);
                return invoke<double, double, double>(fun_ptr_int, list);
            }
            Error invoke1d(double(*fun_ptr)(double), std::initializer_list<asmjit::BaseReg> list) const {
                uint64_t fun_ptr_int = reinterpret_cast<uint64_t>(fun_ptr);
                return invoke<double, double>(fun_ptr_int, list);
            }

            template <uint32_t _id = 0>
            auto make_wrapper2c(T(*fun_ptr)(const c_type&, const c_type&)) const {
                static c_type(*_ptr)(const c_type&, const c_type&) = fun_ptr;

                struct A {
                    static __m128d fun(__m128d a, __m128d b) {
                        c_type  ca = c_type(a.m128d_f64[0], a.m128d_f64[1]),
                                cb = c_type(b.m128d_f64[0], b.m128d_f64[1]);
                        c_type  cret = _ptr(ca, cb);

                        __m128d ret;
                        ret.m128d_f64[0] = cret.real();
                        ret.m128d_f64[1] = cret.imag();
                        return ret;
                    }
                } a;
                return a;
            }

            template <uint32_t _id = 0>
            auto make_wrapper1c(c_type(*fun_ptr)(const c_type&)) const {
                static c_type(*_ptr)(const c_type&) = fun_ptr;
                struct A {
                    static __m128d fun(__m128d a) {
                        c_type   ca = c_type(a.m128d_f64[0], a.m128d_f64[1]);
                        c_type   cret = _ptr(ca);
                        __m128d ret;
                        ret.m128d_f64[0] = cret.real();
                        ret.m128d_f64[1] = cret.imag();
                        return ret;
                    } 
                } a;
                return a;
            }
            Error invoke2c(uint64_t fun_ptr, std::initializer_list<asmjit::BaseReg> list ) const {
                return invoke<__m128d, __m128d, __m128d>(fun_ptr, list);
            }
            Error invoke1c(uint64_t fun_ptr, std::initializer_list<asmjit::BaseReg> list ) const {
                return invoke<__m128d, __m128d>(fun_ptr, list);
            }

            template <uint32_t _id = 0>
            Error wrap_invoke2c(T(*fun_ptr)(const c_type&, const c_type&), std::initializer_list<asmjit::BaseReg> list) const {
                auto wrapper = make_wrapper2c<_id>(fun_ptr);
                uint64_t ptr = reinterpret_cast<uint64_t>(&wrapper.fun);

                return invoke2c(ptr, list);
            }

            x86::Xmm doubleAsXmm(double n) const {
                x86::Xmm r = cc_ptr->newXmmPd();
                cc_ptr->movsd(r, cc_ptr->newDoubleConst(ConstPool::kScopeLocal, n));
                return r;
            }

            x86::Mem complexAsMem(c_type n) const {
                double data[2] = {n.real(), n.imag()};

                return cc_ptr->newConst(ConstPool::kScopeLocal, data, sizeof(double)*2);
            }

            x86::Xmm complexAsXmm(c_type n) const {
                x86::Mem comp   = complexAsMem(n);
                x86::Xmm xmm    = cc_ptr->newXmmPd();
                cc_ptr->movapd(xmm, comp);
                return xmm;
            }

            x86::Mem xmmAsMem(x86::Xmm _xmm) const {
                x86::Mem mem = cc_ptr->newStack(16, 8);
                cc_ptr->movapd(mem, _xmm);
                return mem;
            }
            
            x86::Xmm memAsXmm(x86::Mem _mem) const {
                x86::Xmm xmm = cc_ptr->newXmmPd();
                cc_ptr->movapd(xmm, _mem);
                return xmm;
            }
            x86::Gp memAsPtr(x86::Mem mem) const {
                x86::Gp ptr = cc_ptr->newInt64();
                cc_ptr->lea(ptr, mem);
                return ptr;
            }
            x86::Gp xmmAsPtr(x86::Xmm xmm) const {
                return memAsPtr(xmmAsMem(xmm));
            }
            

            x86::Xmm complex_mul(x86::Xmm& state, x86::Xmm& rhs) const {
                //complex multiplication (xu - yv) + (xv + yu)
                
                x86::Xmm xu_yv = cc_ptr->newXmmPd();
                cc_ptr->movapd(xu_yv, state);           // xu_yv = (x + yi)
                cc_ptr->mulpd(xu_yv, rhs);              // xu_yv = (xu + yvi)
                
                x86::Xmm xv_yu = cc_ptr->newXmmPd();
                cc_ptr->movapd(xv_yu, state);           // xv_yu = (x + yi)
                cc_ptr->shufpd(rhs, rhs, 0b00010001);   // e     = (v + ui)
                
                cc_ptr->mulpd(xv_yu, rhs);              // xv_yu = (xv + yui)
                cc_ptr->hsubpd(xu_yv, xu_yv);           // xu_yv = (xu-yv) + (xu - yvi)
                cc_ptr->haddpd(xv_yu, xv_yu);           // xv_yu = (xv+yu) + (xv + yui)
                cc_ptr->shufpd(xu_yv, xv_yu, 0);        // xu_yv = (xu-yv) + (xv + yui)
                cc_ptr->movapd(state, xu_yv);
                return state;
            }

            x86::Xmm complex_div(x86::Xmm& state, x86::Xmm& rhs) const {
                //complex division d / e = d * (1/e)
                // s = u² + v²
                // d * 1/e = d * ( (u/s) - (v/s)i)

                x86::Xmm s = cc_ptr->newXmmPd();
                cc_ptr->movapd(s, rhs);       // s = (u + vi)
                cc_ptr->mulpd(s, s);        // s = (u² + v²i)
                cc_ptr->haddpd(s, s);       // s = ((u²+v²) + (u²+v²i))
                x86::Xmm res = complexAsXmm(c_type(1.0, -1.0));
                cc_ptr->mulpd(res, rhs);    // res = (u - vi)
                cc_ptr->divpd(res, s);    // res = (u/(u²+v²) - v/(u²+v²i))
                
                return complex_mul(state, res);
            }
        };
    }
}