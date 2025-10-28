#include "components.h"
#include <iostream>
#include <vector>
#include <cassert>
using namespace std;

int main() {
    Circuit circuit(nullptr);
    Node* n1 = circuit.addNode(0);
    Node* n2 = circuit.addNode(1);
    Node* n3 = circuit.addNode(2);
    Node* ground = circuit.addNode(3);
    // v1.short(v2).R = 1000;
    // v1.connect(vg,R=10);
    circuit.groundNode = ground;
    circuit.connect<Resistor>(n1,ground,1000.0);
    circuit.connect<VoltageSource>(n1,n2,5.0);
    circuit.connect<Resistor>(n2,ground,1000.0);
    assert(n1->connections.size() == 2);
    assert(ground->connections.size() == 2);
    assert(circuit.allComponents.size() == 3);

    auto n_voltages = circuit.solveKCL();

    cout << n_voltages << endl;
    cout << "All good." << endl;
    return 0;
};