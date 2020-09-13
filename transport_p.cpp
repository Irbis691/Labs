#include <unordered_map>
#include <istream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <unordered_set>
#include "json.h"

#include "graph.h"
#include "router.h"
#include "test_runner.h"

static const unsigned VERSION = 5;

namespace Request {
	enum class Name {
		STOP,
		BUS,
		ROUTE
	};

	std::ostream& operator<<(std::ostream& stream, const Name& type) {
		switch(type) {
			case Request::Name::STOP:
				stream << "Request::Name::STOP";
				break;
			case Request::Name::BUS:
				stream << "Request::Name::BUS";
				break;
			default:
				stream << "Request::Name::ROUTE";
		}
		return stream;
	}

	enum class RouteRequestType {
		NEW,
		EXISTING
	};

	std::ostream& operator<<(std::ostream& stream, const RouteRequestType& type) {
		switch(type) {
			case Request::RouteRequestType::NEW:
				stream << "Request::RouteRequestType::NEW";
				break;
			case Request::RouteRequestType::EXISTING:
				stream << "Request::RouteRequestType::EXISTING";
		}
		return stream;
	}
}

enum class RouteType {
	CIRCULAR,
	LOOPING
};

std::ostream& operator<<(std::ostream& stream, const RouteType& type) {
	switch(type) {
		case RouteType::CIRCULAR:
			stream << "Request::RouteType::CIRCULAR";
			break;
		case RouteType::LOOPING:
			stream << "Request::RouteType::LOOPING";
	}
	return stream;
}

static const std::unordered_map<std::string, Request::Name> requestNames = {
		{"Stop",  Request::Name::STOP},
		{"Bus",   Request::Name::BUS},
		{"Route", Request::Name::ROUTE}
};

Request::Name ParseRequestName(std::istream& stream) {
	Request::Name requestName;
	static std::string request;
	stream >> std::ws >> request;
	requestName = requestNames.at(request);
	request.clear();
	return requestName;
}

class Coordinates {
	double latitude, longitude;
	constexpr static double factor = 3.1415926535 / 180;

public:
	Coordinates() = default;

	Coordinates(double latitude, double longitude) : latitude(latitude * factor), longitude(longitude * factor) {}

	[[nodiscard]] double CalcDist(const Coordinates& other) const noexcept {
		return acos(
				sin(latitude) * sin(other.latitude) +
				cos(latitude) * cos(other.latitude) * cos(std::abs(longitude - other.longitude))
		) * 6371000;
	}

	bool operator==(const Coordinates& other) const noexcept {
		return latitude == other.latitude && longitude == other.longitude;
	}

	friend std::istream& operator>>(std::istream&, Coordinates&);

	friend std::ostream& operator<<(std::ostream&, const Coordinates&);
};

std::istream& operator>>(std::istream& stream, Coordinates& coordinates) {
	stream >> coordinates.latitude;
	coordinates.latitude *= Coordinates::factor;
	stream.ignore();
	stream >> coordinates.longitude;
	coordinates.longitude *= Coordinates::factor;
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const Coordinates& coordinates) {
	stream << coordinates.latitude << ", " << coordinates.longitude;
	return stream;
}

struct StopInfo {
	std::string name;
	Coordinates coordinates;
	std::unordered_map<std::string, unsigned> distances;

	StopInfo() = default;

	StopInfo(std::string name, const Coordinates coordinates) : name(std::move(name)), coordinates(coordinates) {}

	StopInfo(StopInfo&& other) noexcept : name(std::move(other.name)), coordinates(other.coordinates) {}

	bool operator==(const StopInfo& other) const noexcept {
		return name == other.name && coordinates == other.coordinates;
	}
};

std::ostream& operator<<(std::ostream& stream, const StopInfo& stopInfo) {
	stream << "StopInfo{ name: " << stopInfo.name << ", coordinates: (" << stopInfo.coordinates << ") }";
	return stream;
}

StopInfo ParseNewStop(std::istream& stream) {
	StopInfo bus_stop_info;
	stream >> std::ws;
	std::getline(stream, bus_stop_info.name, ':');
	stream >> bus_stop_info.coordinates;

	if(stream.peek() != '\n') {
		stream.ignore(2);

		std::string distance_line;
		std::getline(stream, distance_line);
		std::istringstream dist_line_stream(distance_line);
		std::string bus_stop;
		while(!dist_line_stream.eof()) {
			unsigned dist;
			dist_line_stream >> dist;
			dist_line_stream.ignore(5);
			std::getline(dist_line_stream, bus_stop, ',');
			bus_stop_info.distances.emplace(std::move(bus_stop), dist);
		}
	}
	return bus_stop_info;
}

struct RouteRequestInfo {
	std::string name;
	Request::RouteRequestType type;

	bool operator==(const RouteRequestInfo& other) const {
		return name == other.name && type == other.type;
	}
};

std::ostream& operator<<(std::ostream& stream, const RouteRequestInfo& rri) {
	stream << "RouteRequestInfo{ number: " << rri.name << ", type: " << rri.type << " }";
	return stream;
}

RouteRequestInfo ParseRouteNameAndType(std::istream& stream) {
	RouteRequestInfo route_info;
	stream >> route_info.name;
	if(route_info.name.back() == ':') {
		route_info.type = Request::RouteRequestType::NEW;
		route_info.name.pop_back();
	} else {
		route_info.type = Request::RouteRequestType::EXISTING;
	}
	return route_info;
}

std::string ParseRouteName(std::istream& stream, const Request::RouteRequestType type) {
	std::string routeNumber;
	stream >> std::ws;
	std::getline(stream, routeNumber, (type == Request::RouteRequestType::NEW) ? ':' : '\n');
	return routeNumber;
}

struct RouteInfo {
	RouteType type;
	std::vector<std::string> stops;

	bool operator==(const RouteInfo& other) const {
		return type == other.type && stops == other.stops;
	}

	RouteInfo() = default;

	RouteInfo(RouteType type, std::vector<std::string> stops) : type(type), stops(std::move(stops)) {}

	RouteInfo(RouteInfo&& other) noexcept : type(other.type), stops(std::move(other.stops)) {}
};

std::ostream& operator<<(std::ostream& stream, const RouteInfo& rri) {
	stream << "RouteInfo{ type: " << rri.type << ", stops: " << rri.stops << '}';
	return stream;
}

RouteInfo ParseNewRoute(std::istream& stream) {
	RouteInfo route_info;
	std::string line;
	stream.ignore();
	std::getline(stream, line);

	char delim = line[line.find_first_of(">-")];
	switch(delim) {
		case '>':
			route_info.type = RouteType::CIRCULAR;
			break;
		case '-':
			route_info.type = RouteType::LOOPING;
	}
	std::stringstream ss(line);
	std::string cur_stop;
	while(std::getline(ss, cur_stop, delim)) {
		if(cur_stop.back() == ' ') {
			cur_stop.pop_back();
		}
		route_info.stops.push_back(std::move(cur_stop));
		ss.ignore();
	}
	return route_info;
}

class DataBase {
	struct StopData;
	using StopStats = std::unordered_map<std::string, StopData>;
	struct RouteData;
	using RouteStats = std::unordered_map<std::string, RouteData>;

	struct EdgeIdRange {
		unsigned min, max;
	};

	struct RouteData {
		using distOptional = std::optional<std::vector<unsigned>>;

		explicit RouteData(RouteInfo routeInfo) : type(routeInfo.type), num_stops(routeInfo.stops.size()),
												  stops(std::move(routeInfo.stops)),
												  unique_stops(stops.begin(), stops.end()) {}

		void InitDists(const StopStats& stop_coords) {
			dists.emplace(distOptional::value_type());
			switch(type) {
				case RouteType::LOOPING:
					num_stops += num_stops - 1;
					dists->reserve(num_stops - 1);
					CalcLoopingDist(stop_coords);
					break;
				default:
					dists->reserve(num_stops - 1);
					CalcCircularDist(stop_coords);
			}
			inStopsNum += num_stops;
		}

	private:
		void CalcLoopingDist(const StopStats& stop_coords) {
			const auto stops_begin = stops.begin(), stops_end = stops.end();
			auto prev_stop_data = &stop_coords.at(*stops_begin);
			auto prev_stop = stops_begin;
			std::vector<unsigned> rev_dists;
			rev_dists.reserve(stops.size() - 1);
			for(auto stop = std::next(stops_begin); stop != stops_end; ++stop) {
				auto cur_stop_data = &stop_coords.at(*stop);
				auto geo_dist_addend = cur_stop_data->coordinates.CalcDist(prev_stop_data->coordinates);
				geo_distance += geo_dist_addend;

				auto it = prev_stop_data->distances.find(*stop);
				if(it != prev_stop_data->distances.end()) {
					real_distance += it->second;
					dists->push_back(it->second);
					if(auto it2 = cur_stop_data->distances.find(*prev_stop); it2 != cur_stop_data->distances.end()) {
						real_distance += it2->second;
						rev_dists.push_back(it2->second);
					} else {
						real_distance += it->second;
						rev_dists.push_back(it->second);
					}
				} else {
					auto& real_dist = cur_stop_data->distances.at(*prev_stop);
					real_distance += real_dist;
					real_distance += real_dist;
					dists->push_back(real_dist);
					rev_dists.push_back(real_dist);
				}

				prev_stop_data = cur_stop_data;
				prev_stop = stop;
			}
			geo_distance += geo_distance;
			std::copy(rev_dists.rbegin(), rev_dists.rend(), std::back_inserter(*dists));
		}

		void CalcCircularDist(const StopStats& stop_coords) {
			const auto stops_begin = stops.begin(), stops_end = stops.end();
			auto prev_stop_data = &stop_coords.at(*stops_begin);
			auto prev_stop = stops_begin;
			for(auto stop = std::next(stops_begin); stop != stops_end; ++stop) {
				auto cur_stop_data = &stop_coords.at(*stop);
				auto geo_dist_addend = cur_stop_data->coordinates.CalcDist(prev_stop_data->coordinates);
				geo_distance += geo_dist_addend;

				if(auto it = prev_stop_data->distances.find(*stop); it != prev_stop_data->distances.end()) {
					real_distance += it->second;
					dists->push_back(it->second);
				} else {
					auto& real_dist_addend = cur_stop_data->distances.at(*prev_stop);
					real_distance += real_dist_addend;
					dists->push_back(real_dist_addend);
				}
				prev_stop_data = cur_stop_data;
				prev_stop = stop;
			}
		}

	public:
		RouteType type;
		unsigned num_stops, real_distance = 0;
		double geo_distance = 0.0;

		std::vector<std::string> stops;
		std::unordered_set<std::string_view> unique_stops;
		mutable distOptional dists = std::nullopt;
	};

	struct StopData {
		Coordinates coordinates;
		std::optional<std::set<std::string_view>> route_set_proxy;
		std::unordered_map<std::string, unsigned> distances;

		void InitProxy(const std::string& stop_name, const RouteStats& routeStats) {
			route_set_proxy = std::set<std::string_view>();
			for(auto &[route, routeData] : routeStats) {
				for(auto& stop : routeData.stops) {
					if(stop == stop_name) {
						route_set_proxy->emplace(route);
						break;
					}
				}
			}
		}
	};

	struct TemporalInfo {
		double waitTime, velocity;
	};

	class GraphBuilder {
		using WeightType = double;
		using Router = Graph::Router<WeightType>;
	public:
		GraphBuilder(const RouteStats& routeStats, const StopStats& stopStats, const TemporalInfo& temporalInfo) :
				graph(inStopsNum + stopStats.size()),
				outStopIDtoName(InitOutStopIDtoStopName(stopStats)),
				nameToOutStopID(InitNameToOutStopID(outStopIDtoName)),
				maxEdgeIDtoRouteName(InitGraphAndMaxEdgeIDtoRouteName(routeStats, temporalInfo)),
				router(graph) {}

		template<typename It>
		struct ItRange {
			It begin, end;
		};

		Graph::DirectedWeightedGraph<double> graph;
		const std::vector<std::string_view> outStopIDtoName;
		const std::unordered_map<std::string_view, unsigned> nameToOutStopID;
		const std::map<unsigned, std::string_view> maxEdgeIDtoRouteName;
		const Router router;

	private:
		std::map<unsigned, std::string_view>
		InitGraphAndMaxEdgeIDtoRouteName(const RouteStats& routeStats, const TemporalInfo temporalInfo) {
			std::map<unsigned, std::string_view> result;
			unsigned inStopID = 0;
			for(auto &[routeName, routeData]: routeStats) {
				auto stops_it = routeData.stops.begin();
				const auto initialOutStopID = nameToOutStopID.at(*(stops_it++));
				auto prevInStopID = inStopID;
				graph.AddEdge({initialOutStopID, inStopID++, temporalInfo.waitTime});
				switch(routeData.type) {
					case RouteType::CIRCULAR:
						__InitInnerPartOfCircularRoute(
								{stops_it, std::prev(routeData.stops.end())},
								*routeData.dists,
								prevInStopID,
								inStopID,
								temporalInfo
						);
						break;
					default:
						__InitInnerPartOfLoopingRoute(
								{stops_it, routeData.stops.end()},
								*routeData.dists,
								prevInStopID,
								inStopID,
								temporalInfo,
								routeData.unique_stops.size() - 1
						);
				}
				graph.AddEdge({prevInStopID, inStopID, routeData.dists->back() / temporalInfo.velocity});
				routeData.dists.reset();
				result.emplace(
						graph.AddEdge({inStopID++, initialOutStopID, 0}),
						routeName
				);
			}
			return result;
		}

		void __InitInnerPartOfLoopingRoute(ItRange<std::vector<std::string>::const_iterator> inner_stops,
										   const std::vector<unsigned>& dists,
										   unsigned& prevInStopID, unsigned& inStopID, const TemporalInfo temporalInfo,
										   unsigned stops_num) {
			std::vector<unsigned> outStopIDs;
			outStopIDs.reserve(stops_num);
			auto dists_it = dists.begin();
			while(inner_stops.begin != inner_stops.end) {
				const auto outStopID = nameToOutStopID.at(*inner_stops.begin);
				outStopIDs.emplace_back(outStopID);
				graph.AddEdge({inStopID, outStopID, 0});
				graph.AddEdge({outStopID, inStopID, temporalInfo.waitTime});
				graph.AddEdge(
						{prevInStopID, inStopID, *(dists_it++) / temporalInfo.velocity}
				);
				prevInStopID = inStopID++;
				++inner_stops.begin;
			}
			const auto outStopID_rend = outStopIDs.rend();
			for(auto outStopID_it = std::next(outStopIDs.rbegin()); outStopID_it != outStopID_rend; ++outStopID_it) {
				graph.AddEdge({inStopID, *outStopID_it, 0});
				graph.AddEdge({*outStopID_it, inStopID, temporalInfo.waitTime});
				graph.AddEdge(
						{prevInStopID, inStopID, *(dists_it++) / temporalInfo.velocity}
				);
				prevInStopID = inStopID++;
			}
		}

		void __InitInnerPartOfCircularRoute(ItRange<std::vector<std::string>::const_iterator> inner_stops,
											const std::vector<unsigned>& dists,
											unsigned& prevInStopID, unsigned& inStopID,
											const TemporalInfo temporalInfo) {
			auto dists_it = dists.begin();
			while(inner_stops.begin != inner_stops.end) {
				const auto outStopID = nameToOutStopID.at(*inner_stops.begin);
				graph.AddEdge({inStopID, outStopID, 0});
				graph.AddEdge({outStopID, inStopID, temporalInfo.waitTime});
				graph.AddEdge(
						{prevInStopID, inStopID, *(dists_it++) / temporalInfo.velocity}
				);
				prevInStopID = inStopID++;
				++inner_stops.begin;
			}
		}

		static std::vector<std::string_view> InitOutStopIDtoStopName(const StopStats& stopStats) {
			std::vector<std::string_view> result;
			result.reserve(stopStats.size());
			for(auto &[stopName, _] : stopStats) {
				result.emplace_back(stopName);
			}
			std::sort(result.begin(), result.end());
			return result;
		}

		static std::unordered_map<std::string_view, unsigned>
		InitNameToOutStopID(const std::vector<std::string_view>& outStopIDtoName) {
			std::unordered_map<std::string_view, unsigned> result;
			const unsigned outStopsNum = outStopIDtoName.size();
			result.reserve(outStopsNum);
			for(unsigned i = 0, minOutStopID = inStopsNum; i != outStopsNum; ++i, ++minOutStopID) {
				result.emplace(outStopIDtoName[i], minOutStopID);
			}
			return result;
		}
	};

	std::istream& input;
	std::ostream& output;
	RouteStats route_stats;
	StopStats stop_stats;

	std::optional<GraphBuilder> graph = std::nullopt;
	TemporalInfo settings;
public:
	static unsigned inStopsNum;

	DataBase(std::istream& input, std::ostream& output) : input(input), output(output) {
		output.precision(6);
	}

	DataBase& FillDB() {
		unsigned n;
		input >> n;
		for(unsigned i = 0; i != n; ++i) {
			switch(ParseRequestName(input)) {
				case Request::Name::BUS:
					ProcessNewRoute(
							ParseRouteName(input, Request::RouteRequestType::NEW)
					);
					break;
				case Request::Name::STOP:
					ProcessNewBusStop(ParseNewStop(input));
				default:
					break;
			}
		}
		for(auto& route: route_stats) {
			route.second.InitDists(stop_stats);
		}
		return *this;
	}

	DataBase& FillDB(const nlohmann::json& requests) {
		ProcessSettings(requests["routing_settings"]);
		for(auto& request: requests["base_requests"]) {
			switch(requestNames.at(request["type"])) {
				case Request::Name::BUS:
					ProcessNewRoute(request);
					break;
				default:
					ProcessNewBusStop(request);
			}
		}
		for(auto& route: route_stats) {
			route.second.InitDists(stop_stats);
		}
		graph.emplace(GraphBuilder(route_stats, stop_stats, settings));
		return *this;
	}

	DataBase& ProcessStatQueries() {
		unsigned n;
		input >> n;
		for(unsigned i = 0; i != n; ++i) {
			std::string name;
			input >> name;
			switch(requestNames.at(name)) {
				case Request::Name::BUS:
					ProcessExistingRoute(ParseRouteName(input, Request::RouteRequestType::EXISTING));
					break;
				default:
					input.ignore();
					std::getline(input, name);
					ProcessExistingStop(name);
			}
		}
		return *this;
	}

	nlohmann::json ProcessStatQueries(const nlohmann::json& requests) {
		auto response = nlohmann::json::array();
		for(auto& request: requests) {
			switch(requestNames.at(request["type"])) {
				case Request::Name::BUS:
					response.emplace_back(ProcessExistingRoute(request));
					break;
				case Request::Name::STOP:
					response.emplace_back(ProcessExistingStop(request));
					break;
				default:
					response.emplace_back(BuildRoute(request));
			}
		}
		return response;
	}

	DataBase& ProcessAll() {
		FillDB();
		return ProcessStatQueries();
	}

	DataBase& ProcessJSON() {
		nlohmann::json json_input;
		input >> json_input;
		FillDB(json_input);
		output << ProcessStatQueries(json_input["stat_requests"]) << '\n';
		return *this;
	}

private:
	nlohmann::json BuildRoute(const nlohmann::json& request) {
		auto& nameToOutStopID = graph->nameToOutStopID;
		graph->router.BuildRoute(
				nameToOutStopID.at((std::string) request["from"]),
				nameToOutStopID.at((std::string) request["to"])
		);
		return {}; // FIXME
	}

	void ProcessNewRoute(const std::string& routeName) {
		route_stats.emplace(
				routeName,
				RouteData{ParseNewRoute(input)}
		);
	}

	void ProcessNewRoute(const nlohmann::json& request) {
		route_stats.emplace(
				request["name"],
				RouteData{
						RouteInfo{
								(request["is_roundtrip"]) ? RouteType::CIRCULAR : RouteType::LOOPING,
								request["stops"]
						}
				}
		);
	}

	void ProcessSettings(const nlohmann::json& request) {
		constexpr auto convertion_multiplier = 1000.0 / 60;
		settings.waitTime = request["bus_wait_time"];
		settings.velocity = (double) request["bus_velocity"] * convertion_multiplier;
	}

	void ProcessExistingRoute(const std::string& routeName) {
		output << "Bus " << routeName << ": ";
		const auto it = route_stats.find(routeName);
		if(it != route_stats.end()) {
			const auto& stats = it->second;
			output << stats.num_stops << " stops on route, "
				   << stats.unique_stops.size() << " unique stops, "
				   << stats.real_distance << " route length, "
				   << stats.real_distance / stats.geo_distance << " curvature\n";
		} else {
			output << "not found\n";
		}
	}

	nlohmann::json ProcessExistingRoute(const nlohmann::json& request) {
		const auto it = route_stats.find(request["name"]);
		if(it != route_stats.end()) {
			const auto& stats = it->second;
			return {
					{"request_id",        request["id"]},
					{"stop_count",        stats.num_stops},
					{"unique_stop_count", stats.unique_stops.size()},
					{"route_length",      stats.real_distance},
					{"curvature",         stats.real_distance / stats.geo_distance}
			};
		}
		return {
				{"request_id",    request["id"]},
				{"error_message", "not found"}
		};
	}

	void ProcessExistingStop(const std::string& stopName) {
		output << "Stop " << stopName << ": ";
		const auto it = stop_stats.find(stopName);
		if(it != stop_stats.end()) {
			const auto& route_set_proxy = it->second.route_set_proxy;
			if(!route_set_proxy) {
				it->second.InitProxy(stopName, route_stats);
			}
			if(!route_set_proxy->empty()) {
				output << "buses ";
				auto route_set_it = route_set_proxy->begin();
				const auto route_set_last = std::prev(route_set_proxy->end());
				while(route_set_it != route_set_last) {
					output << *(route_set_it++) << ' ';
				}
				output << *route_set_it << '\n';
			} else {
				output << "no buses\n";
			}
		} else {
			output << "not found\n";
		}
	}

	nlohmann::json ProcessExistingStop(const nlohmann::json& request) {
		const std::string& name = request["name"];
		const auto it = stop_stats.find(name);
		if(it != stop_stats.end()) {
			const auto& route_set_proxy = it->second.route_set_proxy;
			if(!route_set_proxy) {
				it->second.InitProxy(name, route_stats);
			}
			auto buses = nlohmann::json::array();
			for(auto& bus: *route_set_proxy) {
				buses.emplace_back(bus);
			}
			return {
					{"buses",      std::move(buses)},
					{"request_id", request["id"]}
			};
		}
		return {
				{"request_id",    request["id"]},
				{"error_message", "not found"}
		};
	}

	void ProcessNewBusStop(StopInfo stopInfo) {
		stop_stats.emplace(
				std::move(stopInfo.name),
				StopData{
						stopInfo.coordinates,
						std::nullopt,
						std::move(stopInfo.distances)
				}
		);
	}

	void ProcessNewBusStop(const nlohmann::json& request) {
		stop_stats.emplace(
				request["name"],
				StopData{
						Coordinates{request["latitude"], request["longitude"]},
						std::nullopt,
						request["road_distances"]
				}
		);
	}
};

unsigned DataBase::inStopsNum = 0;

namespace Testing {
	namespace Unit {
		void ParseRequestName() {
			std::string remaining;
			using Name = Request::Name;
			{
				std::istringstream ss("Stop efwwe\nSomething else");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::STOP)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " efwwe\nSomething else")
			}
			{
				std::istringstream ss("Stop efwwe");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::STOP)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " efwwe")
			}
			{
				std::istringstream ss("Stop\nSmth");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::STOP)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, "\nSmth")
			}
			{
				std::istringstream ss("Stop");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::STOP)
				remaining.clear();
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, "")
			}
			{
				std::istringstream ss("Stop 3 3\n");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::STOP)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " 3 3\n")
			}
			{
				std::istringstream ss("Stop 3 3");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::STOP)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " 3 3")
			}
			{
				std::istringstream ss("Bus efjofweoj\nff");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " efjofweoj\nff")
			}
			{
				std::istringstream ss("Bus efjofweoj\n");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " efjofweoj\n")
			}
			{
				std::istringstream ss("Bus\n");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, "\n")
			}
			{
				std::istringstream ss("Bus");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				remaining.clear();
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, "")
			}
			{
				std::istringstream ss("Bus \n");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " \n")
			}
			{
				std::istringstream ss("Bus 23 wqe\n");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " 23 wqe\n")
			}
			{
				std::istringstream ss("Bus 23 wq");
				ASSERT_EQUAL(::ParseRequestName(ss), Name::BUS)
				std::getline(ss, remaining, (char) EOF);
				ASSERT_EQUAL(remaining, " 23 wq")
			}
			std::cerr << "\t\tParseRequestName test passed" << std::endl;
		}

		void ParseRoute() {
			{
				std::istringstream ss("Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka");
				ASSERT_EQUAL(::ParseRequestName(ss), Request::Name::BUS)
				ASSERT_EQUAL(::ParseRouteNameAndType(ss), (RouteRequestInfo{"750", Request::RouteRequestType::NEW}))
				ASSERT_EQUAL(
						::ParseNewRoute(ss),
						(RouteInfo{RouteType::LOOPING, {"Tolstopaltsevo", "Marushkino", "Rasskazovka"}})
				)
			}
			{
				std::istringstream ss(
						"Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > "
						"Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"
				);
				ASSERT_EQUAL(::ParseRequestName(ss), Request::Name::BUS)
				ASSERT_EQUAL(::ParseRouteNameAndType(ss), (RouteRequestInfo{"256", Request::RouteRequestType::NEW}))
				ASSERT_EQUAL(
						::ParseNewRoute(ss),
						(RouteInfo{
								RouteType::CIRCULAR,
								{
										"Biryulyovo Zapadnoye", "Biryusinka", "Universam", "Biryulyovo Tovarnaya",
										"Biryulyovo Passazhirskaya", "Biryulyovo Zapadnoye"
								}
						})
				)
			}
			{
				std::istringstream ss("Bus 342423");
				ASSERT_EQUAL(::ParseRequestName(ss), Request::Name::BUS)
				ASSERT_EQUAL(::ParseRouteNameAndType(ss),
							 (RouteRequestInfo{"342423", Request::RouteRequestType::EXISTING}))
			}
			std::cerr << "\t\tParseRoute test passed" << std::endl;
		}

		void ParseNewBusStop() {
			{
				std::istringstream ss("Stop Rasskazovka: 55.632761, 37.333324");
				ASSERT_EQUAL(::ParseRequestName(ss), Request::Name::STOP)
				ASSERT_EQUAL(::ParseNewStop(ss), (StopInfo{"Rasskazovka", {55.632761, 37.333324}}))
			}
			{
				std::istringstream ss("Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164");
				ASSERT_EQUAL(::ParseRequestName(ss), Request::Name::STOP)
				ASSERT_EQUAL(::ParseNewStop(ss), (StopInfo{"Biryulyovo Passazhirskaya", {55.580999, 37.659164}}))
			}
			std::cerr << "\t\tParseNewBusStop test passed" << std::endl;
		}

		void TestAll() {
			std::cerr << "\tUnit tests:" << std::endl;
			ParseRequestName();
			ParseRoute();
			ParseNewBusStop();
			std::cerr << "\tAll unit tests passed!" << std::endl;
		}
	}
	namespace Integration {
		void TestDB() {
			if constexpr (VERSION < 3) {
				{
					std::istringstream input(
							"10\n"
							"Stop Tolstopaltsevo: 55.611087, 37.20829\n"
							"Stop Marushkino: 55.595884, 37.209755\n"
							"Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
							"Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
							"Stop Rasskazovka: 55.632761, 37.333324\n"
							"Stop Biryulyovo Zapadnoye: 55.574371, 37.6517\n"
							"Stop Biryusinka: 55.581065, 37.64839\n"
							"Stop Universam: 55.587655, 37.645687\n"
							"Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n"
							"Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164\n"
							"3\n"
							"Bus 256\n"
							"Bus 750\n"
							"Bus 751"
					);
					std::ostringstream output;
					DataBase(input, output).ProcessAll();
					ASSERT_EQUAL(
							output.str(),
							"Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length\n"
							"Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length\n"
							"Bus 751: not found\n"
					)
				}
				{
					std::istringstream input("5\n"
											 "Bus gazZviu ncDtm: C06g9m0rff9 > 9CBIn uqPIOjG > C06g9m0rff9\n"
											 "Stop C06g9m0rff9: 43.626432, 39.917507\n"
											 "Bus lw5PH6 Ul3: C06g9m0rff9 - 9CBIn uqPIOjG\n"
											 "Bus wUZPglHbh: C06g9m0rff9 - 9CBIn uqPIOjG\n"
											 "Stop 9CBIn uqPIOjG: 43.634987, 39.813051\n"
											 "10\n"
											 "Bus wUZPglHbh\n"
											 "Bus lw5PH6 Ul3\n"
											 "Bus gazZviu ncDtm\n"
											 "Bus gazZviu ncDtm\n"
											 "Bus lw5PH6 Ul3\n"
											 "Bus wUZPglHbh\n"
											 "Bus gazZviu ncDtm\n"
											 "Bus gazZviu ncDtm\n"
											 "Bus MYb6ycMmNaxXe\n"
											 "Bus wUZPglHbh");
					std::ostringstream output;
					DataBase(input, output).ProcessAll();
					ASSERT_EQUAL(
							output.str(),
							"Bus wUZPglHbh: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus lw5PH6 Ul3: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus gazZviu ncDtm: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus gazZviu ncDtm: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus lw5PH6 Ul3: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus wUZPglHbh: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus gazZviu ncDtm: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus gazZviu ncDtm: 3 stops on route, 2 unique stops, 16921.2 route length\n"
							"Bus MYb6ycMmNaxXe: not found\n"
							"Bus wUZPglHbh: 3 stops on route, 2 unique stops, 16921.2 route length\n"
					)
				}
				{
					std::istringstream input("13\n"
											 "Stop Tolstopaltsevo: 55.611087, 37.20829\n"
											 "Stop Marushkino: 55.595884, 37.209755\n"
											 "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
											 "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
											 "Stop Rasskazovka: 55.632761, 37.333324\n"
											 "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517\n"
											 "Stop Biryusinka: 55.581065, 37.64839\n"
											 "Stop Universam: 55.587655, 37.645687\n"
											 "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n"
											 "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164\n"
											 "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
											 "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
											 "Stop Prazhskaya: 55.611678, 37.603831\n"
											 "6\n"
											 "Bus 256\n"
											 "Bus 750\n"
											 "Bus 751\n"
											 "Stop Samara\n"
											 "Stop Prazhskaya\n"
											 "Stop Biryulyovo Zapadnoye");
					std::ostringstream output;
					DataBase(input, output).ProcessAll();
					ASSERT_EQUAL(
							output.str(),
							"Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length\n"
							"Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length\n"
							"Bus 751: not found\n"
							"Stop Samara: not found\n"
							"Stop Prazhskaya: no buses\n"
							"Stop Biryulyovo Zapadnoye: buses 256 828\n"
					)
				}
			}

			if constexpr (VERSION > 2 && VERSION < 5) {
				{
					DataBase::inStopsNum = 0;
					std::istringstream input("13\n"
											 "Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino\n"
											 "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka\n"
											 "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
											 "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
											 "Stop Rasskazovka: 55.632761, 37.333324\n"
											 "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam\n"
											 "Stop Biryusinka: 55.581065, 37.64839, 750m to Universam\n"
											 "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya\n"
											 "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya\n"
											 "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye\n"
											 "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
											 "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
											 "Stop Prazhskaya: 55.611678, 37.603831\n"
											 "6\n"
											 "Bus 256\n"
											 "Bus 750\n"
											 "Bus 751\n"
											 "Stop Samara\n"
											 "Stop Prazhskaya\n"
											 "Stop Biryulyovo Zapadnoye");
					std::ostringstream output;
					DataBase(input, output).ProcessAll();
					ASSERT_EQUAL(
							output.str(),
							"Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature\n"
							"Bus 750: 5 stops on route, 3 unique stops, 27600 route length, 1.31808 curvature\n"
							"Bus 751: not found\n"
							"Stop Samara: not found\n"
							"Stop Prazhskaya: no buses\n"
							"Stop Biryulyovo Zapadnoye: buses 256 828\n"
					)
				}
			}
			if constexpr (VERSION == 4) {
				DataBase::inStopsNum = 0;
				std::istringstream input("{\n"
										 "  \"base_requests\": [\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Marushkino\": 3900\n"
										 "      },\n"
										 "      \"longitude\": 37.20829,\n"
										 "      \"name\": \"Tolstopaltsevo\",\n"
										 "      \"latitude\": 55.611087\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Rasskazovka\": 9900\n"
										 "      },\n"
										 "      \"longitude\": 37.209755,\n"
										 "      \"name\": \"Marushkino\",\n"
										 "      \"latitude\": 55.595884\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Bus\",\n"
										 "      \"name\": \"256\",\n"
										 "      \"stops\": [\n"
										 "        \"Biryulyovo Zapadnoye\",\n"
										 "        \"Biryusinka\",\n"
										 "        \"Universam\",\n"
										 "        \"Biryulyovo Tovarnaya\",\n"
										 "        \"Biryulyovo Passazhirskaya\",\n"
										 "        \"Biryulyovo Zapadnoye\"\n"
										 "      ],\n"
										 "      \"is_roundtrip\": true\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Bus\",\n"
										 "      \"name\": \"750\",\n"
										 "      \"stops\": [\n"
										 "        \"Tolstopaltsevo\",\n"
										 "        \"Marushkino\",\n"
										 "        \"Rasskazovka\"\n"
										 "      ],\n"
										 "      \"is_roundtrip\": false\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {},\n"
										 "      \"longitude\": 37.333324,\n"
										 "      \"name\": \"Rasskazovka\",\n"
										 "      \"latitude\": 55.632761\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Rossoshanskaya ulitsa\": 7500,\n"
										 "        \"Biryusinka\": 1800,\n"
										 "        \"Universam\": 2400\n"
										 "      },\n"
										 "      \"longitude\": 37.6517,\n"
										 "      \"name\": \"Biryulyovo Zapadnoye\",\n"
										 "      \"latitude\": 55.574371\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Universam\": 750\n"
										 "      },\n"
										 "      \"longitude\": 37.64839,\n"
										 "      \"name\": \"Biryusinka\",\n"
										 "      \"latitude\": 55.581065\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Rossoshanskaya ulitsa\": 5600,\n"
										 "        \"Biryulyovo Tovarnaya\": 900\n"
										 "      },\n"
										 "      \"longitude\": 37.645687,\n"
										 "      \"name\": \"Universam\",\n"
										 "      \"latitude\": 55.587655\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Biryulyovo Passazhirskaya\": 1300\n"
										 "      },\n"
										 "      \"longitude\": 37.653656,\n"
										 "      \"name\": \"Biryulyovo Tovarnaya\",\n"
										 "      \"latitude\": 55.592028\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {\n"
										 "        \"Biryulyovo Zapadnoye\": 1200\n"
										 "      },\n"
										 "      \"longitude\": 37.659164,\n"
										 "      \"name\": \"Biryulyovo Passazhirskaya\",\n"
										 "      \"latitude\": 55.580999\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Bus\",\n"
										 "      \"name\": \"828\",\n"
										 "      \"stops\": [\n"
										 "        \"Biryulyovo Zapadnoye\",\n"
										 "        \"Universam\",\n"
										 "        \"Rossoshanskaya ulitsa\",\n"
										 "        \"Biryulyovo Zapadnoye\"\n"
										 "      ],\n"
										 "      \"is_roundtrip\": true\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {},\n"
										 "      \"longitude\": 37.605757,\n"
										 "      \"name\": \"Rossoshanskaya ulitsa\",\n"
										 "      \"latitude\": 55.595579\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"road_distances\": {},\n"
										 "      \"longitude\": 37.603831,\n"
										 "      \"name\": \"Prazhskaya\",\n"
										 "      \"latitude\": 55.611678\n"
										 "    }\n"
										 "  ],\n"
										 "  \"stat_requests\": [\n"
										 "    {\n"
										 "      \"type\": \"Bus\",\n"
										 "      \"name\": \"256\",\n"
										 "      \"id\": 1965312327\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Bus\",\n"
										 "      \"name\": \"750\",\n"
										 "      \"id\": 519139350\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Bus\",\n"
										 "      \"name\": \"751\",\n"
										 "      \"id\": 194217464\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"name\": \"Samara\",\n"
										 "      \"id\": 746888088\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"name\": \"Prazhskaya\",\n"
										 "      \"id\": 65100610\n"
										 "    },\n"
										 "    {\n"
										 "      \"type\": \"Stop\",\n"
										 "      \"name\": \"Biryulyovo Zapadnoye\",\n"
										 "      \"id\": 1042838872\n"
										 "    }\n"
										 "  ]\n"
										 "}");
				std::ostringstream output;
				DataBase(input, output).ProcessJSON();
				ASSERT_EQUAL(
						output.str(),
						"[{\"curvature\":1.3612391928160086,\"request_id\":1965312327,\"route_length\":5950,\"stop_count\":6,\"unique_stop_count\":5},{\"curvature\":1.3180841159439354,\"request_id\":519139350,\"route_length\":27600,\"stop_count\":5,\"unique_stop_count\":3},{\"error_message\":\"not found\",\"request_id\":194217464},{\"error_message\":\"not found\",\"request_id\":746888088},{\"buses\":[],\"request_id\":65100610},{\"buses\":[\"256\",\"828\"],\"request_id\":1042838872}]\n"
				)
			}
			std::cerr << "\t\tTestDB passed" << std::endl;
		}

		void TestAll() {
			std::cerr << "\tIntegration tests:" << std::endl;
			TestDB();
			std::cerr << "\tAll integration tests passed!" << std::endl;
		}
	}

	void TestAll() {
		std::cerr << "Testing begins:\n" << std::endl;
		Unit::TestAll();
		std::cerr << '\n';
		Integration::TestAll();
		std::cerr << "\nAll tests passed!" << std::endl;
	}
}

//class C {
//private:
//	int a;
//	float b;
//	double c;
//	void* pa;
//	int* p2;
//public:
//	void f1() {};
//
//	void f2() {};
//
//	virtual void f3() {};
//
//	virtual void f4() {};
//
//	virtual void f5() {};
//};
//#include <iostream>

//class A {
//public:
//	A() {
//		std::cout << "A";
//		CallFunc();
//	}
//
//	virtual ~A() { std::cout << "~A"; }
//
//	void CallFunc() { Func(); }
//
//	virtual void Func() = 0;
//};
//
//class B : public A {
//public:
//	B() {
//		std::cout << "B";
//		throw new int;
//	}
//
//	~B() { std::cout << "~B"; }
//
//	virtual void Func() override { std::cout << "F"; }
//
//	void doSomething() {}
//};

//class Worker {
//public:
//	int a;
//
//	Worker(): a(0) {}
//
//	void DoSomething() {
//		int b = 1; int c =2;
//
//		auto func = [this, b, &c]() {
//			c += a + b;
//		};
//
//		a++;
//		b++;
//		c++;
//		func();
//
//		std::cout << c;
//	}
//};

int main() {
	Testing::TestAll();
	DataBase(std::cin, std::cout).ProcessJSON();

//	int ptr1[1] = {10};
//	int* ptr[1];
//	ptr[0] = &ptr1[0];
//	int[] pInt = new int[25];
//	delete pData;

//	std::cout << sizeof(C) << std::endl;
//	std::cout << sizeof(int) << std::endl;
//	std::cout << sizeof(float) << std::endl;
//	std::cout << sizeof(double) << std::endl;
//	std::cout << sizeof(void*) << std::endl;
//	std::cout << sizeof(int*) << std::endl;
//	std::cout << sizeof(double*) << std::endl;

//	throw new int;

//	try {
//		B b;
//		b.doSomething();
//	} catch (...){
//		std::cout << "E";
//	}

//	Worker w;
//	w.DoSomething();

	return 0;
}