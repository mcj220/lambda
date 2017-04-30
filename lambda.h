#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace Lambda {

const unsigned MAX_REDUCE_STEPS = 1024;

struct TooManyStepsError {};

class Expression;
using ExpressionP = std::shared_ptr<Expression>;

class Name;
using NameP = std::shared_ptr<Name>;

class Function;
using FunctionP = std::shared_ptr<Function>;

class Application;
using ApplicationP = std::shared_ptr<Application>;

class Expression
{
public:
	virtual std::pair<bool, ExpressionP> replace(const NameP name, const ExpressionP expr) const = 0;
	virtual void print(std::ostream &os) const = 0;

	virtual ~Expression() {}
};

class Name: public Expression
{
public:
	static NameP create(const std::string &name)
	{
		return std::make_shared<Name>(name);
	}

	explicit Name(const std::string &name): m_name(name) {}

	virtual std::pair<bool, ExpressionP> replace(const NameP name, const ExpressionP expr) const
	{
		if (name->name() == m_name) {
			return std::make_pair(true, expr);
		} else {
			return std::make_pair(false, create(m_name));
		}
	}

	virtual void print(std::ostream &os) const
	{
		os << m_name;
	}

	bool operator==(const Name& other) const
	{
		return this == &other || m_name == other.m_name;
	}

	const std::string &name() const
	{
		return m_name;
	}

private:
	std::string m_name;
};

std::ostream &operator<<(std::ostream &os, const ExpressionP& expr);

class Function: public Expression
{
public:
	static FunctionP create(const NameP vbound, const ExpressionP body)
	{
		return std::make_shared<Function>(vbound, body);
	}

	Function(const NameP vbound, const ExpressionP body):
		m_vbound(vbound),
		m_body(body)
	{
		auto fbody = std::dynamic_pointer_cast<Function>(m_body);
	}

	FunctionP Aconvert(const NameP name) const
	{
		auto p = m_body->replace(m_vbound, name);
		return create(name, p.second);
	}

	ExpressionP Breduce(const ExpressionP expr) const
	{
		auto p = m_body->replace(m_vbound, expr);
		return p.second;
	}

	virtual std::pair<bool, ExpressionP> replace(const NameP name, const ExpressionP expr) const
	{
		auto p = std::dynamic_pointer_cast<Name>(expr);
		if (p && *p == *m_vbound) {
			// Not a very good scheme of alpha-conversion, has pathological cases!
			return Aconvert(Name::create("^" + m_vbound->name()))->replace(name, expr);
		} else {
			if (*name == *m_vbound) {
				return std::make_pair(false, create(m_vbound, m_body));
			} else {
				auto q = m_body->replace(name, expr);
				return std::make_pair(q.first, create(m_vbound, q.second));
			}
		}
	}

	virtual void print(std::ostream &os) const
	{
		os << "λ";
		m_vbound->print(os);
		os << ".";
		m_body->print(os);
	}

	const ExpressionP body() const
	{
		return m_body;
	}

	const NameP vbound() const
	{
		return m_vbound;
	}

private:
	const NameP m_vbound;
	const ExpressionP m_body;
};

class Application: public Expression
{
public:
	static ApplicationP create(const ExpressionP func, const ExpressionP arg)
	{
		return std::make_shared<Application>(func, arg);
	}

	Application(const ExpressionP func, const ExpressionP arg):
		m_func(func),
		m_arg(arg) {}

	virtual std::pair<bool, ExpressionP> replace(const NameP name, const ExpressionP expr) const
	{
		auto p = m_func->replace(name, expr);
		auto q = m_arg->replace(name, expr);
		return std::make_pair(p.first || q.first, create(p.second, q.second));
	}

	virtual void print(std::ostream &os) const
	{
		os << "(" << m_func;
		os << " ";
		m_arg->print(os);
		os << ")";
	}

	ExpressionP apply() const
	{
		auto func = std::dynamic_pointer_cast<Function>(m_func);
		if (func) {
			return func->Breduce(m_arg);
		} else {
			return nullptr;
		}
	}

	const ExpressionP func() const
	{
		return m_func;
	}

	const ExpressionP arg() const
	{
		return m_arg;
	}

private:
	const ExpressionP m_func;
	const ExpressionP m_arg;
};

ExpressionP Nreduce1(const ExpressionP expr);
ExpressionP Areduce1(const ExpressionP expr);

ExpressionP reduce(ExpressionP expr);

namespace Expressions {

// λx.x
const auto zero =
	Function::create(
		Name::create("x"),
		Name::create("x")
	);

// λx.λy.x
const auto select_first =
	Function::create(
		Name::create("x"),
		Function::create(
			Name::create("y"),
			Name::create("x")
		)
	);

// λx.λy.y
const auto select_second =
	Function::create(
		Name::create("x"),
		Function::create(
			Name::create("y"),
			Name::create("y")
		)
	);

const auto true_func = select_first;
const auto false_func = select_second;

// λe1.λe2.λc.((c e1) e2)
const auto cond =
	Function::create(
		Name::create("e1"),
		Function::create(
			Name::create("e2"),
			Function::create(
				Name::create("c"),
				Application::create(
					Application::create(
						Name::create("c"),
						Name::create("e1")
					),
					Name::create("e2")
				)
			)
		)
	);

const auto make_pair = cond;

// λn.(n select_first)
const auto iszero =
	Function::create(
		Name::create("n"),
		Application::create(
			Name::create("n"),
			select_first
		)
	);

// λn.λs.((s false) n)
const auto succ =
	Function::create(
		Name::create("n"),
		Function::create(
			Name::create("s"),
			Application::create(
				Application::create(
					Name::create("s"),
					false_func
				),
				Name::create("n")
			)
		)
	);

const auto one =
	Application::create(
		succ,
		zero
	);

// λn.if iszero n then zero else (n select_second)
const auto pred =
	Function::create(
		Name::create("n"),
		Application::create(
			Application::create(
				Application::create(
					cond,
					Application::create(
						iszero,
						Name::create("n")
					)
				),
				zero
			),
			Application::create(
				Name::create("n"),
				select_second
			)
		)
	);

// λs.(f (s s)
const auto rec1 =
	Function::create(
		Name::create("s"),
		Application::create(
			Name::create("f"),
			Application::create(
				Name::create("s"),
				Name::create("s")
			)
		)
	);

// λf.(λs.(f (s s)) λs.(f (s s)))
const auto recursive =
	Function::create(
		Name::create("f"),
		Application::create(
			rec1,
			rec1
		)
	);

// λf.λx.λy.if iszero x then y else ((f pred x) succ y)
const auto add1 =
	Function::create(
		Name::create("f"),
		Function::create(
			Name::create("x"),
			Function::create(
				Name::create("y"),
				Application::create(
					Application::create(
						Application::create(
							cond,
							// then
							Name::create("y")
						),
						// else
						Application::create(
							Application::create(
								Name::create("f"),
								Application::create(
									pred,
									Name::create("x")
								)
							),
							Application::create(
								succ,
								Name::create("y")
							)
						)
					),
					// cond
					Application::create(
						iszero,
						Name::create("x")
					)
				)
			)
		)
	);

// (recursive add1)
const auto add =
	Application::create(
		recursive,
		add1
	);

// λf.λx.λy.if iszero y then x else ((f pred x) pred y)
const auto sub1 =
	Function::create(
		Name::create("f"),
		Function::create(
			Name::create("x"),
			Function::create(
				Name::create("y"),
				Application::create(
					Application::create(
						Application::create(
							cond,
							// then
							Name::create("x")
						),
						// else
						Application::create(
							Application::create(
								Name::create("f"),
								Application::create(
									pred,
									Name::create("x")
								)
							),
							Application::create(
								pred,
								Name::create("y")
							)
						)
					),
					// cond
					Application::create(
						iszero,
						Name::create("y")
					)
				)
			)
		)
	);

// (recursive sub1)
const auto sub =
	Application::create(
		recursive,
		sub1
	);

// λx.λy.add sub x y sub y x
const auto abs_diff =
	Function::create(
		Name::create("x"),
		Function::create(
			Name::create("y"),
			Application::create(
				Application::create(
					add,
					Application::create(
						Application::create(
							sub,
							Name::create("x")
						),
						Name::create("y")
					)
				),
				Application::create(
					Application::create(
						sub,
						Name::create("y")
					),
					Name::create("x")
				)
			)
		)
	);

const auto equal =
	Function::create(
		Name::create("x"),
		Function::create(
			Name::create("y"),
			Application::create(
				iszero,
				Application::create(
					Application::create(
						abs_diff,
						Name::create("x")
					),
					Name::create("y")
				)
			)
		)
	);

const auto make_obj = make_pair;

const auto type_func =
	Function::create(
		Name::create("obj"),
		Application::create(
			Name::create("obj"),
			select_first
		)
	);

const auto value_func =
	Function::create(
		Name::create("obj"),
		Application::create(
			Name::create("obj"),
			select_second
		)
	);

const auto istype =
	Function::create(
		Name::create("t"),
		Function::create(
			Name::create("obj"),
			Application::create(
				Application::create(
					equal,
					Name::create("t")
				),
				Application::create(
					type_func,
					Name::create("obj")
				)
			)
		)
	);

const auto error_type = zero;

const auto make_error =
	Application::create(
		make_obj,
		error_type
	);

const auto bool_type = one;

const auto isbool =
	Function::create(
		Name::create("x"),
		Application::create(
			Application::create(
				istype,
				bool_type
			),
			Name::create("x")
		)
	);

const auto bool_error =
	Application::create(
		make_error,
		bool_type
	);

#if 0
const auto typed_cond =
	Function::create(
		Name::create("E1"),
		Function::create(
			Name::create("E2"),
			Function::create(
				Name::create("C"),
				Application::create(
					Application::create(
						Application::create(
							cond,
							// then
							Application::create(
								Application::create(
									Application::create(
										cond,
										//then
										Name::create("E1")
									),
									// else
									Name::create("E2")
								),
								// cond
								Application::create(
									value_func,
									Name::create("C")
								)
							)
						),
						// else
						bool_error
					),
					// cond
					Application::create(
						isbool,
						Name::create("C")
					)
				)
			)
		)
	);
#endif

const auto typed_cond =
	Function::create(
		Name::create("E1"),
		Function::create(
			Name::create("E2"),
			Function::create(
				Name::create("C"),
				Application::create(
					Application::create(
						Application::create(
							cond,
							// then
							Application::create(
								Application::create(
									Application::create(
										cond,
										//then
										Name::create("E1")
									),
									// else
									Name::create("E2")
								),
								// cond
								Application::create(
									value_func,
									Name::create("C")
								)
							)
						),
						// else
						bool_error
					),
					// cond
					Application::create(
						isbool,
						Name::create("C")
					)
				)
			)
		)
	);

} // namespace Expressions

} // namespace Lambda
