#ifndef _OCLW_H_
#define _OCLW_H_

#include <CL/cl.h>

int oclw_get_default_platform(cl_platform_id* platform_id);
int oclw_get_default_device(cl_platform_id platform_id, cl_device_id* device_id);
int oclw_create_context(cl_device_id* device_id, cl_context* ctx);
int oclw_destroy_context(cl_context ctx);
int oclw_create_cmd_queue(cl_context ctx, cl_device_id device_id,
                          cl_command_queue* queue);
int oclw_destroy_cmd_queue(cl_command_queue queue);
int oclw_create_program_from_source(cl_context ctx, cl_uint count,
                                    const char* sources[count],
                                    const size_t lengths[count],
                                    cl_program* program);
int oclw_create_program_from_binary(cl_context ctx, cl_device_id device_id,
                                    cl_program* program, size_t length,
                                    unsigned char* binary);
int oclw_destroy_program_object(cl_program program);
int oclw_build_program(cl_program program, cl_device_id device_id,
                       const char* options);
int oclw_create_kernobj_for_function(const char* function, cl_program program,
                                     cl_kernel* kernel);
int oclw_destroy_kernel_object(cl_kernel kernel);
char* oclw_query_device_name(cl_device_id device_id);
char* oclw_query_build_log(cl_program program, cl_device_id device_id);
ssize_t oclw_query_single_binary_size(cl_program program);
unsigned char* oclw_query_single_binary(cl_program program, size_t binary_size);
int oclw_query_event_status(cl_event event, cl_int* event_status);
int oclw_create_memobj(cl_context ctx, cl_mem_flags flags, cl_mem* mem,
                       size_t size, void* host_ptr);
int oclw_destroy_memobj(cl_mem mem);
int oclw_sync_write_memobj(cl_command_queue queue, cl_mem mem, size_t size,
                           void* ptr);
int oclw_sync_read_memobj(cl_command_queue queue, cl_mem mem, size_t size,
                          void* ptr);
int oclw_sync_run_task(cl_command_queue queue, cl_kernel kernel);
int oclw_set_kernel_arg(cl_kernel kernel, cl_uint index, size_t arg_size,
                        void* arg_value, const char* arg_name);

int oclw_set_filter_fold_args(cl_kernel kernel, double min, size_t M, cl_mem* M2,
                              cl_mem* X);
int oclw_set_map_sqrt_exp_args(cl_kernel kernel, size_t N, cl_mem* M1);

#endif // _OCLW_H_
