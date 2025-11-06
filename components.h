#pragma once
#include <vector>
#include <memory> // smart pointers for Circuit
#include <map>
#include <concepts>
#include <utility>
#include <Eigen/Dense>

class Component;

class Node {  
public:
    double voltage = 0.0;
    std::vector<Component*> connections;
    int id;
    Node(int d) : id(d) {}
};


class Component {
public:
    Node* nodeA;
    Node* nodeB;
    virtual ~Component() = default;
    virtual double getCurrent() = 0;

    Component(Node *nA, Node *nB) : nodeA(nA), nodeB(nB) {}
};

class Resistor: public Component {
public:
    double resistance;
    double getCurrent() override{
        return (nodeA->voltage - nodeB->voltage) / resistance;
    };
    Resistor(Node *nA, Node *nB, double R) : Component(nA,nB), resistance(R) {}
};

class CurrentSource : public Component {
public:
    double supply = 0;
    double getCurrent() override{
        return supply;
    }
    CurrentSource(Node *nA, Node *nB, double I) : Component(nA,nB), supply(I) {}
};

class VoltageSource : public Component {
public:
    double supply = 0;
    double current = 0;
    double getCurrent() override{
        return 0;
    }
    VoltageSource(Node *nA, Node *nB, double V) : Component(nA,nB), supply(V) {}
};

template <class T>
concept DerivedComponent = std::derived_from<T, Component>; // only in c++20!!

class Circuit {
    template <DerivedComponent T, class... Args>
    std::unique_ptr<Component> make_component(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
public:
    std::vector<std::unique_ptr<Node>> allNodes;
    std::vector<std::unique_ptr<Component>> allComponents;
    Node* groundNode = nullptr;

    Circuit(Node* gN) : groundNode(gN) {}

    Node* addNode(int id) {
        allNodes.push_back(std::make_unique<Node>(id));
        return allNodes.back().get();
    }

    template <DerivedComponent T, class... Args>
    void connect(Node *a, Node *b, Args&&...args) {
        auto unique_p = make_component<T>(a,b, std::forward<Args>(args)...);
        Component *raw_p = unique_p.get();
        allComponents.push_back(std::move(unique_p)); 
        a->connections.push_back(raw_p);
        b->connections.push_back(raw_p);
               
    }

    Eigen::VectorXd solveKCL() {
        // n is number of non-ground nodes. prep for nxn coefficient matrix
        size_t countNodes = allNodes.size();

        size_t n = (countNodes >0) ? static_cast<size_t>(countNodes - 1) : 0;

        std::vector<double> unknowns;
        std::vector<double> currents(n,0); // for all but the ground node
        std::vector<std::vector<double>> weights(n,std::vector<double>(n,0.0));

        // get valid indicdes for conductance matrix
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

        auto addVoltageSourceConstraint = [&](int aId, int bId, double V){
            const int groundId = groundNode->id;
            
            std::vector<double> new_col(weights.size(),0.0);
            
            if ((aId != (groundId))) new_col[idToWeightIdx[aId]] = -1.0;
            if ((bId != (groundId))) new_col[idToWeightIdx[bId]] = 1.0;

            for (size_t i = 0; i < weights.size(); i++) {
                weights[i].push_back(new_col[i]);
            }

            std::vector<double> new_row(weights[0].size(), 0.0);
            if ((aId != (groundId))) new_row[idToWeightIdx[aId]] = -1.0;
            if ((bId != (groundId))) new_row[idToWeightIdx[bId]] = 1.0;
            
            weights.push_back(std::move(new_row));

            currents.push_back(V);
        };

    
        for (const auto& comp_ptr : allComponents) {
            // Try to cast the component to a CurrentSource
            CurrentSource* cs_ptr = dynamic_cast<CurrentSource*>(comp_ptr.get());
            
            
            if (cs_ptr != nullptr) {
                // add the source to the force vector on the proper node row
                unsigned int nodeAId = cs_ptr->nodeA->id;
                unsigned int nodeBId = cs_ptr->nodeB->id;
                if ((nodeAId != (groundNode->id))) currents[idToWeightIdx[nodeAId]] -= (*comp_ptr).getCurrent();
                if ((nodeBId != (groundNode->id))) currents[idToWeightIdx[nodeBId]] += (*comp_ptr).getCurrent();
    
            } else {
                VoltageSource* vs_ptr = dynamic_cast<VoltageSource*>(comp_ptr.get());
                if (auto* vs = dynamic_cast<VoltageSource*>(comp_ptr.get())) {
                    // this assumes supply is from nodeA(-) to nodeB(+)
                    addVoltageSourceConstraint(vs->nodeA->id,vs->nodeB->id,vs->supply);
                } 
            } 
        }
    
    Eigen::MatrixXd A(weights.size(), weights[0].size());
    for (size_t i = 0; i < weights.size(); ++i) {
        for (size_t j = 0; j < weights[0].size(); ++j) {
            A(i,j) = weights[i][j];
        }
    }

    Eigen::VectorXd b(currents.size());
    for (size_t i = 0; i < currents.size(); ++i) {
        b(i) = currents[i];
    }
    // Solve (robust for small/medium problems)
    Eigen::VectorXd x = A.fullPivLu().solve(b);
    // or: A.colPivHouseholderQr().solve(b);
    Eigen::VectorXd V  = x.head(n);   // node voltages (same ordering)
    Eigen::VectorXd Iv = x.tail(x.size() - n); // currents through voltage sources
    
    return V;
}
};