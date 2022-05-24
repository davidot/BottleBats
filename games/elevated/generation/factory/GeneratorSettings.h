#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "../../../../util/Assertions.h"

namespace Elevated {

class GeneratorSettings;

template<typename OutputType>
class GeneratorFactory {

public:
    virtual ~GeneratorFactory() = default;

    virtual std::unique_ptr<OutputType> visit(GeneratorSettings& settings) = 0;
};

template<typename OutputType, typename Callable>
class LambdaGeneratorFactory : public GeneratorFactory<OutputType> {
private:
    Callable m_call;

public:
    explicit LambdaGeneratorFactory(Callable callable)
        : m_call(std::move(callable))
    {
    }

    std::unique_ptr<OutputType> visit(GeneratorSettings& settings) override
    {
        return m_call(settings);
    }
};

template<typename>
class CompoundGenerator;

template<typename OutputType>
class GeneratorFactories {
private:
    std::vector<std::function<std::unique_ptr<GeneratorFactory<OutputType>>()>> m_factories;
    std::vector<std::string> m_names;

private:
    void addFactory(const std::string& name,
        std::function<std::unique_ptr<GeneratorFactory<OutputType>>()> factory)
    {
#ifdef OUTPUT_FACTORY_NAMES
        std::cerr << "New factory name: \"" << name << "\"\n";
#endif
        m_names.push_back(name);
        m_factories.push_back(std::move(factory));
    }

public:
    template<typename FactoryLambdaType>
    void addFactoryFactory(const std::string& name, FactoryLambdaType factory)
    {
        addFactory(
            name, std::function<std::unique_ptr<GeneratorFactory<OutputType>>()>([factory = std::move(factory)]() {
                using LambdaType = typename std::invoke_result_t<FactoryLambdaType>;
                return std::make_unique<LambdaGeneratorFactory<OutputType, LambdaType>>(factory());
            }));
    }

    template<typename LambdaType,
        typename = std::enable_if_t<std::is_same_v<
            std::unique_ptr<OutputType>, std::invoke_result_t<LambdaType, GeneratorSettings&>>>>
    void addLambdaFactory(const std::string& name, LambdaType lambda)
    {
        addFactory(
            name, std::function<std::unique_ptr<GeneratorFactory<OutputType>>()>([lambda = std::move(lambda)]() {
                return std::make_unique<LambdaGeneratorFactory<OutputType, LambdaType>>(lambda);
            }));
    }

    template<typename Type>
    void addTypedFactory(const std::string& name)
    {
        addFactory(name, std::function<std::unique_ptr<GeneratorFactory<OutputType>>()>([]() { return std::make_unique<Type>(); }));
    }

    [[nodiscard]] size_t size() const { return m_names.size(); }

private:
    [[nodiscard]] const std::vector<std::string>& names() const { return m_names; }

    [[nodiscard]] std::unique_ptr<GeneratorFactory<OutputType>> create(size_t index) const
    {
        return m_factories[index]();
    }

    template<typename>
    friend class CompoundGenerator;

public:
    std::unique_ptr<CompoundGenerator<OutputType>> createGenerator(const std::string& name) const;

    CompoundGenerator<OutputType> createGen(const std::string& name) const;
};

class GeneratorSettings {
public:
    class GeneratorGroup {
    private:
        GeneratorSettings& m_settings;
        std::string m_name;

    public:
        explicit GeneratorGroup(GeneratorSettings& settings, std::string name = "")
            : m_settings(settings)
            , m_name(std::move(name))
        {
            settings.startGroup(m_name);
        };

        ~GeneratorGroup() { m_settings.endGroup(m_name); }
    };

protected:
    virtual void startGroup(const std::string&) = 0;

    virtual void endGroup(const std::string&) = 0;

    virtual void getIntegerValue(std::string_view name, int& value, int min, int max) = 0;

    virtual void getUnsignedValue(std::string_view name, uint32_t& value, int min, int max) = 0;

    virtual void getDoubleValue(std::string_view name, double& value, double min, double max) = 0;

    virtual void addError(const std::string& message, bool error) = 0;

public:
    virtual ~GeneratorSettings() = default;

    virtual void varargsValue(std::string_view name,
        const std::function<void(GeneratorSettings&, size_t, bool)>& func,
        size_t& count)
        = 0;

    void encounteredError(const std::string& message, bool error = true);

    void integerValue(std::string_view name, int& value, int min = -100, int max = 100);

    void unsignedValue(std::string_view name, uint32_t& value, int min = 0, int max = 100);

    void doubleValue(std::string_view name, double& value, double min = 0.0, double max = 1.0);

    virtual void boolValue(std::string_view name, bool& value) = 0;

    virtual void stringValue(std::string_view name, std::string& value) = 0;

    virtual void optionValue(std::string_view name, size_t& value,
        const std::vector<std::string>& options)
        = 0;

    // TODO: add custom value getters:
    // floor / height
    // route

    virtual bool hasFailed() = 0;
};

template<typename OutputType>
class CompoundGenerator {
private:
    std::string m_name;
    size_t m_index;
    const GeneratorFactories<OutputType>& m_factories;
    std::unique_ptr<GeneratorFactory<OutputType>> m_instance = nullptr;

public:
    CompoundGenerator(std::string name, const GeneratorFactories<OutputType>& factories)
        : m_name(std::move(name))
        , m_factories(factories)
    {
        m_index = m_factories.size();
    }

    CompoundGenerator(const CompoundGenerator<OutputType>& copy)
        : m_name(copy.m_name)
        , m_factories(copy.m_factories)
    {
        // we explictly ignore the 'copied' index and instance since there
        // are really the state of the object
        m_index = m_factories.size();
    }

    std::unique_ptr<OutputType> visit(GeneratorSettings& settings)
    {
        size_t preIndex = m_index;
        settings.optionValue(m_name, m_index, m_factories.names());

        if (auto& names = m_factories.names(); m_index >= names.size()) {
            std::ostringstream oss;
            if (!names.empty()) {
                std::copy(names.begin(), names.end() - 1,
                    std::ostream_iterator<std::string>(oss, ", "));
                oss << names.back();
            }
            settings.encounteredError(std::string("Unknown option for factory: ") + m_name + "(" + std::to_string(names.size()) + " possible values: [" + oss.str() + "])");
            return nullptr;
        }

        GeneratorSettings::GeneratorGroup group(settings, m_name);

        // TODO: we do not need the m_instance check i think?
        if (m_index != preIndex || !m_instance) {
            m_instance = m_factories.create(m_index);
        }

        return m_instance->visit(settings);
    }
};

template<typename OutputType>
std::unique_ptr<CompoundGenerator<OutputType>>
GeneratorFactories<OutputType>::createGenerator(const std::string& name) const
{
    return std::make_unique<CompoundGenerator<OutputType>>(name, *this);
}

template<typename OutputType>
CompoundGenerator<OutputType>
GeneratorFactories<OutputType>::createGen(const std::string& name) const
{
    return CompoundGenerator<OutputType>(name, *this);
}

template<typename OutputType>
class VarargsGenerator {
    static_assert(std::is_default_constructible_v<OutputType>,
        "Type must have default constructor");

private:
    std::function<void(GeneratorSettings&, OutputType&)> m_generator;
    std::vector<OutputType> m_values {};
    std::string m_name;

public:
    template<typename Callable>
    VarargsGenerator(Callable callable, std::string name)
        : m_generator(std::function<void(GeneratorSettings&, OutputType&)>(std::move(callable)))
        , m_name(std::move(name))
    {
    }

    VarargsGenerator(const VarargsGenerator<OutputType>& copy)
        : m_generator(copy.m_generator)
        , m_name(copy.m_name)
    {
    }

    const std::vector<OutputType>& visit(GeneratorSettings& baseSettings)
    {
        size_t count = m_values.size();
        std::set<size_t> toRemove;
        baseSettings.varargsValue(
            m_name,
            [this, &toRemove](GeneratorSettings& settings, size_t index, bool removed) {
                if (toRemove.count(index) > 0) {
                    return;
                }
                if (removed) {
                    ASSERT(index < m_values.size());
                    toRemove.insert(index);
                    return;
                }
                if (index >= m_values.size()) {
                    ASSERT(index == m_values.size());
                    m_values.emplace_back();
                }

                m_generator(settings, m_values[index]);
            },
            count);

        auto it = std::remove_if(m_values.begin(), m_values.end(), [&toRemove, this](const OutputType& v) {
            return toRemove.count(&v - &*m_values.begin()) > 0;
        });
        m_values.erase(it, m_values.end());

        ASSERT(m_values.size() == count);
        return values();
    }

    const std::vector<OutputType>& values() const { return m_values; }

protected:
    std::vector<OutputType>& values() { return m_values; }
};

template<typename OutputType>
class VarargsCompoundGenerator : public VarargsGenerator<std::pair<std::unique_ptr<OutputType>, std::unique_ptr<CompoundGenerator<OutputType>>>> {
    using PairType = std::pair<std::unique_ptr<OutputType>, std::unique_ptr<CompoundGenerator<OutputType>>>;

public:
    VarargsCompoundGenerator(const GeneratorFactories<OutputType>& factories,
        const std::string& vname)
        : VarargsGenerator<PairType>(
            [&, name = vname](GeneratorSettings& settings, PairType& pair) {
                auto& [val, factory] = pair;

                if (!factory) {
                    factory = factories.createGenerator(name);
                }

                ASSERT(factory);
                val = std::move(factory->visit(settings));
            },
            vname)
    {
    }

    VarargsCompoundGenerator(const VarargsCompoundGenerator<OutputType>& copy)
        : VarargsGenerator<PairType>(copy)
    {
    }

    std::vector<std::unique_ptr<OutputType>> extractValues()
    {
        auto& pairs = VarargsGenerator<PairType>::values();
        std::vector<std::unique_ptr<OutputType>> outputs;
        outputs.reserve(pairs.size());
        std::transform(pairs.begin(), pairs.end(), std::back_inserter(outputs),
            [](auto& pair) { return std::move(pair.first); });
        return outputs;
    }
};

}
