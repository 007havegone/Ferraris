#include "Geometry.h"

namespace ferraris::tools {

namespace {

using namespace math;
using namespace DirectX;

void
recalculate_normals(mesh& m)
{
	const u32 num_indices{ (u32)m.raw_indices.size() };
	m.normals.reserve(num_indices);

	for (u32 i{ 0 }; i < num_indices; ++i)
	{
		const u32 i0{ m.raw_indices[i] };
		const u32 i1{ m.raw_indices[++i] };
		const u32 i2{ m.raw_indices[++i] };


		XMVECTOR v0{ XMLoadFloat3(&m.positions[i0]) };
		XMVECTOR v1{ XMLoadFloat3(&m.positions[i1]) };
		XMVECTOR v2{ XMLoadFloat3(&m.positions[i2]) };

		XMVECTOR e1{ v1 - v0 };
		XMVECTOR e2{ v2 - v0 };

		XMVECTOR n{ XMVector3Normalize(XMVector3Cross(e1,e2)) };

		XMStoreFloat3(&m.normals[i], n);
		m.normals[i - 1] = m.normals[i];
		m.normals[i - 2] = m.normals[i];
	}
}
/**
*	Split the vertex to have hard or soft edges.
		1. Hard-edge, a vertex have several normal(per triangle), angle(n1 n2) > smoothing_angle
		2. Soft-edge, a vertex have one normal edge, angle(n1,n2) < smoothing_angle 
		We can compare their cosins.

*/
void
process_normals(mesh& m, f32 smothing_angle)
{
	const f32 cos_angle{ XMScalarCos(pi - smothing_angle * pi / 180.f) };
	const bool is_hard_edge{ XMScalarNearEqual(smothing_angle, 180.f, epsilon) };
	const bool is_soft_edge{ XMScalarNearEqual(smothing_angle, 0.f, epsilon) };
	const u32 num_indices{ (u32)m.raw_indices.size() };
	const u32 num_vertices{ (u32)m.positions.size() };
	assert(num_indices && num_vertices);

	m.indices.resize(num_indices);

	utl::vector<utl::vector<u32>> idx_ref(num_vertices);
	// for each vertex, get the idx which ref it.
	for (u32 i{ 0 }; i < num_indices; ++i)
		idx_ref[m.raw_indices[i]].emplace_back(i);

	for (u32 i{ 0 }; i < num_vertices; ++i)
	{
		auto& refs{ idx_ref[i] }; // consider which refer this vertex
		u32 num_refs{ (u32)refs.size() };
		for (u32 j{ 0 }; j < num_refs; ++j)
		{
			m.indices[refs[j]] = (u32)m.vertices.size();
			vertex& v{ m.vertices.emplace_back() };
			v.position = m.positions[m.raw_indices[refs[j]]];

			XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
			if (!is_hard_edge)
			{
				// compute a soft normal
				for (u32 k{ j + 1 }; k < num_refs; ++k)
				{
					// this value represent the cosine of the angle between normals.
					f32 n{ 0.f };
					XMVECTOR n2{ XMLoadFloat3(&m.normals[refs[k]]) };
					if (!is_soft_edge)
					{
						// NOTE: we're accounting for the length of n1 is this calculation because
						//		 it can possibly change in this loop iteration. We assume unit length
						//		 for n2.
						XMStoreFloat(&n, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
					}
					if (is_soft_edge || n > cos_angle)
					{
						n1 += n2;

						m.indices[refs[k]] = m.indices[refs[j]];
						refs.erase(refs.begin() + k);
						--num_refs;
						--k;
					}
				}
			}
			// hard-edge, one normal for per vertex
			XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
		}
	}
		
}
/**
	Vertex processor conditions:
		1. Vertex are unique (except to coerce hard edges)
		2. Normals and USs are per triangle vertex
*/
void
process_vertices(mesh& m, const geometry_import_settings& settings)
{
	assert((m.raw_indices.size() % 3 == 0));
	if (settings.calculate_normals || m.normals.empty())
	{
		recalculate_normals(m);
	}
	process_normals(m, settings.smoothing_angle);
	
	if (!m.uv_sets.empty())
	{
		//process_uvs(m);
	}

	//pack_vertices_static(m);
}
} // anomymous namespace

void
process_scene(scene& scene, const geometry_import_settings& settings)
{
	for (auto& lod : scene.lod_groups)
	{
		for (auto& m : lod.meshes)
		{
			process_vertices(m, settings);
		}
	}
}


void
pack_data(const scene& scene, scene_data& data)
{

}
}