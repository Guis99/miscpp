#include <bitset>
#include <iostream>

int main() {
    std::bitset<128> bits(0xFFFF'FFFF'FFFF'FFFF);

    std::cout << bits << std::endl; 
}