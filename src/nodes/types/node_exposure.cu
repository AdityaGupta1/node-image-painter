#include "node_exposure.hpp"

#include "cuda_includes.hpp"

NodeExposure::NodeExposure()
    : Node("exposure")
{
    addPin(PinType::OUTPUT, "image");

    addPin(PinType::INPUT, "image");
    addPin(PinType::INPUT, "exposure").setNoConnect();
}

__global__ void kernExposure(Texture inTex, float multiplier, Texture outTex)
{
    const int x = (blockIdx.x * blockDim.x) + threadIdx.x;
    const int y = (blockIdx.y * blockDim.y) + threadIdx.y;

    if (x >= inTex.resolution.x || y >= inTex.resolution.y)
    {
        return;
    }

    int idx = y * inTex.resolution.x + x;
    glm::vec4 col = inTex.dev_pixels[idx];
    outTex.dev_pixels[idx] = glm::vec4(glm::vec3(col) * multiplier, col.a);
}

bool NodeExposure::drawPinExtras(const Pin* pin, int pinNumber)
{
    if (pin->pinType == PinType::OUTPUT || pin->hasEdge())
    {
        return false;
    }

    switch (pinNumber)
    {
    case 0: // image
        ImGui::SameLine();
        return NodeUI::ColorEdit4(constParams.color);
    case 1: // exposure
        ImGui::SameLine();
        return NodeUI::FloatEdit(constParams.exposure, 0.01f);
    default:
        throw std::runtime_error("invalid pin number");
    }
}

void NodeExposure::evaluate()
{
    Texture* inTex = getPinTextureOrSingleColor(inputPins[0], ColorUtils::srgbToLinear(constParams.color));

    if (inTex->isSingleColor()) {
        Texture* outTex = nodeEvaluator->requestSingleColorTexture();

        if (constParams.exposure == 0.f) {
            outTex->setSingleColor(inTex->singleColor);
        }
        else
        {
            glm::vec4 outCol = glm::vec4(glm::vec3(inTex->singleColor) * powf(2.f, constParams.exposure), inTex->singleColor.a);
            outTex->setSingleColor(outCol);
        }

        outputPins[0].propagateTexture(outTex);
        return;
    }

    // inTex is not a single color
    if (constParams.exposure == 0.f) {
        outputPins[0].propagateTexture(inTex);
        return;
    }

    // inTex is not a single color and constParams.exposure != 0.f
    Texture* outTex = nodeEvaluator->requestTexture(inTex->resolution);

    const dim3 blockSize(DEFAULT_BLOCK_SIZE_X, DEFAULT_BLOCK_SIZE_Y);
    const dim3 blocksPerGrid = calculateNumBlocksPerGrid(inTex->resolution, blockSize);
    kernExposure<<<blocksPerGrid, blockSize>>>(*inTex, powf(2.f, constParams.exposure), *outTex);

    outputPins[0].propagateTexture(outTex);
}

std::string NodeExposure::debugGetSrcFileName() const
{
    return __FILE__;
}

