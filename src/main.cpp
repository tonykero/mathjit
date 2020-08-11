#define ASMJIT_STATIC
#include <asmjit/asmjit.h>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>
#include <list>
#include <numeric>

#include <stdio.h>
#include <iostream>
#include <string>

using namespace asmjit;

typedef int (*Func)(void);

int test_asmjit() {
  JitRuntime rt;

  CodeHolder code;
  code.init(rt.environment());

  x86::Assembler a(&code);
  a.mov(x86::eax, 1);
  a.ret();

  Func fn;
  Error err = rt.add(&fn, &code);
  if (err) return 1;

  int result = fn();
  printf("%d\n", result);

  rt.release(fn);

  return 0;
}

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

BOOST_FUSION_ADAPT_STRUCT(client::ast::expr, first, ops);
BOOST_FUSION_ADAPT_STRUCT(client::ast::un_op, op, _operand);

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

int test_spirit()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;

    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        auto& calc = client::calculator;

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        boost::spirit::x3::ascii::space_type space;
        client::ast::expr e;
        client::ast::printer print;
        client::ast::eval eval;
        bool r = phrase_parse(iter, end, calc, space, e);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "-------------------------\n";
            print(e);
            std::cout << "\n" << eval(e);
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }
    return 0;
}


int main()
{
    //test_asmjit();
    test_spirit();

    return 0;
}
