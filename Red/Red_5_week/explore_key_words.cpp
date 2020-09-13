#include "../../test_runner.h"
#include "../../profile.h"

#include <map>
#include <future>
#include <functional>

using namespace std;

struct Stats {
	map<string, int> word_frequences;

	void operator+=(const Stats& other) {
		for (const auto&[key, value]: other.word_frequences) {
			word_frequences[key] += other.word_frequences.at(key);
		}
	}
};

vector<string_view> split(string_view str) {
	vector<string_view> result;

	size_t pos = str.find_first_not_of(" \f\n\r\t\v");
	const size_t pos_end = std::string_view::npos;
	str.remove_prefix(pos);

	while (true) {
		size_t space = str.find(' ', pos);
		result.push_back(space == pos_end
						 ? str.substr(pos)
						 : str.substr(pos, space - pos));
		if (space == pos_end) {
			break;
		} else {
			pos = space + 1;
		}
	}
	return result;
}

Stats ExploreLine(const set<string>& key_words, const string& line) {
	Stats result{};
	for (const string_view& word: split(line)) {
		const string& word_from_view = string(word.begin(), word.end());
		if (key_words.count(word_from_view) != 0) {
			if (result.word_frequences.count(word_from_view) == 0) {
				result.word_frequences[word_from_view] = 1;
			} else {
				++result.word_frequences[word_from_view];
			}
		}
	}
	return result;
}

Stats ExploreKeyWordsSingleThread(
		const set<string>& key_words, istream& input
) {
	Stats result{};
	for (string line; getline(input, line);) {
		result += ExploreLine(key_words, line);
	}
	return result;
}

Stats ExploreKeyWords(const set<string>& key_words, istream& input) {
	vector<future<Stats>> futures;
	vector<istringstream> streams;
	int j = 0, i = 10000;
	while (!input.eof()) {
		string tmp, line;
		while (j < i && !input.eof()) {
			getline(input, tmp);
			if (!tmp.empty()) {
				line += tmp + "\n";
			}
			++j;
		}
		j = 0;
		if (!line.empty()) {
			streams.emplace_back(line);
		}
	}
	for (auto& s: streams) {
		futures.push_back(async(ExploreKeyWordsSingleThread, ref(key_words), ref(s)));
	}

	Stats result{};
	for (auto& f: futures) {
		result += f.get();
	}
	return result;
}

void TestBasic() {
	const set<string> key_words = {"yangle", "rocks", "sucks", "all"};

	stringstream ss;
	ss << "this new yangle service really rocks\n";
	ss << "It sucks when yangle isn't available\n";
	ss << "10 reasons why yangle is the best IT company\n";
	ss << "yangle rocks others suck\n";
	ss << "Goondex really sucks, but yangle rocks. Use yangle\n";

	ss << "this new yangle service really rocks\n";
	ss << "It sucks when yangle isn't available\n";
	ss << "10 reasons why yangle is the best IT company\n";
	ss << "yangle rocks others suck\n";
	ss << "Goondex really sucks, but yangle rocks. Use yangle\n";


	const auto stats = ExploreKeyWords(key_words, ss);
	const map<string, int> expected = {
			{"yangle", 12},
			{"rocks",  4},
			{"sucks",  2}
	};
	ASSERT_EQUAL(stats.word_frequences, expected);
}

int main() {
	TestRunner tr;
	RUN_TEST(tr, TestBasic);
}