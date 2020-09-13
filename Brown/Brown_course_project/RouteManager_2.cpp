#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_set>
#include <set>
#include <cmath>
#include <memory>
#include <iterator>

#include "my_json.h"
#include "graph.h"
#include "router.h"

using namespace std;

struct RoutingSettings {
	double bus_wait_time = 0;
	double bus_velocity = 0;
};

struct Stop {
	Stop(string stop_name, double stop_latitude, double stop_longitude)
			: name(move(stop_name)), longitude(stop_longitude), latitude(stop_latitude) {}

	string name;
	double latitude;
	double longitude;
	size_t id = 0;
};

struct Transition {
	Transition(size_t from, size_t to) : from_stop_id(from), to_stop_id(to) {}

	bool operator==(const Transition& rhs) const {
		return this->from_stop_id == rhs.from_stop_id && this->to_stop_id == rhs.to_stop_id;
	}

	size_t from_stop_id;
	size_t to_stop_id;
	double distance = 0;
	double straight_distance = 0;
	set<size_t> usages;
};

class TransitionHash {
public:
	size_t operator()(const Transition& transition) const {
		return transition.from_stop_id * PRIME + transition.to_stop_id;
	}

private:
	const size_t PRIME = 70'368'760'954'879;
};

struct Bus {
	Bus(string bus_name, bool is_bus_cycled) : name(move(bus_name)), is_cycled(is_bus_cycled) {}

	string name;
	bool is_cycled;
	vector<shared_ptr<Transition>> transitions;
	size_t id = 0;

	double route_length = 0;
	double straight_route_length = 0;
};

class InfoHolder {
public:
	void AddStopToBus(size_t stop, size_t bus) {
		bus_to_stop[bus].insert(stop);
	}

	void AddBusToStop(size_t bus, size_t stop) {
		stop_to_bus[stop].insert(bus);
	}

	[[nodiscard]] const set<size_t>& getStopsForBus(size_t bus) const {
		return bus_to_stop.at(bus);
	}

	[[nodiscard]] const set<size_t>& getBusesForStop(size_t stop) const {
		if(stop_to_bus.count(stop) == 0) {
			return empty_set;
		}
		return stop_to_bus.at(stop);
	}

private:
	set<size_t> empty_set; // Temporary solution
	map<size_t, set<size_t>> bus_to_stop;
	map<size_t, set<size_t>> stop_to_bus;
};

class DistanceHolder {
public:
	void AddDistance(size_t from_id, size_t to_id, double distance) {
		distances[{from_id, to_id}] = distance;
		if(distances.count({to_id, from_id}) == 0) {
			distances[{to_id, from_id}] = distance;
		}
	}

	[[nodiscard]] double GetDistance(size_t from_id, size_t to_id) const {
		return distances.at({from_id, to_id});
	}

	static double CalculateStraightDistance(const Stop& first, const Stop& second) {
		const double R = 6371000;
		const double Pi = 3.1415926535 / 180;

		return R * acos(cos(first.latitude * Pi) * cos(second.latitude * Pi) *
						cos((first.longitude - second.longitude) * Pi) +
						sin(first.latitude * Pi) * sin(second.latitude * Pi));
	}

private:
	map<pair<size_t, size_t>, double> distances;
};

class RouteManager {
public:
	void BuildManager() {
		CalculateDistances();
		BuildGraphAndRouter();
	}

	void AddStop(string name, double latitude, double longitude) {
		if(all_stops_by_name.count(name) == 0) {
			AddStopIfNotExist(move(name), latitude, longitude);
		} else {
			all_stops_by_name[name]->latitude = latitude;
			all_stops_by_name[name]->longitude = longitude;
		}
	}

	void AddDistance(string_view from, string_view to, double distance) {
		if(all_stops_by_name.count(to) == 0) {
			AddStopIfNotExist(string(to), 0, 0);
		}
		if(all_stops_by_name.count(from) == 0) {
			AddStopIfNotExist(string(from), 0, 0);
		}
		distance_holder.AddDistance(all_stops_by_name[from]->id, all_stops_by_name[to]->id, distance);
	}

	void AddBus(string name, bool is_cycled, vector<string>& stops) {
		shared_ptr<Bus> bus = make_shared<Bus>(move(name), is_cycled);
		bus->id = current_bus_id;

		AddStopIfNotExist(stops[0], 0, 0);

		info_holder.AddStopToBus(all_stops_by_name[stops[0]]->id, bus->id);
		info_holder.AddBusToStop(bus->id, all_stops_by_name[stops[0]]->id);

		for(size_t i = 1; i < stops.size(); ++i) {
			AddStopIfNotExist(stops[i], 0, 0);
			pair<size_t, size_t> id_pair = {all_stops_by_name[stops[i - 1]]->id, all_stops_by_name[stops[i]]->id};

			info_holder.AddStopToBus(id_pair.second, bus->id);
			info_holder.AddBusToStop(bus->id, id_pair.second);

			if(transitions_by_pair.count(id_pair) == 0) {
				shared_ptr<Transition> transition = make_shared<Transition>(id_pair.first, id_pair.second);
				transitions_by_pair[id_pair] = transition;
			}
			transitions_by_pair[id_pair]->usages.insert(bus->id);
			bus->transitions.push_back(transitions_by_pair[id_pair]);
		}
		if(!is_cycled) {
			for(int i = stops.size() - 2; i >= 0; --i) {
				pair<size_t, size_t> id_pair = {all_stops_by_name[stops[i + 1]]->id, all_stops_by_name[stops[i]]->id};
				if(transitions_by_pair.count(id_pair) == 0) {
					shared_ptr<Transition> transition = make_shared<Transition>(id_pair.first, id_pair.second);
					transitions_by_pair[id_pair] = transition;
				}
				bus->transitions.push_back(transitions_by_pair[id_pair]);
			}
		}

		all_buses_by_name[bus->name] = bus;
		all_buses_by_id[current_bus_id++] = bus;
	}

	void AddRoutingSettings(double waiting_time, double bus_velocity) {
		// converting to meters per second
		routing_settings.bus_velocity = bus_velocity * 16.666;
		routing_settings.bus_wait_time = waiting_time;
	}

	struct BusResponse {
		double curvature = 0;
		size_t unique_stop_count = 0;
		size_t stop_count = 0;
		long long request_id = 0;
		double route_length = 0;
	};

	struct StopResponse {
		size_t request_id = 0;
		set<string_view> buses;
	};

	enum ItemType {
		WAIT, BUS
	};

	struct Item {
		Item(ItemType item_type, double wait) : type(item_type), time(wait) {}

		ItemType type;
		double time;
	};

	struct WaitItem : public Item {
		WaitItem(string stop, double wait) : Item(ItemType::WAIT, wait), stop_name(move(stop)) {}

		string stop_name;
	};

	struct BusItem : public Item {
		BusItem(string bus_name, double time_on_route, size_t span) : Item(ItemType::BUS, time_on_route),
																	  span_count(span),
																	  bus(move(bus_name)) {}

		string bus;
		size_t span_count;
	};

	struct RouteResponse {
		size_t request_id = 0;
		double total_time = 0;
		vector<shared_ptr<Item>> items;
	};

	[[nodiscard]] optional<BusResponse> BuildBusResponse(size_t request_id, const string& bus_name) const {
		if(all_buses_by_name.count(bus_name) == 0) {
			return nullopt;
		}
		BusResponse bus_response;
		shared_ptr<Bus> selected_bus = all_buses_by_name.at(bus_name);
		bus_response.route_length = selected_bus->route_length;
		bus_response.curvature = selected_bus->route_length / selected_bus->straight_route_length;
		bus_response.request_id = request_id;
		bus_response.stop_count = selected_bus->transitions.size() + 1;
		bus_response.unique_stop_count = info_holder.getStopsForBus(selected_bus->id).size();
		return bus_response;
	}

	[[nodiscard]] optional<StopResponse> BuildStopResponse(size_t request_id, const string& stop_name) const {
		if(all_stops_by_name.count(stop_name) == 0) {
			return nullopt;
		}
		StopResponse stop_response;
		shared_ptr<Stop> selected_stop = all_stops_by_name.at(stop_name);
		stop_response.request_id = request_id;
		for(const auto& i : info_holder.getBusesForStop(selected_stop->id)) {
			stop_response.buses.insert(all_buses_by_id.at(i)->name);
		}
		return stop_response;
	}

	[[nodiscard]] optional<RouteResponse> BuildRouteResponse(size_t request_id, const string& from, const string& to) const {
		RouteResponse route_response;
		route_response.request_id = request_id;
		auto route_info = router->BuildRoute(
				all_stops_by_name.at(from)->id, all_stops_by_name.at(to)->id);
		if(!route_info.has_value()) {
			return nullopt;
		}

		double total_time = 0;
		for(size_t i = 0; i < route_info.value().edge_count; ++i) {
			auto edge_id = router->GetRouteEdge(route_info.value().id, i);
			const auto& edge_ptr = edges_by_id.at(edge_id);
			total_time += edge_ptr->total_time;

			route_response.items.push_back(make_shared<WaitItem>(
					all_stops_by_id.at(edge_ptr->from_stop_id)->name, routing_settings.bus_wait_time
			));

			route_response.items.push_back(make_shared<BusItem>(
					all_buses_by_id.at(edge_ptr->bus_id)->name, edge_ptr->total_time - routing_settings.bus_wait_time,
					edge_ptr->span
			));
		}
		route_response.total_time = total_time;
		return route_response;
	}

private:
	void CalculateDistances() {
		for(auto& i : transitions_by_pair) {
			i.second->distance = distance_holder.GetDistance(i.second->from_stop_id, i.second->to_stop_id);
			i.second->straight_distance = DistanceHolder::CalculateStraightDistance(
					*all_stops_by_id[i.second->from_stop_id], *all_stops_by_id[i.second->to_stop_id]);
		}

		for(auto& i : all_buses_by_id) {
			for(auto& j : i.second->transitions) {
				i.second->straight_route_length += j->straight_distance;
				i.second->route_length += j->distance;
			}
		}
	}

	struct GraphEdge {
		GraphEdge(size_t from_stop, size_t to_stop) : from_stop_id(from_stop), to_stop_id(to_stop) {}

		size_t id = 0;
		size_t from_stop_id;
		size_t to_stop_id;
		size_t bus_id;
		double total_time = 0;
		size_t span = 0;
	};

	void BuildGraphAndRouter() {
		// Building graph
		graph = make_unique<Graph::DirectedWeightedGraph<double>>(all_stops_by_name.size());
		for(const auto& i : all_buses_by_id) {
			for(size_t j = 0; j < i.second->transitions.size(); ++j) {
				AddEdgesToGraph(i.second->transitions.begin() + j, i.second->transitions.end(), i.second->id);
			}
		}

		// Building router
		router = make_unique<Graph::Router<double>>(*graph);
	}

	void AddEdgesToGraph(vector<shared_ptr<Transition>>::const_iterator from_,
						 vector<shared_ptr<Transition>>::const_iterator to_, size_t bus_id) {
		double time_for_stop = routing_settings.bus_wait_time;
		for(auto i = from_; i != to_; ++i) {
			time_for_stop += i->get()->distance / routing_settings.bus_velocity;
			shared_ptr<GraphEdge> edge = make_shared<GraphEdge>(from_->get()->from_stop_id, i->get()->to_stop_id);
			edge->total_time = time_for_stop;
			edge->span = i - from_ + 1;
			edge->bus_id = bus_id;
			edge->id = graph->AddEdge(Graph::Edge<double>{edge->from_stop_id, edge->to_stop_id, edge->total_time});
			edges_by_id[edge->id] = edge;
		}
	}

	void AddStopIfNotExist(string name, double latitude, double longitude) {
		if(all_stops_by_name.count(name) == 0) {
			shared_ptr<Stop> stop = make_shared<Stop>(move(name), latitude, longitude);
			stop->id = current_stop_id;
			all_stops_by_id[current_stop_id++] = stop;
			all_stops_by_name[stop->name] = stop;
		}
	}

	size_t current_stop_id = 0;
	size_t current_bus_id = 0;

	InfoHolder info_holder;
	DistanceHolder distance_holder;
	RoutingSettings routing_settings;

	map<pair<size_t, size_t>, shared_ptr<Transition>> transitions_by_pair;
	map<string_view, shared_ptr<Stop>> all_stops_by_name;
	map<size_t, shared_ptr<Stop>> all_stops_by_id;
	map<string_view, shared_ptr<Bus>> all_buses_by_name;
	map<size_t, shared_ptr<Bus>> all_buses_by_id;
	map<size_t, shared_ptr<GraphEdge>> edges_by_id;

	unique_ptr<Graph::DirectedWeightedGraph<double>> graph;
	unique_ptr<Graph::Router<double>> router;
};

namespace CommandReader {

	void ParseStopFromJson(RouteManager& route_manager, const map<string, Json::Node>& stop_info) {
		string stop_name = stop_info.at("name").AsString();
		double latitude = stop_info.at("latitude").AsDouble();
		double longitude = stop_info.at("longitude").AsDouble();
		route_manager.AddStop(stop_name, latitude, longitude);

		for(const auto& i : stop_info.at("road_distances").AsMap()) {
			string to_stop = i.first;
			double distance = i.second.AsDouble();
			route_manager.AddDistance(stop_name, to_stop, distance);
		}
	}

	void ParseBusFromJson(RouteManager& route_manager, const map<string, Json::Node>& bus_info) {
		string bus_name = bus_info.at("name").AsString();
		bool is_cycled = bus_info.at("is_roundtrip").AsBool();
		vector<string> stops;
		stops.reserve(bus_info.at("stops").AsArray().size());

		for(auto& i : bus_info.at("stops").AsArray()) {
			stops.push_back(i.AsString());
		}
		route_manager.AddBus(bus_name, is_cycled, stops);
	}

	void
	ParseStopRequestAndWriteResponse(const RouteManager& route_manager, const map<string, Json::Node>& stop_request,
									 ostream& output) {
		optional<RouteManager::StopResponse> stop_response = route_manager.BuildStopResponse(
				(size_t) stop_request.at("id").AsDouble(),
				stop_request.at("name").AsString());

		if(stop_response.has_value()) {
			output << "{\n\"request_id\": " << stop_response.value().request_id << ",\n"
				   << "\"buses\": [";
			for(auto i = stop_response.value().buses.begin(); i != stop_response.value().buses.end(); ++i) {
				output << "\"" << *i << "\"";
				if(next(i) != stop_response.value().buses.end()) {
					output << ',';
				}
				output << '\n';
			}
			output << "]\n}\n";
		} else {
			output << "{\n\"request_id\": " << (size_t) stop_request.at("id").AsDouble() << ",\n"
				   << "\"error_message\": \"not found\"\n}\n";
		}
	}

	void ParseBusRequestAndWriteResponse(const RouteManager& route_manager, const map<string, Json::Node>& bus_request,
										 ostream& output) {
		optional<RouteManager::BusResponse> bus_response = route_manager.BuildBusResponse(
				(size_t) bus_request.at("id").AsDouble(),
				bus_request.at("name").AsString());

		if(bus_response.has_value()) {
			output << "{\n\"request_id\": " << bus_response.value().request_id << ",\n"
				   << "\"route_length\": " << bus_response.value().route_length << ",\n"
				   << "\"curvature\": " << bus_response.value().curvature << ",\n"
				   << "\"stop_count\": " << bus_response.value().stop_count << ",\n"
				   << "\"unique_stop_count\": " << bus_response.value().unique_stop_count << "\n}\n";
		} else {
			output << "{\n\"request_id\": " << (size_t) bus_request.at("id").AsDouble() << ",\n"
				   << "\"error_message\": \"not found\"\n}\n";
		}
	}

	void
	ParseRouteRequestAndWriteResponse(const RouteManager& route_manager, const map<string, Json::Node>& route_request,
									  ostream& output) {
		optional<RouteManager::RouteResponse> route_response = route_manager.BuildRouteResponse(
				(size_t) route_request.at("id").AsDouble(),
				route_request.at("from").AsString(),
				route_request.at("to").AsString());

		if(route_response.has_value()) {
			output << "{\n\"request_id\": " << route_response.value().request_id << ",\n"
				   << "\"total_time\": " << route_response.value().total_time << ",\n"
				   << "\"items\": [";
			for(auto i = route_response.value().items.begin(); i != route_response.value().items.end(); ++i) {
				output << "{\n\"type\": ";
				if(i->get()->type == RouteManager::ItemType::BUS) {
					output << "\"Bus\",\n";
					auto bus = static_pointer_cast<RouteManager::BusItem>(*i);
					output << "\"bus\": \"" << bus->bus << "\",\n";
					output << "\"span_count\": " << bus->span_count << ",\n";
					output << "\"time\": " << bus->time << "\n}";
				} else {
					output << "\"Wait\",\n";
					auto wait = static_pointer_cast<RouteManager::WaitItem>(*i);
					output << "\"stop_name\": \"" << wait->stop_name << "\",\n";
					output << "\"time\": " << wait->time << "\n}";
				}
				if(next(i) != route_response.value().items.end()) {
					output << ',';
				}
				output << '\n';
			}
			output << "]\n}\n";
		} else {
			output << "{\n\"request_id\": " << (size_t) route_request.at("id").AsDouble() << ",\n"
				   << "\"error_message\": \"not found\"\n}\n";
		}
	}

	void ReadAndWriteJson(RouteManager& route_manager, istream& input, ostream& output) {
		using namespace Json;

		// Reading and parsing
		Document document = Load(input);
		const Node& root = document.GetRoot();

		auto& request_map = root.AsMap();
		auto& base_requests = request_map.at("base_requests").AsArray();
		auto& stat_requests = request_map.at("stat_requests").AsArray();
		auto& routing_settings = request_map.at("routing_settings").AsMap();

		route_manager.AddRoutingSettings(
				routing_settings.at("bus_wait_time").AsDouble(), routing_settings.at("bus_velocity").AsDouble());

		for(auto& i : base_requests) {
			if(i.AsMap().at("type").AsString() == "Stop") {
				ParseStopFromJson(route_manager, i.AsMap());
			} else {
				ParseBusFromJson(route_manager, i.AsMap());
			}
		}

		route_manager.BuildManager();
		// Writing response

		output << '[' << '\n';
		for(auto i = stat_requests.begin(); i != stat_requests.end(); i++) {
			if(i->AsMap().at("type").AsString() == "Stop") {
				ParseStopRequestAndWriteResponse(route_manager, i->AsMap(), output);
			} else if(i->AsMap().at("type").AsString() == "Route") {
				ParseRouteRequestAndWriteResponse(route_manager, i->AsMap(), output);
			} else {
				ParseBusRequestAndWriteResponse(route_manager, i->AsMap(), output); // added response to route command
			}
			if(stat_requests.begin() != stat_requests.end() && i != prev(stat_requests.end())) {
				output << ',';
			}
			output << '\n';
		}
		output << ']';
	}

}

int main() {
    ifstream input("input.txt");
    ofstream output("output.txt");
    RouteManager route_manager;

    CommandReader::ReadAndWriteJson(route_manager, cin, cout);

}