#pragma once

#include <iostream>
#include <istream>
#include <codecvt>
#include <locale>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>

#include "lambda.h"

namespace Lambda {
namespace Parser {

struct Token
{
	enum class Type {
		INVALID,
		LAMBDA,
		DOT,
		L_PAREN,
		R_PAREN,
		DEF,
		REC,
		EQUALS,
		OBJECT,
		INTLITERAL,
		IF,
		THEN,
		ELSE,
		IF_TYPED,
		THEN_TYPED,
		ELSE_TYPED
	};

	Token(): type(Token::Type::INVALID) {}

	Token(Token::Type type, const std::wstring val):
		type(type),
		val(val) {}

	Token::Type type;
	std::wstring val;
};

std::wistream &operator>>(std::wistream &is, Token &tok);
std::wostream &operator<<(std::wostream &os, const Token &tok);

enum class ParentPos {
	EXPRESSION,
	APPLICATION_EXPR,
};

// name -> <function, arity>
using symbol_table = std::map<std::string, std::pair<Lambda::ExpressionP, size_t>>;
using SymbolTableP = std::shared_ptr<symbol_table>;

const symbol_table &builtins();
SymbolTableP newDefaultSymTable();

class ExpressionBuilder
{
	struct ParseContext {

		ParseContext(SymbolTableP syms):
			syms(syms),
			ppos(ParentPos::EXPRESSION) {}

		ParseContext(const ParseContext &other, ParentPos ppos):
			syms(other.syms),
			ppos(ppos) {}

		SymbolTableP syms;
		const ParentPos ppos;
	};

public:
	ExpressionBuilder(std::wistream &is, SymbolTableP syms=nullptr):
		m_tokens(is),
		m_syms(syms ? syms : std::make_shared<symbol_table>()) {}

	ExpressionBuilder(const std::wstring &expression, SymbolTableP syms=nullptr):
		m_ss(expression),
		m_tokens(m_ss),
		m_syms(syms ? syms : std::make_shared<symbol_table>()) {}

	std::pair<symbol_table::key_type, Lambda::ExpressionP> parse1();

private:
	template<typename T> std::pair<bool, std::shared_ptr<T>> parse(ParseContext &);

	std::pair<bool, Lambda::ExpressionP> parseInt(ParseContext &ctx);
	std::pair<bool, Lambda::ExpressionP> substitute(ParseContext &ctx);
	std::pair<bool, Lambda::ApplicationP> parseImplicitApplication(const symbol_table::mapped_type &func, ParseContext &);
	std::pair<bool, Lambda::ApplicationP> parseIfThenElse(ParseContext &ctx);
	std::pair<bool, Lambda::ApplicationP> parseTypedIfThenElse(ParseContext &ctx);

	std::wstringstream m_ss;
	std::wistream &m_tokens;
	SymbolTableP m_syms;

	static std::wstring_convert<std::codecvt_utf8<wchar_t>> s_convert;
};

} // namespace Parser
} // namespace Lambda
