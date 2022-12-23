#pragma once

#include "common.h"
#include "tensor.hpp"

class Module
{
private:
    struct NamedTensor
    {
        std::string name;
        Tensor tensor;
    };

    std::vector<NamedTensor> tensor_list;

public:
    virtual Tensor forward(Tensor x) = 0;

    virtual void printModule(const std::string &prefix) = 0;

    void addTensor(const std::string &name, Tensor tensor)
    {
        tensor_list.emplace_back(NamedTensor{name, tensor});
    }

    virtual void loadWeights(const std::string &path)
    {
        for (auto &named_tensor : tensor_list)
            named_tensor.tensor.load(path + "_" + named_tensor.name);
    }

    virtual void to(DeviceType device)
    {
        for (auto &named_tensor : tensor_list)
            named_tensor.tensor.to(device);
    }
};

class ModuleList : public Module
{
private:
    struct NamedModule
    {
        std::string name;
        std::shared_ptr<Module> module;
    };

    std::vector<NamedModule> module_list;

public:
    void addModule(std::string name, std::shared_ptr<Module> module)
    {
        module_list.emplace_back(NamedModule{name, module});
    }

    bool empty() { return module_list.empty(); }

    void loadWeights(const std::string &path) override
    {
        for (auto &named_module : module_list)
            named_module.module->loadWeights(path + "_" + named_module.name);
    }

    virtual Tensor forward(Tensor x) override
    {
        for (auto &named_module : module_list)
            x = named_module.module->forward(x);
        return x;
    }

    void to(DeviceType device) override
    {
        for (auto &named_module : module_list)
            named_module.module->to(device);
    }

    void printModule(const std::string &prefix) override
    {
        for (auto &named_module : module_list)
            named_module.module->printModule(prefix + "_" + named_module.name);
    }
};