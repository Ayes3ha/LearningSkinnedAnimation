#include "tool.h"

double get_cpu_time(void) {
	LARGE_INTEGER frequency;
	LARGE_INTEGER counter;

	// 获取CPU时钟频率
	QueryPerformanceFrequency(&frequency);
	//printf("%d\n", frequency.QuadPart);
	// 获取CPU的运行次数
	QueryPerformanceCounter(&counter);
	//printf("%d\n", counter.QuadPart);

	// 开始渲染的时间
	return (double)counter.QuadPart / frequency.QuadPart;
}

// 求重心坐标
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
	Vec3f s[2];
	
	for (int i = 0; i < 2; i++) {
		s[i][0] = B[i] - A[i];
		s[i][1] = C[i] - A[i];
		s[i][2] = A[i] - P[i];
	}

	Vec3f c = s[0] ^ s[1];

	if (std::abs(c.z) > 1e-2)
		return Vec3f(1.f - (c.x + c.y)/c.z, c.x/c.z, c.y/c.z);
	return Vec3f(-1, 1, 1);
}