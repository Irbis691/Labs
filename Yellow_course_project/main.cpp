#include "database.h"
#include "date.h"
#include "condition_parser.h"
#include "node.h"

#include <iostream>
#include <stdexcept>

using namespace std;

string trim(const string& str) {
	size_t first = str.find_first_not_of(' ');
	if (string::npos == first) {
		return str;
	}
	return str.substr(first, str.length());
}

string ParseEvent(istream& is) {
	string event;
	getline(is, event);
	return trim(event);
}

int main() {

	Database db;

	for (string line; getline(cin, line);) {
		istringstream is(line);

		string command;
		is >> command;
		if (command == "Add") {
			const auto date = ParseDate(is);
			const auto event = ParseEvent(is);
			db.Add(date, event);
		} else if (command == "Print") {
			db.Print(cout);
		} else if (command == "Del") {
			auto condition = ParseCondition(is);
			auto predicate = [condition](const Date& date, const string& event) {
				return condition->Evaluate(date, event);
			};
			int count = db.RemoveIf(predicate);
			cout << "Removed " << count << " entries" << endl;
		} else if (command == "Find") {
			auto condition = ParseCondition(is);
			auto predicate = [condition](const Date& date, const string& event) {
				return condition->Evaluate(date, event);
			};

			const auto entries = db.FindIf(predicate);
			for (const auto& entry : entries) {
				cout << entry.first << " " << entry.second << endl;
			}
			cout << "Found " << entries.size() << " entries" << endl;
		} else if (command == "Last") {
			try {
				auto result = db.Last(ParseDate(is));
				cout << result << endl;
			} catch (invalid_argument&) {
				cout << "No entries" << endl;
			}
		} else if (command.empty()) {
			continue;
		} else {
			throw logic_error("Unknown command: " + command);
		}
	}

	return 0;
}