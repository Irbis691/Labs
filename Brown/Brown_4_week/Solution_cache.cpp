#include <utility>
#include <unordered_map>
#include <list>
#include <mutex>

#include "Common_cache.h"

using namespace std;

class LruCache : public ICache {
public:
	LruCache(
			shared_ptr<IBooksUnpacker> books_unpacker,
			const Settings& settings
	) : books_unpacker_(move(books_unpacker)),
		settings_(settings) {}

	BookPtr GetBook(const string& book_name) override {
		lock_guard<mutex> g(m);
		if(cache.count(book_name) != 0) {
			rank.remove_if([book_name](const string& book) { return book == book_name; });
			rank.push_front(book_name);
			return cache[book_name];
		} else {
			shared_ptr<const IBook> unpackedBook = books_unpacker_->UnpackBook(book_name);
			rank.push_front(book_name);
			memory_used_by_books_ += unpackedBook->GetContent().size();
			cache[book_name] = unpackedBook;
			while(memory_used_by_books_ > settings_.max_memory) {
				memory_used_by_books_ -= cache[rank.back()]->GetContent().size();
				cache.erase(rank.back());
				rank.pop_back();
			}
			return unpackedBook;
		}
	}

private:
	shared_ptr<IBooksUnpacker> books_unpacker_;
	const Settings& settings_;
	size_t memory_used_by_books_ = 0;
	unordered_map<string, BookPtr> cache;
	list<string> rank;
	mutex m;
};


unique_ptr<ICache> MakeCache(
		shared_ptr<IBooksUnpacker> books_unpacker,
		const ICache::Settings& settings
) {
	return make_unique<LruCache>(move(books_unpacker), settings);
}
