
// ref. http://the-united-front.blogspot.jp/search/label/OpenCL%E3%81%AB%E3%82%88%E3%82%8BGPGPU

#include <CL/cl.h>

#include <stdio.h>
#include <vector>

void GetPlatformIDs(std::vector<cl_platform_id> *out)
{
	out->clear();

	cl_int result;
	cl_uint count;

	result = clGetPlatformIDs(0, nullptr, &count);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When searching number of platforms\n");
		fprintf(stderr, "clGetPlatformIDs() was failed. return == %d\n", result);
		return;
	}

	out->resize(count);

	result = clGetPlatformIDs(count, &out->at(0), nullptr);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform IDs\n");
		fprintf(stderr, "clGetPlatformIDs() failed. return == %d\n", result);
		return;
	}
}

std::string GetPlatformInfo(cl_platform_id platform_ID, cl_platform_info param)
{
	cl_int result;
	size_t required_buff_size;

	result = clGetPlatformInfo(platform_ID, param, 0, nullptr, &required_buff_size);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform info size\n");
		fprintf(stderr, "clGetPlatformInfo() failed. return == %d\n", result);
		return nullptr;
	}
	if (required_buff_size < 1) return std::string();

	std::vector<char> buff(required_buff_size);

	result = clGetPlatformInfo(platform_ID, param, buff.size(), &buff[0], nullptr);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform info\n");
		fprintf(stderr, "clGetPlatformInfo() failed. return == %d\n", result);
		return nullptr;
	}

	char const *left = &buff[0];
	char const *right = left + buff.size();
	while (left < right && right[-1] == 0) right--;
	return std::string(left, right);
}

void GetDeviceIDs(cl_platform_id platform_ID, cl_device_info param, std::vector<cl_device_id> *out)
{
	out->clear();

	cl_uint count;
	cl_int result;

	result = clGetDeviceIDs(platform_ID, param, 0, nullptr, &count);
	if (result != CL_SUCCESS ){
		fprintf(stderr, "When trying to get number of devices\n");
		fprintf(stderr, "clGetDeviceIDs() failed. return == %d\n", result);
		return;
	}
	if (count < 1) return;

	out->resize(count);

	result = clGetDeviceIDs(platform_ID, param, count, &out->at(0), 0 );
	if (result != CL_SUCCESS ){
		fprintf(stderr, "When trying to get device IDs\n");
		fprintf(stderr, "clGetDeviceIDs() failed. return == %d\n", result);
		return;
	}
}

bool GetDeviceInfo(cl_device_id device_ID, cl_device_info param, std::vector<char> *out)
{
	out->clear();

	cl_int result;

	size_t required_buff_size;
	result = clGetDeviceInfo(device_ID, param, 0, nullptr, &required_buff_size );
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get device info size\n");
		fprintf(stderr, "clGetDeviceInfo() failed. return == %d\n", result);
		return false;
	}
	if (required_buff_size < 1) return false;

	out->resize(required_buff_size);

	result = clGetDeviceInfo(device_ID, param, required_buff_size, &out->at(0), nullptr );
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform info\n");
		fprintf(stderr, "clGetPlatformInfo() failed. return == %d\n", result);
		return false;
	}

	return true;
}

std::string GetProgramBuildLog(cl_program program, cl_device_id device_id)
{
	size_t len;
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, nullptr, &len);
	std::vector<char> log(len);
	char *left = &log[0];
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, left, nullptr);
	char *right = left + len;
	while (left < right && right[-1] == 0) right--;
	return std::string(left, right);
}

int main(int argc, char *argv[])
{
	cl_device_id device_id;
	{
		std::vector<cl_platform_id> platformids;
		GetPlatformIDs(&platformids);
		for (cl_platform_id id : platformids) {
			std::vector<cl_device_id> devs;
			GetDeviceIDs(id, CL_DEVICE_TYPE_GPU, &devs);
			if (devs.size() > 0) {
				device_id = devs[0];
			}
		}
	}
	if (!device_id) {
		fprintf(stderr, "Valid device not found\n");
		return 1;
	}

	unsigned char table[9 * 9];

	std::string source =
			"__kernel void kuku(__global uchar *p)"
			"{"
			"    uint i = get_global_id(0);"
			"    uint j = get_global_id(1);"
			"    p[i * 9 + j] = (i + 1) * (j + 1);"
			"}"
			;
	std::string name = "kuku";

	cl_int ret;

	cl_context context = clCreateContext(nullptr, 1, &device_id, nullptr, nullptr, &ret);
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	char const *srcptr = source.c_str();
	size_t srclen = source.size();
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&srcptr, (size_t const *)&srclen, &ret);
	ret = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
	if (ret == CL_BUILD_PROGRAM_FAILURE) {
		std::string s = GetProgramBuildLog(program, device_id);
		fprintf(stderr, "%s\n", s.c_str());
		return 1;
	}
	cl_kernel kernel = clCreateKernel(program, name.c_str(), &ret);

	cl_mem clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, 9 * 9, nullptr, &ret);

	clSetKernelArg(kernel, 0, sizeof(cl_mem), &clmem);

	int dim = 2;
	size_t global_item_size[] = { 9, 9 };
	clEnqueueNDRangeKernel(command_queue, kernel, dim, nullptr, global_item_size, nullptr, 0, nullptr, nullptr);
	clEnqueueReadBuffer(command_queue, clmem, CL_TRUE, 0, sizeof(table), table, 0, nullptr, nullptr);
	clFlush(command_queue);
	clFinish(command_queue);

	clReleaseMemObject(clmem);

	clReleaseKernel(kernel);
	clReleaseProgram(program);

	// print result
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			printf(" %2d", table[i * 9 + j]);
		}
		putchar('\n');
	}

	return 0;
}
