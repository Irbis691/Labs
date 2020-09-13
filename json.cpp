#include <iostream>
#include "json.h"

using namespace std;

namespace Json {

	Document::Document(Node root) : root(move(root)) {
	}

	const Node& Document::GetRoot() const {
		return root;
	}

	Node LoadNode(istream& input);

	Node LoadArray(istream& input) {
		vector<Node> result;

		for(char c; input >> c && c != ']';) {
			if(c != ',') {
				input.putback(c);
			}
			result.push_back(LoadNode(input));
		}

		return Node(move(result));
	}

	Node LoadIntOrDouble(istream& input) {
		string result;
		if(input.peek() == '-') {
			result += '-';
			input.get();
		}
		while (isdigit(input.peek())) {
			result += to_string(input.get() - '0');
		}
		if(input.peek() == '.') {
			result += '.';
			input.get();
			while (isdigit(input.peek())) {
				result += to_string(input.get() - '0');
			}
			return Node(stod(result));
		}
		return Node(stoi(result));
	}

	Node LoadString(istream& input) {
		string line;
		getline(input, line, '"');
		return Node(move(line));
	}

	Node LoadDict(istream& input) {
		map<string, Node> result;

		for(char c; input >> c && c != '}';) {
			if(c == ',') {
				input >> c;
			}

			string key = LoadString(input).AsString();
			input >> c;
			result.emplace(move(key), LoadNode(input));
		}

		return Node(move(result));
	}

	Node LoadBool(istream& input) {
		bool b;
		input >> boolalpha >> b;
		return Node(b);
	}

	Node LoadNode(istream& input) {
		char c;
		input >> c;

		if(c == '[') {
			return LoadArray(input);
		} else if(c == '{') {
			return LoadDict(input);
		} else if(c == '"') {
			return LoadString(input);
		} else {
			input.putback(c);
			c = input.peek();
			if(c == 't' || c == 'f') {
				return LoadBool(input);
			} else {
				return LoadIntOrDouble(input);
			}
		}
	}

	Document Load(istream& input) {
		return Document{LoadNode(input)};
	}

}