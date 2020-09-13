#include "../../test_runner.h"

#include <vector>

using namespace std;

template<typename T>
class Table {
public:
	Table(size_t rowNum, size_t colNum) {
		Resize(rowNum, colNum);
	}

	vector<T>& operator[](size_t index) {
		return table[index];
	}

	const vector<T>& operator[](size_t index) const {
		return table[index];
	}

	void Resize(size_t rowNum, size_t colNum) {
		table.resize(rowNum);
		for (vector<T>& row: table) {
			row.resize(colNum);
		}
	}

	pair<size_t, size_t> Size() const {
		return {table.size(), table[0].size()};
	}

private:
	vector<vector<T>> table;
};

void TestTable() {
	Table<int> t(1, 1);
	ASSERT_EQUAL(t.Size().first, 1u);
	ASSERT_EQUAL(t.Size().second, 1u);
	t[0][0] = 42;
	ASSERT_EQUAL(t[0][0], 42);
	t.Resize(3, 4);
	ASSERT_EQUAL(t.Size().first, 3u);
	ASSERT_EQUAL(t.Size().second, 4u);
}

int main() {
	TestRunner tr;
	RUN_TEST(tr, TestTable);
	return 0;
}
