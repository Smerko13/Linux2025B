#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "block.h"
#include "utils.h"

#include <iostream>
#include <dirent.h>
#include <string>

int main()
{
    bool exitFlag = false;
    std::ifstream file("../blocks_info.txt");
    if(!file.good())
    {
        std::cerr << "No database found. Creating one now." << std::endl;
        std::cerr << "Enter the number of blocks you want: ";
        int numOfBlocks;
        std::cin >> numOfBlocks;
        reloadDB(numOfBlocks);
    }

    std::vector<BLOCK> blocks = loadDB("../blocks_info.txt");

    while(!exitFlag)
    {
        std::cout << "Choose an option:" << std::endl;
        std::cout << "1. print db" << std::endl;
        std::cout << "2. Print block by hash" << std::endl;
        std::cout << "3. Print block by height" << std::endl;
        std::cout << "4. Export data to csv" << std::endl;
        std::cout << "5. Refresh data" << std::endl;
        std::cout << "6. Exit" << std::endl;
        std::cout << "Enter your choice: ";
        int choice;
        std::cin >> choice;

        std::string hash;
        std::string height;
        int numOfBlocks = 0;
    
        switch (choice) {
            case 1:
                prindtDB(blocks);
                break;
            case 2:
                std::cout << "Enter block hash: ";
                std::cin >> hash;
                printBlockByHash(hash, blocks);
                break;
            case 3:
                std::cout << "Enter block height: ";
                std::cin >> height;
                printBlockByHeight(std::stoi(height), blocks);
                break;
            case 4:
                exportToCsv(blocks);
                break;
            case 5:
                 // Call function to refresh data
                std::cout << "Enter the number of blocks to refresh: ";
                std::cin >> numOfBlocks;
                if (numOfBlocks > 0) {
                    reloadDB(numOfBlocks);
                    blocks = loadDB("../blocks_info.txt");
                    std::cout << "Database refreshed with " << numOfBlocks << " blocks." << std::endl;
                } else {
                    std::cout << "Invalid number of blocks." << std::endl;
                }
                break;
            case 6:
                std::cout << "Exiting..." << std::endl;
                exitFlag = true;    
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
        }
        std::cout << std::endl;
    } 
    return 0;
} 


