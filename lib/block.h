#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>

class BLOCK {
private:
    std::string hash;
    int height;
    long long total;
    std::string time;
    std::string relayed_by;
    std::string prev_block;

public:
    BLOCK(std::string hash, int height, long long total, std::string time, std::string relayed_by, std::string prev_block);

    std::string getHash() const;
    int getHeight() const;
    long long getTotal() const;
    std::string getTime() const;
    std::string getRelayedBy() const;
    std::string getPrevBlock() const;

    void setHash(const std::string& hash);
    void setHeight(int height);
    void setTotal(long long total);
    void setTime(const std::string& time);
    void setRelayedBy(const std::string& relayed_by);
    void setPrevBlock(const std::string& prev_block);
};

std::vector<BLOCK> loadDB(const std::string& filename);

#endif