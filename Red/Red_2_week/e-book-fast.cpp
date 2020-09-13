#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

class ReadingManager {
public:
	ReadingManager()
			: user_to_page(MAX_USER_COUNT_ + 1, -1),
			  page_to_users(MAX_PAGE_COUNT_ + 1, 0),
			  user_number(0){}

	void Read(int user_id, int page_count) {
		auto oldPage = user_to_page[user_id];
		if(oldPage > -1) {
			--page_to_users[oldPage];
			--user_number;
		}

		user_to_page[user_id] = page_count;
		++page_to_users[page_count];
		++user_number;
	}

	double Cheer(int user_id) const {
		if (user_to_page[user_id] == -1) {
			return 0;
		}
		if (user_number == 1) {
			return 1;
		}
		int count = 0;
		const auto users_page = user_to_page.at(user_id);
		for (int i = 0; i < users_page; ++i) {
			count += page_to_users[i];
		}
		return count * 1.0 /(user_number - 1);
	}

private:
	static const int MAX_USER_COUNT_ = 100'000;
	static const int MAX_PAGE_COUNT_ = 1000;
	vector<int> user_to_page;
	vector<int> page_to_users;
	int user_number;
};


int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	ReadingManager manager;

	int query_count;
	cin >> query_count;

	for (int query_id = 0; query_id < query_count; ++query_id) {
		string query_type;
		cin >> query_type;
		int user_id;
		cin >> user_id;

		if (query_type == "READ") {
			int page_count;
			cin >> page_count;
			manager.Read(user_id, page_count);
		} else if (query_type == "CHEER") {
			cout << setprecision(6) << manager.Cheer(user_id) << "\n";
		}
	}

	return 0;
}