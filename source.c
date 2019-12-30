#include <stdio.h>
#include <stdlib.h>
#include "time.h"

#include <CL/cl.h>


#define MAX_SOURCE_SIZE (0x100000)

#define CHECK(ret)   if((ret)!=CL_SUCCESS) printf("error at line %d, %d", __LINE__, ret), exit(ret);
inline clock_t chkpoint()
{
    return clock();
}

int type3(void) {
    // Create the two input vectors

    clock_t st, et;
    int currentPage = 0;
    const  long PAGE_SIZE = 0x400*0x200; //global work item size = 1024 polys x 0x200
    const  long LIST_SIZE = 16; //16 bytes

    cl_uchar* C = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE * PAGE_SIZE);
    cl_uchar* A = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE * PAGE_SIZE);
    cl_ulong* startPoly = (cl_ulong*)malloc(sizeof(cl_ulong));

    // Load the kernel source code into the array source_str
    FILE* fp;
    char* source_str;
    size_t source_size;

    fp = fopen("init_ab.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load cl file.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    CHECK(ret);

    CHECK(ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1,
        &device_id, &ret_num_devices));

    // Create an OpenCL context
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    CHECK(ret);
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
        PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), NULL, &ret);
    CHECK(ret);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
        PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), NULL, &ret);
    CHECK(ret);

    cl_mem startPoly_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
        sizeof(cl_ulong), NULL, &ret);
    CHECK(ret);


    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
        (const char**)&source_str, (const size_t*)&source_size, &ret);

    // Build the program
    CHECK(ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL));
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        // Allocate memory for the log
        char* log = (char*)malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        printf("%s\n", log);
        free(log);
        exit(-1);
    }
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "init_a", &ret);
    
    // Set the arguments of the kernel
    CHECK(ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&c_mem_obj));

    // Execute the OpenCL kernel on the list
    size_t global_item_size = PAGE_SIZE; // Process the entire lists
    //size_t local_item_size = 128; // Process in groups of 8
    st = chkpoint();
    for (currentPage = 0; currentPage < 4096; ++currentPage)
    {
        *startPoly = PAGE_SIZE * currentPage;
        CHECK(ret = clEnqueueWriteBuffer(command_queue, startPoly_mem_obj, CL_FALSE, 0, sizeof(cl_ulong), startPoly, 0, NULL, NULL));

        CHECK(ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &startPoly_mem_obj));

        CHECK(ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
            &global_item_size, NULL, 0, NULL, NULL));

        // Read the memory buffer C on the device to the local variable C
        CHECK(ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0,
            PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), C, 0, NULL, NULL));

        // Display the result to the screen
        //for (i = 0; i < LIST_SIZE; i++)
        //    printf("%d + %d = %d\n", A[i]);

        //for (int itm = 0;  itm < 10; itm++)    //show first 10 lines of the page
        //{
        //    for (int i = 0; i < LIST_SIZE; i++)
        //    {
        //        printf(" %2.2X", C[itm * LIST_SIZE  +i]);
        //    }
        //    printf(" --- ");
        //    for (int i = 0; i < LIST_SIZE / 2; i++)
        //    {
        //        printf("%2.2x ", (cl_uchar)((cl_uchar*)startPoly)[i]);
        //    }
        //    printf("\n");
        //}
    }

    et = chkpoint();
    printf("\ntime:%ld - %ld =  %f", et, st, (et - st) / (float)CLOCKS_PER_SEC);
    // Clean up
    //ret = clFlush(command_queue);
    //ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
    ret = clReleaseMemObject(c_mem_obj);
    ret = clReleaseMemObject(startPoly_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    free(A);
    //free(B);
    free(C);
    free(startPoly);
    return 0;
}

int main(int argc, char** argv)
{
    type3();
}