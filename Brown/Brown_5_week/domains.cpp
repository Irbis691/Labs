#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;

vector<string> ReadDomains() {
	size_t count;
	cin >> count;

	vector<string> domains;
	for (size_t i = 0; i < count; ++i) {
		string domain;
		cin >> domain;
		domains.push_back(domain);
	}
	return domains;
}

vector<string> SplitBy(const string& str, const char& ch) {
	string next;
	vector<string> result;

	for (char it : str) {
		if (it == ch) {
			if (!next.empty()) {
				result.push_back(next);
				next.clear();
			}
		} else {
			next += it;
		}
	}
	if (!next.empty())
		result.push_back(next);
	return result;
}

string Join(vector<string>& v) {
	string result;
	for (auto& i : v) {
		result += i + '.';
	}
	return result;
}

bool IsSubdomain(string_view subdomain, string_view domain) {
	size_t i = 0;
	size_t j = 0;
	while (i < subdomain.size() && j < domain.size()) {
		if (subdomain[i++] != domain[j++]) {
			return false;
		}
	}
	return (i == subdomain.size() && domain[--j] == '.')
		   || (j == domain.size() && subdomain[--i] == '.');
}


int main() {
	const vector<string> banned_domains = ReadDomains();
	const vector<string> domains_to_check = ReadDomains();

	vector<string> reversed_banned_domains;
	for (const auto& domain : banned_domains) {
		vector<string> split = SplitBy(domain, '.');
		reverse(begin(split), end(split));
		reversed_banned_domains.push_back(Join(split));
	}
	sort(begin(reversed_banned_domains), end(reversed_banned_domains));

	size_t insert_pos = 0;
	for (string& domain : reversed_banned_domains) {
		if (insert_pos == 0 || !IsSubdomain(domain, reversed_banned_domains[insert_pos - 1])) {
			swap(reversed_banned_domains[insert_pos++], domain);
		}
	}
	reversed_banned_domains.resize(insert_pos);

	vector<string> reversed_domains_to_check;
	for (const auto& domain : domains_to_check) {
		vector<string> split = SplitBy(domain, '.');
		reverse(begin(split), end(split));
		reversed_domains_to_check.push_back(Join(split));
	}
	for (const string_view domain : reversed_domains_to_check) {
		if (const auto it = upper_bound(begin(reversed_banned_domains), end(reversed_banned_domains), domain);
				it != begin(reversed_banned_domains) && IsSubdomain(domain, *prev(it))) {
			cout << "Bad" << endl;
		} else {
			cout << "Good" << endl;
		}
	}
	return 0;
}