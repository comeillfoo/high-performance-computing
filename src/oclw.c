#include <stdio.h>
#include <inttypes.h>

#include "oclw.h"

static const char* oclw_error_msg(cl_int errno)
{
    switch (errno) {
    case CL_INVALID_VALUE: return "invalid value";
    case CL_OUT_OF_HOST_MEMORY: return "failed to allocate resources";
    case CL_SUCCESS: return "success";
    default: return "undefined error code";
    }
}

#define oclw_error(errno, message) \
    printf("%s: OCL[%"PRId32"]: %s at %s:%s:%d\n", (message), (errno), \
           oclw_error_msg(errno), __FILE__, __func__, __LINE__)

int oclw_get_platform(cl_platform_id* platform_id)
{
    cl_int cl_ret = 0;
    cl_uint ret_num_platforms;
    cl_ret = clGetPlatformIDs(1, platform_id, &ret_num_platforms);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to get platform ID");
    return 1;
}

int oclw_select_device(cl_platform_id platform_id, cl_device_id* device_id)
{
    cl_int cl_ret = 0;
    cl_uint ret_num_devices;
    cl_ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, device_id,
                            &ret_num_devices);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to get device ID");
    return 1;
}

int oclw_create_context(cl_device_id* device_id, cl_context* ctx)
{
    cl_int cl_ret = 0;
    *ctx = clCreateContext(NULL, 1, device_id, NULL, NULL, &cl_ret);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to create OCL context");
    return 1;
}

int oclw_destroy_context(cl_context ctx)
{
    cl_int cl_ret = 0;
    cl_ret = clReleaseContext(ctx);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to destroy OCL context");
    return 1;
}
