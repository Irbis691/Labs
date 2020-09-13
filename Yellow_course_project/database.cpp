#include "database.h"

void Database::Add(const Date& date, const string& event) {
	const auto size_before = unique_storage[date].size();
	unique_storage[date].insert(event);
	const auto size_after = unique_storage[date].size();
	if (size_after > size_before) {
		storage[date].push_back(event);
		dates.insert(date);
	}
}

void Database::Print(ostream& s) const {
	for (const auto& item : storage) {
		for (const string& event : item.second) {
			s << item.first << " " << event << endl;
		}
	}
}

pair<Date, string> Database::Last(Date date) const {
	if (storage.empty() || date < storage.begin()->first) {
		throw invalid_argument("");
	}
	const auto nearestDate = *prev(dates.upper_bound(date));
	const vector<string>& events = storage.at(nearestDate);
	return make_pair(nearestDate, events.back());
}

ostream& operator<<(ostream& os, const pair<Date,string>& p){
	os << p.first << " " << p.second;
	return os;
}
