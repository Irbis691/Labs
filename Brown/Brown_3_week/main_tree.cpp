#include "Common.h"
#include "../../test_runner.h"

#include <sstream>

using namespace std;

class ValueExpression : public Expression {
public:

	explicit ValueExpression(int value) : value_(value) {}

	int Evaluate() const override {
		return value_;
	}

	string ToString() const override {
		return to_string(value_);
	}

private:
	int value_;
};

class BiExpression : public Expression {
public:
	explicit BiExpression(ExpressionPtr left_, ExpressionPtr right_) :
			left_(move(left_)),
			right_(move(right_)) {}

protected:
	ExpressionPtr left_;
	ExpressionPtr right_;
};

class SumExpression : public BiExpression {
public:
	explicit SumExpression(ExpressionPtr left_, ExpressionPtr right_)
			: BiExpression(move(left_), move(right_)) {};

	int Evaluate() const override {
		return left_->Evaluate() + right_->Evaluate();
	}

	string ToString() const override {
		return "(" + left_->ToString() + ")" + "+" + "(" + right_->ToString() + ")";
	}
};

class ProductExpression : public BiExpression {
public:
	explicit ProductExpression(ExpressionPtr left_, ExpressionPtr right_)
			: BiExpression(move(left_), move(right_)) {};

	int Evaluate() const override {
		return left_->Evaluate() * right_->Evaluate();
	}

	string ToString() const override {
		return "(" + left_->ToString() + ")" + "*" + "(" + right_->ToString() + ")";
	}
};

// Функции для формирования выражения
ExpressionPtr Value(int value) {
	return make_unique<ValueExpression>(value);
}

ExpressionPtr Sum(ExpressionPtr left, ExpressionPtr right) {
	return make_unique<SumExpression>(move(left), move(right));
}

ExpressionPtr Product(ExpressionPtr left, ExpressionPtr right) {
	return make_unique<ProductExpression>(move(left), move(right));
}

string Print(const Expression* e) {
	if (!e) {
		return "Null expression provided";
	}
	stringstream output;
	output << e->ToString() << " = " << e->Evaluate();
	return output.str();
}

void Test() {
	ExpressionPtr e1 = Product(Value(2), Sum(Value(3), Value(4)));
	ASSERT_EQUAL(Print(e1.get()), "(2)*((3)+(4)) = 14");

	ExpressionPtr e2 = Sum(move(e1), Value(5));
	ASSERT_EQUAL(Print(e2.get()), "((2)*((3)+(4)))+(5) = 19");

	ASSERT_EQUAL(Print(e1.get()), "Null expression provided");
}

int main() {
	TestRunner tr;
	RUN_TEST(tr, Test);
	return 0;
}