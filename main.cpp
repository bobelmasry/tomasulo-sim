#include "reservationStation.cpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <queue>
#include <cstdint>
using namespace std;

static const int MEM_SIZE = 65536;  // number of 16-bit words
vector<uint16_t> memory(MEM_SIZE, 0);

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
string enum_operations[9] = {
    "LOAD",
    "STORE",
    "BEQ",
    "CALL",
    "RET",
    "ADD",
    "SUB",
    "NAND",
    "MULL"
};

struct CycleCost {
    int load = 6;
    int store =6;
    int beq = 1;
    int call = 1;
    int ret = 1;
    int add = 2;
    int sub = 2;
    int nand = 1;
    int mul = 12;
};

CycleCost cycles;

struct Instruction{
    Operation OP;
    int A;
    int Operand1, Operand2, Operand3;
    int tIssue, tExecute, tWrite, tCommit;
    bool renamed = false;
};



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

ReservationStation* reservationStations[13] = {
    &Load1, &Load2, &Store, &Branch1, &Branch2, &Call,
    &Add_Sub1, &Add_Sub2, &Add_Sub3, &Add_Sub4,
    &NAND1, &NAND2, &Mul
};

//Initialize PC and cycle counter
int PC = 0;
int tCycle = 0;

//Initialize the register array and create renaming functions
int registers[128];
int registersDependency[128];
int registersAddr[8];
int topRegisterChanged = 8;
void initRegisterRenaming(){
    for(int i = 0; i < 8; i++){
        registersAddr[i] = i;
    }
}
void initRegisterDependency(){
    for(int i = 0; i < 128; i++){
        registersDependency[i] = -1;
    }
}


// =============================
//  Reorder Buffer (ROB)
//  Circular Queue – Size 8
// =============================

struct ROB_Entry {
    int Num;        // ROB slot index
    Operation Type; // Instruction type
    int Dest;       // Destination register or tag
    int Val;        // Value if ready
    bool Ready;     // Has CDB written the result?
};

// Fixed-size ROB
const int ROB_SIZE = 8;
ROB_Entry ROB[ROB_SIZE];

// Circular queue pointers
int ROB_head = 0;     // Points to oldest instruction (to commit)
int ROB_tail = 0;     // Points to next free entry (to allocate)
int ROB_count = 0;    // Number of valid entries in the ROB


// --------------------------------------------
// Check if ROB is full
// --------------------------------------------
bool isROBFull() {
    return ROB_count == ROB_SIZE;
}

// --------------------------------------------
// Check if ROB is empty
// --------------------------------------------
bool isROBEmpty() {
    return ROB_count == 0;
}

// --------------------------------------------
// Insert a new instruction into the ROB
// Returns: ROB index (tag), or -1 if no space
// --------------------------------------------
int insertInstructionToROB(Operation op, int dest) {

    if (isROBFull()) {
        return -1;   // Insertion failed → ROB full
    }

    int index = ROB_tail;

    ROB[index].Num   = index;
    ROB[index].Type  = op;
    if(op == BEQ || op == CALL || op == RET || op == STORE){
        ROB[index].Dest  = -1;
        //registersDependency[dest] = index;    
    }else{
        ROB[index].Dest  = dest;
        registersDependency[dest] = index;
    }
    ROB[index].Ready = false;

    // Move tail forward in circular buffer
    ROB_tail = (ROB_tail + 1) % ROB_SIZE;
    ROB_count++;

    return index;    // Return ROB tag for RS
}

// --------------------------------------------
// Write a value to an ROB entry (from CDB)
// --------------------------------------------
void writeROB(int robIndex, int value) {
    ROB[robIndex].Val = value;
    ROB[robIndex].Ready = true;
}

// --------------------------------------------
// Commit oldest instruction from ROB (pop head)
// Note: You can add register writeback logic here
// --------------------------------------------
void removeInstructionFromROB() {
    if (isROBEmpty())
        return;

    int index = ROB_head;

    // Commit logic would be inserted here:
    // e.g., commit ROB[index].Val to register file

    // Move head forward in circular buffer
    ROB_head = (ROB_head + 1) % ROB_SIZE;
    ROB_count--;
}

// --------------------------------------------
// Clear the ROB on misprediction / flush
// --------------------------------------------
void clearROB() {
    ROB_head = 0;
    ROB_tail = 0;
    ROB_count = 0;

    // Optional: reset entries
    for (int i = 0; i < ROB_SIZE; i++) {
        ROB[i].Ready = false;
    }
}

//Return the ROB destination holding the desired j or k
int returnOpDest(int operandAddr){
    for(int i = 0; i < 8; i++){
        if(ROB[i].Dest == -1)
            continue;
        if(ROB[i].Dest == operandAddr)
            return i;
    }
    return -1;
}



//Register renaming
//Rules: no changning R1 or R0
void renameRegisters(Instruction& it) {
    it.renamed = true;
    // skip CALL / RET
    if (it.OP == CALL || it.OP == RET)
        return;

    // destination rename only for instructions that write a register
    if ((it.OP == LOAD || it.OP == ADD || it.OP == SUB || it.OP == NAND || it.OP == MUL) &&
        (it.Operand1 != -1 && it.Operand1 != 0 && it.Operand1 != 1))
    {
        registersAddr[it.Operand1] = topRegisterChanged;
        it.Operand1 = topRegisterChanged;
        topRegisterChanged++;
    }

    // rename sources
    if (it.Operand2 != -1 && it.Operand2 != 0 && it.Operand2 != 1) {
        it.Operand2 = registersAddr[it.Operand2];
    }

    if (it.Operand3 != -1 && it.Operand3 != 0 && it.Operand3 != 1) {
        it.Operand3 = registersAddr[it.Operand3];
    }
}

uint16_t memRead(uint16_t address) {
    if (address >= MEM_SIZE) {
        cerr << "Memory Read Error: Address out of range (" << address << ")\n";
        return 0;
    }
    return memory[address];
}

void memWrite(uint16_t address, uint16_t value) {
    if (address >= MEM_SIZE) {
        cerr << "Memory Write Error: Address out of range (" << address << ")\n";
        return;
    }
    memory[address] = value;
}

void executeInstruction() {
    for (int i = 0; i < 13; i++) {
    ReservationStation* rs = reservationStations[i];

    if (rs->isBusy()){
        if(rs->getQj() != 0 || rs->getQk() != 0)
            continue;
        instrRSROB* matchedEntry = nullptr;
        for (auto& entry : Complexes) {
            if (entry.rs == rs) {
                matchedEntry = &entry;
                break;
            }
        }
        Instruction* instrReference = matchedEntry->instr;
        Instruction instr = *(instrReference);

        ROB_Entry* rob_entry = matchedEntry->rb;

    switch (rs->getOp()) {
        if (instr.tExecute == tCycle) {
        case LOAD: {
            int addr = registers[instr.Operand2] + instr.A;   // base + offset
            rs->setA(addr);
            rs->setVj(0);
            rs->setVk(0);
            rob_entry->Val = memRead(addr);
            break;
        }

        case STORE: {//Implement store buffer
            int addr = registers[instr.Operand2] + instr.A;
            rs->setA(addr);
            rs->setVj(0);
            rs->setVk(0);
            break;
        }

        case BEQ: {
            if (registers[instr.Operand1] == registers[instr.Operand2]) {
                rob_entry->Val = instr.A;     // taken
                rob_entry->Dest = 0;
            } else{
                rob_entry->Dest = 1;
            }
            rob_entry->Ready = true;
            break;
        }

        case CALL: {
            rob_entry->Val = PC + 1;         // R1 = return address
            rob_entry->Dest = PC + instr.A;             // jump (relative)
            break;
        }

        case RET: {
            rob_entry->Val = registers[1];             // jump to stored return address
            rob_entry->Ready = true;
            break;
        }

        case ADD: {
            rob_entry->Val = instr.Operand2 + instr.Operand3;
            rob_entry->Ready = true;
            break;
        }

        case SUB: {
            rob_entry->Val = instr.Operand2 - instr.Operand3;
            rob_entry->Ready = true;
            rob_entry->Dest = instr.Operand1;
            break;
        }

        case NAND: {
            rob_entry->Val = ~(instr.Operand2 & instr.Operand3);
            rob_entry->Ready = true;
            rob_entry->Dest = instr.Operand1;
            break;
        }

        case MUL: {
            uint32_t result =
                (uint32_t)instr.Operand2 *
                (uint32_t)instr.Operand3;

            rob_entry->Val = (uint16_t)(result & 0xFFFF);
            rob_entry->Ready = true;
            rob_entry->Dest = instr.Operand1;
            break;
        }
    }
    }
}
    }
}

//Commit the only current Head ROB when ready
void commitROBs(){
    int p = ROB_head;
    if(ROB[p].Ready){
        ROB_Entry* rb;
        Instruction* instr;
        ReservationStation* rs;
        instrRSROB complex;
        for(auto t : Complexes){
            if(t.rb == &(ROB[p])){
                rb = t.rb;
                rs = t.rs;
                instr = t.instr;
                complex = t;
                break;
            }
        }
        instr->tCommit = tCycle;
        rs->clear();
        switch(rb->Type){
            case LOAD: 
                registers[rb->Dest] = rb->Val;
                registersDependency[rb->Dest] = -1;
                break;
            case STORE: //Store buffer
                break;
            case BEQ:
                PC = (rb->Dest == 0) ? rb->Val : PC;
                if(rb->Dest == 0){
                    clearROB();
                }
                break;
            case CALL:
                PC = rb->Val;
                clearROB();
                break;
            case RET:
                PC = registers[1];
                clearROB();
                break;
            case ADD:
                registers[rb->Dest] = rb->Val;
                registersDependency[rb->Dest] = -1;
                break;
            case SUB:
                registers[rb->Dest] = rb->Val;
                registersDependency[rb->Dest] = -1;
                break;
            case NAND:
                registers[rb->Dest] = rb->Val;
                registersDependency[rb->Dest] = -1;
                break;
            case MUL:
                registers[rb->Dest] = rb->Val;
                registersDependency[rb->Dest] = -1;
                break;
        }
        //Send the needed data of the ROB thru CDB to the ReservationStations (and calculate tExecute and tWrite)
        for (int i = 0; i < 13; i++) {
            ReservationStation* rs = reservationStations[i];
            if(rs->getQj() == rb->Num || rs->getQk() == rb->Num){
                initiateRSfromROB(rs, rb, instr);
            }
        }
        //delete the complex from the complexes vector
        Complexes.erase(find(Complexes.begin(), Complexes.end(), complex));

        //Delete ROB
        removeInstructionFromROB();
    }
}

void initiateRSfromROB(ReservationStation*& rs, ROB_Entry* rb, Instruction*& instr){
    CycleCost cc;
    if(rb->Num == rs->getQj()){
        rs->setQj(0);
        rs->setVj(rb->Val);
    }
    if(rb->Num == rs->getQk()){
        rs->setQk(0);
        rs->setVk(rb->Val);
    }
    if(rs->getQj() == 0 && rs->getQk() == 0){
        int tExecution = tCycle;
        switch(instr->OP){
            case LOAD:
                tExecution+=cc.load;
                break;
            case STORE:
                tExecution+=cc.store;
                break;
            case BEQ:
                tExecution+=cc.beq;
                break;
            case CALL:
                tExecution+=cc.call;
                break;
            case RET:
                tExecution+=cc.ret;
                break;
            case ADD:
                tExecution+=cc.add;
                break;
            case SUB:
                tExecution+=cc.sub;
                break;
            case NAND:
                tExecution+=cc.nand;
                break;
            case MUL:
                tExecution+=cc.mul;
                break;
        }
        instr->tExecute = tExecution;
        instr->tWrite = tExecution+1;
    }
}

/// Everything from here to the next big comment is all about importing instructions and changing them into a usable format
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



void printInstruction(Instruction i){
    cout << "Type: " << i.OP << endl;
    cout << "A: " << i.A << endl;
    cout << "Op1: " << i.Operand1 << endl;
    cout << "Op2: " << i.Operand2 << endl;
    cout << "Op3: " << i.Operand3 << endl; 
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
        return CALL;
    if(opcode == "RET")
        return RET;
    if(opcode == "BEQ")
        return BEQ;
    if(opcode == "ADD")
        return ADD;
    if(opcode == "SUB")
        return SUB;
    if(opcode == "NAND")
        return NAND;
    if(opcode == "MUL")
        return MUL;  
}

void parseOffset(Instruction& instr, string Operand){
    MemAddress m = decodeMemAddress(Operand);
    instr.A = m.offset;
    instr.Operand2 = m.baseReg;
}


void assembleInstructions(vector<Instruction>& exec, const vector<string>& instrs) {
    for (const string& inst : instrs) {
        ParsedInstruction p = parseLine(inst);
        Instruction temp;

        try {
            //cout << "Instruction: " << inst << "\n  Opcode: " << p.opcode << "\n";
            temp.Operand3 = -1;
            temp.A = 0;
            temp.Operand1 = -1;
            temp.Operand2 = -1;
            switch (stringOpSwitcher(p.opcode)) {

                case LOAD:
                    temp.OP = LOAD;
                    temp.Operand1 = getRegisterNumber(p.operands.at(0));
                    parseOffset(temp, p.operands.at(1));
                    break;

                case STORE:
                    temp.OP = STORE;
                    temp.Operand1 = getRegisterNumber(p.operands.at(0));
                    parseOffset(temp, p.operands.at(1));
                    break;

                case BEQ:
                    temp.OP = BEQ;
                    temp.Operand1 = getRegisterNumber(p.operands.at(0));
                    temp.Operand2 = getRegisterNumber(p.operands.at(1));
                    temp.A = stoi(p.operands.at(2)); // branch target PC offset
                    break;

                case CALL:
                    temp.OP = CALL;
                    temp.A = stoi(p.operands.at(0)); // numeric only, no label handling
                    break;

                case RET:
                    temp.OP = RET;
                    break;

                case ADD:
                case SUB:
                case NAND:
                case MUL:
                    temp.OP = stringOpSwitcher(p.opcode);
                    temp.Operand1 = getRegisterNumber(p.operands.at(0));
                    temp.Operand2 = getRegisterNumber(p.operands.at(1));
                    temp.Operand3 = getRegisterNumber(p.operands.at(2));
                    break;
            }

            exec.push_back(temp);
        }
        catch (const out_of_range& e) {
            cerr << "Error: Missing operands in instruction -> " << inst << "\n";
        }
        catch (const invalid_argument& e) {
            cerr << "Error: stoi conversion failed for instruction -> " << inst << "\n";
        }
    }
}


//////Issue functions
struct instrRSROB{
    Instruction* instr;
    ReservationStation* rs;
    ROB_Entry* rb;
    
};
void printComplex(instrRSROB it){
    cout << "Type: " << enum_operations[it.instr->OP] << endl;
    cout << "Reservation Station: " << it.rs->getName() << endl;
    cout << "ROB entry: " << it.rb->Num << endl;
}
vector<instrRSROB> Complexes;
//Check validity
bool checkIssueValidity(Instruction& it, ReservationStation*& rs){
    if(isROBFull()){
        return false;
    }else{
        switch (it.OP) {
            case LOAD: {
                if(Load1.isBusy() && Load2.isBusy())
                    return false;
                if(Load1.isBusy()){
                    rs = &Load2;
                }else{
                    rs = &Load1;
                }
                return true;
                break;
            }

            case STORE: {
                cout << Store.isBusy() << endl;
                if(Store.isBusy())
                    return false;
                else{
                    rs = &Store;
                }
                return true;
                break;
            }

            case BEQ: {
                if(Branch1.isBusy() && Branch2.isBusy())
                    return false;
                if(Branch1.isBusy()){
                    rs = &Branch2;
                }else{
                    rs = &Branch1;
                }
                return true;
                break;
            }

            case CALL: {
                if(Call.isBusy())
                    return false;
                else{
                    rs = &Call;
                }
                return true;
                break;
            }

            case RET: {
                if(Call.isBusy())
                    return false;
                else{
                    rs = &Call;
                }
                return true;
                break;
            }

            case ADD: {
                if(Add_Sub1.isBusy() && Add_Sub2.isBusy() && Add_Sub3.isBusy() && Add_Sub4.isBusy())
                    return false;
                if(!Add_Sub1.isBusy()){
                    rs = &Add_Sub1;
                }else if(!Add_Sub2.isBusy()){
                    rs = &Add_Sub2;
                }else if(!Add_Sub3.isBusy()){
                    rs = &Add_Sub3;
                }else{
                    rs = &Add_Sub4;
                }
                return true;
                break;
            }

            case SUB: {
                if(Add_Sub1.isBusy() && Add_Sub2.isBusy() && Add_Sub3.isBusy() && Add_Sub4.isBusy())
                    return false;
                if(!Add_Sub1.isBusy()){
                    rs = &Add_Sub1;
                }else if(!Add_Sub2.isBusy()){
                    rs = &Add_Sub2;
                }else if(!Add_Sub3.isBusy()){
                    rs = &Add_Sub3;
                }else{
                    rs = &Add_Sub4;
                }
                return true;
                break;
            }

            case NAND: {
                if(NAND1.isBusy() && NAND2.isBusy())
                    return false;
                if(NAND1.isBusy()){
                    rs = &NAND2;
                }else{
                    rs = &NAND1;
                }
                return true;
                break;
            }

            case MUL: {
                if(Mul.isBusy())
                    return false;
                else{
                    rs = &Mul;
                }
                return true;
                break;
            }
        }
        return false;
    }
}

bool isOperandReady(int operandAddr){
    if(registersDependency[operandAddr] == -1)
        return true;
    return false;
}


//Populate reservation station
instrRSROB populateReservationStation(Instruction& it, ReservationStation* rs, int rob_index){
    instrRSROB complex;
    complex.instr = &it;
    complex.rs = rs;
    complex.rs->setBusy(true);
    complex.rb = &(ROB[rob_index]);
    switch (it.OP) {
        case LOAD: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(LOAD);
            complex.rs->setA(it.A);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }

            break;
        }

        case STORE: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(STORE);
            complex.rs->setA(it.A);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }
            break;
        }

        case BEQ: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(BEQ);
            complex.rs->setA(it.A + PC + 1);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }
            if(isOperandReady(it.Operand1)){
                complex.rs->setVk(it.Operand1);
            }else{
                complex.rs->setQk(registersDependency[it.Operand1]);
            }
            
            break;
        }

        case CALL: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(CALL);
            complex.rs->setA(it.A + PC + 1);
            registers[1] = PC+1;
            break;
        }

        case RET: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(RET);
            //complex.rs->setA(it.A);
            break;
        }

        case ADD: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(ADD);
            //complex.rs->setA(it.A);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }
            if(isOperandReady(it.Operand3)){
                complex.rs->setVk(it.Operand3);
            }else{
                complex.rs->setQk(registersDependency[it.Operand3]);
            }
            
            break;
        }

        case SUB: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(SUB);
            //complex.rs->setA(it.A);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }
            if(isOperandReady(it.Operand3)){
                complex.rs->setVk(it.Operand3);
            }else{
                complex.rs->setQk(registersDependency[it.Operand3]);
            }
            
            break;
        }

        case NAND: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(NAND);
            //complex.rs->setA(it.A);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }
            if(isOperandReady(it.Operand3)){
                complex.rs->setVk(it.Operand3);
            }else{
                complex.rs->setQk(registersDependency[it.Operand3]);
            }
            
            break;
        }

        case MUL: {
            complex.rs->setDest(complex.rb->Num);
            complex.rs->setOp(MUL);
            //complex.rs->setA(it.A);
            if(isOperandReady(it.Operand2)){
                complex.rs->setVj(it.Operand2);
            }else{
                complex.rs->setQj(registersDependency[it.Operand2]);
            }
            if(isOperandReady(it.Operand3)){
                complex.rs->setVk(it.Operand3);
            }else{
                complex.rs->setQk(registersDependency[it.Operand3]);
            }
            
            break;
        }
    }
    return complex;
}

//Issue Instruction
void issueInstruction(Instruction& it){
    ReservationStation* rs = nullptr;
    
    if(checkIssueValidity(it, rs)){
        PC++;
        it.tIssue = tCycle;
        //rs->setBusy(true);
        //Create an entry in ROB
        //Need to add some functions in the ROB like searching for destination or something
        
        int rob_index = insertInstructionToROB(it.OP, it.Operand1);
        //Change values in the ReservationStation to contain info about the instruction
        //Before I forget, I am making this into its seperate function to handle the switch case
        //But it depends on the ROB_entry for Qj and Qk
        Complexes.push_back(populateReservationStation(it, rs, rob_index));
        printComplex(Complexes.at(Complexes.size()-1));
    }

}

int main() {
    
    //Import the instructions
    vector<string> instructions = readInstructions("instructions.txt");
    vector<Instruction> Executables;

    assembleInstructions(Executables, instructions);
    //Initialize register renaming
    initRegisterRenaming();
    initRegisterDependency();
    while(true){
        tCycle++;
        Instruction currentInstruction = Executables[PC];
        cout << tCycle << " " << PC << endl;

        if(!currentInstruction.renamed)
            renameRegisters(currentInstruction);
        issueInstruction(currentInstruction);
        executeInstruction();
        commitROBs();

        //To break out of the cycle
        if(PC >= Executables.size())
            break;
    }
    for(int i = 0; i < Executables.size(); i++){
        Instruction it = Executables[i];
        cout << "I" << i << endl;
        cout << "Type: " << enum_operations[it.OP] << endl;
        cout << "Timings" << endl;
        cout << "Issued: " << it.tIssue << ", Executed: " << it.tExecute << ", Written: " << it.tWrite << ", Committed: " << it.tCommit << endl;
        cout << endl;
    }
    return 0;
}