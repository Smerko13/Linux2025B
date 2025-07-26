#include "block.h"
#include <iostream>
#include <vector>
#include <fstream>

void prindtDB(std::vector<BLOCK> blocks);
void printBlockByHash(const std::string& hash, const std::vector<BLOCK>& blocks);
void printBlockByHeight(int height, const std::vector<BLOCK>& blocks);
int exportToCsv(const std::vector<BLOCK>& blocks);
void reloadDB(int numOfBlocks);
