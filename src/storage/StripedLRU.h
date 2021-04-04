#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <afina/Storage.h>
#include "ThreadSafeSimpleLRU.h"
#include <array>
#include <functional>
#include <vector>

namespace Afina {
namespace Backend {

class StripedLRU : public SimpleLRU {
public:
    ~StripedLRU() {};
    static std::unique_ptr<StripedLRU> BuildStripedLRU(size_t memory_limit, size_t stripe_count);
    bool Put(const std::string &key, const std::string &value) override;
    bool PutIfAbsent(const std::string &key, const std::string &value) override;
    bool Set(const std::string &key, const std::string &value) override;
    bool Delete(const std::string &key) override;
    bool Get(const std::string &key, std::string &value) override;
private:
    StripedLRU(size_t max_size, size_t stripe_count) : amount_of_stripes(stripe_count) {
        for(size_t i = 0; i < stripe_count; ++i) {
            shards.emplace_back(max_size);
        }
    }
    std::vector<SimpleLRU> shards;
    size_t amount_of_stripes;
};


} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_STRIPED_LRU_H