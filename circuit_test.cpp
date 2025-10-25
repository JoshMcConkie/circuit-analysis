#include "components.h"
#include <iostream>
#include <vector>
using namespace std;

int main() {
    Circuit circuit(nullptr);
    Node* v1 = circuit.addNode(0);
    Node* v2 = circuit.addNode(1);
    Node* v3 = circuit.addNode(2);
    Node* ground = circuit.addNode(3);
    // v1.short(v2).R = 1000;
    // v1.connect(vg,R=10);
    circuit.groundNode = ground;
    circuit.connect<Resistor>(v1,ground,1000);


};