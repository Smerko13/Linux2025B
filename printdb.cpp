#include <vector>
#include <iostream>
#include "block.h"
#include "utils.h"


int main()
{
    std::vector<BLOCK> blocks = loadDB("../blocks_info.txt");

    prindtDB(blocks);

    return 0;
}