#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <iomanip>

using namespace std;

class Date {
public:
	Date(int new_year, int new_month, int new_day);

	int GetYear() const;

	int GetMonth() const;

	int GetDay() const;

private:
	int year;
	int month;
	int day;
};

bool operator<(const Date& lhs, const Date& rhs);

bool operator>(const Date& lhs, const Date& rhs);

bool operator==(const Date& lhs, const Date& rhs);

ostream& operator<<(ostream& stream, const Date& date);

Date ParseDate(istream& date);