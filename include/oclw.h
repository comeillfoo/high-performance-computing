#ifndef _OCLW_H_
#define _OCLW_H_

#include <CL/cl.h>

int oclw_get_platform(cl_platform_id* platform_id);
int oclw_select_device(cl_platform_id platform_id, cl_device_id* device_id);
int oclw_create_context(cl_device_id* device_id, cl_context* ctx);
int oclw_destroy_context(cl_context ctx);
int oclw_create_cmd_queue(cl_context ctx, cl_device_id device_id,
                          cl_command_queue* queue);
int oclw_destroy_cmd_queue(cl_command_queue queue);
int oclw_create_program_from_binary(cl_context ctx, cl_device_id device_id,
                                    cl_program* program, size_t length,
                                    unsigned char* binary);
int oclw_destroy_program_object(cl_program program);
int oclw_create_kernobj_for_function(const char* function, cl_program program,
                                     cl_kernel* kernel);
int oclw_destroy_kernel_object(cl_kernel kernel);
char* oclw_query_device_name(cl_device_id device_id);

#endif // _OCLW_H_
