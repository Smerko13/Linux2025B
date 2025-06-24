#include "block.h"
#include "utils.h"

int main() {
    std::vector<BLOCK> blocks = loadDB("../blocks_info.txt");

    exportToCsv(blocks);

    return 0;
}
