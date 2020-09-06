#include <boost/config/warning_disable.hpp>

#include "mathjit.hpp"

#include <list>
#include <numeric>
#include <iostream>
#include <string>
#include <chrono>

void micro_bench(std::function<void(void)> _fun, uint32_t n = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    for(uint32_t i = 0; i < n; i++) _fun();
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() / static_cast<double>(n) << "us" << std::endl;
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

        auto& calc = mathjit::calculator;

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        boost::spirit::x3::ascii::space_type space;

        mathjit::ast::expr e;
        mathjit::ast::printer print;

        std::unordered_map<char, double> vars = {
            {'x', 12.5},
            {'y', 1.0/3.0}
        };
        std::unordered_map<char, std::complex<double>> c_vars = {
            {'x', std::complex<double>(12.5, 0)},
            {'y', std::complex<double>(1.0/3.0)}
        };
        using base_type = std::complex<double>;
        mathjit::ast::eval<base_type>        eval(c_vars);
        mathjit::ast::jit_eval<base_type>    jit(c_vars);
        bool r = phrase_parse(iter, end, calc, space, e);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "-------------------------\n";
            print(e);
            std::cout << "\n double: " << eval(e) << std::endl;
            //std::cout << "\n complex: " << c_eval(e) << std::endl;
            jit.eval(e);
            std::cout << "\nJIT RESULT: " << jit.compute() << std::endl;

            micro_bench([&]() {eval(e);});
            
            micro_bench([&]() {jit.compute();});
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
    test_spirit();

    return 0;
}
