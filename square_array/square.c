#define PROGRAM_FILE "square.cl"
#define KERNEL_FUNC "square"
#define MAX_CUS 24 // Max number of GPU compute units
#define WG_SIZE 256 // Workgroup size

#include "defs.h"


int main() {

   /* OpenCL structures */
   cl_device_id device;
   cl_context context;
   cl_program program;
   cl_kernel kernel;
   cl_command_queue queue;
   cl_int i, err;
   int ARRAY_SIZE=1000; // size of array
   size_t local_size, global_size;

   /* Data and buffers    */
   // Host input and output vectors
   float *data, *output;
   // Device input and output buffers
   cl_mem input_buffer, out_buffer;

   /* Initialize data */
   // Allocate host arrays
   data=(float*)malloc(ARRAY_SIZE*sizeof(float));
   output=(float*)malloc(ARRAY_SIZE*sizeof(float));

   for(i=0; i<ARRAY_SIZE; i++) {
      data[i] = 1.0f*i;
   }

   /* Create device and context 

   Creates a context containing only one device — the device structure 
   created earlier.
   */
   device = create_device();
   context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);

   /* Build program */
   program = build_program(context, device, PROGRAM_FILE);

   /* Create data buffer  */
   input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float), data, &err); // <=====INPUT
   out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float), output, &err); // <=====OUTPUT

   /* Create a command queue 

   Does not support profiling or out-of-order-execution
   */
   queue = clCreateCommandQueue(context, device, 0, &err);

   /* Create a kernel */
   kernel = clCreateKernel(program, KERNEL_FUNC, &err);

   /* Create kernel arguments 
   
   The integers below represent the position of the kernel argument.
   */
   err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer); // <=====INPUT
   err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &out_buffer); // <=====OUTPUT
   err |= clSetKernelArg(kernel, 2, sizeof(int), &ARRAY_SIZE);

   /*
   • `global_size`: total number of work items that will be 
      executed on the GPU (e.g. total size of your array)
   • `local_size`: size of local workgroup. Each workgroup contains 
      several work items and goes to a compute unit   

   To map our problem onto the underlying hardware we must specify a 
   local and global integer size. The local size defines the number 
   of work items in a work group, on an NVIDIA GPU this is equivalent 
   to the number of threads in a thread block. The global size is the 
   total number of work items launched. the localSize must be a devisor 
   of globalSize and so we calculate the smallest integer that covers 
   our problem domain and is divisible by localSize.

   Notes: 
   • Intel recommends workgroup size of 64-128. Often 128 is minimum to 
   get good performance on GPU
   • On NVIDIA Fermi, workgroup size must be at least 192 for full 
   utilization of cores
   • Optimal workgroup size differs across applications
   */
   // Number of work items in each local work group
   local_size = WG_SIZE;
   // Number of total work items - localSize must be devisor
   global_size = ceil(ARRAY_SIZE/(float)local_size)*local_size;
   //size_t global_size[3] = {ARRAY_SIZE, 0, 0};
   //size_t local_size[3] = {WG_SIZE, 0, 0};
   //global_size = ARRAY_SIZE;
   //local_size = WG_SIZE; 

   /* Enqueue kernel 

   At this point, the application has created all the data structures 
   (device, kernel, program, command queue, and context) needed by an 
   OpenCL host application. Now, it deploys the kernel to a device.

   Of the OpenCL functions that run on the host, clEnqueueNDRangeKernel 
   is probably the most important to understand. Not only does it deploy 
   kernels to devices, it also identifies how many work-items should 
   be generated to execute the kernel (global_size) and the number of 
   work-items in each work-group (local_size).

   clEnqueueNDRangeKernel args:
   3. Number of dimensions of the argument
   5. global size of data that will be handled. If you were dealing with a
      2D image, you would use: 
      size_t global[3] = {nx, ny, 0}
      err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, 
         &local_size, 0, NULL, NULL); 
   */
   err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL); 
   //clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_size, local_size, 0, NULL, NULL); 

   /* Wait for the command queue to get serviced before reading 
   back results */
   clFinish(queue);

   /* Read the kernel's output    */
   clEnqueueReadBuffer(queue, out_buffer, CL_TRUE, 0, sizeof(output), output, 0, NULL, NULL); // <=====GET OUTPUT

   /* Check result */
   for (i=0; i<ARRAY_SIZE; i++) {
      printf("%f ", output[i]);
   } 

   /* Deallocate resources */
   clReleaseKernel(kernel);
   clReleaseMemObject(out_buffer);
   clReleaseMemObject(input_buffer);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);
   return 0;
}
