#include "../../test_runner.h"

#include <iostream>
#include <string>
#include <queue>
#include <set>
#include <stdexcept>

using namespace std;

template<class T>
class ObjectPool {
public:
	T* Allocate() {
		if (!free.empty()) {
			T* ptr = free.front();
			created.insert(ptr);
			free.pop_front();
			return ptr;
		}
		T* ptr = new T;
		created.insert(ptr);
		return ptr;
	}

	T* TryAllocate() {
		if (!free.empty()) {
			T* ptr = free.front();
			created.insert(ptr);
			free.pop_front();
			return ptr;
		}
		return nullptr;
	}

	void Deallocate(T* object) {
		const auto itr = created.find(object);
		if (itr == created.end()) {
			throw invalid_argument("");
		}
		created.erase(itr);
		free.push_back(object);
	}

	~ObjectPool() {
		for (T* item: created) {
			delete item;
		}
		created.clear();

		while (!free.empty()) {
			delete free.front();
			free.pop_front();
		}
	}

private:
	set<T*> created;
	deque<T*> free;
};

void TestObjectPool() {
	ObjectPool<string> pool;

	auto p1 = pool.Allocate();
	auto p2 = pool.Allocate();
	auto p3 = pool.Allocate();

	*p1 = "first";
	*p2 = "second";
	*p3 = "third";

	pool.Deallocate(p2);
	ASSERT_EQUAL(*pool.Allocate(), "second");

	pool.Deallocate(p3);
	pool.Deallocate(p1);
	ASSERT_EQUAL(*pool.Allocate(), "third");
	ASSERT_EQUAL(*pool.Allocate(), "first");

	pool.Deallocate(p1);
}

int main() {
	TestRunner tr;
	RUN_TEST(tr, TestObjectPool);
	return 0;
}