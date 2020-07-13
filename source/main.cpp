#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <switch.h>

using namespace std;

int main() {
	bool showingWelcomeMessage = true;

	vector<char> code;
	string output = "";

	unsigned long cursorPosition = 0;
	enum{edit, run, input} currentState = edit;

	vector<char> cells;
	vector<int> loops;

	unsigned long cellPosition = 0;
	unsigned long codePosition = 0;
	unsigned long previousCursorPosition = 0;

	bool skipLoop;
	string error;

	unsigned long startTime;

	pair<unsigned long, unsigned long> keyPosition(0, 0);
	vector<vector<string>> keyboardKeys{
		{"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"},
		{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"},
		{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", ":"},
		{";", "<", "=", ">", "?", "@", "[", "\\", "]", "^", "_", "`", "{", "|", "}", "~"},
		{"(Space)", "(Tab)", "(Newline)", "(Escape)", "(Null)"}
	};

	while (appletMainLoop()) {
		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

		if (kDown != 0 && showingWelcomeMessage) {
			showingWelcomeMessage = false;
		}

		if (kDown & KEY_PLUS) { break; }

		vector<vector<pair<string, string>>> controls;
		if (currentState == edit) {
			controls = {
					{
							{"+", "Exit"},
							{"-", "Run "}
					},
					{
							{"A", ">"},
							{"Y", "<"}
					},
					{
							{"X", "+"},
							{"B", "-"}
					},
					{
							{"Down", "."},
							{"Up  ", ","}
					},
					{
							{"Left ", "["},
							{"Right", "]"}
					},
					{
							{"L", "Move Left "},
							{"R", "Move Right"}
					},
					{
							{"ZL   ", "Backspace"},
							{"ZL+ZR", "Clear    "}
					}
			};

			if (kDown & KEY_MINUS) {
				output.clear();

				cells = {0};
				loops = {};

				cellPosition = 0;
				codePosition = 0;
				previousCursorPosition = cursorPosition;

				skipLoop = false;
				error = "";

				startTime = chrono::system_clock::now().time_since_epoch() / chrono::milliseconds(1);
				currentState = run;
			}

			map<HidControllerKeys, char> keys{
					{KEY_A, '>'},
					{KEY_Y, '<'},
					{KEY_X, '+'},
					{KEY_B, '-'},
					{KEY_DDOWN, '.'},
					{KEY_DUP, ','},
					{KEY_DLEFT, '['},
					{KEY_DRIGHT, ']'}
			};

			for (pair<HidControllerKeys, char> key : keys) {
				if (kDown & key.first) {
					code.insert(code.begin() + cursorPosition, key.second);
					++cursorPosition;
				}
			}

			if ((kDown & KEY_L) && cursorPosition != 0) { cursorPosition--; }
			if ((kDown & KEY_R) && cursorPosition < code.size()) { cursorPosition++; }

			if ((kDown & KEY_ZL) && !code.empty()) {
				if (kHeld & KEY_ZR) {
					code.clear();
					cursorPosition = 0;
				} else if (cursorPosition > 0) {
					code.erase(code.begin() + cursorPosition - 1);
					--cursorPosition;
				}
			}
		} else if (currentState == run) {
			controls = {
					{
							{"+", "Exit"},
							{"-", "Stop"}
					}
			};

			if (kDown & KEY_MINUS) {
				error = "Code execution was manually stopped.";
			}

			if (codePosition >= code.size() || error != "") {
				if (error != "") {
					output += "\n\x1b[37;41mE: " + error + "\x1b[0m";
				} else {
					unsigned long endTime = chrono::system_clock::now().time_since_epoch() / chrono::milliseconds(1);
					float elapsedTime = float(endTime - startTime) / 1000;
					string timeString = to_string(elapsedTime).substr(0, to_string(elapsedTime).find(".") + 3);
					output += "\n\x1b[37;42mCode execution finished successfully in " + timeString + "s.\x1b[0m";
				}

				cursorPosition = previousCursorPosition;
				currentState = edit;
			} else {
				cursorPosition = codePosition;
				char codeChar = code[codePosition];

				if (skipLoop) {
					if (codeChar == ']') { skipLoop = false; }
				} else if (codeChar == '>') {
					if (cellPosition == cells.size() - 1) {
						cells.push_back(0);
					}

					++cellPosition;
				} else if (codeChar == '<') {
					if (cellPosition == 0) {
						error = "You can't move to cells before the first cell.";
						continue;
					}

					--cellPosition;
				} else if (codeChar == '+') {
					++cells[cellPosition];

					while (code[codePosition + 1] == '+') {
						++cells[cellPosition];
						++codePosition;
					}
				} else if (codeChar == '-') {
					--cells[cellPosition];

					while (code[codePosition + 1] == '-') {
						--cells[cellPosition];
						++codePosition;
					}
				} else if (codeChar == '.') {
					output += cells[cellPosition];

					while (code[codePosition + 1] == '.') {
						output += cells[cellPosition];
						++codePosition;
					}
				} else if (codeChar == ',') {
					currentState = input;
				} else if (codeChar == '[') {
					if (cells[cellPosition] == 0) { skipLoop = true; }
					else { loops.push_back(codePosition); }
				} else if (codeChar == ']') {
					if (loops.empty()) {
						error = "You can't close a loop if there are no open loops.";
						continue;
					}

					if (cells[cellPosition] == 0) { loops.pop_back(); }
					else { codePosition = loops.back(); }
				}

				++codePosition;
			}
		} else if (currentState == input) {
			controls = {
					{
							{"+", "Exit"},
							{"-", "Stop"}
					},
					{
							{"Down", "Move Down"},
							{"Up  ", "Move Up  "}
					},
					{
							{"Left ", "Move Left "},
							{"Right", "Move Right"}
					},
					{
							{"A", "Select"},
							{" ", "      "}
					}
			};

			if (kDown & KEY_MINUS) {
				error = "Code execution was manually stopped.";
				currentState = run;
				continue;
			}

			if ((kDown & KEY_DDOWN) && keyPosition.second < keyboardKeys.size() - 1) {
				++keyPosition.second;
			}
			if ((kDown & KEY_DUP) && keyPosition.second > 0) {
				--keyPosition.second;
			}
			if ((kDown & KEY_DLEFT) && keyPosition.first > 0) {
				--keyPosition.first;
			}
			if ((kDown & KEY_DRIGHT) && keyPosition.first < keyboardKeys[keyPosition.second].size() - 1) {
				++keyPosition.first;
			}

			if (kDown & KEY_A) {
				string selectedKey = keyboardKeys[keyPosition.second][keyPosition.first];
				char keyChar;

				if (selectedKey == "(Space)") { keyChar = ' '; }
				else if (selectedKey == "(Tab)") { keyChar = 9; }
				else if (selectedKey == "(Newline)") { keyChar = '\n'; }
				else if (selectedKey == "(Escape)") { keyChar = 27; }
				else if (selectedKey == "(Null)") { keyChar = 0; }
				else { keyChar = selectedKey.front(); }

				cells[cellPosition] = keyChar;
				currentState = run;
			}
		}

		consoleInit(nullptr);

		for (unsigned long i = 0; i < code.size(); ++i) {
			cout << string(cursorPosition == i ? "\x1b[30;47m" : "") << code[i] << string(cursorPosition == i ? "\x1b[0m" : "");

			if (i + 1 == code.size() && cursorPosition == code.size()) {
				cout << "\x1b[47m \x1b[0m";
			}
		}

		if (code.empty()) {
			cout << "\x1b[47m \x1b[0m";
		}

		if (showingWelcomeMessage) {
			cout << "                           \x1b[30;47m[ Welcome to Brewfuck. ]\x1b[0m";
		}

		int emptyEditorLines = 27 - ((code.size() / 80) + count(code.begin(), code.end(), '\n'));
		for (int i = 0; i < emptyEditorLines; ++i) {
			cout << "\n";
		}

		cout << "\x1b[47m";
		for (int i = 0; i < 80; ++i) {
			cout << " ";
		}
		cout << "\x1b[0m";

		cout << output;

		if (currentState != input) {
			int emptyOutputLines = 15 - ((output.size() / 80) + count(output.begin(), output.end(), '\n'));
			for (int i = 0; i < emptyOutputLines; ++i) {
				cout << "\n";
			}
		} else {
			cout << "\n";
			for (vector<string> line : keyboardKeys) {
				cout << " ";
				for (string key : line) {
					bool isSelected = keyboardKeys[keyPosition.second][keyPosition.first] == key;

					if (isSelected) { cout << "\x1b[30;47m"; }
					cout << key;
					if (isSelected) { cout << "\x1b[0m"; }

					if (key != line.back()) { cout << "  "; }
				}

				cout << "\n\n";
				if (line != keyboardKeys.back()) { cout << "\n"; }
			}
		}

		for (int i = 0; i < 2; ++i) {
			string row = "";
			for (vector<pair<string, string>> column : controls) {
				string trimmedKey = column[i].first;
				while (trimmedKey.back() == ' ') {
					trimmedKey.pop_back();
				}

				row += (trimmedKey != "" ? "\x1b[30;47m" : "") + column[i].first + "\x1b[0m " + column[i].second + "    ";
			}

			while (row.back() == ' ') {
				row.pop_back();
			}

			cout << row;
			if (i < (2 - 1)) { cout << "\n"; }
		}

		consoleUpdate(nullptr);
	}

	consoleExit(nullptr);
	return 0;
}
