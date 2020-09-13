#include <iostream>
#include <queue>
#include <map>

using namespace std;

struct Booking {
	int64_t time;
	string hotel_name;
	int client_id;
	int room_count;
};

class BookingSystem {
public:
	BookingSystem() : last_booking_time(INT64_MIN) {}

	void Book(const string& hotel_name, const Booking& booking) {
		last_booking_time = booking.time;
		all_bookings.push(booking);

		if (booked_rooms.count(hotel_name) == 0) {
			booked_rooms[hotel_name] = booking.room_count;
		} else {
			booked_rooms[hotel_name] += booking.room_count;
		}

		if (hotel_clients.count(hotel_name) == 0
			|| (hotel_clients.count(hotel_name) != 0
				&& hotel_clients[hotel_name].count(booking.client_id) == 0)) {
			hotel_clients[hotel_name][booking.client_id] = 1;
		} else {
			++hotel_clients[hotel_name][booking.client_id];
		}

		Adjust();
	}

	int Clients(const string& hotel_name) const {
		if (hotel_clients.count(hotel_name) == 0) {
			return 0;
		}
		return hotel_clients.at(hotel_name).size();
	}

	int Rooms(const string& hotel_name) const {
		if (booked_rooms.count(hotel_name) == 0) {
			return 0;
		}
		return booked_rooms.at(hotel_name);
	}

private:
	void Adjust() {
		while (!all_bookings.empty()
			   && all_bookings.front().time <= last_booking_time - SEC_IN_DAY) {
			auto booking = all_bookings.front();
			all_bookings.pop();
			booked_rooms[booking.hotel_name] -= booking.room_count;
			--hotel_clients[booking.hotel_name][booking.client_id];
			if (hotel_clients[booking.hotel_name][booking.client_id] == 0) {
				hotel_clients[booking.hotel_name].erase(booking.client_id);
			}
			if (hotel_clients[booking.hotel_name].empty()) {
				hotel_clients.erase(booking.hotel_name);
			}
		}
	}

	static const int64_t SEC_IN_DAY = 86400;
	int64_t last_booking_time;

	queue<Booking> all_bookings;
	map<string, int> booked_rooms;
	map<string, map<int, int>> hotel_clients;
};

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int query_count;
	cin >> query_count;

	BookingSystem bs;

	for (int query_id = 0; query_id < query_count; ++query_id) {
		string query_type;
		cin >> query_type;

		string hotel_name;
		if (query_type == "BOOK") {
			int64_t time;
			int client_id, room_count;
			cin >> time >> hotel_name >> client_id >> room_count;
			Booking b{time, hotel_name, client_id, room_count};
			bs.Book(hotel_name, b);
		} else {
			cin >> hotel_name;
			if (query_type == "CLIENTS") {
				cout << bs.Clients(hotel_name) << "\n";
			} else {
				cout << bs.Rooms(hotel_name) << "\n";
			}
		}
	}

	return 0;
}
