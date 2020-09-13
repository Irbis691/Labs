#pragma once

#include "date.h"

#include <set>
#include <map>
#include <iostream>
#include <algorithm>

class Database {
public:
	void Add(const Date& date, const string& event);

	template<typename Func>
	int RemoveIf(Func predicate) {
		int counter = 0;
		for (auto& pair : storage) {
			const auto& date = pair.first;
			auto iterator = stable_partition(pair.second.begin(), pair.second.end(),
											 [date, predicate](string event) { return predicate(date, event); });
			pair.second.erase(pair.second.begin(), iterator);
			set<string> unique_events;
			for (const auto& event: pair.second) {
				unique_events.insert(event);
			}
			unique_storage[date] = unique_events;
			counter += iterator - pair.second.begin();
		}
		vector<Date> removed_dates;
		for (const auto& date: dates) {
			if (storage.at(date).empty()) {
				storage.erase(date);
				unique_storage.erase(date);
				removed_dates.push_back(date);
			}
		}
		for (const auto& date: removed_dates) {
			dates.erase(date);
		}
		return counter;
	}

	template<typename Func>
	vector<pair<Date, string>> FindIf(Func predicate) const {
		vector<pair<Date, string>> result;
		for (auto& pair: storage) {
			auto events = pair.second;
			const auto date = pair.first;
			auto iterator = stable_partition(events.begin(), events.end(),
											 [date, predicate](string event) { return predicate(date, event); });
			for (auto iter = events.begin(); iter != iterator; ++iter) {
				result.emplace_back(date, *iter);
			}
		}
		return result;
	}

	void Print(ostream& s) const;

	pair<Date, string> Last(Date date) const;

private:
	map<Date, vector<string>> storage;
	map<Date, set<string>> unique_storage;
	set<Date> dates;
};

ostream& operator<<(ostream& os, const pair<Date, string>& p);