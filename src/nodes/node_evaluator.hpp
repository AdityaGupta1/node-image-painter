#pragma once

#include "node.hpp"
#include "edge.hpp"
#include "texture.hpp"

#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "cuda_includes.hpp"
#include <glm/glm.hpp>
#include <GL/glew.h>

class Node;
class Pin;
class Edge;

class NodeEvaluator
{
private:
    Node* outputNode{ nullptr };

    std::unordered_map<glm::ivec2, std::vector<std::unique_ptr<Texture>>, ResolutionHash> textures;
    Texture* outputTexture{ nullptr };

    std::vector<Texture*> requestedTextures;

public:
    GLuint viewerTex;

    const glm::ivec2 outputResolution;

    NodeEvaluator(glm::ivec2 outputResolution);
    ~NodeEvaluator();

    void init();

    void setOutputNode(Node* outputNode);

    Texture* requestTexture(glm::ivec2 resolution);
    Texture* requestTexture(); // defaults to output resolution
    Texture* requestUniformTexture(); // resolution = (0, 0)

    Texture* getOutputTexture() const;
    void setOutputTexture(Texture* texture);
    bool hasOutputTexture() const;

    void setChangedNode(Node* changedNode);

    void evaluate();
};
