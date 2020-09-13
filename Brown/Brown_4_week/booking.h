#include <algorithm>

namespace RAII {
	template<typename Provider>
	class Booking {
	public:
		Booking(Provider* provider, int counter) : provider_(provider), counter_(counter) {}

		Booking(const Booking&) = delete;

		Booking(Booking&& other) noexcept : provider_(other.provider_), counter_(other.counter_) {
			other.provider_ = nullptr;
		};

		Booking& operator=(const Booking&) = delete;

		Booking& operator=(Booking&& other) noexcept {
			std::swap(provider_, other.provider_);
			std::swap(counter_, other.counter_);
			return *this;
		};

		~Booking() {
			if(provider_ != nullptr) {
				provider_->CancelOrComplete(*this);
			}
		}

	private:
		Provider* provider_;
		int counter_;
	};
}