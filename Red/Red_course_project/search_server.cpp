#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

vector<string> SplitIntoWords(const string& line) {
	istringstream words_input(line);
	return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

int CountWordInLine(const string& line, const string& word) {
	istringstream words_input(line);
	return count(istream_iterator<string>(words_input), istream_iterator<string>(), word);
}

SearchServer::SearchServer(istream& document_input) {
	UpdateDocumentBaseSingleThread(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {}

void SearchServer::UpdateDocumentBaseSingleThread(istream& document_input) {
	InvertedIndex new_index;

	for(string current_document; getline(document_input, current_document);) {
		new_index.Add(current_document);
	}

	index = move(new_index);
}

void SearchServer::AddQueriesStream(
		istream& query_input, ostream& search_results_output
) {
	futures.push_back(async(&SearchServer::AddQueriesStreamSingleThread, this, ref(query_input),
							ref(search_results_output)));
}

void SearchServer::AddQueriesStreamSingleThread(
		istream& query_input, ostream& search_results_output
) {
	auto indexSize = index.IndexSize();
	vector<size_t> docid_count(indexSize);
	for(string current_query; getline(query_input, current_query);) {
		docid_count.assign(indexSize, 0);

		vector<string> words = SplitIntoWords(current_query);

		for(const auto& word : words) {
			for(const auto[docid, hitcount] : index.Lookup(word)) {
				docid_count[docid] += hitcount;
			}
		}
		int i = 0;
		vector<pair<size_t, size_t>> search_results;
		for(auto item: docid_count) {
			if(item != 0) {
				search_results.emplace_back(i, item);
			}
			++i;
		}

		partial_sort(
				begin(search_results),
				begin(search_results) + min<size_t>(5, search_results.size()),
				end(search_results),
				[](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
					int64_t lhs_docid = lhs.first;
					auto lhs_hit_count = lhs.second;
					int64_t rhs_docid = rhs.first;
					auto rhs_hit_count = rhs.second;
					return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
				}
		);

		search_results_output << current_query << ':';
		for(auto[docid, hitcount] : Head(search_results, 5)) {
			search_results_output << " {"
								  << "docid: " << docid << ", "
								  << "hitcount: " << hitcount << '}';
		}
		search_results_output << endl;
	}
}

void InvertedIndex::Add(const string& document) {
	docs.push_back(document);

	const size_t docid = docs.size() - 1;
	set<string> words;
	for(const auto& word : SplitIntoWords(document)) {
		if(words.find(word) == words.end()) {
			words.insert(word);
			index[word].emplace_back(docid, CountWordInLine(document, word));
		}
	}
}

vector<pair<size_t, size_t>> InvertedIndex::Lookup(const string& word) const {
	if(auto it = index.find(word); it != index.end()) {
		return it->second;
	} else {
		return {};
	}
}