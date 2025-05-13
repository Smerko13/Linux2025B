#include "utils.h"


void prindtDB(std::vector<BLOCK> blocks)
{
    for(int i = 0 ; i < blocks.size() ; i++)
    {
        std::cout << "Hash: " << blocks[i].getHash() << std::endl;
        std::cout << "Height: " << blocks[i].getHeight() << std::endl;
        std::cout << "Total: " << blocks[i].getTotal() << std::endl;
        std::cout << "Time: " << blocks[i].getTime() << std::endl;
        std::cout << "Relayed By: " << blocks[i].getRelayedBy() << std::endl;
        std::cout << "Previous Block: " << blocks[i].getPrevBlock() << std::endl;
        if(i != blocks.size() - 1)
            std::cout << "\t\t|\n\t\t|\n\t\tV" << std::endl;
    }
}

void printBlockByHash(const std::string& hash, const std::vector<BLOCK>& blocks) {
    
    for (const auto& block : blocks) {
        if (block.getHash() == hash) {
            std::cout << "Printing block with hash: " << hash << std::endl;
            std::cout << "Hash: " << block.getHash() << "\n";
            std::cout << "Height: " << block.getHeight() << "\n";
            std::cout << "Total: " << block.getTotal() << "\n";
            std::cout << "Time: " << block.getTime() << "\n";
            std::cout << "Relayed By: " << block.getRelayedBy()<< "\n";
            std::cout << "Prev Block: " << block.getPrevBlock() << "\n";
            return;
        }
    }
    std::cout << "Block with hash " << hash << " not found.\n";
}

void printBlockByHeight(int height, const std::vector<BLOCK>& blocks) {

    for (const auto& block : blocks) {
        if (block.getHeight() == height) {
            std::cout << "Printing block with height: " << height << std::endl;
            std::cout << "Hash: " << block.getHash() << "\n";
            std::cout << "Height: " << block.getHeight() << "\n";
            std::cout << "Total: " << block.getTotal() << "\n";
            std::cout << "Time: " << block.getTime() << "\n";
            std::cout << "Relayed By: " << block.getRelayedBy() << "\n";
            std::cout << "Prev Block: " << block.getPrevBlock() << "\n";
            return;
        }
    }
    std::cout << "Block with height " << height << " not found.\n";
}

int exportToCsv(const std::vector<BLOCK>& blocks){
    std::ofstream file("blocks_info.csv");

    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return -1;
    }

    // Write the header row of CSV
    file << "Hash,Height,Total,Time,Relayed By,Prev Block\n";

    // Write each block's data
    for (const auto& block : blocks) {
        file << block.getHash() << ","
             << block.getHeight() << ","
             << block.getTotal() << ","
             << block.getTime() << ","
             << block.getRelayedBy() << ","
             << block.getPrevBlock() << "\n";
    }

    file.close();

    std::cout << "Data successfully exported" << std::endl;

    return 0;
}

void reloadDB(int numOfBlocks){
    if (numOfBlocks > 0) {
        std::string bash_script = "cd ../ && ./get_blocks.sh " + std::to_string(numOfBlocks);
        int result = system(bash_script.c_str());
        if (result != 0) {
            std::cout << "Error running BASH script" << std::endl;
            exit(1);
        }
        std::cout << "Data refreshed successfully." << std::endl;
        } else {
            std::cout << "Invalid number of blocks." << std::endl;
        }
}
