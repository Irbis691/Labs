#include <ctime>
#include <iostream>
#include <optional>
#include <vector>

using namespace std;

class Date {
public:
	Date(int year, int month, int day) : month_(month), year_(year), day_(day) {}

	time_t AsTimestamp() const {
		std::tm t;
		t.tm_sec = 0;
		t.tm_min = 0;
		t.tm_hour = 0;
		t.tm_mday = day_;
		t.tm_mon = month_ - 1;
		t.tm_year = year_ - 1900;
		t.tm_isdst = 0;
		return mktime(&t);
	}

private:
	int month_, year_, day_;
};

int ComputeDaysDiff(const Date& date_to, const Date& date_from) {
	const time_t timestamp_to = date_to.AsTimestamp();
	const time_t timestamp_from = date_from.AsTimestamp();
	static const int SECONDS_IN_DAY = 60 * 60 * 24;
	return (timestamp_to - timestamp_from) / SECONDS_IN_DAY;
}

Date GetDateFromStringView(vector<string_view> s) {
	return {stoi(s[0].data()), stoi(s[1].data()), stoi(s[2].data())};
}

class Budget {
public:
	Budget() {
		earned_.reserve(37000);
		spent_.reserve(37000);
	}

	double ComputeIncome(const Date& from, const Date& to) const {
		double income = 0;
		auto startIdx = DateToIdx(from);
		auto endIdx = DateToIdx(to);
		for(int i = startIdx; i <= endIdx; ++i) {
			income += earned_[i] - spent_[i];
		}
		return income;
	}

	void Earn(const Date& from, const Date& to, double income) {
		double income_per_day = income / (ComputeDaysDiff(to, from) + 1);
		auto startIdx = DateToIdx(from);
		auto endIdx = DateToIdx(to);
		for(int i = startIdx; i <= endIdx; ++i) {
			earned_[i] += income_per_day;
		}
	}

	void Spend(const Date& from, const Date& to, double outcome) {
		double outcome_per_day = outcome / (ComputeDaysDiff(to, from) + 1);
		auto startIdx = DateToIdx(from);
		auto endIdx = DateToIdx(to);
		for(int i = startIdx; i <= endIdx; ++i) {
			spent_[i] += outcome_per_day;
		}
	}

	void PayTax(const Date& from, const Date& to, int percentage) {
		auto startIdx = DateToIdx(from);
		auto endIdx = DateToIdx(to);
		for(int i = startIdx; i <= endIdx; ++i) {
			earned_[i] *= 1 - percentage / 100.0;
		}
	}

private:
	vector<double> earned_;
	vector<double> spent_;

	int DateToIdx(const Date& date) const {
		return ComputeDaysDiff(date, {2000, 1, 1});
	}
};

pair<string_view, optional<string_view>> SplitTwoStrict(string_view s, string_view delimiter = " ") {
	const size_t pos = s.find(delimiter);
	if(pos == s.npos) {
		return {s, nullopt};
	} else {
		return {s.substr(0, pos), s.substr(pos + delimiter.length())};
	}
}

vector<string_view> Split(string_view s, string_view delimiter = " ") {
	vector<string_view> parts;
	if(s.empty()) {
		return parts;
	}
	while(true) {
		const auto[lhs, rhs_opt] = SplitTwoStrict(s, delimiter);
		parts.push_back(lhs);
		if(!rhs_opt) {
			break;
		}
		s = *rhs_opt;
	}
	return parts;
}

int main() {
	int q;
	cin >> q;
	string operation, from, to;
	Budget budget;

	for(int i = 0; i < q; ++i) {
		cin >> operation >> from >> to;
		Date dateFrom = GetDateFromStringView(Split(from, "-"));
		Date dateTo = GetDateFromStringView(Split(to, "-"));
		if(operation == "Earn") {
			string income;
			cin >> income;
			budget.Earn(dateFrom, dateTo, stod(income));
		} else if(operation == "ComputeIncome") {
			cout << fixed << budget.ComputeIncome(dateFrom, dateTo) << endl;
		} else if(operation == "PayTax") {
			string percentage;
			cin >> percentage;
			budget.PayTax(dateFrom, dateTo, stoi(percentage));
		} else {
			string outcome;
			cin >> outcome;
			budget.Spend(dateFrom, dateTo, stod(outcome));
		}
	}

}
