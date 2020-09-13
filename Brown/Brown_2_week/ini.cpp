#include "ini.h"

#include <iostream>

namespace Ini {

	Section& Document::AddSection(const string& name) {
		if(sections.count(name) == 0) {
			sections[name] = {};
		}
		return sections.at(name);
	}

	size_t Document::SectionCount() const {
		return sections.size();
	}

	const Section& Document::GetSection(const string& name) const {
		return sections.at(name);
	}

	string_view Unquote(string_view value) {
		if(!value.empty() && value.front() == '[') {
			value.remove_prefix(1);
		}
		if(!value.empty() && value.back() == ']') {
			value.remove_suffix(1);
		}
		return value;
	}

	pair<string_view, string_view> Split(string_view line, char by) {
		size_t pos = line.find(by);
		string_view left = line.substr(0, pos);

		if(pos < line.size() && pos + 1 < line.size()) {
			return {left, line.substr(pos + 1)};
		} else {
			return {left, string_view()};
		}
	}

	Document Load(istream& input) {
		Document result;
		Section* section = nullptr;
		for(string line; getline(input, line);) {
			if(line[0] == '[') {
				line = Unquote(line);
				section = &result.AddSection(line);
			} else if(!line.empty()){
				auto split = Split(line, '=');
				section->insert(split);
			}
		}
		return result;
	}
}
