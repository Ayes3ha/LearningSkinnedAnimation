#include <windows.h>
#include <stdio.h>
//#include <stdlib.h>
#include <iostream>
#include <thread>

#include "public.h"
#include "tool.h"
#include "geometry.h"
#include "mesh.h"
#include "graphic.h"
#include "tgaimage.h"


using namespace std;

// 关键变量
static HWND hwnd;			// 窗口id
static HDC screen_dc;		//                                                                              
// BITMAP
static HBITMAP screen_hb;	// 新的bitmap
static HBITMAP screen_ob;	// 老的bitmap
unsigned int* screen_fb;	// 缓冲区

// *******消息响应相关*********
// 按键缓冲区
int keys_buffer[512];
// 测试回调函数中的消息系统重定向！
short zDelta = 0;
// 响应标志
// 若响应了一些会改变矩阵输入信息的信息后，flag变为true，我们更新MVP矩阵
bool flag = false;

// ****************************
// 灯光位置
Vec3f light_pos(1.f, 1.f, 1.5f);
// LookAt矩阵相关
// 相机位置
Vec3f camera_pos(0.f, 0.f, 5.f);
// Camera看着/对准的地方
Vec3f center(0, 0, 0);
// 用于定位x轴用的临时y轴(y cross z = x)
// 这样定位出来的x轴的正方向是 从左眼到右眼
Vec3f up(0, -1, 0);		// 图像为什么会是倒过来的呢？？？不知道往哪儿改，暂时改这里吧


// 颜色
COLORREF white_color = 255 | (255) << 8 | (255) << 16 | (255) << 24;
COLORREF black_color = 0 | (0) << 8 | (0) << 16 | (255) << 24;
COLORREF back_color	 = 128 | (128) << 8 | (128) << 16 | (255) << 24;

// Z-buffer
float* zbuffer;

// 读取的模型文件路径
//string objPath = "./obj/Charmander.obj";
//string objPath = "./obj/african_head.obj";
const string objPath = "./obj/diablo3_pose.obj";
//const string objPath = "./obj/mary/Marry.obj";

// 纹理路径
//const char* texPath = "./obj/african_head_diffuse.tga";
const char* texPath = "./obj/diablo3_pose_diffuse.tga";
//const char* texPath = "./obj/mary/Marry_diffuse.tga";

// 模型
Mesh* mesh;
// TGA图像
TGAImage* image;
int byteDepth = 24;		// 比特深度

// 画
void Draw(HWND hwnd);

// 清空帧缓存
void clear_frameBuffer(unsigned int* frameBuffer, int width, int height, COLORREF color);
// 清空深度缓存
void clear_zbuffer(float* zbuffer, int width, int height);
// Shader结构优化&纹理
void Draw_2(HWND hwnd, Renderer renderer);
// Gouruad着色
void Draw_2_Gouruad(HWND hwnd, Renderer renderer, Vec3f lightPos);	
// 将vs与ps结合
void Draw_2_vs_and_ps(HWND hwnd, Renderer renderer);
// 尝试快速化渲染
void Draw_3_mutil(HWND hwnd, Renderer renderer);
// 尝试快速存储...（将第一次processShader得到的a2v存储起来，后续步骤就可以省去processShader）
void Draw_4_fastStorage(HWND hwnd, Renderer renderer, vector<a2v>* a2v_s_);
// 渲染单个三角形
void Draw_5_single(HWND hwnd, Renderer renderer);
// 尝试优化Shader和光栅化方面的东西
void Draw_6_test_Shader(HWND hwnd, Renderer renderer);

// 初始化窗口类并注册
void init_Register_wndclass(WNDCLASS& wndclass, HINSTANCE hInstance, TCHAR szAppName[]);
// 设置bitmapinfo
void set_bitmap(BITMAPINFO& bi, HDC& screen_dc, HBITMAP& screen_hb, HBITMAP& screen_ob,
	unsigned int*& screen_fb, LPVOID ptr);

// 回调
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// 主程序
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, PSTR szCmdLine, int iCmdShow)
//（1）程序ID
//（2）0
//（3）执行程序的命令行
//（4）指定窗口初始为最小化or最大化
{
	//************************************************************************************************
	// 准备工作
	//************************************************************************************************
	//1、声明关键变量
	static TCHAR szAppName[] = TEXT("bitmap!");			// 窗口类名
	MSG msg;											// 消息结构
	WNDCLASS wndclass;									// 窗口类
	BITMAPINFO bi;										// 存储bitmap各种信息的结构

	// 分配缓冲区
	LPVOID ptr = new LPVOID[width * height];

	// Console
	AllocConsole();								//创建控制台
	FILE* stream;
	freopen_s(&stream, "CON", "r", stdin);		// 重定向输入流
	freopen_s(&stream, "CON", "w", stdout);		// 重定向输出流

	//2、设置窗口类的属性
	//3、注册窗口类
	init_Register_wndclass(wndclass, hInstance, szAppName);
	
	//4、创建窗口实例
	hwnd = CreateWindow(
		szAppName,				// 窗口类名
		TEXT("bitmap_Test"),	// 窗口主题
		WS_OVERLAPPEDWINDOW,	// 窗口风格
		CW_USEDEFAULT,			// x偏移
		CW_USEDEFAULT,			// y偏移
		width + 15,				// 宽度
		height + 40,			// 高度
		NULL,					// 父窗口
		NULL,					// 菜单
		hInstance,				// 程序ID
		NULL					// 创建参数
	);

	// *****设置bitmap*****
	set_bitmap(bi, screen_dc, screen_hb, screen_ob, screen_fb, ptr);

	//5、显示窗口
	ShowWindow(hwnd, iCmdShow);
	//6、更新窗口
	UpdateWindow(hwnd);

	//************************************************************************************************
	//// **6、设置位图缓冲区的像素颜色**（fragment shader应该在这）

	// ***** new ******
	// 加载模型与纹理
	mesh = new Mesh(objPath, texPath);
	// 创建深度缓冲区
	float* zbuffer = new float[width * height];
	for (int i = 0; i < width * height; i++) zbuffer[i] = -(std::numeric_limits<float>::max)();
	// TGAImage
	image = new TGAImage(width, height, byteDepth);

	//************************************************************************************************
	// 模型变换
	float FOV = 45.f;
	float aspect = 4.f / 3.f;
	int n = 10;
	int f = 1000;
	//// 填充所有矩阵
	//Transformer transformer(world_width, world_height, world_depth, 0, 0, 0,
	//	camera_pos, center, up, 
	//	FOV, aspect, n, f,
	//	width, height, depth, v_x, v_y);
	// 不填充Model矩阵
	Transformer transformer(camera_pos, center, up,
		FOV, aspect, n, f,
		width, height, depth, v_x, v_y);

	int render_model = 1;
	// Renderer
	Renderer renderer(width, height, screen_fb, zbuffer, mesh, render_model, black_color, image, transformer);

	// 填充背景颜色
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) 
			screen_fb[j * width + i] = back_color;
	}

	//// 点填充模型
	//PointModel(mesh, color, screen_fb, width, height);

	//// 线填充模型
	//LineModel(mesh, color, screen_fb, width, height);

	// *************************** 绘制模型 ****************************
	//vector<a2v> a2v_s_;

	//for (int i = 0; i < mesh->nums_faces(); i++) {
	//	a2v a_s[3];
	//	v2f v_s[3];
	//	for (int j = 0; j < 3; j++) {
	//		a_s[j] = renderer.processShader(i, j);
	//		a2v_s_.push_back(a_s[j]);
	//		v_s[j] = renderer.vertexShader(a_s[j]);
	//	}
	//}

	// *************************** 测试时间！***************************
	//// 计算帧数
	//double test_time_1 = 0;
	//double test_time_2 = 0;

	//// 开始渲染的时间
	//test_time_1 = get_cpu_time();


	//test_time_2 = get_cpu_time();
	//
	////cout << "time_1: " << test_time_1 << endl;
	////cout << "time_2: " << test_time_2 << endl;
	//cout << "本帧渲染所花时间为：" << test_time_2 - test_time_1 << endl;

	//// **************************************************************
	//// 测试：单个三角形
	//Vec3f screen_coords[3];
	//screen_coords[0] = Vec3f(-0.1, -0.3, 0.3);
	//screen_coords[1] = Vec3f(0.4, 0.2, 0.2);
	//screen_coords[2] = Vec3f(0.7, 0.3, 0.1);
	//for (int k = 0; k < 3; k++) {
	//	screen_coords[k].x = (screen_coords[k].x + 1) * 0.5 * width;
	//	screen_coords[k].y = (screen_coords[k].y + 1) * 0.5 * (height);
	//	screen_coords[k].z = (screen_coords[k].z + 1) * 0.5;
	//	//screen_coords[k].z = screen_coords[k].z + 1;
	//	//cout << "screen_coords[" << k << "].z: " << screen_coords[k].z << endl;
	//}
	//DrawTriangle_barycentric_zbuffer(screen_coords, color, screen_fb, zbuffer, width, height);


	// **************************************************************
	//HDC hDC = GetDC(hwnd);
	//// **7、将颜色数据由源DC转换到目标DC（也就是CreateCompatibleDC()创建的内存设备上下文）**
	//BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	//ReleaseDC(hwnd, hDC);


	//************************************************************************************************
	// 消息/渲染循环
	//************************************************************************************************

	// 计算帧数
	int num_frames = 0;				// 帧数
	float print_time = 0;			// 

	// 开始渲染的时间
	print_time = get_cpu_time();

	unsigned int count = 0;

	//7、消息循环
	while (1) {
		// 获取渲染完的时间（第一次不算）
		float curr_time = 0;
		curr_time = get_cpu_time();

		// 清空缓冲区
		clear_frameBuffer(screen_fb, width, height, back_color);
		clear_zbuffer(zbuffer, width, height);
		
		if (keys_buffer[0x41]) camera_pos.x -= 0.2;
		if (keys_buffer[0x44]) camera_pos.x += 0.2;
		if (keys_buffer[0x57]) camera_pos.y += 0.2;
		if (keys_buffer[0x53]) camera_pos.y -= 0.2;
		if (keys_buffer[0x46]) camera_pos.z = -camera_pos.z;

		if (zDelta > 0) camera_pos.z -= 0.2;
		if (zDelta < 0) camera_pos.z += 0.2;

		zDelta = 0;

		// 更新矩阵
		renderer.get_transformer().LookAt(camera_pos, center, up);
		//renderer.get_transformer().update_MVP();
		renderer.get_transformer().update_MVP_without_model();
		
		//PointModel(hwnd, renderer, white_color, screen_dc);
		Draw_2(hwnd, renderer);
		//Draw_2_vs_and_ps(hwnd, renderer);
		//Draw_2_Gouruad(hwnd, renderer, light_pos);
		//Draw_4_fastStorage(hwnd, renderer, &a2v_s_);
		//Draw_4_fastStorage(hwnd, renderer);

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			
			msg.lParam = (DWORD)&renderer;
			//（1）翻译消息
			TranslateMessage(&msg);
			//（2）分派消息
			DispatchMessage(&msg);
		}
		else {
			// 没响应也要画！
			//clear_frameBuffer(screen_fb, width, height);
			//Draw_2(hwnd, renderer);
			//Draw(hwnd);
		}

		// 计算帧数
		num_frames += 1;
		if (curr_time - print_time >= 1) {
			int sum_millis = (int)((curr_time - print_time) * 1000);
			float avg_millis = (float)sum_millis / num_frames;

			printf("fps: %3d , avg: %f ms\n", num_frames, avg_millis);

			num_frames = 0;
			print_time = curr_time;

		}
	}

	//************************************************************************************************
	// 释放内存
	//************************************************************************************************

	// 释放控制台
	FreeConsole();

	// 释放内存
	delete[](ptr);
	ptr = NULL;

	delete(mesh);
	mesh = NULL;

	delete[](zbuffer);
	zbuffer = NULL;

	delete image;
	image = NULL;


	//************************************************************************************************

	return msg.wParam;
}
 

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	//1、声明关键变量
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;

	//2、处理消息
	switch (message) {
	case WM_SIZE:
	{
		//light_pos.z += 1.f;
		//cout << "**** lightPos.z: " << light_pos.z << endl;
		Draw(hwnd);
		break;
	}

	case WM_LBUTTONDOWN:
	{
		light_pos.z += 1.f;
		//flag = true;

		break;
	}

	case WM_RBUTTONDOWN:
	{
		light_pos.z -= 1.f;
		//flag = true;

		break;
	}

	case WM_MOUSEWHEEL:
	{
		//flag = true;
		zDelta = wParam >> 16;	
		break;
	}

	case WM_KEYDOWN:
	{
		//flag = true;
		keys_buffer[wParam & 511] = 1;
		break;
	}
	case WM_KEYUP:
	{
		keys_buffer[wParam & 511] = 0;
		break;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

// 画
void Draw(HWND hwnd)
{
	HDC hDC = GetDC(hwnd);
	// **7、将颜色数据由源DC转换到目标DC（也就是CreateCompatibleDC()创建的内存设备上下文）**
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}

void clear_frameBuffer(unsigned int* frameBuffer, int width, int height, COLORREF color) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			frameBuffer[j * width + i] = color;
		}
	}
}
// 清空深度缓存
void clear_zbuffer(float* zbuffer, int width, int height) {
	int zero = 0;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			zbuffer[j * width + i] = zero;
		}
	}
}

void Draw_2(HWND hwnd, Renderer renderer) {
	for (int i = 0; i < renderer.get_nums_faces(); i++) {
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			v_s[j] = renderer.vertexShader(renderer.processShader(i, j));
		}

		// 测试新的Shader结构
		DrawTriangle_barycentric_Shader(v_s, light_pos, renderer);
	}

	HDC hDC = GetDC(hwnd);
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}
void Draw_2_Gouruad(HWND hwnd, Renderer renderer, Vec3f lightPos) {
	for (int i = 0; i < renderer.get_nums_faces(); i++) {
		a2v a_s[3];
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			a_s[j] = renderer.processShader(i, j);
			v_s[j] = renderer.vertexShader_Gouruad(a_s[j], lightPos);
		}

		// 测试新的Shader结构
		DrawTriangle_barycentric_Shader_Gouruad(v_s, renderer);
	}

	HDC hDC = GetDC(hwnd);
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}

// 将vs与ps结合
void Draw_2_vs_and_ps(HWND hwnd, Renderer renderer) {
	for (int i = 0; i < renderer.get_nums_faces(); i++) {
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			v_s[j] = renderer.vertexShader_and_proecc(i, j);
		}

		// 测试新的Shader结构
		DrawTriangle_barycentric_Shader(v_s, light_pos, renderer);
		//DrawTriangle_barycentric_zbuffer(v_s, renderer);
	}

	HDC hDC = GetDC(hwnd);
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}

// 多线程版本
void Draw_3_mutil(HWND hwnd, Renderer renderer) {
	for (int i = 0; i < renderer.get_nums_faces(); i++) {
		a2v a_s[3];
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			a_s[j] = renderer.processShader(i, j);
			v_s[j] = renderer.vertexShader(a_s[j]);
		}

		//thread t(DrawTriangle_barycentric_Shader, v_s, light_pos, renderer);
		//t.detach();
	}

	HDC hDC = GetDC(hwnd);
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}

// 尝试快速读取......?稍微快了一点点 1.656 -> 1.625
void Draw_4_fastStorage(HWND hwnd, Renderer renderer, vector<a2v>* a2v_s_) {
	for (int i = 0; i < renderer.get_nums_faces(); i++) {
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			v_s[j] = renderer.vertexShader((*a2v_s_)[i*3 + j]);
		}

		DrawTriangle_barycentric_Shader(v_s, light_pos, renderer);
	}

	HDC hDC = GetDC(hwnd);
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}

// 渲染单个三角形测试速度
void Draw_5_single(HWND hwnd, Renderer renderer) {
	// 画一个三角形
	a2v a_s_2[3];
	v2f v_s_2[3];
	a_s_2[0].vertex_native = Vec4f(-0.3, -0.3, -0.3, 1);
	a_s_2[1].vertex_native = Vec4f(-0.5, 0.1, -0.3, 1);
	a_s_2[2].vertex_native = Vec4f(0.8, 0.6, 0.6, 1);

	for (int i = 0; i < 3; i++) v_s_2[i] = renderer.vertexShader(a_s_2[i]);

	DrawTriangle_barycentric_Shader_single(v_s_2, light_pos, renderer, white_color);
}

// 尝试优化Shader和光栅化方面的东西
void Draw_6_test_Shader(HWND hwnd, Renderer renderer) {
	for (int i = 0; i < mesh->nums_faces(); i++) {
		a2v a_s[3];
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			a_s[j] = renderer.processShader(i, j);
			v_s[j] = renderer.vertexShader(a_s[j]);
		}

		// 测试新的Shader结构
		DrawTriangle_barycentric_Shader_test(v_s, light_pos, renderer);
		//DrawTriangle_barycentric_zbuffer(v_s, renderer);
	}
}

// 初始化窗口并注册
void init_Register_wndclass(WNDCLASS& wndclass, HINSTANCE hInstance, TCHAR szAppName[]) {
	//2、设置窗口类的属性
	wndclass.style = CS_HREDRAW | CS_VREDRAW;						// 窗口风格
	wndclass.lpfnWndProc = WndProc;									//回调
	wndclass.cbClsExtra = 0;										// 0		
	wndclass.cbWndExtra = 0;										// 0
	wndclass.hInstance = hInstance;									// 程序ID
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);				// 图标
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);					// 光标
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//背景色
	wndclass.lpszMenuName = NULL;									// 菜单
	wndclass.lpszClassName = szAppName;								//窗口类名

	//3、注册窗口类
	if (!RegisterClass(&wndclass)) {
		MessageBox(NULL, TEXT("This program can only run under Windows NT!"), szAppName, MB_ICONERROR);
		return;
	}
}

// 设置bitmapinfo
void set_bitmap(BITMAPINFO& bi, HDC& screen_dc, HBITMAP& screen_hb, HBITMAP& screen_ob,
	unsigned int*& screen_fb, LPVOID ptr) {
	// **1、设置bitmap**
	bi.bmiHeader.biSize = sizeof(BITMAPINFO);		// 检测结构大小
	bi.bmiHeader.biWidth = width;					// bitmap的宽，左->右
	bi.bmiHeader.biHeight = height;					// bitmap的高，下->上
	bi.bmiHeader.biPlanes = 1;						// 指定平台，必为1
	bi.bmiHeader.biBitCount = 32;					// 指定最大颜色数为2^32
	bi.bmiHeader.biCompression = BI_RGB;			// 压缩模式
	bi.bmiHeader.biSizeImage = width * height * 4;	// bitmap的大小
	bi.bmiHeader.biXPelsPerMeter = 0;				// 位图设备的水平分辨率
	bi.bmiHeader.biYPelsPerMeter = 0;				// 位图设备的垂直分辨率
	bi.bmiHeader.biClrUsed = 0;						// 实际用到的颜色表的颜色索引数
	bi.bmiHeader.biClrImportant = 0;				// 显示位图要用到的颜色索引数

	// bitmap
	// **2、创建内存设备环境句柄**
	HDC hdc = GetDC(hwnd);
	screen_dc = CreateCompatibleDC(hdc);
	ReleaseDC(hwnd, hdc);

	// **3、创建DIBSection**
	screen_hb = CreateDIBSection(screen_dc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);

	// **4、用ob保存hb的上一个状态**
	screen_ob = (HBITMAP)SelectObject(screen_dc, screen_hb);

	// **5、将void*类型的指针强制转化为unsigned int*指针后赋值给fb
	screen_fb = (unsigned int*)ptr;
}



