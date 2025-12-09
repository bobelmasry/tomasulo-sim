#include <string>
using namespace std;

class ReservationStation {
private:
    string name;   // e.g., "Add1"
    bool busy;          // true if reservation station is occupied

    string Op;     // operation (ADD, SUB, MUL, DIV, LOAD, STORE)

    float Vj;           // Value of operand j (if available)
    float Vk;           // Value of operand k (if available)

    string Qj;     // reservation station producing Vj ("" if ready)
    string Qk;     // reservation station producing Vk ("" if ready)

    int A;              // Used for loads/stores: effective address
    int dest;           // ROB entry this reservation station writes to

public:
    ReservationStation(const string& name = "")
        : name(name), busy(false), Op(""),
          Vj(0), Vk(0), Qj(""), Qk(""),
          A(0), dest(-1) {}

    // Setters
    void setBusy(bool b) { busy = b; }
    void setOp(const string& op) { Op = op; }
    void setVj(float v) { Vj = v; Qj = ""; }
    void setVk(float v) { Vk = v; Qk = ""; }
    void setQj(const string& q) { Qj = q; }
    void setQk(const string& q) { Qk = q; }
    void setA(int addr) { A = addr; }
    void setDest(int d) { dest = d; }

    // Getters
    string getName() const { return name; }
    bool isBusy() const { return busy; }
    string getOp() const { return Op; }
    float getVj() const { return Vj; }
    float getVk() const { return Vk; }
    string getQj() const { return Qj; }
    string getQk() const { return Qk; }
    int getA() const { return A; }
    int getDest() const { return dest; }

    // Reset 
    void clear() {
        busy = false;
        Op = "";
        Vj = Vk = 0;
        Qj = Qk = "";
        A = 0;
        dest = -1;
    }
};
