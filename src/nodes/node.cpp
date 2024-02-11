#include "node.hpp"

#include "ImGui/imnodes.h"

Pin::Pin(int id, Node* node, PinType pinType, const std::string& name)
    : id(id), node(node), pinType(pinType), name(name)
{}

Node* Pin::getNode() const
{
    return this->node;
}

const std::unordered_set<Edge*>& Pin::getEdges() const
{
    return this->edges;
}

void Pin::addEdge(Edge* edge)
{
    this->edges.insert(edge);
}

void Pin::removeEdge(Edge* edge)
{
    this->edges.erase(edge);
}

void Pin::clearEdges()
{
    this->edges.clear();
}

Texture* Pin::getSingleTexture() const
{
    if (this->edges.empty())
    {
        return nullptr;
    }

    return (*edges.begin())->getTexture();
}

void Pin::propagateTexture(Texture* texture)
{
    for (auto& edge : this->edges)
    {
        edge->setTexture(texture);
    }
}

void Pin::clearTextures()
{
    for (auto& edge : this->edges)
    {
        edge->clearTexture();
    }
}

bool Pin::draw()
{
    if (pinType == PinType::INPUT)
    {
        ImNodes::BeginInputAttribute(this->id);
    }
    else
    {
        ImNodes::BeginOutputAttribute(this->id);
    }

    ImGui::Text(name.c_str());

    if (pinType == PinType::INPUT)
    {
        ImNodes::EndInputAttribute();
    }
    else
    {
        ImNodes::EndOutputAttribute();
    }

    return false;
}

int Node::nextId = 0;

Node::Node(std::string name)
    : name(name), id(Node::nextId)
{
    Node::nextId += NODE_ID_STRIDE;
}

void Node::addPin(PinType type, const std::string& name)
{
    int pinId = this->id + inputPins.size() + outputPins.size() + 1;
    auto& pinVector = (type == PinType::INPUT) ? inputPins : outputPins;
    pinVector.emplace_back(pinId, this, type, name);
}

void Node::addPin(PinType type)
{
    addPin(type, type == PinType::INPUT ? "input" : "output");
}

unsigned int Node::getTitleBarColor() const
{
    return IM_COL32(11, 109, 191, 255);
}

unsigned int Node::getTitleBarSelectedColor() const
{
    return IM_COL32(81, 148, 204, 255);
}

void Node::clearInputTextures()
{
    for (auto& inputPin : this->inputPins)
    {
        inputPin.clearTextures();
    }
}

Pin& Node::getPin(int pinId)
{
    int localPinId = pinId - this->id - 1;
    int numInputPins = inputPins.size();
    if (localPinId < numInputPins)
    {
        return inputPins[localPinId];
    }
    else
    {
        return outputPins[localPinId - numInputPins];
    }
}

void Node::setNodeEvaluator(NodeEvaluator* nodeEvaluator)
{
    this->nodeEvaluator = nodeEvaluator;
}

bool Node::draw()
{
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, this->getTitleBarColor());
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, this->getTitleBarSelectedColor());
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, this->getTitleBarSelectedColor());

    ImNodes::BeginNode(this->id);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(this->name.c_str());
    ImNodes::EndNodeTitleBar();

    bool didParameterChange = false;
    for (auto& inputPin : inputPins)
    {
        didParameterChange |= inputPin.draw();
    }

    for (auto& outputPin : outputPins)
    {
        didParameterChange |= outputPin.draw();
    }

    ImNodes::EndNode();

    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();

    return didParameterChange;
}
