/***************************************************************************
 * Description: algorithm exxample
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-05-11 11:08:17
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "algo_example.h"
#include "../Logger.h"
#include "gain_control.h"

#define VERSION "0.1.2"
#define MAX_BUF_SIZE 1024

static char log_temp_buf[256] = {0};

typedef struct algo_handle {
    char param1;
    float param2;
    char param3[MAX_BUF_SIZE];
    float *param4;
} algo_handle_t, *p_algo_handle_t;

int get_algo_version(char *version)
{
    if (version == NULL) {
        LOG_MSG_CF(LOG_ERROR, "version is NULL");
        return E_VERSION_BUFFER_NULL;
    }
    strcpy(version, VERSION);
    return E_OK;
}

void *algo_init()
{
    void *algo_handle = (p_algo_handle_t)malloc(sizeof(algo_handle_t));
    if (algo_handle == NULL) {
        LOG_MSG_CF(LOG_ERROR, "allocate for algo_handle_t failed");
        return NULL;
    }
    memset(algo_handle, 0, sizeof(algo_handle_t));
    LOG_MSG_CF(LOG_INFO, "algo_init OK");
    return algo_handle;
}

void algo_deinit(void *algo_handle)
{
    if (!algo_handle) {
        LOG_MSG_CF(LOG_ERROR, "algo_handle is NULL");
        return;
    }
    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
    if (algo_handle_ptr->param4 != NULL) {
        free(algo_handle_ptr->param4);
        algo_handle_ptr->param4 = NULL;
    }
    free(algo_handle);
    algo_handle = NULL;
    LOG_MSG_CF(LOG_INFO, "algo_deinit OK");
}

int algo_set_param(void *algo_handle, algo_param_t cmd, void *param, int param_size)
{
    if (algo_handle == NULL) {
        LOG_MSG_CF(LOG_ERROR, "algo_handle is NULL");
        return E_ALGO_HANDLE_NULL;
    }
    if (param == NULL) {
        LOG_MSG_CF(LOG_ERROR, "param is NULL");
        return E_PARAM_BUFFER_NULL;
    }

    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
    switch (cmd) {
    case ALGO_PARAM1: {
        if (param_size != sizeof(char)) {
            LOG_MSG_CF(LOG_ERROR, "Received param size %d Bytes is not correct. Expected size is %zu", param_size, sizeof(char));
            return E_PARAM_SIZE_INVALID;
        }
        algo_handle_ptr->param1 = *(char *)param;
        LOG_MSG_CF(LOG_INFO, "set param1: %c", algo_handle_ptr->param1);
        break;
    }
    case ALGO_PARAM2: {
        if (param_size != sizeof(float)) {
            LOG_MSG_CF(LOG_ERROR, "Received param size: %d Bytes is not correct. Expected size is %zu", param_size, sizeof(float));
            return E_PARAM_SIZE_INVALID;
        }
        algo_handle_ptr->param2 = *(float *)param;
        LOG_MSG_CF(LOG_INFO, "set param2: %.1f", algo_handle_ptr->param2);
        break;
    }
    case ALGO_PARAM3: {
        if (param_size > MAX_BUF_SIZE) {
            LOG_MSG_CF(LOG_ERROR, "Received param size: %d Bytes is too large. Max size is %u", param_size, MAX_BUF_SIZE);
            return E_PARAM_SIZE_INVALID;
        }
        memset(algo_handle_ptr->param3, 0, MAX_BUF_SIZE);
        memcpy(algo_handle_ptr->param3, param, param_size);
        LOG_MSG_CF(LOG_INFO, "set param3: %s", algo_handle_ptr->param3);
        break;
    }
    case ALGO_PARAM4: {
        if (param_size > MAX_BUF_SIZE) {
            LOG_MSG_CF(LOG_ERROR, "Received param size: %d Bytes is too large. Max size is %u", param_size, MAX_BUF_SIZE);
            return E_PARAM_SIZE_INVALID;
        }
        algo_handle_ptr->param4 = (float *)malloc(param_size);
        if (algo_handle_ptr->param4 == NULL) {
            LOG_MSG_CF(LOG_ERROR, "allocate %d Bytes for param4 failed", param_size);
            return E_ALLOCATE_FAILED;
        }
        memcpy(algo_handle_ptr->param4, param, param_size);
        break;
    }
    default:
        LOG_MSG_CF(LOG_ERROR, "cmd %d is invalid", cmd);
        return E_PARAM_OUT_OF_RANGE;
    }
    return E_OK;
}

int algo_get_param(void *algo_handle, algo_param_t cmd, void *param, int param_size)
{
    if (algo_handle == NULL) {
        LOG_MSG_CF(LOG_ERROR, "algo_handle is NULL");
        return E_ALGO_HANDLE_NULL;
    }
    if (param == NULL) {
        LOG_MSG_CF(LOG_ERROR, "param is NULL");
        return E_PARAM_BUFFER_NULL;
    }

    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;
    switch (cmd) {
    case ALGO_PARAM1: {
        if (param_size != sizeof(char)) {
            LOG_MSG_CF(LOG_ERROR, "param_size is not correct");
            return E_PARAM_SIZE_INVALID;
        }
        *(char *)param = algo_handle_ptr->param1;
        LOG_MSG_CF(LOG_INFO, "get param1: %c", algo_handle_ptr->param1);
        break;
    }
    case ALGO_PARAM2: {
        if (param_size != sizeof(float)) {
            LOG_MSG_CF(LOG_ERROR, "param_size is not correct");
            return E_PARAM_SIZE_INVALID;
        }
        *(float *)param = algo_handle_ptr->param2;
        LOG_MSG_CF(LOG_INFO, "get param2: %.1f", algo_handle_ptr->param2);
        break;
    }
    case ALGO_PARAM3: {
        if (param_size > MAX_BUF_SIZE) {
            LOG_MSG_CF(LOG_ERROR, "param_size is too large");
            return E_PARAM_SIZE_INVALID;
        }
        memcpy(param, algo_handle_ptr->param3, param_size);
        LOG_MSG_CF(LOG_INFO, "get param3: %s", algo_handle_ptr->param3);
        break;
    }
    case ALGO_PARAM4: {
        if (param_size > MAX_BUF_SIZE) {
            LOG_MSG_CF(LOG_ERROR, "param_size: %d is too large than MAX_BUF_SIZE %d", param_size, MAX_BUF_SIZE);
            return E_PARAM_SIZE_INVALID;
        }
        memcpy(param, algo_handle_ptr->param4, param_size);
        break;
    }
    default:
        LOG_MSG_CF(LOG_ERROR, "cmd is invalid");
        return E_PARAM_OUT_OF_RANGE;
    }
    return E_OK;
}

int algo_process(void *algo_handle, float *input, float *output, int block_size)
{
    if (algo_handle == NULL) {
        return E_ALGO_HANDLE_NULL;
    }
    if (input == NULL) {
        LOG_MSG_CF(LOG_ERROR, "input is NULL");
        return E_PARAM_BUFFER_NULL;
    }
    if (output == NULL) {
        LOG_MSG_CF(LOG_ERROR, "output is NULL");
        return E_PARAM_BUFFER_NULL;
    }
    if (block_size <= 0) {
        LOG_MSG_CF(LOG_ERROR, "block_size is not correct");
        return E_PARAM_SIZE_INVALID;
    }
    p_algo_handle_t algo_handle_ptr = (p_algo_handle_t)algo_handle;

    if (algo_handle_ptr->param2 == 0.0f) {
        memcpy(output, input, block_size * sizeof(float));
        return E_OK;
    }

    for (int i = 0; i < block_size; i++) {
        float result = input[i] * dBChangeToFactor(algo_handle_ptr->param2);
        output[i] = result;
    }

    return E_OK;
}
