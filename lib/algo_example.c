/***************************************************************************
 * Description: algorithm exxample
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-05-11 11:08:17
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "algo_example.h"

#define VERSION "0.1.0"
#define MAX_BUF_SIZE 1024

typedef struct algo_handle {
    char param1;
    float param2;
    char param3[MAX_BUF_SIZE];
    float *param4;
} algo_handle_t, *p_algo_handle_t;

int get_algo_version(char *version)
{
    if (version == NULL) {
        // LOGE("version is NULL");
        return -1;
    }
    strcpy(version, VERSION);
    return 0;
}

void *algo_init()
{
    void *algo_handle = (p_algo_handle_t)malloc(sizeof(algo_handle_t));
    if (algo_handle == NULL) {
        // LOGE("allocate for algo_handle_t failed");
        return NULL;
    }
    memset(algo_handle, 0, sizeof(algo_handle_t));
    // LOGI("algo_init OK");
    return algo_handle;
}

void algo_deinit(void *algo_handle)
{
    if (!algo_handle) {
        // LOGE("algo_handle is NULL");
        return;
    }
    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
    if (algo_handle_ptr->param4 != NULL) {
        free(algo_handle_ptr->param4);
        algo_handle_ptr->param4 = NULL;
    }
    free(algo_handle);
    algo_handle = NULL;
    // LOGI("algo_deinit OK");
}

int algo_set_param(void *algo_handle, algo_param_t cmd, void *param, int param_size)
{
    if (algo_handle == NULL) {
        // LOGE("algo_handle is NULL");
        return -1;
    }
    if (param == NULL) {
        // LOGE("param is NULL");
        return -1;
    }

    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
    switch (cmd) {
    case ALGO_PARAM1: {
        if (param_size != sizeof(char)) {
            // LOGE("param_size is not correct");
            return -1;
        }
        algo_handle_ptr->param1 = *(char *)param;
        // LOGI("set param1: %c", algo_handle_ptr->param1);
        break;
    }
    case ALGO_PARAM2: {
        if (param_size != sizeof(float)) {
            // LOGE("param_size is not correct");
            return -1;
        }
        algo_handle_ptr->param2 = *(float *)param;
        // LOGI("set param2: %d", algo_handle_ptr->param2);
        break;
    }
    case ALGO_PARAM3: {
        if (param_size > MAX_BUF_SIZE) {
            // LOGE("param_size is too large");
            return -1;
        }
        memset(algo_handle_ptr->param3, 0, MAX_BUF_SIZE);
        memcpy(algo_handle_ptr->param3, param, param_size);
        // LOGI("set param3: %s", algo_handle_ptr->param3);
        break;
    }
    case ALGO_PARAM4: {
        if (param_size > MAX_BUF_SIZE) {
            // LOGE("param_size is too large");
            return -1;
        }
        algo_handle_ptr->param4 = (float *)malloc(param_size);
        if (algo_handle_ptr->param4 == NULL) {
            // LOGE("allocate for param4 failed");
            return -1;
        }
        memcpy(algo_handle_ptr->param4, param, param_size);
        break;
    }
    default:
        // LOGE("cmd is invalid");
        return -1;
    }
    return 0;
}

int algo_get_param(void *algo_handle, algo_param_t cmd, void *param, int param_size)
{
    if (algo_handle == NULL) {
        // LOGE("algo_handle is NULL");
        return -1;
    }
    if (param == NULL) {
        // LOGE("param is NULL");
        return -1;
    }

    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
    switch (cmd) {
    case ALGO_PARAM1: {
        if (param_size != sizeof(char)) {
            // LOGE("param_size is not correct");
            return -1;
        }
        *(char *)param = algo_handle_ptr->param1;
        // LOGI("get param1: %c", algo_handle_ptr->param1);
        break;
    }
    case ALGO_PARAM2: {
        if (param_size != sizeof(float)) {
            // LOGE("param_size is not correct");
            return -1;
        }
        *(float *)param = algo_handle_ptr->param2;
        // LOGI("get param2: %d", algo_handle_ptr->param2);
        break;
    }
    case ALGO_PARAM3: {
        if (param_size > MAX_BUF_SIZE) {
            // LOGE("param_size is too large");
            return -1;
        }
        memcpy(param, algo_handle_ptr->param3, param_size);
        // LOGI("get param3: %s", algo_handle_ptr->param3);
        break;
    }
    case ALGO_PARAM4: {
        if (param_size > MAX_BUF_SIZE) {
            // LOGE("param_size is too large");
            return -1;
        }
        memcpy(param, algo_handle_ptr->param4, param_size);
        break;
    }
    default:
        // LOGE("cmd is invalid");
        return -1;
    }
    return 0;
}

int algo_process(void *algo_handle, float *input, float *output, int block_size)
{
    if (algo_handle == NULL) {
        // $1
        return -1;
    }
    if (input == NULL) {
        // LOGE("input is NULL");
        return -1;
    }
    if (output == NULL) {
        // LOGE("output is NULL");
        return -1;
    }
    if (block_size <= 0) {
        // LOGE("block_size is not correct");
        return -1;
    }
    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
 
    for (int i = 0; i < block_size; i++) {
        float result = input[i] * algo_handle_ptr->param2;
        if (result > 1.0f) {
            output[i] = 1.0f;
        } else if (result < -1.0f) {
            output[i] = -1.0f;
        } else {
            output[i] = result;
        }
    }

    return 0;
}
