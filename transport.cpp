#include <iostream>
#include <set>
#include <optional>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include "json.h"

using namespace std;

class DB {
public:
	void CreateDBRequests(istream& in_stream = cin) {
		const auto request_count = ReadNumberOnLine<size_t>(in_stream);
		for(size_t i = 0; i < request_count; ++i) {
			string request_str;
			getline(in_stream, request_str);
			AddToDB(request_str);
		}
	}

	void CreateDBRequests(const vector<Json::Node>& base_requests) {
		const auto request_count = base_requests.size();
		for(size_t i = 0; i < request_count; ++i) {
			AddToDBJson(base_requests[i]);
		}
	}

	void ToDBRequests(istream& in_stream = cin) {
		const auto request_count = ReadNumberOnLine<size_t>(in_stream);
		for(size_t i = 0; i < request_count; ++i) {
			string request_str;
			getline(in_stream, request_str);
			GetFromDB(request_str);
		}
	}

	void ToDBRequests(const vector<Json::Node>& stat_requests) {
		const auto request_count = stat_requests.size();
		cout << "[" << endl;
		for(size_t i = 0; i < request_count; ++i) {
			GetFromDBJson(stat_requests[i]);
			if(i < request_count - 1) {
				cout << ",\n";
			} else {
				cout << "\n";
			}
		}
		cout << "]" << endl;
	}

private:
	pair<string_view, optional<string_view>> SplitTwoStrict(string_view s, string_view delimiter = " ") {
		const size_t pos = s.find(delimiter);
		if(pos == s.npos) {
			return {s, nullopt};
		} else {
			return {s.substr(0, pos), s.substr(pos + delimiter.length())};
		}
	}

	pair<string_view, string_view> SplitTwo(string_view s, string_view delimiter = " ") {
		const auto[lhs, rhs_opt] = SplitTwoStrict(s, delimiter);
		return {lhs, rhs_opt.value_or("")};
	}

	string ReadToken(string_view& s, string_view delimiter = " ") {
		const auto[lhs, rhs] = SplitTwo(s, delimiter);
		s = rhs;
		return string(lhs);
	}

	enum class RequestType {
		STOP,
		BUS
	};

	const unordered_map<string, RequestType> STR_TO_REQUEST_TYPE = {
			{"Stop", RequestType::STOP},
			{"Bus",  RequestType::BUS},
	};

	template<typename Number>
	Number ReadNumberOnLine(istream& stream) {
		Number number;
		stream >> number;
		string dummy;
		getline(stream, dummy);
		return number;
	}

	optional<RequestType> ConvertRequestTypeFromString(const string& type_str) {
		if(const auto it = STR_TO_REQUEST_TYPE.find(type_str);
				it != STR_TO_REQUEST_TYPE.end()) {
			return it->second;
		} else {
			return nullopt;
		}
	}

	vector<string> parseAndAddToBusStops(string_view& request_str, const string& busName, const string& delimiter) {
		vector<string> buss_stops;
		while(request_str != "") {
			auto stop = ReadToken(request_str, delimiter);
			Stops[stop].insert(busName);
			buss_stops.push_back(stop);
		}
		return buss_stops;
	}

	vector<string> AddBussRoute(string_view request_str, const string& busName) {
		if(request_str.find(">") != std::string_view::npos) {
			return parseAndAddToBusStops(request_str, busName, " > ");
		} else {
			vector<string> buss_stops = parseAndAddToBusStops(request_str, busName, " - ");
			vector<string> temp(buss_stops.begin(), buss_stops.end() - 1);
			buss_stops.insert(buss_stops.end(), temp.rbegin(), temp.rend());
			return buss_stops;
		}
	}

	vector<string> AddBussRoute(const vector<Json::Node>& stops, bool isRoundTrip, const string& busName) {
		vector<string> buss_stops;
		for(const auto& stop: stops) {
			const auto& stopStr = stop.AsString();
			buss_stops.push_back(stopStr);
			Stops[stopStr].insert(busName);
		}
		if(!isRoundTrip) {
			vector<string> temp(buss_stops.begin(), buss_stops.end() - 1);
			buss_stops.insert(buss_stops.end(), temp.rbegin(), temp.rend());
		}
		return buss_stops;
	}

	pair<double, double> AddStopWithCoords(string_view& request_str) {
		return make_pair(stod(ReadToken(request_str, ", ")),
						 stod(ReadToken(request_str, ", ")));
	}

	void parseAndAddToStopsDistance(string_view& request_str, const string& fromStop) {
		while(request_str != "") {
			long distance = stol(ReadToken(request_str, "m to "));
			string toStop = ReadToken(request_str, ", ");
			StopsDistance[make_pair(fromStop, toStop)] = distance;
		}
	}

	void parseAndAddToStopsDistance(const Json::Node& node, const string& fromStop) {
		auto distances = node.AsMap();
		for(const auto&[key, value]: distances) {
			StopsDistance[make_pair(fromStop, key)] = value.AsInt();
		}
	}

	void AddToDB(string_view request_str) {
		const auto request_type = ConvertRequestTypeFromString(ReadToken(request_str));
		if(request_type) {
			const string key = ReadToken(request_str, ": ");
			switch(*request_type) {
				case RequestType::STOP:
					StopsCoords[key] = AddStopWithCoords(request_str);
					parseAndAddToStopsDistance(request_str, key);
					break;
				case RequestType::BUS:
					Buses[key] = AddBussRoute(request_str, key);
					break;
			}
		}
	}

	void AddToDBJson(const Json::Node& node) {
		map<string, Json::Node> m = node.AsMap();
		const auto request_type = ConvertRequestTypeFromString(m["type"].AsString());
		if(request_type) {
			auto name = m["name"].AsString();
			switch(*request_type) {
				case RequestType::STOP: {
					StopsCoords[name] = make_pair(m["longitude"].AsDouble(), m["latitude"].AsDouble());
					parseAndAddToStopsDistance(m["road_distances"], name);
					break;
				}
				case RequestType::BUS:
					Buses[name] = AddBussRoute(m["stops"].AsArray(), m["is_roundtrip"].AsBool(), name);
					break;
			}
		}
	}

	static double ToRad(double grad) {
		return grad * 3.1415926535 / 180.0;
	}

	double CalculateGeographicLength(const vector<string>& stops) {
		double length = 0;
		for(size_t i = 0; i < stops.size() - 1; ++i) {
			pair<double, double> lhsCoords = StopsCoords[stops[i]];
			pair<double, double> rhsCoords = StopsCoords[stops[i + 1]];
			double lhsLatInRad = ToRad(lhsCoords.second);
			double lhsLonInRad = ToRad(lhsCoords.first);
			double rhsLatInRad = ToRad(rhsCoords.second);
			double rhsLonInRad = ToRad(rhsCoords.first);
			length += acos(sin(lhsLatInRad) * sin(rhsLatInRad)
						   + cos(lhsLatInRad) * cos(rhsLatInRad) * cos(abs(lhsLonInRad - rhsLonInRad)))
					  * 6371000;
		}
		return length;
	}

	double CalculateRoadLength(const vector<string>& stops) {
		double length = 0;
		for(size_t i = 0; i < stops.size() - 1; ++i) {
			auto stopsPair = make_pair(stops[i], stops[i + 1]);
			if(StopsDistance.count(stopsPair) == 0) {
				length += StopsDistance[make_pair(stops[i + 1], stops[i])];
			} else {
				length += StopsDistance[stopsPair];
			}
		}
		return length;
	}

	string BusInfo(const string& bus) {
		if(Buses.count(bus) != 0) {
			string result = "Bus " + bus + ": ";
			vector<string> stops = Buses[bus];
			result += to_string(stops.size()) + " stops on route, ";
			double geographicLength = CalculateGeographicLength(stops);
			double length = CalculateRoadLength(stops);
			sort(stops.begin(), stops.end());
			auto it = unique(stops.begin(), stops.end());
			stops.erase(it, stops.end());
			result += to_string(stops.size()) + " unique stops, "
					  + to_string(length) + " route length, "
					  + to_string(length / geographicLength) + " curvature";
			return result;
		}
		return "Bus " + bus + ": not found";
	}

	string StopInfo(const string& stop) {
		if(StopsCoords.count(stop) == 0) {
			return "Stop " + stop + ": not found";
		}
		string result;
		for(const auto& bus: Stops[stop]) {
			result += bus + " ";
		}
		if(result.empty()) {
			return "Stop " + stop + ": no buses";
		}
		return "Stop " + stop + ": buses " + result;
	}

	void GetFromDB(string_view request_str) {
		const auto request_type = ConvertRequestTypeFromString(ReadToken(request_str));
		if(request_type) {
			const string key = ReadToken(request_str, ": ");
			switch(*request_type) {
				case RequestType::STOP:
					cout << StopInfo(key) << endl;
					break;
				case RequestType::BUS:
					cout << BusInfo(key) << endl;
					break;
			}
		}
	}

	void GetFromDBJson(const Json::Node& node) {
		cout << "{" << endl;
		map<string, Json::Node> m = node.AsMap();
		const auto request_type = ConvertRequestTypeFromString(m["type"].AsString());
		if(request_type) {
			auto name = m["name"].AsString();
			switch(*request_type) {
				case RequestType::STOP: {
					if(StopsCoords.count(name) == 0) {
						cout << "\"request_id\": " << m["id"].AsInt() << ",\n";
						cout << R"("error_message": "not found")" << "\n";
					} else {
						set<string> stops = Stops[name];
						if(stops.empty()) {
							cout << "\"buses\": [],\n";
							cout << "\"request_id\": " << m["id"].AsInt() << "\n";
						} else {
							cout << "\"buses\": [" << "\n";
							for(auto it = stops.begin(); it != stops.end(); ++it) {
								cout << "\"" << *it << "\"";
								if(it != --stops.end()) {
									cout << ",\n";
								} else {
									cout << "\n" << "]," << "\n";
								}
							}
							cout << "\"request_id\": " << m["id"].AsInt() << "\n";
						}
					}
					break;
				}
				case RequestType::BUS: {
					if(Buses.count(name) != 0) {
						auto stops = Buses[name];
						auto length = CalculateRoadLength(stops);
						cout << "\"route_length\": " << length << ",\n";
						cout << "\"request_id\": " << m["id"].AsInt() << ",\n";
						cout << "\"curvature\": " << length / CalculateGeographicLength(stops) << ",\n";
						cout << "\"stop_count\": " << stops.size() << ",\n";
						sort(stops.begin(), stops.end());
						auto it = unique(stops.begin(), stops.end());
						stops.erase(it, stops.end());
						cout << "\"unique_stop_count\": " << stops.size() << "\n";
					} else {
						cout << "\"request_id\": " << m["id"].AsInt() << ",\n";
						cout << R"("error_message": "not found")" << "\n";
					}
					break;
				}
			}
		}
		cout << "}";
	}

	unordered_map<string, pair<double, double>> StopsCoords;
	unordered_map<string, vector<string>> Buses;
	unordered_map<string, set<string>> Stops;
	map<pair<string, string>, long> StopsDistance;
};

int main() {
	cout.precision(6);
	auto document = Json::Load(cin);
	DB db;
	auto m = document.GetRoot().AsMap();
	Json::Node base_requests = m["base_requests"];
	db.CreateDBRequests(base_requests.AsArray());
	Json::Node stat_requests = m["stat_requests"];
	db.ToDBRequests(stat_requests.AsArray());
	weak_ptr<Json::Node> p;

	return 0;
}