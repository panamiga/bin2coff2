#include <string>
#include <iostream>

extern const char _binary_main_cpp_start;
extern const char _binary_main_cpp_end;

const char * source_start = &_binary_main_cpp_start;
const int source_length = &_binary_main_cpp_end - &_binary_main_cpp_start;

int main()
{    
    std::cout << std::string(source_start, source_length) << std::endl;
    return 0;
}
