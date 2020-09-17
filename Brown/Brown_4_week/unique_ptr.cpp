#include "../../test_runner.h"

#include <cstddef>
#include <functional>

template<typename T>
class UniquePtr {
private:
	T* t;
public:
	UniquePtr() : t(nullptr) {};

	explicit UniquePtr(T* ptr) : t(ptr) {}

	UniquePtr(const UniquePtr&) = delete;

	UniquePtr(UniquePtr&& other) {
		t = other.Release();
	}

	UniquePtr& operator=(const UniquePtr&) = delete;

	UniquePtr& operator=(nullptr_t) {
		if(t != nullptr) {
			delete t;
		}
		t = nullptr;
		return *this;
	}

	UniquePtr& operator=(UniquePtr&& other) {
		t = other.Release();
		return *this;
	}

	~UniquePtr() {
		delete t;
	}

	T& operator*() const {
		return *t;
	}

	T* operator->() const {
		return t;
	}

	T* Release() {
		T* temp = t;
		t = nullptr;
		return temp;
	}

	void Reset(T* ptr) {
		delete t;
		t = ptr;
	}

	void Swap(UniquePtr& other) {
		UniquePtr temp(std::move(*this));
		t = other.Release();
		other.t = temp.Release();
	}

	T* Get() const {
		return t;
	}
};


struct Item {
	static int counter;
	int value;

	Item(int v = 0) : value(v) {
		++counter;
	}

	Item(const Item& other) : value(other.value) {
		++counter;
	}

	~Item() {
		--counter;
	}
};

int Item::counter = 0;


void TestLifetime() {
	Item::counter = 0;
	{
		UniquePtr<Item> ptr(new Item);
		ASSERT_EQUAL(Item::counter, 1);

		ptr.Reset(new Item);
		ASSERT_EQUAL(Item::counter, 1);
	}
	ASSERT_EQUAL(Item::counter, 0);

	{
		UniquePtr<Item> ptr(new Item);
		ASSERT_EQUAL(Item::counter, 1);

		auto rawPtr = ptr.Release();
		ASSERT_EQUAL(Item::counter, 1);

		delete rawPtr;
		ASSERT_EQUAL(Item::counter, 0);
	}

	{
		UniquePtr<Item> ptr(new Item(1));
		UniquePtr<Item> ptr1(new Item(2));
		ptr.Swap(ptr1);
	}
	ASSERT_EQUAL(Item::counter, 0);
}

void TestGetters() {
	UniquePtr<Item> ptr(new Item(42));
	ASSERT_EQUAL(ptr.Get()->value, 42);
	ASSERT_EQUAL((*ptr).value, 42);
	ASSERT_EQUAL(ptr->value, 42);
}

void ForEach(const std::vector<int>& val, function<void(int)> func) {
	for(int v: val) {
		func(v);
	}
}

int main() {
	TestRunner tr;
	RUN_TEST(tr, TestLifetime);
	RUN_TEST(tr, TestGetters);
}
