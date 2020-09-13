#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <iostream>
#include <type_traits>

namespace Json {
    class Node;

    struct bool_type {
        bool value;
    };

    struct double_type {
        double value;
    };

    struct map_type {
        std::map<std::string, Node> value;
    };

    struct vector_type {
        std::vector<Node> value;
    };

    struct string_type {
        std::string value;
    };

    class Node : std::variant<vector_type,
            map_type,
            double_type, bool_type,
            string_type> {
    public:
        using variant::variant;

        const auto& AsArray() const {
            return std::get<vector_type>(*this).value;
        }
        const auto& AsMap() const {
            return std::get<map_type>(*this).value;
        }
        double AsDouble() const {
            return std::get<double_type>(*this).value;
        }
        const auto& AsString() const {
            return std::get<string_type>(*this).value;
        }
        bool AsBool() const {
            return std::get<bool_type>(*this).value;
        }
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root;
    };

    Document Load(std::istream& input);

}