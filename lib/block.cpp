#include "block.h"
#include <fstream>
#include <sstream>

BLOCK::BLOCK(std::string hash, int height, long long total, std::string time, std::string relayed_by, std::string prev_block)
    : hash(hash), height(height), total(total), time(time), relayed_by(relayed_by), prev_block(prev_block) {}

// Getters
std::string BLOCK::getHash() const { return hash; }
int BLOCK::getHeight() const { return height; }
long long BLOCK::getTotal() const { return total; }
std::string BLOCK::getTime() const { return time; }
std::string BLOCK::getRelayedBy() const { return relayed_by; }
std::string BLOCK::getPrevBlock() const { return prev_block; }

// Setters
void BLOCK::setHash(const std::string& hash) { this->hash = hash; }
void BLOCK::setHeight(int height) { this->height = height; }
void BLOCK::setTotal(long long total) { this->total = total; }
void BLOCK::setTime(const std::string& time) { this->time = time; }
void BLOCK::setRelayedBy(const std::string& relayed_by) { this->relayed_by = relayed_by; }
void BLOCK::setPrevBlock(const std::string& prev_block) { this->prev_block = prev_block; }

std::vector<BLOCK> loadDB(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<BLOCK> blocks;
    std::string line;
    std::string hash, time, relayed_by, prev_block;
    int height;
    long long total;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        if (line.find("hash:") != std::string::npos) {
            hash = line.substr(5);  // remove "hash:"
        } else if (line.find("height:") != std::string::npos) {
            height = std::stoi(line.substr(7));  // remove "height:"
        } else if (line.find("total:") != std::string::npos) {
            total = std::stoll(line.substr(6));  // remove "total:"
        } else if (line.find("time:") != std::string::npos) {
            time = line.substr(5);  // remove "time:"
        } else if (line.find("relayed_by:") != std::string::npos) {
            relayed_by = line.substr(12);  // remove "relayed_by:"
        } else if (line.find("prev_block:") != std::string::npos) {
            prev_block = line.substr(11);  // remove "prev_block:"
            blocks.push_back(BLOCK(hash, height, total, time, relayed_by, prev_block));
        }
    }
    file.close();
    return blocks;
}