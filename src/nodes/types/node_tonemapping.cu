#include "node_tonemapping.hpp"

#include "cuda_includes.hpp"

std::vector<const char*> NodeToneMapping::toneMappingOptions = { "none", "AgX", "AgX (golden)", "AgX (punchy)", "reinhard", "ACES filmic" };

NodeToneMapping::NodeToneMapping()
    : Node("tone mapping")
{
    addPin(PinType::OUTPUT, "image");

    addPin(PinType::INPUT, "image");
    addPin(PinType::INPUT, "tone mapping").setNoConnect();
}

unsigned int NodeToneMapping::getTitleBarColor() const
{
    return IM_COL32(130, 0, 0, 255);
}

unsigned int NodeToneMapping::getTitleBarHoveredColor() const
{
    return IM_COL32(190, 0, 0, 255);
}

bool NodeToneMapping::drawPinExtras(const Pin* pin, int pinNumber)
{
    if (pin->pinType == PinType::OUTPUT)
    {
        return false;
    }

    switch (pinNumber)
    {
    case 0: // image
        return false;
    case 1: // tone mapping
        ImGui::SameLine();
        return NodeUI::Dropdown(selectedToneMapping, toneMappingOptions);
    default:
        throw std::runtime_error("invalid pin number");
    }
}

__host__ __device__ glm::vec4 applyToneMapping(glm::vec4 col, int toneMapping)
{
    glm::vec3 rgb = glm::max(glm::vec3(col), 0.f);

    switch (toneMapping)
    {
    case 0:
        break;
    case 1:
        rgb = ColorUtils::AgX(rgb, 0);
        break;
    case 2:
        rgb = ColorUtils::AgX(rgb, 1);
        break;
    case 3:
        rgb = ColorUtils::AgX(rgb, 2);
        break;
    case 4:
        rgb = ColorUtils::reinhard(rgb);
        break;
    case 5:
        rgb = ColorUtils::ACESFilm(rgb);
        break;
    }

    return glm::vec4(rgb, col.a);
}

__global__ void kernApplyToneMapping(Texture inTex, int toneMapping, Texture outTex)
{
    const int x = (blockIdx.x * blockDim.x) + threadIdx.x;
    const int y = (blockIdx.y * blockDim.y) + threadIdx.y;

    if (x >= outTex.resolution.x || y >= outTex.resolution.y)
    {
        return;
    }

    const int idx = y * inTex.resolution.x + x;
    glm::vec4 outCol = applyToneMapping(inTex.getColor<TextureType::MULTI>(idx), toneMapping);
    outTex.setColor<TextureType::MULTI>(idx, outCol);
}

void NodeToneMapping::_evaluate()
{
    Texture* inTex = getPinTextureOrUniformColor(inputPins[0], glm::vec4(0, 0, 0, 1));

    if (inTex->isUniform())
    {
        Texture* outTex = nodeEvaluator->requestUniformTexture();
        outTex->setUniformColor(applyToneMapping(inTex->getUniformColor<TextureType::MULTI>(), selectedToneMapping));
        outputPins[0].propagateTexture(outTex);
        return;
    }

    Texture* outTex = nodeEvaluator->requestTexture<TextureType::MULTI>(inTex->resolution);

    const dim3 blockSize(DEFAULT_BLOCK_SIZE_2D_X, DEFAULT_BLOCK_SIZE_2D_Y);
    const dim3 blocksPerGrid = calculateNumBlocksPerGrid(inTex->resolution, blockSize);
    kernApplyToneMapping<<<blocksPerGrid, blockSize>>>(*inTex, selectedToneMapping, *outTex);

    outputPins[0].propagateTexture(outTex);
}
