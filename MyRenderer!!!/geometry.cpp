#include <cassert>

#include "geometry.h"

// *******************************************************************************
// ����
// *******************************************************************************
//����ά�������䵽��ά or ��ά
Vec3f Vec2f::embed_3() {
	Vec3f vf;

	for (int i = 0; i < 2; i++) vf[i] = val[i];
	vf[2] = 1.f;

	return vf;
}
Vec4f Vec2f::embed_4() {
	Vec4f vf;

	for (int i = 0; i < 2; i++) vf[i] = val[i];
	vf[2] = 1.f;
	vf[3] = 1.f;

	return vf;
}


// ����ά�������䵽��ά
Vec4f Vec3f::embed_4() {
	Vec4f ret;

	for (int i = 0; i < 3; i++) ret[i] = val[i];
	ret[3] = 1.f;

	return ret;
}

// ��ά��3->2
Vec2f Vec3f::proj_2() {
	Vec2f ret;

	for (int i = 0; i < 2; i++)
		ret[i] = val[i];
	return ret;
}

// ��ά
Vec2f Vec4f::proj_2() {
	Vec2f ret;

	for (int i = 0; i < 2; i++)
		ret[i] = val[i];
	return ret;
}

Vec3f Vec4f::proj_3() {
	Vec3f ret;

	for (int i = 0; i < 3; i++)
		ret[i] = val[i];
	return ret;
}

// *******************************************************************************
// ����
// *******************************************************************************

Matrix::Matrix(int r, int c) : rows(r), cols(c) {
	// ��ʼ�����󣬲�����ÿ��Ԫ����Ϊ0
	m = std::vector<std::vector<float> >(r, std::vector<float>(c, 0.f));
}

// ��ӡ����
void Matrix::printfMat() {
	std::cout << "������ϢΪ��" << std::endl;
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			std::cout << m[r][c] << " ";
		}
		std::cout << std::endl;
	}
}

// ������
void Matrix::fillMat(std::initializer_list<float> il) {
	int count = 0;

	for (auto ptr = il.begin(); ptr != il.end(); ptr++) {	
		m[count / rows][count % rows] = *ptr;
		count++;
	}
}

// ����һ��ָ��ά�ȵĵ�λ����
void Matrix::identity(int dimensions) {
	assert(dimensions > 0);

	for (int i = 0; i < dimensions; i++) {
		for (int j = 0; j < dimensions; j++) {
			m[i][j] = (i == j ? 1.f : 0.f);
		}
	}
}


std::vector<float>& Matrix::operator[] (int i){
	assert(i >= 0 && i < rows);
	return m[i];
}

// �������
Matrix Matrix::operator* (Matrix& m1) {
	assert(cols == m1.rows);

	Matrix result(rows, m1.cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < m1.cols; j++) {
			result[i][j] = 0.f;

			for (int k = 0; k < cols; k++) {
				result[i][j] += m[i][k] * m1[k][j];
			}
		}
	}
	return result;
}

// ������˾���
Vec4f Matrix::operator* (Vec4f& vf) {
	Vec4f ret(0, 0, 0, 0);	// �ǵð��´���������������������0������
	if ((rows == cols) && rows == 4 && cols == 4) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				ret[i] += m[i][j] * vf[j];
			}
			//ret.printfVec();
		}
	}
	else {
		std::cout << "������˾���ʱ�������⣺ά�Ȳ��Եȡ�\n" << std::endl;
		return ret;
	}

	return ret;
}



// *******************************************************************************
// �任��
// *******************************************************************************
// ģ�ͱ任
Matrix ModelTrans(int world_width, int world_height, int world_depth,
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

	return model;
}

// �۲�任
Matrix LookAt(Vec3f camera_pos, Vec3f center, Vec3f up) {
	// ȷ���������������
	//Vec3f z = (center - camera_pos).normalized();	// ��Ϊֻ��Ҫ��������Ҫ����һ��
	Vec3f z = (camera_pos - center).normalized();	// ��Ϊֻ��Ҫ��������Ҫ����һ��
	//z.normalize();	
	Vec3f x = (up ^ z).normalized();
	//x.normalize();
	Vec3f y = (z ^ x).normalized();

	//��1���Ƚ�camera_pos�Ƶ�ԭ��(0, 0, 0)
	Matrix Trans(4, 4);
	Trans.identity(4);
	for (int i = 0; i < 3; i++) {
		Trans[i][3] = -camera_pos[i];
	}

	//��2��������ĸ�����ת��������������
	Matrix Minv(4, 4);
	Minv.identity(4);
	for (int i = 0; i < 3; i++) {
		Minv[0][i] = x[i];
		Minv[1][i] = y[i];
		Minv[2][i] = z[i];
	}

	return Minv * Trans;
}

// ͶӰ
// ����ͶӰ orthogonal -> ����һ��ͬ��ԳƵ�������
Matrix orthoProjection(int w, int h, int n, int f) {
	Matrix projection(4, 4);
	projection.identity(4);

	std::cout << "w: " << w << " h: " << h << " n: " << n << " f: " << f << std::endl;

	projection[0][0] = (float)2 / w;
	projection[1][1] = (float)2 / h;
	projection[2][2] = (float)1 / (f - n);
	projection[2][3] = (float)-n / (f - n);

	projection[3][3] = 1;

	//projection.printfMat();

	return projection;
}

// ͸��ͶӰ perspective
//��1��ֱ��ȷ��������ʽ
Matrix perspProjection(int w, int h, int n, int f) {
	Matrix projection(4, 4);
	//projection.identity(4);

	projection[0][0] = (float)2 * n / w;
	projection[1][1] = (float)2 * n / h;
	projection[2][2] = (float)f / (f - n);
	projection[2][3] = (float)-f * n / (f - n);

	projection[3][2] = 1.f;

	return projection;
}

//��2��FOVʽ
Matrix perspProjection(float FOV, float aspect, int n, int f) {
	Matrix projection(4, 4);

	float tan_FOV_half = std::tan((FOV / 2) * PI / 180.f);
	float cot_FOV_half = 1 / std::tan((FOV / 2) * PI / 180.f);

	projection[0][0] = (float)cot_FOV_half / aspect;
	projection[1][1] = (float)cot_FOV_half;
	projection[2][2] = (float)f / (f - n);
	projection[2][3] = (float)-f * n / (f - n);

	projection[3][2] = 1.f;	// ��������w������ԭ����z��͸�ӳ�����ʱ�����z�͵��ڳ�����z

	return projection;
}

// �ӿڱ任
Matrix ViewPort(int width, int height, int depth) {
	Matrix viewport;

	//��1��viewport: ��+1 => ��[-1, 1]ӳ�䵽[0, 1]
	Matrix vp_addonce(4, 4);
	vp_addonce.identity(4);
	vp_addonce[0][3] = 1.f;
	vp_addonce[1][3] = 1.f;
	vp_addonce[2][3] = 1.f;

	//��2��viewport��������
	Matrix vp_scale(4, 4);
	vp_scale[0][0] = (float)width / 2;
	vp_scale[1][1] = (float)height / 2;
	vp_scale[2][2] = (float)depth / 2;
	vp_scale[3][3] = 1.f;

	viewport = vp_scale * vp_addonce;

	return viewport;
}


