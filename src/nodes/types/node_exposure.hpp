#pragma once

#include "nodes/node.hpp"

class NodeExposure : public Node
{
private:
    glm::vec4 backupCol{ NodeUI::defaultBackupVec4 };
    float backupExposure{ 0.f };

public:
    NodeExposure();

protected:
    bool drawPinExtras(const Pin* pin, int pinNumber) override;
    void evaluate() override;

    std::string debugGetSrcFileName() const override;
};
