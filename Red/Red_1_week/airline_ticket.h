#pragma once

#include <string>
#include <vector>
#include <iomanip>

using namespace std;

struct Date {
	int year, month, day;
};

struct Time {
	int hours, minutes;
};

struct AirlineTicket {
	string from;
	string to;
	string airline;
	Date departure_date;
	Time departure_time;
	Date arrival_date;
	Time arrival_time;
	int price;
};

istream& operator>>(istringstream& is, Date& date) {
	int year, month, day;
	is >> year;
	is.ignore();
	is >> month;
	is.get();
	is >> day;
	date = {year, month, day};
	return is;
}

istream& operator>>(istringstream& is, Time& time) {
	int hours, minutes;
	is >> hours;
	is.get();
	is >> minutes;
	time = {hours, minutes};
	return is;
}

bool operator<(const Date& lhs, const Date& rhs) {
	return vector<int>{lhs.year, lhs.month, lhs.day} <
		   vector<int>{rhs.year, rhs.month, rhs.day};
}

ostream& operator<<(ostream& stream, const Date& date) {
	stream << setw(4) << setfill('0') << date.year <<
		   "-" << setw(2) << setfill('0') << date.month <<
		   "-" << setw(2) << setfill('0') << date.day;
	return stream;
}

bool operator<(const Time& lhs, const Time& rhs) {
	return vector<int>{lhs.hours, lhs.minutes} <
		   vector<int>{rhs.hours, rhs.minutes};
}

bool operator==(const Time& lhs, const Time& rhs) {
	return vector<int>{lhs.hours, lhs.minutes} ==
		   vector<int>{rhs.hours, rhs.minutes};
}

ostream& operator<<(ostream& stream, const Time& time) {
	stream << setw(2) << setfill('0') << time.hours <<
		   "-" << setw(2) << setfill('0') << time.minutes;
	return stream;
}