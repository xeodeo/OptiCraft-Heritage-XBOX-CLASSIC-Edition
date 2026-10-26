#pragma once

enum class Facing
{
	NONE = -1,
	DOWN,
	UP,
	NORTH,
	SOUTH,
	WEST,
	EAST,
};

struct FacingUtil
{
	static constexpr int oppositeSide[6] = {1, 0, 3, 2, 5, 4};
	static constexpr int offsetsXForSide[6] = {0, 0, 0, 0, -1, 1};
	static constexpr int offsetsYForSide[6] = {-1, 1, 0, 0, 0, 0};
	static constexpr int offsetsZForSide[6] = {0, 0, -1, 1, 0, 0};
};
