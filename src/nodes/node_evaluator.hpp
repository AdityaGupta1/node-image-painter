#pragma once

#include "node.hpp"
#include "edge.hpp"
#include "../texture.hpp"

#include <unordered_map>

#include <glm/glm.hpp>
#include <cuda_runtime.h>

class Node;
class Pin;
class Edge;

class NodeEvaluator
{
private:
    Node* outputNode{ nullptr };

    std::unordered_map<glm::ivec2, std::vector<Texture>, ResolutionHash> textures;

public:
    NodeEvaluator();
    ~NodeEvaluator();

    void setOutputNode(Node* outputNode);

    Texture requestTexture(glm::ivec2 resolution);

    void evaluate();
};