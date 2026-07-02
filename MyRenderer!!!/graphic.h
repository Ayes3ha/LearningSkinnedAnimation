#include <windows.h>
#include <stdio.h>


#include "geometry.h"
#include "tool.h"
#include "mesh.h"
#include "public.h"
#include "tgaimage.h"

#pragma once 

// ************************************************************************************
// 一些shader结构
// ************************************************************************************
typedef struct a2v {
	Vec4f vertex_native;
	Vec3f normal_native;
	Vec2f texcoord_native;
} a2v;

// vs --> fs 的结构
typedef struct v2f {
	Vec3f vertex;
	Vec3f normal;
	Vec2f texcoord;
	Color color_tex;
	//Vec3f ndc_coord;
	float intensity;
} v2f;

// ************************************************************************************
// 颜色结构COLORREF相关
// ************************************************************************************

// 装填颜色结构COLORREF
void load_COLORREF(COLORREF& color, BYTE R, BYTE G, BYTE B, BYTE A);
// 颜色*float
COLORREF multiply_COLORREF(COLORREF color, float alpha);

// ************************************************************************************
// 变换器——存储各种用于顶点变换的矩阵
// ************************************************************************************
class Transformer {
private:
	Matrix model_mat;
	Matrix lookAt_mat;
	Matrix proj_mat;
	Matrix viewPort_mat;
	Matrix MVP_mat;
	int width, height;	// 屏幕映射相关

public:
	int v_x, v_y;		// v_x和v_y是视口偏移量

public:
	Transformer(Matrix model, Matrix lookAt, Matrix proj, Matrix viewPort_mat);
	// 填充所有矩阵
	Transformer(int t_world_width, int t_world_height, int t_world_depth, float x, float y, float z,
		Vec3f camera_pos, Vec3f center, Vec3f up, 
		float FOV, float aspect, int n, int f, 
		int width, int height, int depth, int v_x, int v_y);
	// 不填充Model矩阵
	Transformer(Vec3f camera_pos, Vec3f center, Vec3f up,
		float FOV, float aspect, int n, int f,
		int width, int height, int depth, int v_x, int v_y);
	Matrix update_MVP();		// 组合MVP三矩阵
	Matrix update_MVP_without_model();			// 不更新model矩阵
	Matrix get_viewPort();	// 返回屏幕映射所需要的矩阵

	// 模型变换
	Matrix ModelTrans(int world_width, int world_height, int world_depth,
		float x, float y, float z);
	// 观察变换
	Matrix LookAt(Vec3f camera_pos, Vec3f center, Vec3f up);
	// 投影 -> 均为同轴对称的视域体
	// 正交 orthogonal
	Matrix orthoProjection(int w, int h, int n, int f);
	// 透视 Perspective
	//（1）直接确定视域体式
	Matrix perspProjection(int w, int h, int n, int f);
	//（2）FOV式
	Matrix perspProjection(float FOV, float aspect, int n, int f);
	// 屏幕映射
	Matrix ViewPort(int width, int height, int depth, int x, int y);

};

// ************************************************************************************
// 渲染器
// ************************************************************************************
class Renderer {
private:
	int width;
	int height;
	unsigned int* screen_fb;
	float* zbuffer;
	Mesh* mesh;
	int render_model;			// 渲染模式：1:普通模式（三角形），2:线框模式，3:点模式
	COLORREF background;		// 背景颜色
	Transformer transformer;	// 变换结构
	TGAImage* image;			// 纹理/TGAImage类
	
public:
	Renderer(int r_width, int r_height, unsigned int*& r_screen_fb, float* r_zbuffer, Mesh*& r_mesh,
		int r_render_model, COLORREF r_background, TGAImage* r_image, Transformer r_Transformer) :
		width(r_width), height(r_height), screen_fb(r_screen_fb), zbuffer(r_zbuffer), mesh(r_mesh),
		render_model(r_render_model), background(r_background), image(r_image), transformer(r_Transformer)
	{}
	//Renderer(int width, int height, unsigned int*& screen_fb, float* zbuffer, Mesh*& mesh,
	//	int render_model, COLORREF background) :
	//	width(), height(), screen_fb(), zbuffer(), mesh(),
	//	render_model(), background()
	//{}
	// 过程处理着色器：输入面索引iface和0-2的索引n找到顶点，并将其的相关数据存储在a2v结构中
	virtual a2v processShader(int iface, int n);	// 这个名字是我自创的！
	// 顶点着色器：输入a2v，处理后返回v2f
	virtual v2f vertexShader(a2v a);
	// 高德洛着色
	virtual v2f vertexShader_Gouruad(a2v a, Vec3f lightPos);
	// 将proecssShader和vertexShader结合在一起
	virtual v2f vertexShader_and_proecc(int iface, int n);
	// 片元着色器：输入该片元的重心坐标与源颜色，进行处理，bool返回值表示是否丢弃该片元
	virtual bool fragmentShader(v2f v, Vec3f bar, COLORREF& color, Vec3f lightPos);
	// 高德洛着色
	bool fragmentShader_Gouruad(COLORREF& c, float intensity);
	// 渲染zbuffer用
	virtual bool fragmentShader(Color& c, float z);

	Color diffuse(Vec2i tex_coord);		// 读取diffuse纹理
	int get_nums_faces();				// 获取该渲染器所属的模型面的数量
	unsigned int* get_screen_fb();			// 获取screen_fb
	float* get_zbuffer();				// 获取zbuffer
	Transformer& get_transformer();			// 获取变换设备
	int get_width();
	int get_height();

};


// ************************************************************************************
// 基础图元绘制
// ************************************************************************************
// 画线
void DrawLine(Vec2i v1, Vec2i v2, COLORREF color, unsigned int* screen_fb, const int width, const int height);
// 点模式
void PointModel(HWND hwnd, Renderer renderer, COLORREF color, HDC screen_dc);
// 线框模式
void LineModel(Mesh* mesh, COLORREF color, unsigned int* screen_fb, const int width, const int height);

// 画三角形
// 扫描线画法
void DrawTriangle_scan_my(Vec2i v1, Vec2i v2, Vec2i v3, COLORREF color, unsigned int* screen_fb, int width, int height);

// 重心坐标法
void DrawTriangle_barycentric(Vec3f* ptr, COLORREF color, unsigned int* screen_fb, float* zbuffer, const int width, const int height);
void DrawTriangle_b_test(Vec4f* ptr, COLORREF color, unsigned int* screen_fb, float* zbuffer, const int width, const int height);

// 测试加了shader的新结构
void DrawTriangle_barycentric_Shader(v2f* v_s, Vec3f lightPos, Renderer renderer);
// 逐顶点——高德洛着色
void DrawTriangle_barycentric_Shader_Gouruad(v2f* v_s, Renderer renderer);
// 逐像素
void DrawTriangle_barycentric_Shader_pixel(v2f* v_s, Vec3f lightPos, Renderer renderer);

void DrawTriangle_barycentric_Shader_origin(v2f* v_s, Vec3f lightPos, Renderer renderer);
// 新结构的三角形测试
void DrawTriangle_barycentric_Shader_single(v2f* v_s, Vec3f lightPos, Renderer renderer, COLORREF color);

//  重心坐标填充，输出深度图
void DrawTriangle_barycentric_zbuffer(v2f* v_s, Renderer renderer);

// 背面剔除
bool is_back_facing(Vec3f a, Vec3f b, Vec3f c);
// 视锥裁剪
int transform_check_cvv(Vec4f v);

// ************************************************************************************
// Shading
// ************************************************************************************
// 着色频率
// （待定）Blinn-Phong着色模型-点-Gouruad shading
// Blinn-Phong着色模型
float Blinn_Phong_shading(Vec3f vertPos, Vec3f normal, Vec3f lightPos);


void DrawTriangle_barycentric_Shader_test(v2f* v_s, Vec3f lightPos, Renderer renderer);


