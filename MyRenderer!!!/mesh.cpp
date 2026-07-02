#include "mesh.h"

// 读取obj文件
Mesh::Mesh(const string filePath, const char* tex_path) {
	// 创建读取文件流
	ifstream in;
	in.open(filePath, ifstream::in);
	if (!in.is_open()) {
		cout << "打开obj文件失败！" << endl;
	}

	//cout << "123" << endl;

	// 用于存储每行的数据
	string line;

	int c = 0;
	while (!in.eof()) {
		getline(in, line);
		istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;	// 将"v"读到char类型的trash，同时前进一个字符的位置
			Vec3f v;
			iss >> v.x;
			iss >> v.y;
			iss >> v.z;

			verts_.push_back(v);
		}
		else if (!line.compare(0, 3, "vt ")) {
			iss >> trash >> trash;
			Vec2f v;
			iss >> v.x;
			iss >> v.y;

			uvs_.push_back(v);
		}
		else if (!line.compare(0, 3, "vn ")) {
			iss >> trash >> trash;
			Vec3f v;
			iss >> v.x;
			iss >> v.y;
			iss >> v.z;

			norms_.push_back(v);
		}
		else if (!line.compare(0, 2, "f ")) {
			iss >> trash;

			int v, u, n;

			int cnt = 0;
			while (iss >> v >> trash >> u >> trash >> n) {
				faces_vert_.push_back(--v);
				faces_uv_.push_back(--u);
				faces_norm_.push_back(--n);

				cnt++;
			}

			if (cnt != 3) {
				cerr << "该模型不支持三角形输出！" << endl;
				in.close();
				return;
			}
		}
	}
	// 关闭文件流
	in.close();
	// 显示模型信息
	cout << "** v: " << nums_verts() << " f: " << nums_faces() << " vt: " << uvs_.size() << " vn: " << norms_.size() << " **" << endl;

	load_diffuse_tex(tex_path, diffuseMap_);
}

// 计算该模型的顶点个数
int Mesh::nums_verts() {
	return verts_.size();
}

// 计算该模型的面数
int Mesh::nums_faces() {
	return faces_vert_.size() / 3;
}

// 获取第n的顶点位置
Vec3f Mesh::get_vert_(int n) {
	return verts_[n];
}

// 获取第iface个面的第n点
Vec3f Mesh::get_vert_(int iface, int n) {
	return verts_[faces_vert_[iface * 3 + n]];
}

// 获取第n的顶点纹理坐标
Vec2f Mesh::get_uv_(int n) {
	return uvs_[n];
}
// 获取第iface个面的第n顶点的纹理坐标
//Vec2f Mesh::get_uv_(int iface, int n) {
//	return uvs_[faces_uv_[iface * 3 + n]];
//}
Vec2f Mesh::get_uv_(int iface, int n) {
	Vec2f uv = uvs_[faces_uv_[iface * 3 + n]];
	return Vec2f(uv.x * diffuseMap_.get_width(), uv.y * diffuseMap_.get_height());
}

// 获取第n的顶点法线
Vec3f Mesh::get_norm_(int n) {
	return norms_[n];
}
// 获取第iface个面的第n顶点的法线
Vec3f Mesh::get_norm_(int iface, int n) {
	return norms_[faces_norm_[iface * 3 + n]];
}

// 获取第n个面的顶点索引
int Mesh::get_faces_vert_(int n) {
	return faces_vert_[n];
}

// 读取纹理
void Mesh::load_diffuse_tex(const char* texPath, TGAImage& image) {
	// 读取纹理
	image.read_tga_file(texPath);
}

// 读取纹理
//Color Mesh::diffuse(Vec2i tex_coord, TGAImage& image) {
//	return image.get(tex_coord.x, tex_coord.y);
//}
Color Mesh::diffuse(Vec2i tex_coord) {
	return diffuseMap_.get(tex_coord.x, tex_coord.y);
}



