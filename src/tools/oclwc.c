#include <stdio.h>
#include <getopt.h>
#include <CL/cl.h>

#include "oclw.h"
#include "futils.h"

#define LOGC "oclwc: "

static int arg_parse(int argc, char** argv);

int main(int argc, char** argv)
{
    int ret = arg_parse(argc, argv);
    if (ret) goto exit;
    cl_platform_id cl_platform_id = NULL;
    cl_device_id cl_device_id = NULL;
    cl_context cl_context = NULL;
    cl_program cl_program = NULL;
    char* build_log = NULL;
    ssize_t binary_size;
    unsigned char* binary = NULL;

    const size_t target_count = argc - 2;
    size_t count = 0;
    FILE* fp = NULL;
    ssize_t file_size = -1;
    char** sources = NULL;
    size_t* lengths = NULL;

    sources = malloc(sizeof(char*) * target_count);
    if (!sources) {
        ret = 1;
        goto exit;
    }
    lengths = malloc(sizeof(size_t) * target_count);
    if (!lengths) goto free_sources;

    for (count = 0; count < target_count; ++count) {
        const char* path = argv[count + 2];
        fp = fopen(path, "r");
        if (!fp) {
            perror(LOGC "Unable to open file");
            ret = 1;
            goto free_sources;
        }
        file_size = fget_size_verbose(fp);
        if (file_size == -1) goto free_sources;
        lengths[count] = sizeof(char) * file_size;

        sources[count] = malloc(lengths[count]);
        if (!sources[count]) goto free_sources;

        if (fread(sources[count], sizeof(char), file_size,
                  fp) < file_size) {
            fprintf(stderr, LOGC "READ FAILED[%zu/%zu]: unable to read source"
                    " at %s\n", count + 1, target_count, path);
            free(sources[count]);
            ret = fclose_verbose(fp) | 1;
            goto free_sources;
        }
        ret = fclose_verbose(fp);
        if (ret) goto free_sources;
        printf(LOGC "READ SUCCEED[%zu/%zu] %s\n", count + 1, target_count, path);
    }

    ret = oclw_get_default_platform(&cl_platform_id);
    if (ret) goto free_sources;
    ret = oclw_get_default_device(cl_platform_id, &cl_device_id);
    if (ret) goto free_sources;
    ret = oclw_create_context(&cl_device_id, &cl_context);
    if (ret) goto free_sources;
    ret = oclw_create_program_from_source(cl_context, target_count,
                                          (const char**) sources,
                                          (const size_t*) lengths, &cl_program);
    if (ret) goto free_context;

    ret = oclw_build_program(cl_program, cl_device_id, "-Werror -cl-std=CL3.0");
    if (ret) {
        build_log = oclw_query_build_log(cl_program, cl_device_id);
        fprintf(stderr, LOGC "BUILD FAILED\n");
        if (!build_log)
            goto free_program;
        fprintf(stderr, "---- BUILD LOG START ----\n%s\n---- BUILD LOG END ----\n",
                build_log);
        free(build_log);
        goto free_program;
    }
    fprintf(stderr, LOGC "BUILD SUCCEED\n");

    binary_size = oclw_query_single_binary_size(cl_program);
    if (binary_size == -1) {
        ret = 1;
        goto free_program;
    }
    binary = oclw_query_single_binary(cl_program, binary_size);
    if (!binary) {
        ret = 1;
        goto free_program;
    }

    fp = fopen(argv[1], "wb");
    if (!fp) {
        perror(LOGC "unable to open file on writing");
        ret = 1;
        goto save_end;
    }
    if (fwrite(binary, sizeof(unsigned char), binary_size, fp) < binary_size) {
        fprintf(stderr, LOGC "unable to write program binary to file\n");
        ret = 1;
    }
    ret |= fclose_verbose(fp);

save_end:
    fprintf(stderr, LOGC "SAVE %s: writing program to %s\n",
            ret ? "FAILED" : "SUCCEED", argv[1]);
    free(binary);
free_program:
    ret |= oclw_destroy_program_object(cl_program);
free_context:
    ret |= oclw_destroy_context(cl_context);
free_sources:
    for (size_t i = 0; i < count; ++i) free(sources[i]);
    free(sources);
exit:
    return ret;
}

static int arg_parse(int argc, char** argv)
{
    if (argc > 2) return 0;
    printf("Usage: %s TARGET FILE [FILE...]\n\n", argv[0]);
    printf("Arguments:\n");
    printf("   TARGET    Path where to store built program\n");
    printf("   FILE      Path to file for compile\n");
    return 1;
}
