#pragma once
#include <vector>
#include <memory> // smart pointers for Circuit
#include <map>
class Node {
   
    public:
    double voltage = 0.0;
    std::vector<Component*> connections;
    int id = -1;

    // std::vector<Node> getConnections() {

    //     for (const auto& comp_ptr : connections)
    // }
};

class Component {
public:
    Node* nodeA;
    Node* nodeB;
    double ohms = 0;
    virtual double getCurrent() = 0;
};

class Resistor: public Component {
public:
    double resistance;
    double getCurrent() override{
        return (nodeA->voltage - nodeB->voltage) / resistance;
    };


};

class CurrentSource : public Component {
public:
    double current = 0;
    double getCurrent() override{
        return current;
    }
};

// class VoltageSource : public Component {
// public:
//     double current = 0;
//     double getCurrent() override{
//         return current;
//     }
// };

class Circuit {
public:
    std::vector<std::unique_ptr<Node>> allNodes;
    std::vector<std::unique_ptr<Component>> allComponents;
    const Node* groundNode;

    void solveKCL() {
        // n is number of non-ground nodes. prep for nxn coefficient matrix
        size_t countNodes = allNodes.size();

        size_t n = (countNodes >0) ? static_cast<size_t>(countNodes - 1) : 0;

        // std::vector<double> voltages;
        std::vector<double> currents(n,0); // for all but the ground node
        std::vector<std::vector<double>> weights(n,std::vector<double>(n,0.0));


        unsigned int matrix_index = 0;
        std::map<unsigned int,unsigned int> idToWeightIdx;
        for (const auto&  node_ptr : allNodes) {
            int nodeId = node_ptr->id;
            if (node_ptr.get() != groundNode) {
                idToWeightIdx[nodeId] = matrix_index++;
            }

        }



        for (const auto&  node_ptr : allNodes) {
            if (node_ptr.get() == groundNode) continue;
            unsigned int thisNodeIndex = idToWeightIdx[node_ptr->id];
            for (const auto& comp_ptr : node_ptr->connections) {
                // get connected node and resistance values
                
                Resistor* resistor_ptr = dynamic_cast<Resistor*>(comp_ptr);

                if (resistor_ptr != nullptr) { // check if its a resistor
                    double resistance = resistor_ptr->resistance;
                    ////////------->>>> need to update for when nodea a or node b are ground nodes
                    unsigned int nodeAId = resistor_ptr->nodeA->id;
                    unsigned int nodeBId = resistor_ptr->nodeB->id;
                    
                    // if the other node is ground, only update the main node
                    if ((resistor_ptr->nodeA != groundNode) && (resistor_ptr->nodeB != groundNode)) {   
                        unsigned int nodeAIndex = idToWeightIdx[resistor_ptr->nodeA->id];
                        unsigned int nodeBIndex = idToWeightIdx[resistor_ptr->nodeB->id];
                        if (nodeAIndex == thisNodeIndex) {
                            weights[thisNodeIndex][nodeBIndex] -= 1.0 / resistance;
                        } else if (nodeBIndex == thisNodeIndex) { 
                            weights[thisNodeIndex][nodeAIndex] -= 1.0 / resistance; 
                        }
                    }
                     // begin updating matrix nth row
                        
                    weights[thisNodeIndex][thisNodeIndex] += 1.0 / resistance;

                }

            }
        }

        for (const auto& comp_ptr : allComponents) {
            // Try to cast the component to a CurrentSource
            CurrentSource* cs = dynamic_cast<CurrentSource*>(comp_ptr.get());

            
            
            if (cs != nullptr) {
                // add the source to the force vector on the proper node row
                unsigned int nodeAId = cs->nodeA->id;
                unsigned int nodeBId = cs->nodeB->id;
                if ((nodeAId != (groundNode->id))) currents[idToWeightIdx[nodeAId]] -= (*comp_ptr).getCurrent();
                if ((nodeBId != (groundNode->id))) currents[idToWeightIdx[nodeBId]] += (*comp_ptr).getCurrent();
    
            }
        }
        
    
    }



};