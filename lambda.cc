#include "lambda.h"

using std::ostream;
using std::dynamic_pointer_cast;

namespace Lambda {

ostream &operator<<(ostream &os, const ExpressionP& expr)
{
	if (!expr) {
		os << "<null expression>";
	}
	expr->print(os);
	return os;
}

ExpressionP Nreduce1(const ExpressionP expr)
{
	if (auto app = dynamic_pointer_cast<Application>(expr)) {
		if (auto reduced = app->apply()) {
			return reduced;
		} else if (auto new_func = Nreduce1(app->func())) {
			return Application::create(new_func, app->arg());
		} else if (auto new_arg = Nreduce1(app->arg())) {
			return Application::create(app->func(), new_arg);
		}
	} else if (auto func = dynamic_pointer_cast<Function>(expr)) {
		auto new_body = Nreduce1(func->body());
		if (new_body) {
			return Function::create(func->vbound(), new_body);
		}
	}

	return nullptr;
}

ExpressionP Areduce1(const ExpressionP expr)
{
	if (auto app = dynamic_pointer_cast<Application>(expr)) {
		if (auto new_arg = Nreduce1(app->arg())) {
			return Application::create(app->func(), new_arg);
		} else if (auto reduced = app->apply()) {
			return reduced;
		} else if (auto new_func = Nreduce1(app->func())) {
			return Application::create(new_func, app->arg());
		}
	} else if (auto func = dynamic_pointer_cast<Function>(expr)) {
		auto new_body = Nreduce1(func->body());
		if (new_body) {
			return Function::create(func->vbound(), new_body);
		}
	}

	return nullptr;
}

ExpressionP reduce(ExpressionP expr)
{
	ExpressionP result{expr};

	for(auto i=MAX_REDUCE_STEPS; i>0; ++i) {
		auto interm = Areduce1(result);
		if (!interm) {
			return result;
		}
		result = interm;
	}

	throw TooManyStepsError{};
}

} // namespace Lambda
