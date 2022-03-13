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

template <typename Key, typename Value>
class ConcurrentMap {
public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	explicit ConcurrentMap(size_t bucket_count)
		: subBuckets_(bucket_count), mutexes_(bucket_count), bucket_count_(bucket_count)
	{}

	struct Access
	{
		std::lock_guard<std::mutex> lock_guard;
		Value& ref_to_value;
	};

	Access operator[](const Key& key)
	{
		const uint64_t dict_num = GetDictionaryNumber(key);

		return Access{ std::lock_guard<std::mutex>(mutexes_[dict_num]), subBuckets_[dict_num][key] };
	}

	void erase(const Key& key)
	{
		const uint64_t dict_num = GetDictionaryNumber(key);

		std::lock_guard<std::mutex> guard(mutexes_[dict_num]);

		subBuckets_[dict_num].erase(key);
	}

	std::map<Key, Value> BuildOrdinaryMap()
	{
		std::map<Key, Value> ordinaryMap;

		for (size_t i = 0; i < bucket_count_; ++i)
		{
			std::lock_guard<std::mutex> guard(mutexes_[i]);
			ordinaryMap.merge(subBuckets_[i]);
		}

		return ordinaryMap;
	}

private:
	std::vector<std::map<Key, Value>> subBuckets_;
	std::vector<std::mutex> mutexes_;
	size_t bucket_count_;

	uint64_t GetDictionaryNumber(const Key& key)
	{
		return static_cast<uint64_t>(key) % bucket_count_;
	}
};
