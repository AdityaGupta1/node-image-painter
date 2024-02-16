#pragma once

#define NODE_ID_STRIDE 32

#include "node_evaluator.hpp"
#include "node_ui_elements.hpp"
#include "texture.hpp"
#include "color_utils.hpp"

#include "ImGui/imgui.h"

#include <vector>
#include <string>
#include <unordered_set>
#include <stdexcept>

class Edge;
class Node;
class NodeEvaluator;

enum class PinType
{
    INPUT, OUTPUT
};

class Pin
{
private:
    Node* node{ nullptr };
    std::unordered_set<Edge*> edges;

public:
    const int id;
    const PinType pinType;
    const std::string name;

    Pin(int id, Node* node, PinType pinType, const std::string& name);

    Node* getNode() const;
    const std::unordered_set<Edge*>& getEdges() const;
    bool hasEdge() const;

    void addEdge(Edge* edge);
    void removeEdge(Edge* edge);
    void clearEdges();

    // utility function to get single texture for input pins (nullptr if no connected edge)
    Texture* getSingleTexture() const;

    void propagateTexture(Texture* texture);
    void clearTextures();
};

class Node
{
    friend class NodeEvaluator;

private:
    static int nextId;

protected:
    const std::string name;

    NodeEvaluator* nodeEvaluator{ nullptr };

    Node(std::string name);

    void addPin(PinType type, const std::string& name);
    void addPin(PinType type);

    virtual unsigned int getTitleBarColor() const;
    virtual unsigned int getTitleBarSelectedColor() const;

    virtual void evaluate() = 0;
    Texture* getPinTextureOrSingleColor(const Pin& pin, glm::vec4 col);
    Texture* getPinTextureOrSingleColor(const Pin& pin, float col);
    void clearInputTextures();

    virtual bool drawPinExtras(const Pin* pin, int pinNumber);

public:
    const int id;

    std::vector<Pin> inputPins;
    std::vector<Pin> outputPins;

    Pin& getPin(int pinId);

    void setNodeEvaluator(NodeEvaluator* nodeEvaluator);

    bool draw();
};
