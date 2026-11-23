#include "MapGenMineshaft.h"

#include <algorithm>
#include <cstdlib>

#include "StructureMineshaftStart.h"
#include "java/Arithmetic.h"

MapGenMineshaft::MapGenMineshaft(const std::map<std::string, std::string> &options)
	: field_82673_e(0.01), useCustomChance(true)
{
	for (const auto &entry : options)
	{
		if (entry.first == "chance")
		{
			char *endPtr = nullptr;
			const double v = std::strtod(entry.second.c_str(), &endPtr);
			if (endPtr != entry.second.c_str() && *endPtr == '\0')
				field_82673_e = v;
		}
	}
}

bool MapGenMineshaft::canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ)
{
	const bool roll = useCustomChance ? (rand.nextDouble() < field_82673_e) : (rand.nextInt(100) == 0);
	return roll &&
	       rand.nextInt(80) < std::max(JavaArithmetic::intAbs(chunkX), JavaArithmetic::intAbs(chunkZ));
}

StructureStart *MapGenMineshaft::getStructureStart(int_t chunkX, int_t chunkZ)
{
	return new StructureMineshaftStart(worldObj, rand, chunkX, chunkZ);
}
