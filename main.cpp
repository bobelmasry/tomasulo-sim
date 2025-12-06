#include "reservationStation.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

int main() {


    // Fetching the binary instructions from the instructions.txt file
    ifstream infile("instructions.txt"); // Open file
    if (!infile) {
        cerr << "Error: Could not open file.\n";
        return 1;
    }

    vector<string> instructions;
    string line;

    // Read each line from the file
    while (getline(infile, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        instructions.push_back(line);
    }

    infile.close();

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
    
    
    return 0;
}