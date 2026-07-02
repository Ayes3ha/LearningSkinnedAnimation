#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include "geometry.h"
#include "tgaimage.h"

#pragma once

using namespace std;

class Mesh
{
private:
	vector<Vec3f>verts_;
	vector<Vec2f>uvs_;
	vector<Vec3f>norms_;
	vector<int>faces_vert_;
	vector<int>faces_uv_;
	vector<int>faces_norm_;
	char* texPath;
	TGAImage diffuseMap_;


public:
	// ��ȡ�ļ�
	Mesh(const string filePath, const char* tex_path);
	int nums_verts();						// �����ģ�͵Ķ������
	int nums_faces();						// �����ģ�͵�����
	Vec3f get_vert_(int n);					// ��ȡ��n�Ķ���λ��
	Vec3f get_vert_(int iface, int n);		// ��ȡ��iface����ĵ�n��
	Vec2f get_uv_(int n);					// ��ȡ��n�Ķ�����������
	Vec2f get_uv_(int iface, int n);		// ��ȡ��iface����ĵ�n�������������
	Vec3f get_norm_(int n);					// ��ȡ��n�Ķ��㷨��
	Vec3f get_norm_(int iface, int n);		// ��ȡ��iface����ĵ�n����ķ���
	int get_faces_vert_(int n);				// ��ȡ��n����Ķ�������

	// ��ȡ�����ļ�
	void load_diffuse_tex(const char* texPath, TGAImage& image);
	// ��ȡdiffuse����
	Color diffuse(Vec2i tex_coord);
};




