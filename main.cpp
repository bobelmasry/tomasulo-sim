#include "reservationStation.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
using namespace std;

string trim(string s) {
    // Remove leading spaces
    while (!s.empty() && isspace(s.front()))
        s.erase(s.begin());

    // Remove trailing spaces
    while (!s.empty() && isspace(s.back()))
        s.pop_back();

    return s;
}

struct ParsedInstruction {
    string opcode;
    vector<string> operands;   // rA, rB, rC, offset, label, etc.
};

ParsedInstruction parseLine(const string& line) {
    ParsedInstruction inst;

    string cleaned = line;
    // Remove commas
    for (char& c : cleaned) {
        if (c == ',') c = ' ';
    }

    // Tokenize by spaces
    stringstream ss(cleaned);
    ss >> inst.opcode;  // First token is always opcode (LOAD, ADD, BEQ...)

    string token;
    while (ss >> token) {
        inst.operands.push_back(token);
    }

    return inst;
}

vector<string> readInstructions(const string& filename) {
    ifstream infile(filename);
    vector<string> instructions;
    string line;

    if (!infile) {
        cerr << "Error: Could not open file " << filename << "\n";
        return instructions;
    }

    while (getline(infile, line)) {
        line = trim(line);

        // skip empty lines
        if (line.empty()) continue;

        // Skip comments '#'
        if (line[0] == '#') continue;

        instructions.push_back(line);
    }

    infile.close();
    return instructions;
}

int main() {

    // Initializing all the reservation stations
    ReservationStation Load1("Load1");
    ReservationStation Load2("Load2");

    ReservationStation Store("Store");

    ReservationStation Branch1("Branch1");
    ReservationStation Branch2("Branch2");

    ReservationStation Call("Call");

    ReservationStation Add_Sub1("Add/Sub1");
    ReservationStation Add_Sub2("Add/Sub2");
    ReservationStation Add_Sub3("Add/Sub3");
    ReservationStation Add_Sub4("Add/Sub4");

    ReservationStation NAND1("NAND1");
    ReservationStation NAND2("NAND2");
    
    ReservationStation Mul("Mul");
    
    vector<string> instructions = readInstructions("instructions.txt");

    for (const string& inst : instructions) {
        ParsedInstruction p = parseLine(inst);

        cout << "Instruction: " << inst << "\n";
        cout << "  Opcode: " << p.opcode << "\n";

        // Print operands
        for (int i = 0; i < p.operands.size(); i++) {
            cout << "  Operand " << i << ": " << p.operands[i] << "\n";
        }

        cout << "\n";
    }

    
    return 0;
}