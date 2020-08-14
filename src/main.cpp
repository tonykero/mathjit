#define ASMJIT_STATIC
#include <asmjit/asmjit.h>

#include <boost/config/warning_disable.hpp>

#include "grammar.hpp"
#include "eval.hpp"

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
            std::cout << "\n" << eval(e) << std::endl;
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
