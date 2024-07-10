/* **************************************************************
 * @Description: header for algorithm example
 * @Date: 2024-05-16 17:24:47
 * @Version: 0.1.0
 * @Author: pandapan@aactechnologies.com
 * @Copyright (c) 2024 by @AAC Technologies, All Rights Reserved. 
 **************************************************************/

#ifndef _ALGO_EXAMPLE_H
#define _ALGO_EXAMPLE_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define E_OK                   0
#define E_VERSION_BUFFER_NULL -1
#define E_ALGO_HANDLE_NULL    -2
#define E_PARAM_BUFFER_NULL   -3
#define E_PARAM_SIZE_INVALID  -4
#define E_ALLOCATE_FAILED     -5
#define E_PARAM_OUT_OF_RANGE  -6

typedef enum algo_param {
    ALGO_PARAM_START = 0,
    ALGO_PARAM1,
    ALGO_PARAM2,
    ALGO_PARAM3,
    ALGO_PARAM4,
    ALGO_PARAM_END,
} algo_param_t;

int get_algo_version(char *version);
void *algo_init();
void algo_deinit(void *algo_handle);
int algo_set_param(void *algo_handle, algo_param_t cmd, void *param, int param_size);
int algo_get_param(void *algo_handle, algo_param_t cmd, void *param, int param_size);
int algo_process(void *algo_handle, float *input, float *output, int block_size);

#endif
