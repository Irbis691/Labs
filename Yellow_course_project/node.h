#pragma once

#include "date.h"
#include <memory>

enum class Comparison {
	Less,
	LessOrEqual,
	Greater,
	GreaterOrEqual,
	Equal,
	NotEqual
};

enum class LogicalOperation {
	And, Or
};

class Node {
public:
	virtual bool Evaluate(const Date& date, const string& event) = 0;
};

class EmptyNode : public Node {
public:
	bool Evaluate(const Date& date, const string& event) override;
};

class DateComparisonNode : public Node {
public:
	DateComparisonNode(const Comparison cmp, const Date& date);

	bool Evaluate(const Date& date, const string& event) override;

private:
	const Comparison _cmp;
	const Date _date;
};

class EventComparisonNode : public Node {
public:
	EventComparisonNode(const Comparison cmp, const string& event);

	bool Evaluate(const Date& date, const string& event) override;

private:
	const Comparison _cmp;
	const string _event;
};

class LogicalOperationNode : public Node {
public:
	LogicalOperationNode(const LogicalOperation lo, const shared_ptr <Node>& left, const shared_ptr <Node>& right);

	bool Evaluate(const Date& date, const string& event) override;

private:
	const LogicalOperation lo;
	const shared_ptr<Node> left;
	const shared_ptr<Node> right;
};