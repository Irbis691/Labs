#pragma once

#include <cstdlib>
#include <iostream>

using namespace std;

template<typename T>
class SimpleVector {
public:
	SimpleVector() {
		data = end_ = nullptr;
		capacity_ = 0;
	}

	explicit SimpleVector(size_t capacity) {
		data = new T[capacity];
		end_ = data + capacity;
		capacity_ = capacity;
	}

	~SimpleVector() {
		delete[] data;
		delete end_;
	}

	T& operator[](size_t index) {
		return data[index];
	}

	T* end() {
		return end_;
	}

	T* begin() {
		return data;
	}

	size_t Size() const {
		return end_ - data;
	}

	size_t Capacity() const {
		return capacity_;
	}

	void PushBack(const T& value) {
		if (Capacity() == 0) {
			data = new T[1];
			end_ = data;
			capacity_ = 1;
		} else if (Size() == Capacity()) {
			capacity_ = 2 * capacity_;
			T* temp = new T[capacity_];
			for (size_t i = 0; i < Size(); ++i) {
				temp[i] = data[i];
			}
			delete[] data;
			*data = *temp;
			++end_;
			data[Size()] = value;
		} else {
			++end_;
			data[Size() - 1] = value;
		}
	}

private:
	T* data;
	T* end_;
	size_t capacity_;
};
