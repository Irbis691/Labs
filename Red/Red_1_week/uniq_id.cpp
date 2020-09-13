#include <string>
#include <vector>
using namespace std;

#define IMP(l) qwe##l
#define IMPL(line) IMP(line)
#define UNIQ_ID IMPL(__LINE__)

int main() {
	int UNIQ_ID = 0;
	string UNIQ_ID = "hello";
	vector<string> UNIQ_ID = {"hello", "world"};
	vector<int> UNIQ_ID = {1, 2, 3, 4};
}