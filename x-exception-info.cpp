#include <stdexcept>
#include <typeinfo>
#include <iostream>


int main() {

    try {
        throw std::runtime_error("bla");
    }
    catch (const std::exception &e) {
        std::cout << typeid(e).name() << " " << e.what() << std::endl;
    }



}