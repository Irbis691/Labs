#include <vector>
#include <iostream>

using namespace std;

template<typename T>
class Deque {
public:
	Deque() = default;

	bool Empty() const {
		return front.empty() && back.empty();
	}

	size_t Size() const {
		return front.size() + back.size();
	}

	void PushFront(T item) {
		front.push_back(item);
	}

	void PushBack(T item) {
		back.push_back(item);
	}

	T& At(size_t index) {
		if (index < 0 || index >= Size()) {
			throw out_of_range("");
		}
		if (index >= front.size()) {
			return back.at(index - front.size());
		} else {
			return front.at(front.size() - 1 - index);
		}
	}

	const T& At(size_t index) const {
		if (index < 0 || index >= Size()) {
			throw out_of_range("");
		}
		if (index >= front.size()) {
			return back.at(index - front.size());
		} else {
			return front.at(front.size() - 1 - index);
		}
	}

	T& operator[](size_t index) {
		return At(index);
	}

	const T& operator[](size_t index) const {
		return At(index);
	}

	T& Front() {
		return At(0);
	}

	const T& Front() const {
		return At(0);
	}

	T& Back() {
		return At(Size() - 1);
	}

	const T& Back() const {
		return At(Size() - 1);
	}

private:
	vector<T> front;
	vector<T> back;
};