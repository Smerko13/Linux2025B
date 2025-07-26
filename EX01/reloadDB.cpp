#include "utils.h"
#include <iostream>
#include <dirent.h>
#include <string>

int main(){
    int numOfBlocks;
    std::cout << "Enter the number of blocks to refresh: ";
    std::cin >> numOfBlocks;
    reloadDB(numOfBlocks);
    return 0;
}
