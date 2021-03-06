/*
Copyright (C) 2011 Baptiste Wicht

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include "Timer.hpp"
#include "Graph.hpp"
#include "CallGraph.hpp"
#include "Infos.hpp"
#include "InfosOld.hpp"
#include "Analyzer.hpp"
#include "Parameters.hpp"
#include "GraphReader.hpp"
#include "Utils.hpp"

using std::vector;
using std::cout;
using std::endl;
using std::string;
using std::ofstream;

using namespace inlining;

bool filter(string name) {
    return name.find("'") != string::npos || name.find("(") != string::npos || name.find(".") != string::npos;
}

void transform(string& name) {
    if (name.find("@@") != string::npos) {
        name = name.substr(0, name.find("@@"));
    }
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        cout << "Not enough arguments. Provide at least the dot file to read" << endl;

        return 1;
    }

    Parameters::init();

    Infos infos;
    GraphReader reader(infos);
    CallGraph* graph = reader.read(argv[1]);

    ofstream stream;
    stream.open("main.cpp");

    stream << "//Auto generated by inlining analyzer" << endl;
    stream << "#include <iostream>" << endl << endl;

    stream << "using std::cout;" << endl;
    stream << "using std::endl;" << endl << endl;

    string mainFunction = "";

    //Print the declarations
    FunctionIterator first, last;
    for (boost::tie(first, last) = graph->functions(); first != last; ++first) {
        string name = (*graph)[*first].name;

        transform(name);

        if (filter(name)) {
            continue;
        }

        if ((*graph)[*first].calls == 0) {
            mainFunction = name;
        }

        //Declaration
        stream << "void FFF" << name << "() __attribute__ ((noinline));" << endl;
    }

    //Print the definitions
    for (boost::tie(first, last) = graph->functions(); first != last; ++first) {
        string name = (*graph)[*first].name;

        transform(name);

        if (filter(name)) {
            continue;
        }

        //Definition
        stream << "void FFF" << name << "() {" << endl;
        stream << "\tcout << \"I'm in " << name << "\" << endl;" << endl;

        OutCallSiteIterator it, end;
        for (boost::tie(it, end) = boost::out_edges(*first, *graph->getGraph()); it != end; ++it) {
            string called = (*graph)[boost::target(*it, *graph->getGraph())].name;

            transform(called);

            if (filter(called)) {
                continue;
            }

            stream << "\tFFF" << called << "();" << endl;
        }

        stream << "}" << endl << endl;
    }

    //Write main function
    stream << "int main(){" << endl;
    stream << "\tcout << \"I'm in main()\" << endl;" << endl;
    stream << "\tFFF" << mainFunction << "();" << endl;
    stream << "\treturn 0;" << endl;
    stream << "}" << endl;

    stream.close();

    delete graph;

    return 0;
}
