#include <array>
#include <fstream>
#include <ios>
#include <iostream>
#include <locale>
#include <sstream>

#include <boost/filesystem.hpp>

#include "lambda.h"
#include "parser.h"

using std::cerr;
using std::cout;
using std::endl;
using std::getline;
using std::locale;
using std::make_shared;
using std::wcout;
using std::wifstream;
using std::wstring;

using boost::filesystem::exists;

using Lambda::ExpressionP;
using Lambda::Nreduce1;
using Lambda::Areduce1;
using Lambda::Parser::ExpressionBuilder;
using Lambda::Parser::newDefaultSymTable;

int main(int argc, char *argv[])
{
	wcout.imbue(locale("en_US.UTF-8"));

	if (argc == 1) {
		cerr << "REPL not yet implemented" << endl;
		return 1;
	}

	auto syms = newDefaultSymTable();

	for(int i=1; i<argc; ++i) {
		if (!exists(argv[i])) {
			cerr << "File \"" << argv[i] << "\" does not exist" << endl;
			return 1;
		}

		wifstream wfs{};
		wfs.imbue(locale("en_US.UTF-8"));
		wfs.open(argv[i]);

		do {
			wstring ws{};

			do {
				wstring line{};
				getline(wfs, line);
				line = line.substr(0, line.find(L"--"));
				auto pos = line.find_first_of(L'\\');
				if (pos == wstring::npos) {
					ws += line;
					break;
				} else {
					ws += line.substr(0, pos);
				}
			} while (true);

			if (wfs.eof() || wfs.bad()) {
				break;
			}

			auto eb = ExpressionBuilder(ws, syms);

			ExpressionP expr;
			auto p = eb.parse1();
			do {
				expr = p.second;
				if (p.first.empty()) {
					cout << "---" << endl;
					wcout << "Eval \"" << ws << "\"" << endl;
					auto interm = expr;
					int i=0;
					do {
						auto interm2 = interm;
						//cout << "=> " << interm2 << endl;
						interm = Nreduce1(interm);
						if (!interm) {
							cout << "... => " << interm2 << endl;
						}
						++i;
					} while (interm);
				} else {
					if (expr) {
						cout << "DEF " << p.first << ":";
						cout << syms->at(p.first).second;
						cout << " = " << expr << std::endl;
					}
				}
				p = eb.parse1();
			} while (p.second != nullptr);
		} while (true);
	}

	return 0;
}
