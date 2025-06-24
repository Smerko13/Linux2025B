#include "block.h"
#include "utils.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Error when invoiking the program.\n";
        std::cout << "Usage: " << argv[0] << " [--hash <hash>] | [--height <height>]\n";
        return 1;
    }

    std::string option = argv[1];
    std::string value = argv[2];

    std::vector<BLOCK> blocks = loadDB("../blocks_info.txt");

    if (option == "--hash") {
        printBlockByHash(value, blocks);
    } else if (option == "--height") {
        try {
            int height = std::stoi(value); 
            printBlockByHeight(height, blocks);
        } catch (const std::invalid_argument&) {
            std::cout << "Unable to convert height, check your values!.\n";
            return 1;
        }
    } else {
        std::cout << "Error when invoiking the program.\n";
        std::cout << "Usage: " << argv[0] << " [--hash <hash>] | [--height <height>]\n";
        return 1;
    }

    return 0;
}

