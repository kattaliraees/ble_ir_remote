#include "ir_code.hpp"
#include <iostream> 

ir_code::ir_code (string name) {

    geekname = name;
}

void ir_code::printname () {
    cout << "Name = " << geekname;
}