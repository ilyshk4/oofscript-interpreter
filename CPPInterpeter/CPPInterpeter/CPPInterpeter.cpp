#include <iostream>
#include <vector>
#include <conio.h>
#include <string>
#include <functional>
#include <map>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

enum element {
	tvalue, tplus, tminus, tmultiply, tdivide, tgroupopen, tgroupclose, tcomma, tstring
};

map<string, function<vector<double>(vector<double>)>> functions = map<string, function<vector<double>(vector<double>)>>();
map<string, vector<double>> variables = map<string, vector<double>>();

void addfunc(string designation, function<vector<double>(vector<double>)> f) {
	functions.insert(pair<string, function<vector<double>(vector<double>)>>(designation, f));
}
void addvar(string designation) {
	variables.insert(pair<string, vector<double>>(designation, vector<double>() ));
}

void addvar(string designation, double value) {
	variables.insert(pair<string, vector<double>>(designation, vector<double> { value }));
}

void addvar(string designation, vector<double> value) {
	variables.insert(pair<string, vector<double>>(designation, value));
}

bool splitByDelimiter(string& str, string& dl, string& var, string& expr) {
	if (str.find(dl) != string::npos) {
		int delimiter = str.find(dl);
		var = str.substr(0, delimiter);
		expr = str.substr(delimiter + 2, str.length() - delimiter);
		return true;
	}
	return false;
}

bool hasIndex(string& str, int& index) {
	if (str.find('[') != string::npos && str.find(']') != string::npos) {
		int start = str.find('[') + 1;
		int end = str.find(']');
		index = stoi(str.substr(start, end - start).c_str());
		str = str.substr(0, start - 1);
		return true;
	} else return false;
}

bool parseVar(string& key, vector<double>& value) {
	int index = 0; bool check = hasIndex(key, index);
	if (variables.count(key)) {
		if (check)
			value = vector<double>{ variables[key][index] };
		else
			value = variables[key];

		return true;
	}
	return false;
}

vector<double> evaluateGroup(vector<element> elements, vector<double> values)
{
	unsigned int index = 0;

	index = 0;
	for(unsigned int j = 0; j < elements.size(); j++) {
		element& i = elements[j];
		if (i == tvalue)
			index++;
		if (i == tmultiply || i == tdivide) {
			double& a = values[index - 1];
			double& b = values[index];
			a = i == tmultiply ? a * b : a / b;
			values.erase(values.begin() + index);
			elements.erase(elements.begin() + j, elements.begin() + j + 2);
			j--;
		}
	}

	vector<double> result = vector<double>();
	index = 0;
	double value = 0;
	for (unsigned int j = 0; j < elements.size(); j++) {
		element& i = elements[j];
		if (i == tvalue) {
			value += values[index] * (j > 0 && elements[j - 1] == tminus ? -1 : 1);
			index++;
		}
		if (i == tcomma || j == elements.size() - 1)
		{
			result.push_back(value);
			value = 0;
		}
	}	
	return result;
}

vector<double> evaluate(tuple<vector<element>, vector<double>, vector<string>> tuple)
{
	vector<element> elements = get<0>(tuple);
	vector<double> values = get<1>(tuple);
	vector<string> strs = get<2>(tuple);

	int f = 0, v = 0;
	for (unsigned int i = 0; i < elements.size(); i++) {
		if (elements[i] == tstring) {
			string key = strs[f];
			vector<double> val;
			if (parseVar(key, val)) {
				vector<element> resem = vector<element>();
				for (unsigned int i = 0; i < val.size(); i++)
				{
					resem.push_back(tcomma);
					resem.push_back(tvalue);
				}
				resem.erase(resem.begin());

				elements.erase(elements.begin() + i);
				elements.insert(elements.begin() + i, resem.begin(), resem.end());

				values.insert(values.begin() + v, val.begin(), val.end());
				strs.erase(strs.begin() + f);
				f--;
			}
			f++;
		}
		if (elements[i] == tvalue)
			v++;
	}

	elements.insert(elements.begin(), tgroupopen);
	elements.push_back(tgroupclose);

	while (find(elements.begin(), elements.end(), tgroupopen) != elements.end()) {
		unsigned int depth = 0, maxDepth = 0, groupBegin = 0, groupEnd = 0, valuesBegin = 0, valuesEnd = 0, varIndex = 0;
		for (unsigned int j = 0; j < elements.size(); j++) {
			if (elements[j] == tgroupopen) {
				depth++;
				if (depth >= maxDepth) {
					maxDepth = depth;
					groupBegin = j;
				}
			}
			else if (elements[j] == tgroupclose) {
				if (depth == maxDepth)
					groupEnd = j;
				depth--;
			}
		}

		for (unsigned int j = 0; j < groupBegin; j++)
			if (elements[j] == tvalue)
				valuesBegin++;
		
		valuesEnd = valuesBegin;
		for (unsigned int j = groupBegin + 1; j < groupEnd; j++)
			if (elements[j] == tvalue)
				valuesEnd++;

		vector<double> result = evaluateGroup
		(
			vector<element>(elements.begin() + groupBegin + 1, elements.begin() + groupEnd),
			vector<double>(values.begin() + valuesBegin, values.begin() + valuesEnd)
		);

		if (groupBegin > 0) {
			if (elements[groupBegin - 1] == tstring) {
				for (unsigned int i = 0; i < groupBegin - 1; i++)
					if (elements[i] == tstring)
						varIndex++;
				string var = strs[varIndex];

				if (functions.count(var)) {
					result = functions[var](result);
					strs.erase(strs.begin() + varIndex);
					groupBegin--;
				}
			}
		}

		vector<element> resem = vector<element>();

		for (unsigned int i = 0; i < result.size(); i++)
		{
			resem.push_back(tcomma);
			resem.push_back(tvalue);
		}
		resem.erase(resem.begin());
		
		elements.erase(elements.begin() + groupBegin, elements.begin() + groupEnd + 1);
		elements.insert(elements.begin() + groupBegin, resem.begin(), resem.end());

		values.erase(values.begin() + valuesBegin, values.begin() + valuesEnd);
		values.insert(values.begin() + valuesBegin, result.begin(), result.end());
	}

	return values;
}

tuple<vector<element>, vector<double>, vector<string>> parse(string expression) {
	vector<element> elements = vector<element>();
	vector<double> values = vector<double>();
	vector<string> vars = vector<string>();

	for (unsigned int i = 0; i < expression.length(); i++) {
		char& c = expression[i];
		if (c == ' ')
			continue;

		if (isdigit(c) || c == '.')
		{
			string number = "";

			for (unsigned int a = i; a < expression.length(); a++) {
				char& n = expression[a];
				if (isdigit(n) || n == '.')
					number += n;
				else break;
			}

			i += number.length() - 1;
			values.push_back(atof(number.c_str()));
			elements.push_back(tvalue);
		}

		if (isalpha(c))
		{
			string str = "";

			for (unsigned int a = i; a < expression.length(); a++) {
				char& n = expression[a];
				if (isdigit(n) || isalpha(n) || n == '[' || n == ']')
					str += n;
				else break;
			}

			i += str.length() - 1;

			vars.push_back(str);
			elements.push_back(tstring);
		}

		switch (c)
		{
		default:
			break;
		case '+':
			elements.push_back(tplus);
			break;
		case '-':
			elements.push_back(tminus);
			break;
		case '/':
			elements.push_back(tdivide);
			break;
		case '*':
			elements.push_back(tmultiply);
			break;
		case '(':
			elements.push_back(tgroupopen);
			break;
		case ')':
			elements.push_back(tgroupclose);
			break;
		case ',':
			elements.push_back(tcomma);
			break;
		}
	}

	return tuple<vector<element>, vector<double>, vector<string>>(elements, values, vars);
}

vector<double>& var(string key) {
	if (!variables.count(key))
		addvar(key);
	return variables[key];
}

bool getBrackets(vector<string>& lines, int start, pair<int, int>& brackets) {
	int depth = 0, maxDepth = 0, groupBegin = 0, groupEnd = 0;
	for (unsigned int j = start; j < lines.size(); j++) {
		if (lines[j].find('{') != string::npos) {
			depth++;
			if (depth >= maxDepth) {
				maxDepth = depth;
				groupBegin = j;
			}
		}
		else if (lines[j].find('}') != string::npos) {
			if (depth == maxDepth)
				groupEnd = j;
			depth--;
		}
	}

	if (depth == 0)
	{
		brackets = pair<int, int>(groupBegin, groupEnd);
		return true;
	}
	return false;
}

void handleLine(vector<string>& lines, int lineIndex, int& iterator) {
	string line = lines[lineIndex];
	if (line.size() == 0)
		return;
	for (int i = 0; line[i] != '\0'; i++)
		if (line[i] == ' ')
			line.erase(line.begin() + i);
	string key, expr, assign = ":=", append = ",=", add = "+=", sub = "-=", mult = "*=", div = "/=", incr = "++", decr = "--";
	int index = 0;
	if (splitByDelimiter(line, assign, key, expr)) {
		vector<double> val = evaluate(parse(expr));
		if (hasIndex(key, index))
			var(key)[index] = val[0];
		else
			var(key) = val;
	}
	else if (splitByDelimiter(line, append, key, expr)) {
		vector<double> val = evaluate(parse(expr));
		var(key).insert(var(key).end(), val.begin(), val.end());
	}
	else if (splitByDelimiter(line, add, key, expr)) {
		vector<double> val = evaluate(parse(expr));
		if (hasIndex(key, index))
			var(key)[index] += val[0];
		else
			if (val.size() == var(key).size())
				for (unsigned int i = 0; i < val.size(); i++)
					var(key)[i] += val[i];
			else
				for (auto& i : var(key))
					i += val[0];
	}
	else if (splitByDelimiter(line, sub, key, expr)) {
		vector<double> val = evaluate(parse(expr));
		if (hasIndex(key, index))
			var(key)[index] -= val[0];
		else
			if (val.size() == var(key).size())
				for (unsigned int i = 0; i < val.size(); i++)
					var(key)[i] -= val[i];
			else
				for (auto& i : var(key))
					i -= val[0];
	}
	else if (splitByDelimiter(line, mult, key, expr)) {
		vector<double> val = evaluate(parse(expr));
		if (hasIndex(key, index))
			var(key)[index] *= val[0];
		else
			if (val.size() == var(key).size())
				for (unsigned int i = 0; i < val.size(); i++)
					var(key)[i] *= val[i];
			else
				for (auto& i : var(key))
					i *= val[0];
	}
	else if (splitByDelimiter(line, div, key, expr)) {
		vector<double> val = evaluate(parse(expr));
		if (hasIndex(key, index))
			var(key)[index] /= val[0];
		else
			if (val.size() == var(key).size())
				for (unsigned int i = 0; i < val.size(); i++)
					var(key)[i] /= val[i];
			else
				for (auto& i : var(key))
					i /= val[0];
	}
	else if (splitByDelimiter(line, incr, key, expr)) {
		if (hasIndex(key, index))
			var(key)[index] ++;
		else
			for (auto& i : var(key))
				i++;
	}
	else if (splitByDelimiter(line, decr, key, expr)) {
		if (hasIndex(key, index))
			var(key)[index] --;
		else
			for (auto& i : var(key))
				i--;
	}
	else if (line.find("if(") != string::npos && line.find(")") != string::npos){
		string condition = line.substr(3, (line.size() - 4));
		for (int i = lineIndex; i < lines.size(); i++) {
			pair<int, int> brackets; 
			if (getBrackets(lines, i, brackets)) {
				iterator = (evaluate(parse(condition))[0] >= 1) ? brackets.first : brackets.second;
			}
		}
	}
	else if (line == "quit."){
		iterator = lines.size();
	}
	else if (line.substr(0, 4) == "jump")
	{
		int targetLine = stoi(line.substr(4, line.size() - 4).c_str()) - 1;
		iterator = targetLine;
	}
	else if (line != "{" && line != "}"){
		evaluate(parse(line));
	}
}

int main(int argc, char* argv[])
{
	addfunc("sin", [](vector<double> args) {
		return vector<double> { sin(args[0]) };
		});

	addfunc("cos", [](vector<double> args) {
		return vector<double> { cos(args[0]) };
		});

	addfunc("atan2", [](vector<double> args) {
		return vector<double> { atan2(args[0], args[1]) };
		});

	addfunc("asin", [](vector<double> args) {
		return vector<double> { asin(args[0]) };
		});

	addfunc("acos", [](vector<double> args) {
		return vector<double> { acos(args[0]) };
		});

	addfunc("tan", [](vector<double> args) {
		return vector<double> { tan(args[0]) };
		});

	addfunc("atan", [](vector<double> args) {
		return vector<double> { atan(args[0]) };
		});

	addfunc("print", [](vector<double> args) {
		cout << "[";
		for (auto& a : args)
			cout << a << ", ";
		cout << (args.size() > 0 ? "\b\b]" : "]");
		return vector<double> { 0 };
		});	

	addfunc("println", [](vector<double> args) {
		cout << "[";
		for (auto& a : args)
			cout << a << ", ";
		cout << (args.size() > 0 ? "\b\b]" : "]") << endl;
		return vector<double> { 0 };
		});

	addfunc("input", [](vector<double> args) {
		string input = "";
		getline(cin, input);
		return evaluate(parse(input));
		});

	addfunc("average", [](vector<double> args) {
		double avg = 0;
		for (auto& i : args)
			avg += i;
		avg /= args.size();
		return vector<double> { avg };
		});

	addfunc("max", [](vector<double> args) {
		double max = 0;
		for (auto& i : args)
			if (i > max)
				max = i;
		return vector<double> { max };
		});

	addfunc("min", [](vector<double> args) {
		double min = 0;
		for (auto& i : args)
			if (i < min)
				min = i;
		return vector<double> { min };
		});

	addfunc("summ", [](vector<double> args) {
		double summ = 0;
		for (auto& i : args)
			summ += i;
		return vector<double> { summ };
		});

	addvar("pi", 3.141592653589793238462643383279502884L);

	ifstream file;
	string line;
	file.open(argv[1]);
	vector<string> lines = vector<string>();
	while (getline(file, line))
		lines.push_back(line);
	file.close();

	for (int i = 0; i < lines.size(); i++) {
		handleLine(lines, i, i);
	}
}