#pragma once

#include <elevated/generation/factory/GeneratorSettings.h>
#include <deque>

namespace Elevated {

class VisualGeneratorSettings : public GeneratorSettings {
private:
    struct GroupData {
        std::string name;
        bool visible;

        GroupData(std::string name, bool visible);
    };

    std::deque<GroupData> m_groups;

    char* label(std::string_view name);

    bool shouldRender();

public:
    void varargsValue(std::string_view name,
                      const std::function<void(GeneratorSettings&, size_t, bool)>& func,
                      size_t& count) override;
    void boolValue(std::string_view name, bool& value) override;
    void stringValue(std::string_view name, std::string& value) override;
    void optionValue(std::string_view name, size_t& value,
                     const std::vector<std::string>& options) override;
    bool hasFailed() override;

protected:
    void startGroup(const std::string& name) override;
    void endGroup(const std::string& name) override;
    void getIntegerValue(std::string_view name, int& value, int min, int max) override;
    void getUnsignedValue(std::string_view name, uint32_t& value, int min, int max) override;
    void getDoubleValue(std::string_view name, double& value, double min, double max) override;
    void addError(const std::string& message, bool error) override;
};

}   // namespace Elevated::Visualizer
