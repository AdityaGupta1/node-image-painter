#include "node_uvgradient.hpp"

#include "cuda_includes.hpp"

NodeUvGradient::NodeUvGradient()
    : Node("uv gradient")
{
    addPin(PinType::OUTPUT);
}

__global__ void kernUvGradient(Texture outTex)
{
    const int x = (blockIdx.x * blockDim.x) + threadIdx.x;
    const int y = (blockIdx.y * blockDim.y) + threadIdx.y;

    if (x >= outTex.resolution.x || y >= outTex.resolution.y)
    {
        return;
    }

    glm::vec2 uv = glm::vec2(x, y) / glm::vec2(outTex.resolution);
    outTex.dev_pixels[y * outTex.resolution.x + x] = glm::vec4(uv, 0, 1);
}

void NodeUvGradient::evaluate()
{
    Texture* outTex = nodeEvaluator->requestTexture();

    const dim3 blockSize(16, 16);
    const dim3 blocksPerGrid(outTex->resolution.x / 16 + 1, outTex->resolution.y / 16 + 1);
    kernUvGradient<<<blocksPerGrid, blockSize>>>(*outTex);

    outputPins[0].propagateTexture(outTex);
}