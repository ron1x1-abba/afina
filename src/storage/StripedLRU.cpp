#include "StripedLRU.h"

namespace Afina {
namespace Backend {

std::unique_ptr<StripedLRU> StripedLRU::BuildStripedLRU(size_t memory_limit, size_t stripe_count) {
    size_t stripe_limit = memory_limit / stripe_count;
    if (stripe_limit < 1024 * 1024) {
        throw std::runtime_error("Parameters of strip is chosen wrong!");
    }
    StripedLRU* storage = new StripedLRU(stripe_limit, stripe_count);
    return std::unique_ptr<StripedLRU>(storage);
}

bool StripedLRU::Put(const std::string &key, const std::string &value) {
    std::hash<std::string> hash_key;
    return shards[hash_key(key) % amount_of_stripes].Put(key, value);
}

bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    std::hash<std::string> hash_key;
    return shards[hash_key(key) % amount_of_stripes].PutIfAbsent(key, value);
}

bool StripedLRU::Set(const std::string &key, const std::string &value) {
    std::hash<std::string> hash_key;
    return shards[hash_key(key) % amount_of_stripes].Set(key, value);
}

bool StripedLRU::Delete(const std::string &key) {
    std::hash<std::string> hash_key;
    return shards[hash_key(key) % amount_of_stripes].Delete(key);
}

bool StripedLRU::Get(const std::string &key, std::string &value) {
    std::hash<std::string> hash_key;
    return shards[hash_key(key) % amount_of_stripes].Get(key, value);
}

}
}