#include <math.h>

#include "my_json.h"

using namespace std;

namespace Json {

    Document::Document(Node root) : root(move(root)) {
    }

    const Node &Document::GetRoot() const {
        return root;
    }

    Node LoadNode(istream &input);

    Node LoadArray(istream &input) {
        vector<Node> result;

        for (char c; input >> c && c != ']';) {
            if (c != ',') {
                input.putback(c);
            }
            result.push_back(LoadNode(input));
        }

        return Node(vector_type{move(result)});
    }

    Node LoadDouble(istream &input) {
        double result = 0;
        bool is_negative = false;
        if (input.peek() == '-') {
            is_negative = true;
            input.ignore(1);
        }
        while (isdigit(input.peek())) {
            result *= 10;
            if (is_negative) {
                result -= input.get() - '0';
            } else {
                result += input.get() - '0';
            }
        }
        if (input.peek() == '.') {
            input.ignore(1);
            int after_point_count = 0;
            while (isdigit(input.peek())) {
                ++after_point_count;
                result *= 10;
                if (is_negative) {
                    result -= input.get() - '0';
                } else {
                    result += input.get() - '0';
                }
            }
            result /= std::pow(10, after_point_count);
        }
        return Node(double_type{result});
    }

    Node LoadString(istream &input) {
        string line;
        getline(input, line, '\"');
        return Node(string_type{string(move(line))});
    }

    Node LoadDict(istream &input) {
        map<string, Node> result;

        for (char c; input >> c && c != '}';) {
            if (c == ',') {
                input >> c;
            }

            string key = LoadString(input).AsString();
            input >> c;
            result.emplace(move(key), LoadNode(input));
        }

        return Node(map_type{move(result)});
    }

    Node LoadBool(istream &input) {

        string message;
        char c;
        input >> c;
        if (c == 't') {
            input.ignore(3);
        } else {
            input.ignore(4);
        }
        return Node(bool_type{bool(c == 't')});
    }

    Node LoadNode(istream &input) {
        char c;
        input >> c;

        if (c == '[') {
            return LoadArray(input);
        } else if (c == '{') {
            return LoadDict(input);
        } else if (c == '\"') {
            return LoadString(input);
        } else if (c == 't' || c == 'f') {
            input.putback(c);
            return LoadBool(input);
        } else {
            input.putback(c);
            return LoadDouble(input);
        }
    }

    Document Load(istream &input) {
        return Document{LoadNode(input)};
    }

}