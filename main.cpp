#include "reservationStation.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
using namespace std;

int getRegisterNumber(const string& reg) {
    if (reg.size() < 2 || reg[0] != 'r')
        return -1; // does not have an r

    return stoi(reg.substr(1));  // convert everything after 'r'
}

struct MemAddress {
    int offset;
    int baseReg;
};

// input: "4(r2)"
MemAddress decodeMemAddress(const string& op) {
    MemAddress m{0, 0};

    // get '(' and ')'
    size_t l = op.find('(');
    size_t r = op.find(')');

    // offset
    m.offset = stoi(op.substr(0, l));

    // base register name: r2
    string reg = op.substr(l + 1, r - l - 1);
    m.baseReg = getRegisterNumber(reg);

    return m;
}

int encodeSigned5(int value) {
    // make sure value fits in 5 bits
    if (value < -16 || value > 15) {
        cout << "Offset out of 5-bit signed range\n";
        return 0;
    }

    // make it a 5-bit two's complement
    if (value < 0)
        value = (1 << 5) + value;  // add 32 to wrap

    return value & 0x1F;  // keep only 5 bits
}


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
enum Operation {
    LOAD,
    STORE,
    BEQ,
    CALL,
    RET,
    ADD,
    SUB,
    NAND,
    MUL
};
struct Instruction{
    Operation OP;
    int A;
    int Operand1, Operand2, Operand3;
};

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

Operation stringOpSwitcher(string opcode){
    if(opcode == "LOAD")
        return LOAD;
    if(opcode == "STORE")
        return STORE;
    if(opcode == "CALL")
        return LOAD;
    if(opcode == "RET")
        return LOAD;
    if(opcode == "BEQ")
        return LOAD;
    if(opcode == "ADD")
        return LOAD;
    if(opcode == "SUB")
        return LOAD;
    if(opcode == "NAND")
        return LOAD;
    if(opcode == "MUL")
        return LOAD;  
}

void parseOffset(Instruction& instr, string Operand){
    int opLen = Operand.length();
    instr.Operand2 = stoi(Operand.substr(opLen-2, 1));
    size_t l = Operand.find('(');
    instr.A = stoi(Operand.substr(0, l));
}

void assembleInstructions(vector<Instruction>& exec, vector<string> instrs){
    for (const string& inst : instrs) {
        ParsedInstruction p = parseLine(inst);
        Instruction temp;
        cout << "Instruction: " << inst << "\n";
        cout << "  Opcode: " << p.opcode << "\n";
        switch(stringOpSwitcher(p.opcode)){
            case LOAD:
                temp.OP = LOAD;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                parseOffset(temp, p.operands[1]);
                break;
            case STORE:
                temp.OP = STORE;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                parseOffset(temp, p.operands[1]);
                break;
            case BEQ:
                temp.OP = BEQ;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                temp.Operand2 = stoi(p.operands[1].substr(1));
                temp.A = stoi(p.operands[2]);
                break;
            case CALL:
                temp.OP = CALL;
                temp.A = stoi(p.operands[0]);
                break;
            case RET:
                temp.OP = CALL;
                break;
            case ADD:
                temp.OP = ADD;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                temp.Operand2 = stoi(p.operands[1].substr(1));
                temp.Operand3 = stoi(p.operands[2].substr(1));
                break;
            case SUB:
                temp.OP = SUB;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                temp.Operand2 = stoi(p.operands[1].substr(1));
                temp.Operand3 = stoi(p.operands[2].substr(1));
                break;
            case NAND:
                temp.OP = NAND;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                temp.Operand2 = stoi(p.operands[1].substr(1));
                temp.Operand3 = stoi(p.operands[2].substr(1));
                break;
            case MUL:
                temp.OP = MUL;
                temp.Operand1 = stoi(p.operands[0].substr(1));
                temp.Operand2 = stoi(p.operands[1].substr(1));
                temp.Operand3 = stoi(p.operands[2].substr(1));
                break;
        }
        cout << "\n";
    }
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
    vector<Instruction> Executables;
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