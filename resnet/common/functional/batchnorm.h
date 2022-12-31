#pragma once

#include "common.h"

__global__ void deviceBatchNorm2dKernel(float *input_data, float *output,
                                        float *mean_data, float *var_data,
                                        float *weight_data, float *bias_data,
                                        float eps, int batch_size, int num_channels,
                                        int height, int width, int total_channels);

void hostBatchNorm2d(float *input_data, float *output,
                     float *mean_data, float *var_data,
                     float *weight_data, float *bias_data,
                     float eps, int batch_size, int num_channels,
                     int height, int width);

__global__ void deviceBatchNorm2dReluKernel(float *input_data, float *output,
                                            float *mean_data, float *var_data,
                                            float *weight_data, float *bias_data,
                                            float eps, int batch_size, int num_channels,
                                            int height, int width, int total_channels);

void hostBatchNorm2dRelu(float *input_data, float *output,
                         float *mean_data, float *var_data,
                         float *weight_data, float *bias_data,
                         float eps, int batch_size, int num_channels,
                         int height, int width);