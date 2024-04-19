#include "node_fileinput.hpp"

#include "cuda_includes.hpp"

#include "stb_image.h"
#include "tinyexr.h"
#include <filesystem>

std::vector<const char*> NodeFileInput::colorSpaceOptions = { "linear", "sRGB" };

NodeFileInput::NodeFileInput()
    : Node("file input")
{
    addPin(PinType::OUTPUT, "image");

    addPin(PinType::INPUT, "color space").setNoConnect();
}

unsigned int NodeFileInput::getTitleBarColor() const
{
    return IM_COL32(7, 94, 11, 255);
}

unsigned int NodeFileInput::getTitleBarHoveredColor() const
{
    return IM_COL32(47, 153, 53, 255);
}

__global__ void kernSrgbToLinear(Texture tex)
{
    const int idx = (blockIdx.x * blockDim.x) + threadIdx.x;

    if (idx >= tex.getNumPixels())
    {
        return;
    }

    tex.setColor<TextureType::MULTI>(idx, ColorUtils::srgbToLinear(tex.getColor<TextureType::MULTI>(idx)));
}

void NodeFileInput::reloadFile()
{
    if (texFile != nullptr) {
        --texFile->numReferences;
        texFile = nullptr;
    }

    bool isExr = isFileExr();

    float* host_pixels = nullptr;
    int width, height;
    if (isExr)
    {
        const char* err = nullptr;

        int ret = LoadEXR(&host_pixels, &width, &height, filePath.c_str(), &err);

        if (ret != TINYEXR_SUCCESS)
        {
            if (err)
            {
                fprintf(stderr, "ERR : %s\n", err);
                FreeEXRErrorMessage(err);
            }

            return;
        }
    }
    else
    {
        stbi_ldr_to_hdr_gamma(selectedColorSpace == 0 ? 1.0f : 2.2f); // 1.0f if linear, 2.2f if sRGB

        int channels;
        host_pixels = stbi_loadf(filePath.c_str(), &width, &height, &channels, 4);
    }

    if (host_pixels == nullptr) {
        return;
    }

    texFile = nodeEvaluator->requestTexture<TextureType::MULTI>(glm::ivec2(width, height));
    CUDA_CHECK(cudaMemcpy(texFile->getDevPixels<TextureType::MULTI>(), host_pixels, width * height * 4 * sizeof(float), cudaMemcpyHostToDevice));

    if (isExr)
    {
        free(host_pixels);

        if (selectedColorSpace == 1) // sRGB
        {
            const dim3 blockSize(DEFAULT_BLOCK_SIZE_1D);
            const dim3 blocksPerGrid = calculateNumBlocksPerGrid(texFile->getNumPixels(), blockSize);
            kernSrgbToLinear<<<blocksPerGrid, blockSize>>>(*texFile);
        }
    }
    else
    {
        stbi_image_free(host_pixels);
    }
}

bool NodeFileInput::isFileExr() const
{
    return std::filesystem::path(filePath).extension().string() == ".exr";
}

bool NodeFileInput::drawPinExtras(const Pin* pin, int pinNumber)
{
    ImGui::SameLine();

    bool didParameterChange;

    if (pin->pinType == PinType::INPUT)
    {
        switch (pinNumber)
        {
        case 0: // color space
            didParameterChange = NodeUI::Dropdown(selectedColorSpace, colorSpaceOptions);
            break;
        default:
            throw std::runtime_error("invalid pin number");
        }
    }
    else
    {
        switch (pinNumber)
        {
        case 0: // file input
            didParameterChange = NodeUI::FilePicker(&filePath, { "Image Files (.png, .jpg, .jpeg, .exr)", "*.png *.jpg *.jpeg *.exr" });

            if (didParameterChange)
            {
                selectedColorSpace = isFileExr() ? 0 : 1; // linear if EXR, sRGB otherwise
            }

            break;
        default:
            throw std::runtime_error("invalid pin number");
        }
    }

    if (didParameterChange) {
        needsReloadFile = true;
    }
    return didParameterChange;
}

void NodeFileInput::_evaluate()
{
    if (needsReloadFile) {
        reloadFile();
    }

    Texture* outTex;
    if (texFile == nullptr)
    {
        outTex = nodeEvaluator->requestUniformTexture();
        outTex->setUniformColor(glm::vec4(0, 0, 0, 1));
    }
    else
    {
        outTex = texFile;
    }

    if (needsReloadFile)
    {
        ++outTex->numReferences; // cache this texture; numReferences is decremented by reloadFile()
        needsReloadFile = false;
    }
    outputPins[0].propagateTexture(outTex);
}
