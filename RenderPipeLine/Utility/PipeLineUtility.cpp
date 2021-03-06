#include "PipeLineUtility.h"
#include "..\Framework\RenderDevice.h"
#include "..\Mathematics\Matrix.h"
#include "..\Framework\Camera.h"
#include "..\Mathematics\MathUtil.h"
#include "..\Mathematics\Float4.h"
#include <tuple>
#include <functional>
#include "RenderContext.h"
#include "RenderBuffer.h"
using namespace std;

Matrix RenderPipeLine::m_ViewPortMatrix;

void RenderPipeLine::DrawLine(int2 position0, int2 position1, DWORD color, DrawLineType type)
{
	int startX	= position0.x;
	int startY	= position0.y;
	int endX	= position1.x;
	int endY	= position1.y;

	switch (type)
	{
	case DDA:
	{
		float dx = endX - startX;
		float dy = endY - startY;

		float steps = std::max<float>(std::abs(dx), std::abs(dy));

		float increx = dx / steps;
		float increy = dy / steps;

		float xi = startX;
		float yi = startY;

		for (int i = 0; i < steps; i++)
		{
			RenderDevice::getSingletonPtr()->DrawPixel(std::lround(xi), std::lround(yi), color);

			xi += increx;
			yi += increy;
		}
	}
	break;
	case Bresenham:
	{
		int dx = static_cast<int>(endX - startX);
		int dy = static_cast<int>(endY - startY);

		int ux = (dx > 0) ? 1 : -1;
		int uy = (dy > 0) ? 1 : -1;

		int xi = static_cast<int>(startX);
		int yi = static_cast<int>(startY);

		int eps = 0;
		dx = std::abs(dx);
		dy = std::abs(dy);

		if (dx > dy)
		{
			for (; xi != std::round(endX); xi += ux)
			{
				RenderDevice::getSingletonPtr()->DrawPixel(xi, yi, color);
				eps += dy;
				if ((eps << 1) >= dx)
				{
					yi += uy;
					eps -= dx;
				}
			}
		}
		else
		{
			for (; yi != std::round(endY); yi += uy)
			{
				RenderDevice::getSingletonPtr()->DrawPixel(xi, yi, color);
				eps += dx;
				if ((eps << 1) >= dy)
				{
					xi += ux;
					eps -= dy;
				}
			}
		}
	}
	break;
	default:
		break;
	}
	
}

void RenderPipeLine::DrawCall(const RenderContext* context, const RenderBuffer& renderBuffer)
{
	auto vertices	= renderBuffer.vertices;
	auto indices	= renderBuffer.indices;
	auto normals = renderBuffer.normals;
	auto colors = renderBuffer.colors;

	auto VertexShader = [](const vector<float3>& vertices, vector<float3>& normals, const Matrix& vp, vector<float4>& vertexOutPut)
	{
		for (auto vertex : vertices)
		{
			vertexOutPut.push_back(float4(vertex.x, vertex.y, vertex.z, 1.0f)*vp);
		}

		for (auto& normal : normals)
		{
			auto n = float4(normal.x, normal.y, normal.z, 0.0f)*vp;
			normal = MathUtil::Normalize(float3(n.x, n.y, n.z));
		}
	};

	auto vp = context->ViewMatrix * context->ProjMatrix;
	vector<float4> vertexOutPut;
	VertexShader(vertices, normals, vp, vertexOutPut);

	for (auto& vertex : vertexOutPut)
	{
		vertex = MathUtil::Homogenous(vertex);
	}

	// TriangleSetup
	auto indexLength = indices.size();
	for (int index = 0; index < indexLength; index +=3 )
	{
		auto index0 = indices[index + 0];
		auto index1 = indices[index + 1];
		auto index2 = indices[index + 2];

		auto v0 = vertexOutPut[index0];
		auto v1 = vertexOutPut[index1];
		auto v2 = vertexOutPut[index2];

		auto color0 = colors[index0];
		auto color1 = colors[index1];
		auto color2 = colors[index2];

		auto normal0 = normals[index0];
		auto normal1 = normals[index1];
		auto normal2 = normals[index2];

		auto faceNormal = MathUtil::Cross(float3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z), float3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z));
		auto viewDir = MathUtil::Normalize(float3(0.0f, 0.0f, 0.0f) - float3(v0.x, v0.y, v0.z));
		auto visible = faceNormal * viewDir >= 0.0f;

		if (visible)
		{
			v0 = v0 * m_ViewPortMatrix;
			v1 = v1 * m_ViewPortMatrix;
			v2 = v2 * m_ViewPortMatrix;

			Rasterize_Barycentric(context,
				tuple<int2, float4, float3>(int2(v0.x, v0.y), color0, normal0),
				tuple<int2, float4, float3>(int2(v1.x, v1.y), color1, normal1),
				tuple<int2, float4, float3>(int2(v2.x, v2.y), color2, normal2));
			//Rasterize_Standard(tuple<int2, float4>(int2(v0.x, v0.y), color0), tuple<int2, float4>(int2(v1.x, v1.y), color1), tuple<int2, float4>(int2(v2.x, v2.y), color2));
			//Rasterize_WireFrame(int2(v0.x, v0.y), int2(v1.x, v1.y), int2(v2.x, v2.y));
		}
	}
}

void RenderPipeLine::Rasterize_Standard(tuple<int2, float4> v0, tuple<int2, float4> v1, tuple<int2, float4> v2)
{
	auto position0 = ref(get<0>(v0));
	auto position1 = ref(get<0>(v1));
	auto position2 = ref(get<0>(v2));

	// 在 y 轴上 进行排序 使 v0 <= v1 < = v2
	if (position1.get().y < position0.get().y)
	{
		std::swap(v1, v0);
	}

	if (position2.get().y < position0.get().y)
	{
		std::swap(v2, v0);
	}

	if (position2.get().y < position1.get().y)
	{
		std::swap(v2, v1);
	}

	// y0 == y1 则是平底三角形
	if (MathUtil::IsEqual(position0.get().y, position1.get().y))
	{
		std::swap(v2, v0);

		if (position2.get().x < position1.get().x)
		{
			std::swap(v2, v1);
		}

		// 光栅化三角形
		RasterizeFace_Standard(v0, v1, v2);
	}

	// y1 == y2 则是平顶三角形
	else if (MathUtil::IsEqual(position1.get().y, position2.get().y))
	{
		if (position2.get().x < position1.get().x)
		{
			std::swap(v2, v1);
		}

		// 光栅化三角形
		RasterizeFace_Standard(v0, v1, v2);
	}

	// 任意三角形
	else
	{
		// 分裂三角形
		auto vertices = SplitTriangle_Standard(v0, v1, v2);

		for (int i = 0; i < vertices.size(); i += 3)
		{
			// 光栅化三角形
			Rasterize_Standard(vertices[i + 0], vertices[i + 1], vertices[i + 2]);
		}
	}
}

vector<tuple<int2, float4>> RenderPipeLine::SplitTriangle_Standard(tuple<int2, float4> v0, tuple<int2, float4> v1, tuple<int2, float4> v2)
{
	vector<tuple<int2, float4>> vertices;

	auto position0 = get<0>(v0);
	auto position1 = get<0>(v1);
	auto position2 = get<0>(v2);

	auto color0 = get<1>(v0);
	auto color1 = get<1>(v1);
	auto color2 = get<1>(v2);

	float dy1_0 = position1.y - position0.y;
	float dy2_0 = position2.y - position0.y;
	float dx2_0 = position2.x - position0.x;
	
	float4 dc2_0 = color2 - color0;

	// dx(new-0)/dx(2-0) = dy(1-0)/dy(2-0) => newX = 0 + dy(1-0)/dy(2-0)*dx(2-0)
	float newX = position0.x + dx2_0 * (dy1_0 / dy2_0);

	float4 newColor = color0 + dc2_0 * (dy1_0 / dy2_0);

	tuple<int2, float4> newVertex(int2(newX, position1.y), newColor);

	//////////////////////////////////////////////////////////////////////////
	// v0, v1, v2
	vertices.push_back(v0);
	vertices.push_back(v1);
	vertices.push_back(newVertex);

	//////////////////////////////////////////////////////////////////////////
	// v3, v4, v5
	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(newVertex);

	return vertices;
}

void RenderPipeLine::RasterizeFace_Standard(tuple<int2, float4> v0, tuple<int2, float4> v1, tuple<int2, float4> v2)
{
	int x0 = get<0>(v0).x;
	int y0 = get<0>(v0).y;

	int x1 = get<0>(v1).x;
	int y1 = get<0>(v1).y;

	int x2 = get<0>(v2).x;
	int y2 = get<0>(v2).y;

	float4 color0 = get<1>(v0);
	float4 color1 = get<1>(v1);
	float4 color2 = get<1>(v2);

	// 检测三角形是否为直线
	if (MathUtil::IsEqual(x0, x1) && MathUtil::IsEqual(x1, x2) || MathUtil::IsEqual(y0, y1) && MathUtil::IsEqual(y1, y2))
		return;

	bool topface = y0 > y2;

	// 设置扫描线起点x及终点x
	float xstart	= (topface ? x1 : x0);
	float xend		= (topface ? x2 : x0);
	float4 cstart	= (topface ? color1 : color0);
	float4 cend		= (topface ? color2 : color0);

	// 设置扫描线起始y及终点y
	int ystart= (topface ? y2 : y0);
	int yend  = (topface ? y0 : y2);

	// 计算三角形的高
	float dy = yend - ystart;
	float inv_dy = 1.0f / dy;

	// 计算左斜边积分
	float dxdyl = (topface ? (x0 - x1) : (x1 - x0)) * inv_dy;
	float4 dcdyl = (topface ? (color0 - color1) : (color1 - color0)) * inv_dy;

	// 计算右斜边积分
	float dxdyr = (topface ? (x0 - x2) : (x2 - x0)) * inv_dy;
	float4 dcdyr = (topface ? (color0 - color2) : (color2 - color0)) * inv_dy;

	//DWORD color = (255 << 24) + (255 << 16) + (255 << 8) + 255;

	for (int yi = ystart; yi <= yend; yi++)
	{
		float4 color = cstart;
		float4 dcdx = (cend - cstart) * (1.0f / (xend - xstart));
		for (int xi = xstart; xi <= xend; xi++)
		{
			float r = min(color.x, 1);
			float g = min(color.y, 1);
			float b = min(color.z, 1);

			DWORD curColor = (255 << 24) + ((int)(r * 255) << 16) + ((int)(g * 255) << 8) + (int)(b * 255);

			// 绘制像素点
			RenderDevice::getSingletonPtr()->DrawPixel(xi, yi, curColor);
			color += dcdx;
		}

		//计算下一条扫描线起点及终点
		xstart += dxdyl;
		xend += dxdyr;
		cstart += dcdyl;
		cend += dcdyr;
	}
}

void RenderPipeLine::Rasterize_Barycentric(const RenderContext* context, tuple<int2, float4, float3> v0, tuple<int2, float4, float3> v1, tuple<int2, float4, float3> v2)
{
	auto edgeFunction = [](const int2& a, const int2& b, const int2& c)
	{
		return (c.y - a.y) * (b.x - a.x) - (c.x - a.x) * (b.y - a.y);
	};

	auto boundMin = [](const int& a, const int& b, const int& c)
	{
		return min(min(a, b), min(a, c));
	};

	auto boundMax = [](const int& a, const int& b, const int& c)
	{
		return max(max(a, b), max(a, c));
	};

	int2 position0 = get<0>(v0);
	int2 position1 = get<0>(v1);
	int2 position2 = get<0>(v2);

	float4 color0 = get<1>(v0);
	float4 color1 = get<1>(v1);
	float4 color2 = get<1>(v2);

	float3 normal0 = get<2>(v0);
	float3 normal1 = get<2>(v1);
	float3 normal2 = get<2>(v2);

	int area = edgeFunction(position0, position1, position2);

	int xMin = floor(boundMin(position0.x, position1.x, position2.x));
	int xMax = ceil(boundMax(position0.x, position1.x, position2.x));
	int yMin = floor(boundMin(position0.y, position1.y, position2.y));
	int yMax = ceil(boundMax(position0.y, position1.y, position2.y));

	for (int x = xMin; x <= xMax; ++x)
	{
		for (int y = yMin; y <= yMax; ++y)
		{
			int2 p(x, y);

			float w0 = static_cast<float>(edgeFunction(position1, position2, p));
			float w1 = static_cast<float>(edgeFunction(position2, position0, p));
			float w2 = static_cast<float>(edgeFunction(position0, position1, p));

			if (w0 >= 0 && w1 >= 0 && w2 >= 0)
			{
				w0 /= area;
				w1 /= area;
				w2 /= area;

				//////////////////////////////////////////////////////////////////////////
				// color
				float4 newColor = color0 * w0 + color1 * w1 + color2 * w2;

				float r = min(newColor.x, 1);
				float g = min(newColor.y, 1);
				float b = min(newColor.z, 1);

				DWORD color = (255 << 24) + ((int)(r * 255) << 16) + ((int)(g * 255) << 8) + (int)(b * 255);
				//DWORD color = (255 << 24) + ((int)(255) << 16) + ((int)(255) << 8) + (int)(255);

				// PixelShader
				RenderDevice::getSingletonPtr()->DrawPixel(std::lround(x), std::lround(y), color);
			}
		}
	}
}

void RenderPipeLine::Rasterize_WireFrame(int2 v0, int2 v1, int2 v2)
{
	DWORD color = (255 << 24) + (255 << 16) + (255 << 8) + 255;
	DrawLine(v0, v1, color, DrawLineType::DDA);
	DrawLine(v0, v2, color, DrawLineType::DDA);
	DrawLine(v1, v2, color, DrawLineType::DDA);
}

void RenderPipeLine::SetViewPortData(int width, int height)
{
	float alpha = (0.5f * width - 0.5f);
	float beta = (0.5f * height - 0.5f);

	m_ViewPortMatrix = Matrix(
		float4(alpha,	0.0f,	0.0f,	0.0f),
		float4(0.0f,   -beta,	0.0f,	0.0f),
		float4(0.0f,	0.0f,	1.0f,	0.0f),
		float4(alpha,	beta,	0.0f,	1.0f));
}
