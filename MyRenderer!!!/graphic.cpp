#include "graphic.h"

#include <iostream>
#include <limits>

#define screen_fbb(x, y, color) (screen_fb[y * width + x] = color)

// ************************************************************************************
// 变换器——存储各种用于顶点变换的矩阵
// ************************************************************************************
Transformer::Transformer(Matrix model, Matrix lookAt, Matrix proj, Matrix viewPort) {
	model_mat = model;
	lookAt_mat = lookAt;
	proj_mat = proj;
	viewPort_mat = viewPort;

	update_MVP();
}
// 根据输入信息计算全部变换矩阵
Transformer::Transformer(int t_world_width, int t_world_height, int t_world_depth, 
	float x, float y, float z,
	Vec3f camera_pos, Vec3f center, Vec3f up,
	float FOV, float aspect, int n, int f,
	int width, int height, int depth, int v_x, int v_y) {
	ModelTrans(t_world_width, t_world_height, t_world_depth, x, y, z);
	LookAt(camera_pos, center, up);
	perspProjection(FOV, aspect, n, f);
	ViewPort(width, height, depth, v_x, v_y);
}

// 不填充Model矩阵
Transformer::Transformer(Vec3f camera_pos, Vec3f center, Vec3f up,
	float FOV, float aspect, int n, int f,
	int width, int height, int depth, int v_x, int v_y) {
	LookAt(camera_pos, center, up);
	perspProjection(FOV, aspect, n, f);
	//ViewPort(width, height, depth, v_x, v_y);

	this->width = width;
	this->height = height;
	this->v_x = v_x;
	this->v_y = v_y;
}

// 组合/更新MVP三矩阵
Matrix Transformer::update_MVP() {
	Matrix mvp_mat(4, 4);
	mvp_mat = lookAt_mat * model_mat;
	mvp_mat = proj_mat * mvp_mat;

	MVP_mat = mvp_mat;

	return mvp_mat;
	//return MVP_mat;
}
// 不更新model矩阵
Matrix Transformer::update_MVP_without_model() {
	MVP_mat = proj_mat * lookAt_mat;
	return MVP_mat;
}

// 返回屏幕映射所需要的矩阵
Matrix Transformer::get_viewPort() {
	return viewPort_mat;
}

// 模型变换
Matrix Transformer::ModelTrans(int world_width, int world_height, int world_depth,
	float x, float y, float z) {
	Matrix model;

	Matrix m_scale(4, 4);
	m_scale.identity(4);
	m_scale[0][0] = world_width;
	m_scale[1][1] = world_height;
	m_scale[2][2] = world_depth;

	Matrix m_translate(4, 4);
	m_translate.identity(4);
	m_translate[0][3] = x;
	m_translate[1][3] = y;
	m_translate[2][3] = z;

	Matrix m_scale_invert(4, 4);
	m_scale_invert.identity(4);
	m_scale_invert[0][0] = (float)1 / world_width;
	m_scale_invert[1][1] = (float)1 / world_height;
	m_scale_invert[2][2] = (float)1 / world_depth;

	Matrix temp_mat = m_translate * m_scale;
	//temp_mat.printfMat();
	model = m_scale_invert * temp_mat;

	model_mat = model;

	return model;
}

// 观察变换
Matrix Transformer::LookAt(Vec3f camera_pos, Vec3f center, Vec3f up) {
	// 确定摄像机的三条轴
	//Vec3f z = (center - camera_pos).normalized();	// 因为只需要方向，所以要做归一化
	Vec3f z = (camera_pos - center).normalized();	// 因为只需要方向，所以要做归一化
	//z.normalize();	
	Vec3f x = (up ^ z).normalized();
	//x.normalize();
	Vec3f y = (z ^ x).normalized();

	//（1）先将camera_pos移到原点(0, 0, 0)
	Matrix Trans(4, 4);
	Trans.identity(4);
	for (int i = 0; i < 3; i++) {
		Trans[i][3] = -camera_pos[i];
	}

	//（2）将相机的各轴旋转到世界坐标轴上
	Matrix Minv(4, 4);
	Minv.identity(4);
	for (int i = 0; i < 3; i++) {
		Minv[0][i] = x[i];
		Minv[1][i] = y[i];
		Minv[2][i] = z[i];
	}

	Matrix temp = Minv * Trans;
	lookAt_mat = temp;

	return temp;
}

// 投影
// 正交投影 orthogonal -> 这是一个同轴对称的视域体
Matrix Transformer::orthoProjection(int w, int h, int n, int f) {
	Matrix projection(4, 4);
	projection.identity(4);

	std::cout << "w: " << w << " h: " << h << " n: " << n << " f: " << f << std::endl;

	projection[0][0] = (float)2 / w;
	projection[1][1] = (float)2 / h;
	projection[2][2] = (float)1 / (f - n);
	projection[2][3] = (float)-n / (f - n);

	projection[3][3] = 1;

	//projection.printfMat();
	proj_mat = projection;
	return projection;
}

// 透视投影 perspective
//（1）直接确定视域体式
Matrix Transformer::perspProjection(int w, int h, int n, int f) {
	Matrix projection(4, 4);
	//projection.identity(4);

	projection[0][0] = (float)2 * n / w;
	projection[1][1] = (float)2 * n / h;
	projection[2][2] = (float)f / (f - n);
	projection[2][3] = (float)-f * n / (f - n);

	projection[3][2] = 1.f;

	proj_mat = projection;
	return projection;
}

//（2）FOV式
Matrix Transformer::perspProjection(float FOV, float aspect, int n, int f) {
	Matrix projection(4, 4);

	float tan_FOV_half = std::tan((FOV / 2) * PI / 180.f);
	float cot_FOV_half = 1 / std::tan((FOV / 2) * PI / 180.f);

	projection[0][0] = (float)cot_FOV_half / aspect;
	projection[1][1] = (float)cot_FOV_half;
	projection[2][2] = (float)f / (f - n);
	projection[2][3] = (float)-f * n / (f - n);

	projection[3][2] = 1.f;	// 令向量的w等于其原本的z，透视除法的时候除以z就等于除掉了z

	proj_mat = projection;
	return projection;
}

// 视口变换
Matrix Transformer::ViewPort(int width, int height, int depth, int x, int y) {
	Matrix viewport;
	viewport.identity(4);

	////（1）viewport: 先+1 => 将[-1, 1]映射到[0, 1]
	//Matrix vp_addonce(4, 4);
	//vp_addonce.identity(4);
	//vp_addonce[0][3] = 1.f;
	//vp_addonce[1][3] = 1.f;
	//vp_addonce[2][3] = 1.f;

	////（2）viewport：后缩放
	//Matrix vp_scale(4, 4);
	//vp_scale[0][0] = (float)width / 2;
	//vp_scale[1][1] = (float)height / 2;
	//vp_scale[2][2] = (float)depth / 2;
	//vp_scale[3][3] = 1.f;

	//viewport = vp_scale * vp_addonce;

	viewport[0][3] = x + (float)width / 2;
	viewport[1][3] = y + (float)height / 2;
	viewport[2][3] = (float)depth / 2;

	viewport[0][0] = (float)width / 2;
	viewport[1][1] = (float)height / 2;
	viewport[2][2] = (float)depth / 2;

	viewPort_mat = viewport;
	return viewport;
}

// ************************************************************************************
// 渲染器
// ************************************************************************************
a2v Renderer::processShader(int iface, int n) {
	a2v a;

	// 获取该顶点位置
	a.vertex_native = mesh->get_vert_(iface, n).embed_4();
	a.normal_native = mesh->get_norm_(iface, n);
	a.texcoord_native = mesh->get_uv_(iface, n);

	return a;
}

//// 执行所有矩阵变换

//v2f Renderer::vertexShader(a2v a) {
//	v2f v;
//
//	//变换模块
//	Vec4f clip_coord;
//	Vec3f ndc_coord;
//	Vec3f screen_coord;
//
//	Vec4f temp;
//	// MVP
//	clip_coord = transformer.update_MVP() * a.vertex_native;
//	// 透视除法
//	ndc_coord = Vec3f(
//		clip_coord.x / clip_coord.w,
//		clip_coord.y / clip_coord.w,
//		clip_coord.z / clip_coord.w
//	);
//
//	// 屏幕映射
//	temp = clip_coord / clip_coord.w;
//	screen_coord = (transformer.get_viewPort() * temp).proj_3();
//
//	//Vec4f temp;
//	//// MVP
//	//clip_coord = transformer.update_MVP_without_model() * a.vertex_native;
//
//	//// 屏幕映射
//	//temp = clip_coord / clip_coord.w;
//	//screen_coord = (transformer.get_viewPort() * temp).proj_3();
//
//	Vec3f normal_coord = a.normal_native;
//	normal_coord.normalize();
//
//	v.vertex = screen_coord;
//	v.normal = normal_coord;
//	v.texcoord = a.texcoord_native;
//	//v.ndc_coord = ndc_coord;
//	//cout << "x: " << v.vertex.x << " y: " << v.vertex.y << " z: " << v.vertex.z
//	//	<< " w: " << v.vertex.w << endl;
//	return v;
//}

// 不使用model，且不用矩阵做屏幕映射 --> 平均多了12帧！！！！！
v2f Renderer::vertexShader(a2v a) {
	v2f v;
	//cout << "a: x: " << a.vertex_native.x << " y: " << a.vertex_native.y << " z: " << a.vertex_native.z
	//	<< " w: " << a.vertex_native.w << endl;

	//变换模块
	Vec4f clip_coord;
	Vec3f screen_coord;

	Vec4f ndc_coord;	// ndc应该是以三维向量表示的
	// MVP
	clip_coord = transformer.update_MVP_without_model() * a.vertex_native;
	//clip_coord = transformer.update_MVP() * a.vertex_native;
	
	//cout << "x: " << clip_coord.x << " y: " << clip_coord.y << " z: " << v.vertex.z
	//	<< " w: " << clip_coord.w << endl;
	// 透视除法
	ndc_coord = clip_coord / clip_coord.w;
	// 屏幕映射
	//（1）矩阵方式
	//screen_coord = (transformer.get_viewPort() * temp).proj_3();
	// 直接数乘 -> 多了4帧！！！
	//cout << transformer.v_y << endl;
	screen_coord.x = (ndc_coord.x + 1.f) * 0.5 * width + transformer.v_x;
	screen_coord.y = (ndc_coord.y + 1.f) * 0.5 * height + transformer.v_y;
	screen_coord.z = (ndc_coord.z + 1.f) * 0.5 * depth;


	Vec3f normal_coord = a.normal_native;
	normal_coord.normalize();

	v.vertex = screen_coord;
	v.normal = normal_coord;
	v.texcoord = a.texcoord_native;
	//v.ndc_coord = Vec3f(
	//	clip_coord.x / clip_coord.w,
	//	clip_coord.y / clip_coord.w,
	//	clip_coord.z / clip_coord.w
	//);
	//cout << "x: " << v.vertex.x << " y: " << v.vertex.y << " z: " << v.vertex.z
	//	<< " w: " << v.vertex.w << endl;
	return v;
}

// 高德洛着色
v2f Renderer::vertexShader_Gouruad(a2v a, Vec3f lightPos) {
	v2f v;

	//变换模块
	Vec4f clip_coord;
	Vec3f screen_coord;

	Vec4f ndc_coord;	// ndc应该是以三维向量表示的
	// MVP
	clip_coord = transformer.update_MVP_without_model() * a.vertex_native;
	// 透视除法
	ndc_coord = clip_coord / clip_coord.w;
	// 屏幕映射
	screen_coord.x = (ndc_coord.x + 1.f) * 0.5 * width + transformer.v_x;
	screen_coord.y = (ndc_coord.y + 1.f) * 0.5 * height + transformer.v_y;
	screen_coord.z = (ndc_coord.z + 1.f) * 0.5 * depth;

	Vec3f normal_coord = a.normal_native;
	normal_coord.normalize();

	// 光照模型
	float diffuse = max(0, normal_coord * lightPos);
	v.intensity = diffuse;

	v.vertex = screen_coord;
	v.normal = normal_coord;
	v.texcoord = a.texcoord_native;
	//v.ndc_coord = ndc_coord;
	//cout << "x: " << v.vertex.x << " y: " << v.vertex.y << " z: " << v.vertex.z
	//	<< " w: " << v.vertex.w << endl;
	return v;
}

// 将proecssShader和vertexShader结合在一起
v2f Renderer::vertexShader_and_proecc(int iface, int n) {
	v2f v;

	//变换模块
	Vec4f clip_coord = mesh->get_vert_(iface, n).embed_4();
	Vec3f screen_coord;

	Vec4f ndc_coord;	// ndc应该是以三维向量表示的
	// MVP
	clip_coord = transformer.update_MVP_without_model() * clip_coord;

	//cout << "x: " << clip_coord.x << " y: " << clip_coord.y << " z: " << v.vertex.z
	//	<< " w: " << clip_coord.w << endl;
	// 透视除法
	ndc_coord = clip_coord / clip_coord.w;


	screen_coord.x = (ndc_coord.x + 1.f) * 0.5 * width + transformer.v_x;
	screen_coord.y = (ndc_coord.y + 1.f) * 0.5 * height + transformer.v_y;
	screen_coord.z = (ndc_coord.z + 1.f) * 0.5 * depth;

	v.vertex = screen_coord;
	v.normal = mesh->get_norm_(iface, n);
	v.normal.normalize();
	v.texcoord = mesh->get_uv_(iface, n);

	return v;
}


bool Renderer::fragmentShader(v2f v, Vec3f bar, COLORREF& color, Vec3f lightPos) {
	// 插值在fragmentShader外面完成，传进来的v2f中的数据都是插值过的

	// 获取光照因子
	float factor = Blinn_Phong_shading(v.vertex, v.normal, lightPos);

	color = multiply_COLORREF(color, factor);

	return false;
}

bool Renderer::fragmentShader(Color& c, float z) {
	c.color = multiply_COLORREF(c.color, z);

	return false;
}

bool Renderer::fragmentShader_Gouruad(COLORREF& c, float intensity) {
	c = multiply_COLORREF(c, intensity);

	return false;
}

int Renderer::get_width() {
	return width;
}
int Renderer::get_height() {
	return height;
}

// 读取diffuse纹理
Color Renderer::diffuse(Vec2i tex_coord) {
	return mesh->diffuse(tex_coord);
}
// 获取该渲染器所属的模型面的数量
int Renderer::get_nums_faces() {
	return mesh->nums_faces();
}

// 获取screen_fb
unsigned int* Renderer::get_screen_fb() {
	return screen_fb;
}

// 获取zbuffer
float* Renderer::get_zbuffer() {
	return zbuffer;
}
// 获取变换设备
Transformer& Renderer::get_transformer() {
	return transformer;
}

// ************************************************************************************
// 颜色结构COLORREF相关
// ************************************************************************************
// 装填颜色结构COLORREF
void load_COLORREF(COLORREF& color, BYTE R, BYTE G, BYTE B, BYTE A) {
	color = (B) | (G << 8) | (R << 16) | (A << 24);
}

// 颜色*float
COLORREF multiply_COLORREF(COLORREF color, float alpha) {
	//cout << "1、color: "  <<hex << color << endl;
	//BYTE A = (color & 0xff000000) >> 24;
	//BYTE R = (color & 0x00ff0000) >> 16;
	//BYTE G = (color & 0x0000ff00) >> 8;
	//BYTE B = (color & 0x000000ff) >> 0;
	int A = (color & 0xff000000) >> 24;
	int R = (color & 0x00ff0000) >> 16;
	int G = (color & 0x0000ff00) >> 8;
	int B = (color & 0x000000ff) >> 0;

	//cout << R << " " << G << " " << B << " " << A << " "<< endl;
	//cout << *dec << endl;

	//A = A * alpha;
	//R = R * alpha;
	//G = G * alpha;
	//B = B * alpha;
	A = (int)(A * alpha);
	R = (int)(R * alpha);
	G = (int)(G * alpha);
	B = (int)(B * alpha);
	//A *= alpha;
	//R *= alpha;
	//G *= alpha;
	//B *= alpha;

	COLORREF color_result = (B) | (G << 8) | (R << 16) | (A << 24);
	//printf("R: %x\n", R);
	//printf("G: %x\n", G);
	//printf("B: %x\n", B);
	//printf("A: %x\n", A);
	return color_result;
}

// ************************************************************************************
// 基础图元绘制
// ************************************************************************************
// 画线
void DrawLine(Vec2i v0, Vec2i v1, COLORREF color, unsigned int* screen_fb, int width, int height) {
	// **
	bool steep = false;
	if (std::abs(v0.x - v1.x) < std::abs(v0.y - v1.y)) {
		std::swap(v0.x, v0.y);
		std::swap(v1.x, v1.y);
		steep = true;
	}
	// 交换数据，使得v1的x和y都小于v2
	if (v0.x >= v1.x) {
		std::swap(v0.x, v1.x);
		std::swap(v0.y, v1.y);
	}

	// 画线
	for (int x = v0.x; x <= v1.x; x++) {
		float a = (x - v0.x) / (float)(v1.x - v0.x);
		int y = a * (v1.y - v0.y) + v0.y;
		//printf("第%d次-a：%d,\t b：%d\n", i, a, b);
		//printf("x: %d, y：%d\n", i, b);
		
		if (steep)
			screen_fbb(y, x, color);
		else
			screen_fbb(x, y, color);
	}
}

// 点模式
void PointModel(HWND hwnd, Renderer renderer, COLORREF color, HDC screen_dc) {
	unsigned int* screen_fb_1 = renderer.get_screen_fb();
	for (int i = 0; i < renderer.get_nums_faces(); i++) {
		a2v a_s[3];
		v2f v_s[3];
		for (int j = 0; j < 3; j++) {
			a_s[j] = renderer.processShader(i, j);
			v_s[j] = renderer.vertexShader(a_s[j]);
		}

		for (int j = 0; j < 3; j++) {
			int index = (int)(v_s[j].vertex.y * width + v_s[j].vertex.x);
			//cout << "x: " << v_s[j].vertex.x << " y: " << v_s[j].vertex.y << " z: " << v_s[j].vertex.z << endl;
			
			screen_fb_1[index] = color;
		}
	}

	HDC hDC = GetDC(hwnd);
	BitBlt(hDC, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hDC);
}

// 线框模式
void LineModel(Mesh* mesh, COLORREF color, unsigned int* screen_fb, const int width, const int height) {
	for (int i = 0; i < mesh->nums_faces(); i++) {
		Vec3f v[3];
		for (int j = 0; j < 3; j++) {
			v[j] = mesh->get_vert_(i, j);
		}

		Vec2i vv[3];
		for (int k = 0; k < 3; k++) {
			vv[k].x = (v[k].x + 1) * 0.5 * width;
			vv[k].y = (v[k].y + 1) * 0.5 * (height - 1);
		}

		DrawLine(vv[0], vv[1], color, screen_fb, width, height);
		DrawLine(vv[1], vv[2], color, screen_fb, width, height);
		DrawLine(vv[2], vv[0], color, screen_fb, width, height);
	}
}

// 画三角形
// 扫描线
void DrawTriangle_scan_my(Vec2i v0, Vec2i v1, Vec2i v2, COLORREF color, unsigned int* screen_fb, const int width, const int height) {
	//（1）先给三个点排序
	if (v0.y > v1.y) std::swap(v0, v1);
	if (v0.y > v2.y) std::swap(v0, v2);
	if (v1.y > v2.y) std::swap(v1, v2);

	//printf("v0.x: %d,  v0.y: %d\n", v0.x, v0.y);
	//printf("v1.x: %d,  v1.y: %d\n", v1.x, v1.y);
	//printf("v2.x: %d,  v2.y: %d\n", v2.x, v2.y);

	int total_height = v2.y - v0.y;

	// 下部分
	for (int y = v0.y; y < v1.y; y++) {
		int segment_height = v1.y - v0.y + 1;
		float alpha = (float)(y - v0.y) / total_height;
		float beta = (float)(y - v0.y) / segment_height;
		Vec2i x0 = v0 + (v2 - v0) * alpha;
		Vec2i x1 = v0 + (v1 - v0) * beta;
		if (x0.x > x1.x) std::swap(x0, x1);
		for (int x = x0.x; x <= x1.x; x++)
			screen_fb[y * width + x] = color;
	}

	// 上部分
	for (int y = v1.y; y < v2.y; y++) {
		int segment_height = v2.y - v1.y + 1;
		float alpha = (float)(y - v0.y) / total_height;
		float beta = (float)(y - v1.y) / segment_height;
		Vec2i x0 = v0 + (v2 - v0) * alpha;
		Vec2i x1 = v1 + (v2 - v1) * beta;
		if (x0.x > x1.x) std::swap(x0, x1);
		for (int x = x0.x; x <= x1.x; x++)
			screen_fb[y * width + x] = color;
	}

}

// 重心坐标填充，输出正常光照图像
void DrawTriangle_barycentric(Vec3f* ptr, COLORREF color, unsigned int* screen_fb, float* zbuffer, const int width, const int height) {
	// 确定包围盒
	Vec2f bboxmin( (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax( (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, ptr[i][0]);
		bboxmin.y = min(bboxmin.y, ptr[i][1]);

		bboxmax.x = max(bboxmax.x, ptr[i][0]);
		bboxmax.y = max(bboxmax.y, ptr[i][1]);
	}

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 求得该点的重心坐标
			Vec3f c = barycentric(
				Vec2f(ptr[0].x, ptr[0].y),
				Vec2f(ptr[1].x, ptr[1].y),
				Vec2f(ptr[2].x, ptr[2].y),
				Vec2f(P.x, P.y));


			// 深度插值
			float z = ptr[0][2] * c.x + ptr[1][2] * c.y + ptr[2][2] * c.z;

			if (c.x <= 0 || c.y <= 0 || c.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer[P.x * width + P.y] > z) continue;		// 判断zbuffer

			zbuffer[P.x * width + P.y] = z;

			COLORREF color_result = multiply_COLORREF(color, 1);

			screen_fbb(P.x, P.y, color_result);
		}
	}
}

// 画单个三角形----测试用的
void DrawTriangle_b_test(Vec4f* ptr, COLORREF color, unsigned int* screen_fb, float* zbuffer, const int width, const int height) {
	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, ptr[i][0]);
		bboxmin.y = min(bboxmin.y, ptr[i][1]);

		bboxmax.x = max(bboxmax.x, ptr[i][0]);
		bboxmax.y = max(bboxmax.y, ptr[i][1]);
	}

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 求得该点的重心坐标
			Vec3f c = barycentric(
				Vec2f(ptr[0].x, ptr[0].y),
				Vec2f(ptr[1].x, ptr[1].y),
				Vec2f(ptr[2].x, ptr[2].y),
				Vec2f(P.x, P.y));


			// 简单裁剪
			if (P.x > width || P.y > height || P.x < 0 || P.y < 0) continue;


			// 深度插值
			float z = ptr[0][2] * c.x + ptr[1][2] * c.y + ptr[2][2] * c.z;

			if (c.x <= 0 || c.y <= 0 || c.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer[P.x * width + P.y] > z) continue;		// 判断zbuffer

			zbuffer[P.x * width + P.y] = z;

			COLORREF color_result = multiply_COLORREF(color, 1);

			screen_fbb(P.x, P.y, color_result);
		}
	}
}

// 背面剔除
bool is_back_facing(Vec3f a, Vec3f b, Vec3f c) {
	float signed_area = a.x * b.y - a.y * b.x +
		b.x * c.y - b.y * c.x +
		c.x * a.y - c.y * a.x;   //|AB AC|
	return signed_area <= 0;
}

bool my_BackFace_Culling(Vec3f a, Vec3f b, Vec3f c) {
	Vec3f P = (b - a) ^ (c - b);
	
	if (P.z >= 0) return true;
	else if (P.z < 0) return false;

	return true;
}

void DrawTriangle_barycentric_Shader(v2f* v_s, Vec3f lightPos, Renderer renderer) {
	// 背面剔除
	if (!my_BackFace_Culling(v_s[0].vertex, v_s[1].vertex, v_s[2].vertex)) return;

	//// 简单裁剪
	//if (v_s[0].vertex.x >= width || v_s[1].vertex.x >= width || v_s[2].vertex.x >= width ||
	//	v_s[0].vertex.y >= height || v_s[1].vertex.y >= height || v_s[2].vertex.y >= height) 
	//	return;

	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		if (P.x >= width || P.x < 0) continue;
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 2D裁剪 —— 视平面裁剪
			//if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;
			if (P.y >= height || P.y < 0) continue;

			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;
			
			int index = (int)(P.y * width + P.x);
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer
			//****************************************************************
			// 片元插值
			//****************************************************************
			// 纹理插值
			Vec2i tex(0, 0);
			for (int i = 0; i < 3; i++) {
				tex.x += v_s[i].texcoord.x * bar[i];
				tex.y += v_s[i].texcoord.y * bar[i];
			}
			// 将插值结果放到一个新的v2f中，并输入到fragmentShader。
			v2f v;
			v.vertex = Vec3f(P.x, P.y, z);
			v.normal = Vec3f(
				v_s[0].normal[0] * bar.x + v_s[1].normal[0] * bar.y + v_s[2].normal[0] * bar.z,
				v_s[0].normal[1] * bar.x + v_s[1].normal[1] * bar.y + v_s[2].normal[1] * bar.z,
				v_s[0].normal[2] * bar.x + v_s[1].normal[2] * bar.y + v_s[2].normal[2] * bar.z
			);
			v.normal.normalize();
			//v.texcoord = Vec2f(1, 1);
			v.color_tex = renderer.diffuse(tex);


			COLORREF color_result = v.color_tex.color;

			bool discard = renderer.fragmentShader(v, bar, color_result, lightPos);
			if (!discard) {
				zbuffer_1[index] = z;
				//cout << "P.x: " << P.x << " P.y: " << P.y << endl;
				screen_fb_1[index] = color_result;
			}
		}
	}
}

void DrawTriangle_barycentric_Shader_Gouruad(v2f* v_s, Renderer renderer) {
	if (is_back_facing(v_s[0].vertex, v_s[1].vertex, v_s[2].vertex)) {
		return;
	}

	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 简单裁剪
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;

			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;
			// 纹理插值
			Vec2i tex(0, 0);
			for (int i = 0; i < 3; i++) {
				tex.x += v_s[i].texcoord.x * bar[i];
				tex.y += v_s[i].texcoord.y * bar[i];
			}

			int index = (int)(P.y * width + P.x);
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer

			//****************************************************************
			// 片元插值
			//****************************************************************
			// 将插值结果放到一个新的v2f中，并输入到fragmentShader。
			v2f v;
			//v.vertex = Vec3f(P.x, P.y, z);
			// 光强插值
			float intensity = v_s[0].intensity * bar.x + v_s[1].intensity * bar.y + v_s[2].intensity * bar.z;
			//v.texcoord = Vec2f(1, 1);
			v.color_tex = renderer.diffuse(tex);

			COLORREF color_result = v.color_tex.color;

			bool discard = renderer.fragmentShader_Gouruad(color_result, intensity);
			//bool discard = false;
			if (!discard) {
				zbuffer_1[index] = z;
				//cout << "P.x: " << P.x << " P.y: " << P.y << endl;
				screen_fb_1[index] = color_result;
			}
		}
	}
}

void DrawTriangle_barycentric_Shader_pixel(v2f* v_s, Vec3f lightPos, Renderer renderer) {
	if (is_back_facing(v_s[0].vertex, v_s[1].vertex, v_s[2].vertex)) {
		return;
	}

	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 简单裁剪
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;

			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;
			// 纹理插值
			Vec2i tex(0, 0);
			for (int i = 0; i < 3; i++) {
				tex.x += v_s[i].texcoord.x * bar[i];
				tex.y += v_s[i].texcoord.y * bar[i];
			}

			int index = (int)(P.y * width + P.x);
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer

			//****************************************************************
			// 片元插值
			//****************************************************************
			// 将插值结果放到一个新的v2f中，并输入到fragmentShader。
			v2f v;
			v.vertex = Vec3f(P.x, P.y, z);
			v.normal = Vec3f(
				v_s[0].normal[0] * bar.x + v_s[1].normal[0] * bar.y + v_s[2].normal[0] * bar.z,
				v_s[0].normal[1] * bar.x + v_s[1].normal[1] * bar.y + v_s[2].normal[1] * bar.z,
				v_s[0].normal[2] * bar.x + v_s[1].normal[2] * bar.y + v_s[2].normal[2] * bar.z
			);
			//v.texcoord = Vec2f(1, 1);
			v.color_tex = renderer.diffuse(tex);


			COLORREF color_result = v.color_tex.color;

			bool discard = renderer.fragmentShader(v, bar, color_result, lightPos);
			if (!discard) {
				zbuffer_1[index] = z;
				//cout << "P.x: " << P.x << " P.y: " << P.y << endl;
				screen_fb_1[index] = color_result;
			}
		}
	}
}

void DrawTriangle_barycentric_Shader_origin(v2f* v_s, Vec3f lightPos, Renderer renderer) {

	if (is_back_facing(v_s[0].vertex, v_s[1].vertex, v_s[2].vertex)) {
		return;
	}


	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 简单裁剪
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;

			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;
			// 纹理插值
			Vec2i tex(0, 0);
			for (int i = 0; i < 3; i++) {
				tex.x += v_s[i].texcoord.x * bar[i];
				tex.y += v_s[i].texcoord.y * bar[i];
			}

			int index = (int)(P.y * width + P.x);
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer

			//****************************************************************
			// 片元插值
			//****************************************************************
			// 将插值结果放到一个新的v2f中，并输入到fragmentShader。
			v2f v;
			v.vertex = Vec3f(P.x, P.y, z);
			v.normal = Vec3f(
				v_s[0].normal[0] * bar.x + v_s[1].normal[0] * bar.y + v_s[2].normal[0] * bar.z,
				v_s[0].normal[1] * bar.x + v_s[1].normal[1] * bar.y + v_s[2].normal[1] * bar.z,
				v_s[0].normal[2] * bar.x + v_s[1].normal[2] * bar.y + v_s[2].normal[2] * bar.z
			);
			v.texcoord = Vec2f(1, 1);
			v.color_tex = renderer.diffuse(tex);


			COLORREF color_result = v.color_tex.color;

			bool discard = renderer.fragmentShader(v, bar, color_result, lightPos);
			if (!discard) {
				zbuffer_1[index] = z;
				//cout << "P.x: " << P.x << " P.y: " << P.y << endl;
				screen_fb_1[P.y * width + P.x] = color_result;
			}
		}
	}
}



void DrawTriangle_barycentric_Shader_test(v2f* v_s, Vec3f lightPos, Renderer renderer) {
	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	COLORREF white_color_ = 255 | (255) << 8 | (255) << 16 | (255) << 24;
	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 简单裁剪
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;

			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;
			//// 纹理插值
			//Vec2i tex(0, 0);
			//for (int i = 0; i < 3; i++) {
			//	tex.x += v_s[i].texcoord.x * bar[i];
			//	tex.y += v_s[i].texcoord.y * bar[i];
			//}

			int index = (int)(P.y * width + P.x);
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer

			//****************************************************************
			// 片元插值
			//****************************************************************
			// 将插值结果放到一个新的v2f中，并输入到fragmentShader。
			//v2f v;
			//v.vertex = Vec4f(P.x, P.y, z, 1.f);
			////v.normal = Vec3f(
			////	v_s[0].normal[0] * bar.x + v_s[1].normal[0] * bar.y + v_s[2].normal[0] * bar.z,
			////	v_s[0].normal[1] * bar.x + v_s[1].normal[1] * bar.y + v_s[2].normal[1] * bar.z,
			////	v_s[0].normal[2] * bar.x + v_s[1].normal[2] * bar.y + v_s[2].normal[2] * bar.z
			////);
			//v.normal = Vec3f(1, 1, 1);
			//v.texcoord = Vec2f(1, 1);
			//v.color_tex = renderer.diffuse(tex);


			//COLORREF color_result = v.color_tex.color;

			//bool discard = renderer.fragmentShader(v, bar, color_result, lightPos);
			//if (!discard) {
			//	zbuffer_1[index] = z;
			//	//screen_fbb(P.x, P.y, color_result);
			//	//cout << "P.x: " << P.x << " P.y: " << P.y << endl;
			//	screen_fb_1[P.y * width + P.x] = color_result;
			//}
			zbuffer_1[index] = z;
			screen_fb_1[P.y * width + P.x] = white_color_;
		}
	}
}

// 新结构的三角形测试
void DrawTriangle_barycentric_Shader_single(v2f* v_s, Vec3f lightPos, Renderer renderer, COLORREF color) {
	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 简单裁剪
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;

			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;

			int index = (int)(P.y * width + P.x);
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer

			zbuffer_1[index] = z;
			//screen_fbb(P.x, P.y, color_result);
			//cout << "P.x: " << P.x << " P.y: " << P.y << endl;
			screen_fb_1[P.y * width + P.x] = color;
		}
	}
}

// 重心坐标填充，输出深度图
void DrawTriangle_barycentric_zbuffer(v2f* v_s, Renderer renderer) {
	// 确定包围盒
	Vec2f bboxmin((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());
	Vec2f bboxmax((std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = min(bboxmin.x, v_s[i].vertex[0]);
		bboxmin.y = min(bboxmin.y, v_s[i].vertex[1]);

		bboxmax.x = max(bboxmax.x, v_s[i].vertex[0]);
		bboxmax.y = max(bboxmax.y, v_s[i].vertex[1]);
	}

	float* zbuffer_1 = renderer.get_zbuffer();
	unsigned int* screen_fb_1 = renderer.get_screen_fb();

	// 绘制
	Vec2i P(0, 0);
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			// 求得该点的重心坐标
			Vec3f bar = barycentric(
				Vec2f(v_s[0].vertex.x, v_s[0].vertex.y),
				Vec2f(v_s[1].vertex.x, v_s[1].vertex.y),
				Vec2f(v_s[2].vertex.x, v_s[2].vertex.y),
				Vec2f(P.x, P.y));

			// 深度插值
			float z = v_s[0].vertex[2] * bar.x + v_s[1].vertex[2] * bar.y + v_s[2].vertex[2] * bar.z;
			
			int index = (int)(P.y * width + P.x);
			if (bar.x <= 0 || bar.y <= 0 || bar.z <= 0) continue;		// 通过重心坐标判断该点是否在三角形内
			if (zbuffer_1[index] > z) continue;		// 判断zbuffer

			Color color_result;
			color_result.color= 0xffffffff;
			
			bool discard = renderer.fragmentShader(color_result, z);
			if (!discard) {
				zbuffer_1[index] = z;
				//screen_fbb(P.x, P.y, color_result);
				screen_fb_1[P.y * width + P.x] = color_result.color;
			}
		}
	}
}

// ************************************************************************************
// Shading
// ************************************************************************************
// 着色频率
// （待定）Blinn-Phong着色模型-点-Gouruad shading
// Blinn-Phong着色模型 -> 这里实现的是unity里的平行光
float Blinn_Phong_shading(Vec3f vertPos, Vec3f normal, Vec3f lightPos) {
	lightPos.normalize();

	float diffuse = max(0, normal * lightPos);

	float factor = diffuse;

	return factor;
}

// 视锥裁剪
int transform_check_cvv(Vec4f v) {
	float w = v.w;
	int check = 0;
	if (v.z < 0.0f) check |= 1;
	if (v.z > w) check |= 2;
	if (v.x < -w) check |= 4;
	if (v.x > w) check |= 8;
	if (v.y < -w) check |= 16;
	if (v.y > w) check |= 32;
	return check;
}




