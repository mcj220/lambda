#include <algorithm>
#include <cwchar>
#include <ios>
#include <memory>
#include <deque>

#include "parser.h"

using std::all_of;
using std::codecvt_utf8;
using std::dynamic_pointer_cast;
using std::isalnum;
using std::locale;
using std::make_pair;
using std::make_shared;
using std::noskipws;
using std::out_of_range;
using std::pair;
using std::runtime_error;
using std::skipws;
using std::deque;
using std::wstring;
using std::wstring_convert;
using std::wstringstream;

namespace Lambda {
namespace Parser {

const symbol_table &builtins()
{
	const static symbol_table bins = {
		{"builtin_zero", {Expressions::zero, 0}},
		{"builtin_one", {Expressions::one, 0}},
		{"builtin_select_first", {Expressions::select_first, 2}},
		{"builtin_select_second", {Expressions::select_second, 2}},
		{"builtin_true", {Expressions::true_func, 0}},
		{"builtin_false", {Expressions::false_func, 0}},
		{"builtin_cond", {Expressions::cond, 3}},
		{"builtin_make_pair", {Expressions::make_pair, 2}},
		{"builtin_iszero", {Expressions::iszero, 1}},
		{"builtin_succ", {Expressions::succ, 1}},
		{"builtin_pred", {Expressions::pred, 1}},
		{"builtin_add", {Expressions::add, 2}},
		{"builtin_sub", {Expressions::sub, 2}},
		{"builtin_abs_diff", {Expressions::abs_diff, 2}},
		{"builtin_equal", {Expressions::equal, 2}},
		{"builtin_make_obj", {Expressions::make_obj, 2}},
		{"builtin_type", {Expressions::type_func, 1}},
		{"builtin_value", {Expressions::value_func, 1}},
		{"builtin_istype", {Expressions::istype, 2}},
		{"builtin_error_type", {Expressions::error_type, 2}},
		{"builtin_make_error", {Expressions::make_error, 0}},
		{"builtin_bool_type", {Expressions::bool_type, 0}},
		{"builtin_isbool", {Expressions::isbool, 1}},
		{"builtin_bool_error", {Expressions::bool_error, 0}}
	};

	return bins;
}

SymbolTableP newDefaultSymTable()
{
	return make_shared<symbol_table>(builtins());
}

template<> pair<bool, ExpressionP> ExpressionBuilder::parse(ParseContext &ctx);
template<> pair<bool, NameP> ExpressionBuilder::parse(ParseContext &ctx);
template<> pair<bool, FunctionP> ExpressionBuilder::parse(ParseContext &ctx);
template<> pair<bool, ApplicationP> ExpressionBuilder::parse(ParseContext &ctx);

std::wostream &operator<<(std::wostream &os, const Token &tok)
{
	os << tok.val;
	return os;
}

std::wistream &operator>>(std::wistream &is, Token &token)
{
	if (!is.good()) {
		return is;
	}

	wchar_t c;
	is >> c;

	// Single character tokens
	if (!is.fail()) {
		if (c == L'λ') {
			token = Token(Token::Type::LAMBDA, wstring(1, c));
		} else if (c == '.') {
			token = Token(Token::Type::DOT, wstring(1, c));
		} else if (c == '(') {
			token = Token(Token::Type::L_PAREN, wstring(1, c));
		} else if (c == ')') {
			token = Token(Token::Type::R_PAREN, wstring(1, c));
		} else if (c == '=') {
			token = Token(Token::Type::EQUALS, wstring(1, c));
		} else {
			std::wstring word;
			
			is >> noskipws;
			while (!is.fail()) {
				if (!isalnum(c) && c != '_') {
					is.clear();
					is.putback(c);
					break;
				}

				word += c;

				if (!is.good()) {
					break;
				}

				is >> c;
			}
			is.clear();
			is >> skipws;

			if (word == L"def") {
				token = Token(Token::Type::DEF, word);
			} else if (word == L"rec") {
				token = Token(Token::Type::REC, word);
			} else if (word == L"if") {
				token = Token(Token::Type::IF, word);
			} else if (word == L"then") {
				token = Token(Token::Type::THEN, word);
			} else if (word == L"else") {
				token = Token(Token::Type::ELSE, word);
			} else if (word == L"IF") {
				token = Token(Token::Type::IF_TYPED, word);
			} else if (word == L"THEN") {
				token = Token(Token::Type::THEN_TYPED, word);
			} else if (word == L"ELSE") {
				token = Token(Token::Type::ELSE_TYPED, word);
			} else if (all_of(word.begin(), word.end(), [](wchar_t c){ return isdigit(c); })) {
				token = Token(Token::Type::INTLITERAL, word);
			} else {
				token = Token(Token::Type::OBJECT, word);
			}
		}
	}

#if 0
	if (token.type == Token::Type::INVALID) {
		std::wcout << "Read token [INVALID]" << std::endl;
	} else {
		std::wcout << "Read token [" << token.val << "]" << std::endl;
	}
#endif

	return is;
}

pair<symbol_table::key_type, ExpressionP> ExpressionBuilder::parse1()
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.eof() || startpos == -1) {
		return make_pair("_", nullptr);
	}

	if (!m_tokens.good() || startpos == -1) {
		throw runtime_error("Could not parse");
	}

	Token tok;
	m_tokens >> tok;
	if (m_tokens.eof()) {
		return make_pair("_", nullptr);
	}
	bool rec = (tok.type == Token::Type::REC);
	if (m_tokens.good() && (tok.type == Token::Type::DEF || rec)) {
		m_tokens >> tok;
		if (m_tokens.good() && tok.type == Token::Type::OBJECT) {
			auto name = s_convert.to_bytes(tok.val);
			if (m_syms->find(name) != m_syms->end()) {
				throw runtime_error("Redefinition of symbol \"" + name + "\"");
			}
			m_tokens >> tok;
			deque<NameP> varq;
			while (m_tokens.good() && tok.type == Token::Type::OBJECT) {
				varq.push_back(Name::create(s_convert.to_bytes(tok.val)));
				m_tokens >> tok;
			}
			if (m_tokens.good() && tok.type == Token::Type::EQUALS) {
				auto s2 = std::make_shared<symbol_table>(*m_syms);
				ParseContext ctx{s2};
				const auto self = Name::create("self^");

				(*s2)[name] = make_pair(self, varq.size());

				auto p = parse<Expression>(ctx);
				if (p.first) {
					auto q = p.second;
					auto varq2 = varq;
					size_t nargs = varq.size();
					while (!varq.empty()) {
						q = Function::create(varq.back(), q);
						varq.pop_back();
					}
					if (rec) {
						q = Application::create(
							Expressions::recursive,
							Function::create(
								self,
								q
							)
						);
					}
					(*m_syms)[name] = make_pair(q, nargs);
					return make_pair(name, q);
				}
			}
		}
		throw runtime_error("Could not parse");
	}

	m_tokens.setstate(flags);
	m_tokens.seekg(startpos);

	if (m_tokens.eof()) {
		return make_pair("_", nullptr);
	}

	ParseContext ctx{m_syms};
	auto p = parse<Expression>(ctx);
	if (p.first) {
		return make_pair("", p.second);
	}

	if (m_tokens.eof()) {
		return make_pair("_", nullptr);
	}

	throw runtime_error("Could not parse");
}

template<> pair<bool, ExpressionP> ExpressionBuilder::parse(ParseContext &ctx)
{
	{
		auto p = parseIfThenElse(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}

	{
		auto p = parseTypedIfThenElse(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}
	
	{
		auto p = parseInt(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}

	{
		auto p = substitute(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}

	{
		auto p = parse<Name>(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}

	{
		auto p = parse<Function>(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}

	{
		auto p = parse<Application>(ctx);
		if (p.first) {
			return make_pair(true, p.second);
		}
	}

	return make_pair(false, nullptr);
}

pair<bool, ExpressionP> ExpressionBuilder::parseInt(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (!m_tokens.fail() && tok.type == Token::Type::INTLITERAL) {
			unsigned num;
			wstringstream(tok.val) >> num;
			ExpressionP result = Expressions::zero;
			for(; num > 0; --num) {
				result = Application::create(Expressions::succ, result);
			}

			return make_pair(true, result);
		}
		m_tokens.setstate(flags);
		m_tokens.seekg(startpos);
	}

	return make_pair(false, nullptr);
}

pair<bool, ExpressionP> ExpressionBuilder::substitute(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (!m_tokens.fail() && tok.type == Token::Type::OBJECT) {
			auto sym = s_convert.to_bytes(tok.val);
			auto it = ctx.syms->find(sym);
			if (it != ctx.syms->end()) {
				auto p = parseImplicitApplication(it->second, ctx);
				if (p.first) {
					return make_pair(true, p.second);
				} else {
					return make_pair(true, it->second.first);
				}
			}
		}
		m_tokens.setstate(flags);
		m_tokens.seekg(startpos);
	}

	return make_pair(false, nullptr);
}

template<> pair<bool, NameP> ExpressionBuilder::parse(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (!m_tokens.fail() && tok.type == Token::Type::OBJECT) {
			return make_pair(true, Name::create(s_convert.to_bytes(tok.val)));
		}
		m_tokens.setstate(flags);
		m_tokens.seekg(startpos);
	}

	return make_pair(false, nullptr);
}

template<> pair<bool, FunctionP> ExpressionBuilder::parse(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (m_tokens.good() && tok.type == Token::Type::LAMBDA) {
			m_tokens >> tok;
			if (m_tokens.good() && tok.type == Token::Type::OBJECT) {
				auto name = s_convert.to_bytes(tok.val);
				ParseContext c2{ctx, ParentPos::EXPRESSION};
				m_tokens >> tok;
				if (m_tokens.good() && tok.type == Token::Type::DOT) {
					auto q = parse<Function>(c2);
					if (q.first) {
						return make_pair(true, Function::create(Name::create(name), q.second));
					}
					auto r = parse<Expression>(c2);
					if (r.first) {
						return make_pair(true, Function::create(Name::create(name), r.second));
					}
				}
			}
		}
		m_tokens.setstate(flags);
		m_tokens.seekg(startpos);
	}

	return make_pair(false, nullptr);
}

template<> pair<bool, ApplicationP> ExpressionBuilder::parse(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (m_tokens.good() && tok.type == Token::Type::L_PAREN) {
			ParseContext c2{ctx, ParentPos::APPLICATION_EXPR};
			auto p = parse<Expression>(c2);
			if (p.first) {
				auto q = parse<Expression>(ctx);
				if (m_tokens.good() && q.first) {
					m_tokens >> tok;
					if (!m_tokens.fail() && tok.type == Token::Type::R_PAREN) {
						return make_pair(true, Application::create(p.second, q.second));
					}
				}
			}
		}
		m_tokens.setstate(flags);
		m_tokens.seekg(startpos);
	}

	return make_pair(false, nullptr);
}

pair<bool, ApplicationP> ExpressionBuilder::parseImplicitApplication(const symbol_table::mapped_type &func, ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	ExpressionP result = func.first;
	auto nargs = func.second;
	if (nargs >= 1) {
		for(auto nlevel = nargs-1; true; --nlevel) {
			ParseContext c2{ctx, ParentPos::EXPRESSION};
			auto p = parse<Expression>(c2);
			if (p.first) {
				result = Application::create(result, p.second);
				if (nlevel == 0) {
					auto startpos = m_tokens.tellg();
					auto flags = m_tokens.rdstate();
					Token tok;
					m_tokens >> tok;
					if (ctx.ppos != ParentPos::APPLICATION_EXPR ||
							tok.type != Token::Type::R_PAREN) {
						m_tokens.setstate(flags);
						m_tokens.seekg(startpos);
						return make_pair(true, dynamic_pointer_cast<Application>(result));
					} else {
						break;
					}
				}
			} else {
				break;
			}
		}
	}

	m_tokens.setstate(flags);
	m_tokens.seekg(startpos);

	return make_pair(false, nullptr);
}

pair<bool, ApplicationP> ExpressionBuilder::parseIfThenElse(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (m_tokens.good() && tok.type == Token::Type::IF) {
			ParseContext c2{ctx, ParentPos::EXPRESSION};
			auto pred = parse<Expression>(c2);
			if (m_tokens.good() && pred.first) {
				m_tokens >> tok;
				if (m_tokens.good() && tok.type == Token::Type::THEN) {
					auto then_expr = parse<Expression>(c2);
					if (m_tokens.good() && then_expr.first) {
						m_tokens >> tok;
						if (m_tokens.good() && tok.type == Token::Type::ELSE) {
							auto else_expr = parse<Expression>(c2);
							if (else_expr.first) {
								return make_pair(true,
									Application::create(
										Application::create(
											Application::create(Expressions::cond, then_expr.second),
												else_expr.second), pred.second));
							}
						}
					}
				}
			}
		}
	}

	m_tokens.setstate(flags);
	m_tokens.seekg(startpos);

	return make_pair(false, nullptr);
}

// TODO consolidate with parseIfThenElse
pair<bool, ApplicationP> ExpressionBuilder::parseTypedIfThenElse(ParseContext &ctx)
{
	auto startpos = m_tokens.tellg();
	auto flags = m_tokens.rdstate();

	if (m_tokens.good() && startpos != -1) {
		Token tok;
		m_tokens >> tok;
		if (m_tokens.good() && tok.type == Token::Type::IF_TYPED) {
			ParseContext c2{ctx, ParentPos::EXPRESSION};
			auto pred = parse<Expression>(c2);
			if (m_tokens.good() && pred.first) {
				m_tokens >> tok;
				if (m_tokens.good() && tok.type == Token::Type::THEN_TYPED) {
					auto then_expr = parse<Expression>(c2);
					if (m_tokens.good() && then_expr.first) {
						m_tokens >> tok;
						if (m_tokens.good() && tok.type == Token::Type::ELSE_TYPED) {
							auto else_expr = parse<Expression>(c2);
							if (else_expr.first) {
								return make_pair(true,
									Application::create(
										Application::create(
											Application::create(
												Expressions::typed_cond,
												then_expr.second
											),
											else_expr.second
										),
										pred.second
									)
								);
							}
						}
					}
				}
			}
		}
	}

	m_tokens.setstate(flags);
	m_tokens.seekg(startpos);

	return make_pair(false, nullptr);
}

wstring_convert<codecvt_utf8<wchar_t>> ExpressionBuilder::s_convert{};

} // namespace Parser
} // namespace Lambda
