#pragma once

namespace Engine
{
	enum class LIGHT_TYPE
	{
		POINT,
		SPOT,
		DIRECTIONAL,
		END
	};

	enum class AXIS_TYPE
	{
		X,
		Y,
		Z,
		END
	};

	enum class TEXTURE_MAP_TYPE
	{
		NONE,
		DIFFUSE,
		NORMAL,
		SPECULAR = 0b100,
		EMISSIVE = 0b1000,
		END
	};

	enum class SCENE_TYPE
	{
		CURRENT,
		NEXT,
		END
	};

	enum class BINDABLE_TYPE
	{
		NONE,
		VERTEX_BUFFER,
		INDEX_BUFFER,
		VERTEX_SHADER,
		HULL_SHADER,
		DOMAIN_SHADER,
		GEOMETRY_SHADER,
		PIXEL_SHADER,
		TEXTURE,
		MATERIAL,
		TRANSFORM,
		INPUTLAYOUT,
		TOPOLOGY,
		MESH,
		TERRAIN,
		COLLIDER_LINE,
		COLLIDER_SPHERE,
		COLLIDER_MESH,
		COLLIDER_OBB,
		ANIMATION,
		AGENT,
		NAV_MESH,
		LIGHT,
		PARTICLE,
		DECAL,
		PAPERBURN,
		FLUID,
		SKYBOX,
		CLOTH,
		CAMERA,
		DRAWABLE,
		BLEND_STATE,
		DEPTH_STENCIL_STATE,
		RASTERIZER_STATE,
		MOUSE,
		UIRENDERER,
		SOUND,
		UI_FRAME,
		UI_IMAGE,
		UI_GAUGE,
		END
	};

	enum class COLLIDER_TYPE
	{
		NONE,
		LINE,
		SPHERE,
		MESH,
		TERRAIN,
		OBB,
		END
	};

	enum class COLLISION_CHANNEL
	{
		NORMAL = 0x1,
		UI = 0x2,
		END
	};

	// Object-type tag for per-collider pair filtering. Orthogonal to
	// COLLISION_CHANNEL (which separates camera-pass: NORMAL vs UI).
	// Each Collider stores:
	//   m_eGroup — which group THIS collider belongs to (single bit)
	//   m_eMask  — which groups it WANTS to collide with (OR'd bits)
	// Pair passes filter iff (A.group & B.mask) && (B.group & A.mask).
	// Defaults (DEFAULT / ALL) preserve the pre-filter behaviour for any
	// collider that hasn't been categorised yet.
	enum class COLLISION_GROUP : unsigned int
	{
		NONE        = 0,
		DEFAULT     = 1u << 0,
		PLAYER      = 1u << 1,
		ENEMY       = 1u << 2,
		BULLET      = 1u << 3,
		PICKUP      = 1u << 4,
		CAMERA_LINE = 1u << 5,
		TERRAIN     = 1u << 6,
		TOWER       = 1u << 7,
		ALL         = 0xFFFFFFFFu
	};

	inline COLLISION_GROUP operator|(COLLISION_GROUP a, COLLISION_GROUP b)
	{
		return static_cast<COLLISION_GROUP>(
			static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
	}
	inline unsigned int operator&(COLLISION_GROUP a, COLLISION_GROUP b)
	{
		return static_cast<unsigned int>(a) & static_cast<unsigned int>(b);
	}

	enum class COLLISION_TYPE
	{
		BEGIN,
		STAY,
		LAST,
		END
	};

	enum class BOUNDING_VOLUME_TYPE
	{
		NONE,
		SPHERE,
		BOX,
		ELIPSOID,
		CYLINDER,
		END
	};

	enum class OBJECT_TYPE
	{
		NONE,
		BIND,
		DRAW,
		COLLIDER,
		END
	};

	enum class RENDER_LAYER
	{
		OPACUE,
		DECAL,
		ALPHA,
		BLUR,
		UI,
		END
	};

	enum class CAMERA_TYPE
	{
		NORMAL,
		UI,
		END
	};
}