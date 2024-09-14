#ifndef _OCLW_H_
#define _OCLW_H_

#include <CL/cl.h>

int oclw_get_platform(cl_platform_id* platform_id);
int oclw_select_device(cl_platform_id platform_id, cl_device_id* device_id);
int oclw_create_context(cl_device_id* device_id, cl_context* ctx);
int oclw_destroy_context(cl_context ctx);

#endif // _OCLW_H_
