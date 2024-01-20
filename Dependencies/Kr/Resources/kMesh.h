#pragma once
#include "kRenderShared.h"

static kVertex3D kCubeVertices[] = {
	// Front
	{kVec3(-1, -1, -1), kVec3(0, 0, -1), kVec2(0, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, -1, -1), kVec3(0, 0, -1), kVec2(1, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, +1, -1), kVec3(0, 0, -1), kVec2(1, 1), kVec4(1, 1, 1, 1)},
	{kVec3(-1, +1, -1), kVec3(0, 0, -1), kVec2(0, 1), kVec4(1, 1, 1, 1)},

	// Right
	{kVec3(+1, -1, -1), kVec3(+1, 0, 0), kVec2(0, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, -1, +1), kVec3(+1, 0, 0), kVec2(1, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, +1, +1), kVec3(+1, 0, 0), kVec2(1, 1), kVec4(1, 1, 1, 1)},
	{kVec3(+1, +1, -1), kVec3(+1, 0, 0), kVec2(0, 1), kVec4(1, 1, 1, 1)},

	// Back
	{kVec3(+1, -1, +1), kVec3(0, 0, +1), kVec2(0, 0), kVec4(1, 1, 1, 1)},
	{kVec3(-1, -1, +1), kVec3(0, 0, +1), kVec2(1, 0), kVec4(1, 1, 1, 1)},
	{kVec3(-1, +1, +1), kVec3(0, 0, +1), kVec2(1, 1), kVec4(1, 1, 1, 1)},
	{kVec3(+1, +1, +1), kVec3(0, 0, +1), kVec2(0, 1), kVec4(1, 1, 1, 1)},

	// Left
	{kVec3(-1, -1, +1), kVec3(-1, 0, 0), kVec2(0, 0), kVec4(1 ,1, 1, 1)},
	{kVec3(-1, -1, -1), kVec3(-1, 0, 0), kVec2(1, 0), kVec4(1 ,1, 1, 1)},
	{kVec3(-1, +1, -1), kVec3(-1, 0, 0), kVec2(1, 1), kVec4(1 ,1, 1, 1)},
	{kVec3(-1, +1, +1), kVec3(-1, 0, 0), kVec2(0, 1), kVec4(1 ,1, 1, 1)},

	// Bottom
	{kVec3(-1, -1, -1), kVec3(0, -1, 0), kVec2(0, 0), kVec4(1, 1, 1, 1)},
	{kVec3(-1, -1, +1), kVec3(0, -1, 0), kVec2(1, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, -1, +1), kVec3(0, -1, 0), kVec2(1, 1), kVec4(1, 1, 1, 1)},
	{kVec3(+1, -1, -1), kVec3(0, -1, 0), kVec2(0, 1), kVec4(1, 1, 1, 1)},

	// Top
	{kVec3(-1, +1, -1), kVec3(0, +1, 0), kVec2(0, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, +1, -1), kVec3(0, +1, 0), kVec2(1, 0), kVec4(1, 1, 1, 1)},
	{kVec3(+1, +1, +1), kVec3(0, +1, 0), kVec2(1, 1), kVec4(1, 1, 1, 1)},
	{kVec3(-1, +1, +1), kVec3(0, +1, 0), kVec2(0, 1), kVec4(1, 1, 1, 1)},
};

static u32 kCubeIndices[] = {
	0,  1,  2,  0,  2,  3,  // Front
	4,  5,  6,  4,  6,  7,  // Right
	8,  9,  10, 8,  10, 11, // Back
	12, 13, 14, 12, 14, 15, // Left
	16, 17, 18, 16, 18, 19, // Bottom
	20, 21, 22, 20, 22, 23, // Top
};
