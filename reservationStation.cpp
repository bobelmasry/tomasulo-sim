#include <string>
using namespace std;

class ReservationStation {
private:
    string name;   // e.g., "Add1"
    bool busy;          // true if reservation station is occupied

    string Op;     // operation (ADD, SUB, MUL, DIV, LOAD, STORE)

    int Vj;           // Value of operand j (if available)
    int Vk;           // Value of operand k (if available)

    int Qj;     // reservation station producing Vj ("" if ready)
    int Qk;     // reservation station producing Vk ("" if ready)

    int A;              // Used for loads/stores: effective address
    int dest;           // ROB entry this reservation station writes to

public:
    ReservationStation(const string& name = "")
        : name(name), busy(false), Op(""),
          Vj(0), Vk(0), Qj(-1), Qk(-1),
          A(0), dest(-1) {}

    // Setters
    void setBusy(bool b) { busy = b; }
    void setOp(const string& op) { Op = op; }
    void setVj(int v) { Vj = v; Qj = -1; }
    void setVk(int v) { Vk = v; Qk = -1; }
    void setQj(const int& q) { Qj = q; }
    void setQk(const int& q) { Qk = q; }
    void setA(int addr) { A = addr; }
    void setDest(int d) { dest = d; }

    // Getters
    string getName() const { return name; }
    bool isBusy() const { return busy; }
    string getOp() const { return Op; }
    int getVj() const { return Vj; }
    int getVk() const { return Vk; }
    int getQj() const { return Qj; }
    int getQk() const { return Qk; }
    int getA() const { return A; }
    int getDest() const { return dest; }

    // Reset 
    void clear() {
        busy = false;
        Op = "";
        Vj = Vk = 0;
        Qj = Qk = -1;
        A = 0;
        dest = -1;
    }
};
