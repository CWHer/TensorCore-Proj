#pragma once

#include "common.h"

__global__ void dimBatchNorm2dKernel(float *input_data, float *mean_data, float *var_data,
                                     float *weight_data, float *bias_data,
                                     float eps, int batch_size, int num_channels,
                                     int height, int width,
                                     int element_per_thread);

void hostBatchNorm2d(float *input_data, float *mean_data, float *var_data,
                     float *weight_data, float *bias_data,
                     float eps, int batch_size, int num_channels,
                     int height, int width);