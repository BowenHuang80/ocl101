#include <stdio.h>
#include <stdlib.h>
#include "time.h"

#include <CL/cl.h>


#define MAX_SOURCE_SIZE (0x100000)

#define CHECK(ret)   if((ret)!=0) printf("error at line %d, %d", __LINE__, ret);
inline clock_t chkpoint()
{
    return clock();
}
int main(void) {
    // Create the two input vectors

    clock_t st, et;
    const  long PAGE_SIZE = 0x400*0x200; //1024 polys x 
    const  LIST_SIZE = 16; //16 bytes
    //UInt8 poly[6] = { 0 };
    //cl_uchar* A = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE * PAGE_SIZE);
    //cl_uchar* B = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE);
    cl_uchar* C = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE * PAGE_SIZE);
    //cl_uchar* D = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE);
    //cl_uchar* E = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE);
    //cl_uchar* poly = (cl_uchar*)malloc(sizeof(cl_uchar) * LIST_SIZE * PAGE_SIZE);

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

    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1,
        &device_id, &ret_num_devices);
    CHECK(ret);
    // Create an OpenCL context
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    CHECK(ret);
    // Create memory buffers on the device for each vector 
    //cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
    //    PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), NULL, &ret);
    //CHECK(ret);
    //cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
    //    LIST_SIZE * sizeof(int), NULL, &ret);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
        PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), NULL, &ret);
    //cl_mem d_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
    //    LIST_SIZE * sizeof(int), NULL, &ret);
    //cl_mem e_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
    //    LIST_SIZE * sizeof(int), NULL, &ret);
    //cl_mem poly_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
    //    PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), NULL, &ret);


    // Copy the lists A and B to their respective memory buffers
    //ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
    //    LIST_SIZE * sizeof(int), A, 0, NULL, NULL);
    //ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0,
    //    LIST_SIZE * sizeof(int), B, 0, NULL, NULL);

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
    }
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "init_a", &ret);
    
    // Set the arguments of the kernel
    CHECK(ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&c_mem_obj));
    //CHECK(ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&c_mem_obj));
    //CHECK(ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&poly_mem_obj));

    // Execute the OpenCL kernel on the list
    size_t global_item_size = PAGE_SIZE; // Process the entire lists
    size_t local_item_size = 16; // Process in groups of 8
    st = chkpoint();

    CHECK(ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
        &global_item_size, NULL, 0, NULL, NULL));

    // Read the memory buffer C on the device to the local variable C

    ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0,
        PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), C, 0, NULL, NULL);
    //ret = clEnqueueReadBuffer(command_queue, poly_mem_obj, CL_TRUE, 0,
    //    PAGE_SIZE * LIST_SIZE * sizeof(cl_uchar), poly, 0, NULL, NULL);
    et = chkpoint();
    printf("\ntime:%ld - %ld =  %f", et, st, (et - st)/ (float)CLOCKS_PER_SEC);
    // Display the result to the screen
    //for (i = 0; i < LIST_SIZE; i++)
    //    printf("%d + %d = %d\n", A[i]);

    //for (int pg = 0;  pg < PAGE_SIZE/2; pg++)
    //{
    //    for (int i = 0; i < LIST_SIZE; i++)
    //    {
    //        printf(" %2.2X", C[pg*LIST_SIZE +i]);
    //    }
    //    printf(" --- ");
    //    for (int i = 0; i < LIST_SIZE / 2; i++)
    //    {
    //        printf("%2.2X ", poly[pg * LIST_SIZE + i]);
    //    }
    //    printf("\n");
    //}
    // Clean up
    //ret = clFlush(command_queue);
    //ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    //ret = clReleaseMemObject(a_mem_obj);
    //ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(c_mem_obj);
    //ret = clReleaseMemObject(d_mem_obj);
    //ret = clReleaseMemObject(e_mem_obj);
    //ret = clReleaseMemObject(poly_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    //free(A);
    //free(B);
    free(C);
    //free(poly);
    return 0;
}

