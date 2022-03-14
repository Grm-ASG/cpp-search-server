#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <mutex>
#include <string>
#include <vector>
#include <execution>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	explicit ConcurrentMap(size_t bucket_count)
		: buckets(bucket_count)
	{}

	struct Access
	{
		std::lock_guard<std::mutex> lock_guard;
		Value& ref_to_value;
	};

	Access operator[](const Key& key)
	{
		const uint64_t dict_num = GetDictionaryNumber(key);
        
        auto& bucket = buckets[dict_num];
        
		return Access{ std::lock_guard<std::mutex>(bucket.m), bucket.dict[key] };
	}

	void erase(const Key& key)
	{
		const uint64_t dict_num = GetDictionaryNumber(key);
        auto& bucket = buckets[dict_num];
		std::lock_guard<std::mutex> guard(bucket.m);

		bucket.dict.erase(key);
	}

	std::map<Key, Value> BuildOrdinaryMap()
	{
		std::map<Key, Value> ordinaryMap;

		for (size_t i = 0; i < buckets.size(); ++i)
		{
            auto& bucket = buckets[i];
			std::lock_guard<std::mutex> guard(bucket.m);
			ordinaryMap.merge(bucket.dict);
		}

		return ordinaryMap;
	}

private:
    struct Bucket
    {
        std::map<Key, Value> dict;
        std::mutex m;
    };
    
    std::vector<Bucket> buckets;

	uint64_t GetDictionaryNumber(const Key& key)
	{
		return static_cast<uint64_t>(key) % buckets.size();
	}
};
