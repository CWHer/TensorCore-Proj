#pragma once

#include "common.h"

class Tensor
{
private:
    int64_t total_size;
    int32_t n_dim;
    std::vector<int32_t> shape;
    std::vector<int32_t> strides;
    float *data;
    DeviceType device;

public:
    Tensor() : data(nullptr), device(DeviceType::UNKNOWN) {}

    Tensor(const std::vector<int> &shape,
           DeviceType device = DeviceType::CPU)
    {
        this->shape = shape;
        n_dim = shape.size();
        strides = std::vector<int>(n_dim);
        strides.back() = 1;
        for (int i = n_dim - 1; i > 0; i--)
            strides[i - 1] = strides[i] * shape[i];
        total_size = strides[0] * shape[0];

        switch (device)
        {
        case DeviceType::CPU:
            data = new float[total_size];
            break;
        case DeviceType::CUDA:
            checkCudaErrors(cudaMalloc(&data, total_size * sizeof(float)));
            break;
        default:
            checkCppErrorsMsg(true, "Unknown device type");
        }
        this->device = device;
    }

    ~Tensor()
    {
        if (data != nullptr)
        {
            switch (device)
            {
            case DeviceType::CPU:
                delete[] data;
                break;
            case DeviceType::CUDA:
                checkCudaErrors(cudaFree(data));
                break;
            default:
                checkCppErrorsMsg(true, "Unknown device type");
            }
        }
    }

    void load(const std::string &file_path)
    {
        // NOTE: content,
        //   1KB header (int_32 shape)
        //   indefinite data (float_32)
        static const int DATA_OFFSET = 1024;

        std::ifstream fin;
        fin.open(file_path, std::ios::binary);
        checkCppErrorsMsg(!fin.is_open(), ("Cannot open file: " + file_path).c_str());

        // clang-format off
        shape.reserve(DATA_OFFSET / sizeof(int32_t));
        for (int i = 0; i < DATA_OFFSET / sizeof(int32_t); i++)
        {
            int32_t dim;
            fin.read(reinterpret_cast<char *>(&dim), sizeof(int32_t));
            if (dim == 0) break;
            shape.push_back(dim);
        }
        // clang-format on
        n_dim = shape.size();
        strides = std::vector<int>(n_dim);
        strides.back() = 1;
        for (int i = n_dim - 1; i > 0; i--)
            strides[i - 1] = strides[i] * shape[i];
        total_size = strides[0] * shape[0];
        this->data = new float[total_size];
        this->device = DeviceType::CPU;

        fin.seekg(DATA_OFFSET, std::ios::beg);
        fin.read(reinterpret_cast<char *>(data), total_size * sizeof(float));

        fin.close();
    }

public:
    // HACK: DO NOT support save
    // void save(const std::string &path) {}

    bool empty() { return data == nullptr; }

    Tensor clone()
    {
        Tensor cloned_tensor(shape, device);
        switch (device)
        {
        case DeviceType::CPU:
            std::copy(data, data + total_size, cloned_tensor.data);
            break;
        case DeviceType::CUDA:
            checkCudaErrors(cudaMemcpy(cloned_tensor.data, data,
                                       total_size * sizeof(float),
                                       cudaMemcpyDeviceToDevice));
            break;
        default:
            checkCppErrorsMsg(true, "Unknown device type");
        }
        return cloned_tensor;
    }

    float *data_ptr() { return data; }

    int64_t totalSize() { return total_size; }

    float index(const std::vector<int> &indices)
    {
        int offset = 0;
        for (int i = 0; i < indices.size(); i++)
            offset += indices[i] * strides[i];

        switch (device)
        {
        case DeviceType::CPU:
            return data[offset];
        case DeviceType::CUDA:
        {
            // HACK: FIXME: this is extremely inefficient
            auto h_data = std::make_shared<float>();
            checkCudaErrors(cudaMemcpy(h_data.get(), data + offset, sizeof(float),
                                       cudaMemcpyDeviceToHost));
            return *h_data;
        }
        default:
            checkCppErrorsMsg(true, "Unknown device type");
        }
    }

    std::vector<int> sizes() { return shape; }

    void view(const std::vector<int> &shape)
    {
        // HACK: DO NOT support -1
        this->shape = shape;
        n_dim = shape.size();
        strides.resize(n_dim);
        strides.back() = 1;
        for (int i = n_dim - 1; i > 0; i--)
            strides[i - 1] = strides[i] * shape[i];
        checkCppErrorsMsg(strides[0] * shape[0] != total_size, "Invalid shape");
    }

    void to(DeviceType device)
    {
        if (this->device == device)
            return;

        float *data;
        this->device = device;
        switch (device)
        {
        case DeviceType::CPU:
            data = new float[total_size];
            checkCudaErrors(cudaMemcpy(data, this->data, total_size * sizeof(float),
                                       cudaMemcpyDeviceToHost));
            checkCudaErrors(cudaFree(this->data));
            break;

        case DeviceType::CUDA:
            checkCudaErrors(cudaMalloc(&data, total_size * sizeof(float)));
            checkCudaErrors(cudaMemcpy(data, this->data, total_size * sizeof(float),
                                       cudaMemcpyHostToDevice));
            delete[] this->data;
            break;

        default:
            checkCppErrorsMsg(true, "Unknown device type");
        }
        this->data = data;
    }

    DeviceType getDevice() { return device; }

public:
    friend std::ostream &operator<<(std::ostream &out, const Tensor &x)
    {
        std::cout << "Tensor(";
        for (int i = 0; i < x.n_dim; i++)
            std::cout << x.shape[i] << ", ";
        static auto deviceType = [](DeviceType device)
        {
            switch (device)
            {
            case DeviceType::CPU:
                return "CPU";
            case DeviceType::CUDA:
                return "CUDA";
            default:
                return "Unknown";
            }
        };
        std::cout << deviceType(x.device) << ")" << std::endl;
        return out;
    }
};