#include <stdio.h>
#include <inttypes.h>

#include "oclw.h"

static const char* oclw_error_msg(cl_int errno)
{
    switch (errno) {
    case CL_INVALID_VALUE: return "invalid value";
    case CL_OUT_OF_HOST_MEMORY: return "failed to allocate resources on host";
    case CL_INVALID_BINARY: return "not a valid binary";
    case CL_INVALID_PROGRAM: return "not a valid program object";
    case CL_INVALID_PROGRAM_EXECUTABLE:
        return "no successfully built executable";
    case CL_INVALID_KERNEL_NAME: return "kernel name is not found in program";
    case CL_INVALID_KERNEL_DEFINITION:
        return "function definition differs from built for device";
    case CL_OUT_OF_RESOURCES: return "failed to allocate resources on device";
    case CL_SUCCESS: return "success";
    default: return "undefined error code";
    }
}

#define oclw_error(errno, message) \
    fprintf(stderr, "%s: OCL[%"PRId32"]: %s at %s:%s:%d\n", (message), (errno), \
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
    cl_int cl_ret = clReleaseContext(ctx);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to destroy OCL context");
    return 1;
}

int oclw_create_cmd_queue(cl_context ctx, cl_device_id device_id,
                          cl_command_queue* queue)
{
    cl_int cl_ret = 0;
    *queue = clCreateCommandQueueWithProperties(ctx, device_id, NULL, &cl_ret);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to create command queue");
    return 1;
}

int oclw_destroy_cmd_queue(cl_command_queue queue)
{
    cl_int cl_ret = clReleaseCommandQueue(queue);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to destroy command queue");
    return 1;
}

int oclw_create_program_from_binary(cl_context ctx, cl_device_id device_id,
                                    cl_program* program, size_t length,
                                    unsigned char* binary)
{
    cl_int cl_ret = 0;
    cl_int binary_status[1];
    *program = clCreateProgramWithBinary(ctx, 1,
                                         (const cl_device_id[]) { device_id },
                                         (const size_t[]) { length },
                                         (const unsigned char*[]) { binary },
                                         binary_status, &cl_ret);

    if (cl_ret == CL_SUCCESS && binary_status[0] == CL_SUCCESS) return 0;
    if (cl_ret != CL_SUCCESS)
        oclw_error(cl_ret, "Unable to create program object with single binary");
    if (binary_status[0] != CL_SUCCESS)
        oclw_error(binary_status[0], "Unable to load program binary for specified device");
    return 1;
}

int oclw_destroy_program_object(cl_program program)
{
    cl_int cl_ret = clReleaseProgram(program);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to destroy program object");
    return 1;
}

#define oclw_error_function(errno, function) \
    fprintf(stderr, \
            "Unable to create kernel for function \'%s\': OCL[%"PRId32"]: %s at" \
            " %s:%s:%d\n", (function), (errno), \
            oclw_error_msg(errno), __FILE__, __func__, __LINE__)

int oclw_create_kernobj_for_function(const char* function, cl_program program,
                                     cl_kernel* kernel)
{
    cl_int cl_ret = 0;
    *kernel = clCreateKernel(program, function, &cl_ret);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error_function(cl_ret, function);
    return 1;
}

int oclw_destroy_kernel_object(cl_kernel kernel)
{
    cl_int cl_ret = clReleaseKernel(kernel);
    if (cl_ret == CL_SUCCESS) return 0;
    oclw_error(cl_ret, "Unable to destroy kernel object");
    return 1;
}

char* oclw_query_device_name(cl_device_id device_id)
{
    char* device_name = NULL;
    size_t length = 0;
    cl_int cl_ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &length);
    if (!length) return NULL;

    device_name = malloc(sizeof(char) * length);
    if (!device_name) return NULL;
    cl_ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, length, device_name, NULL);
    if (cl_ret == CL_SUCCESS) return device_name;
    free(device_name);
    oclw_error(cl_ret, "Unable to get device name");
    return NULL;
}

// int oclw_query_build_log(cl_program program, cl_device_id device_id)
// {
//     cl_program_build_info param_name = CL_PROGRAM_BUILD_LOG;
//     size_t param_value_size;
//     unsigned char param_value[42];
//     size_t param_value_size_ret = 0;
//     cl_int cl_ret = clGetProgramBuildInfo(program, device_id, param_name,
//                                           param_value_size, param_value,
//                                           &param_value_size_ret);
//     if (cl_ret != CL_SUCCESS) {
//         oclw_error(cl_ret, "Unable to query build log");
//         return 1;
//     }
//     return 0;
// }
