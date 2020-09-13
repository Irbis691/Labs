#include "../../test_runner.h"

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

using namespace std;

struct Record {
	string id;
	string title;
	string user;
	int timestamp;
	int karma;
};


class Database {
public:
	bool Put(const Record& record) {
		auto[it, inserted] = db.insert(
				{record.id, Data{record, {}, {}, {}}}
		);

		if(!inserted) {
			return false;
		}

		Data& data = it->second;
		const Record* r = &data.record;
		data.timestamp_iterator = timestamp_index.insert({record.timestamp, r});
		data.karma_iterator = karma_index.insert({record.karma, r});
		data.user_iterator = user_index.insert({record.user, r});
		return true;
	}

	const Record* GetById(const string& id) const {
		auto it = db.find(id);
		if(it == db.end()) {
			return nullptr;
		}
		return &it->second.record;
	}

	bool Erase(const string& id) {
		auto it = db.find(id);
		if(it == db.end()) {
			return false;
		}
		const auto& data = it->second;
		timestamp_index.erase(data.timestamp_iterator);
		karma_index.erase(data.karma_iterator);
		user_index.erase(data.user_iterator);
		db.erase(it);
		return true;
	}

	template<typename Callback>
	void RangeByTimestamp(int low, int high, Callback callback) const {
		auto begin = timestamp_index.lower_bound(low);
		auto end = timestamp_index.upper_bound(high);
		for(auto it = begin; it != end; ++it) {
			if(!callback(*it->second)) {
				break;
			}
		}
	}

	template<typename Callback>
	void RangeByKarma(int low, int high, Callback callback) const {
		auto begin = karma_index.lower_bound(low);
		auto end = karma_index.upper_bound(high);
		for(auto it = begin; it != end; ++it) {
			if(!callback(*it->second)) {
				break;
			}
		}
	}

	template<typename Callback>
	void AllByUser(const string& user, Callback callback) const {
		auto begin = user_index.lower_bound(user);
		auto end = user_index.upper_bound(user);
		for(auto it = begin; it != end; ++it) {
			if(!callback(*it->second)) {
				break;
			}
		}
	}

private:
	template<typename Type>
	using Index = multimap<Type, const Record*>;

	struct Data {
		Record record;
		Index<int>::iterator karma_iterator;
		Index<int>::iterator timestamp_iterator;
		Index<string>::iterator user_iterator;
	};
	unordered_map<string, Data> db;
	Index<int> karma_index;
	Index<int> timestamp_index;
	Index<string> user_index;
};

void TestRangeBoundaries() {
	const int good_karma = 1000;
	const int bad_karma = -10;

	Database db;
	db.Put({"id1", "Hello there", "master", 1536107260, good_karma});
	db.Put({"id2", "O>>-<", "general2", 1536107260, bad_karma});

	int count = 0;
	db.RangeByKarma(bad_karma, good_karma, [&count](const Record&) {
		++count;
		return true;
	});

	ASSERT_EQUAL(2, count);
}

void TestSameUser() {
	Database db;
	db.Put({"id1", "Don't sell", "master", 1536107260, 1000});
	db.Put({"id2", "Rethink life", "master", 1536107260, 2000});

	int count = 0;
	db.AllByUser("master", [&count](const Record&) {
		++count;
		return true;
	});

	ASSERT_EQUAL(2, count);
}

void TestReplacement() {
	const string final_body = "Feeling sad";

	Database db;
	db.Put({"id", "Have a hand", "not-master", 1536107260, 10});
	db.Erase("id");
	db.Put({"id", final_body, "not-master", 1536107260, -10});

	auto record = db.GetById("id");
	ASSERT(record != nullptr);
	ASSERT_EQUAL(final_body, record->title);
}

int main() {
	TestRunner tr;
	RUN_TEST(tr, TestRangeBoundaries);
	RUN_TEST(tr, TestSameUser);
	RUN_TEST(tr, TestReplacement);
	return 0;
}
