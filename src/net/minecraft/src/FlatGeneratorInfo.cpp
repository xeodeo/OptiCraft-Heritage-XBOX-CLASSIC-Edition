#include "FlatGeneratorInfo.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "BiomeGenBase.h"
#include "Block.h"
#include "BlockGrass.h"
#include "WorldHeight.h"

namespace
{
std::vector<std::string> splitAll(const std::string &str, char delim)
{
	std::vector<std::string> parts;
	std::size_t start = 0;
	while (true)
	{
		const std::size_t pos = str.find(delim, start);
		if (pos == std::string::npos)
		{
			parts.push_back(str.substr(start));
			break;
		}
		parts.push_back(str.substr(start, pos - start));
		start = pos + 1;
	}
	return parts;
}

std::vector<std::string> splitLimit2(const std::string &str, char delim)
{
	std::vector<std::string> parts;
	const std::size_t pos = str.find(delim);
	if (pos == std::string::npos)
	{
		parts.push_back(str);
	}
	else
	{
		parts.push_back(str.substr(0, pos));
		parts.push_back(str.substr(pos + 1));
	}
	return parts;
}

bool parseStrictInt(const std::string &s, int_t &out)
{
	if (s.empty())
		return false;
	char *endPtr = nullptr;
	const long val = std::strtol(s.c_str(), &endPtr, 10);
	if (endPtr == s.c_str() || *endPtr != '\0')
		return false;
	out = static_cast<int_t>(val);
	return true;
}

int_t parseIntWithDefault(const std::string &s, int_t defaultVal)
{
	int_t parsed = defaultVal;
	return parseStrictInt(s, parsed) ? parsed : defaultVal;
}

std::string toLowerAscii(std::string s)
{
	for (char &c : s)
	{
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c - 'A' + 'a');
	}
	return s;
}
} // namespace

void FlatGeneratorInfo::func_82645_d()
{
	int_t y = 0;
	for (FlatLayerInfo &layer : flatLayers)
	{
		layer.setMinY(y);
		y += layer.getLayerCount();
	}
}

jstring FlatGeneratorInfo::toString() const
{
	std::ostringstream sb;
	sb << 2 << ';';

	for (std::size_t i = 0; i < flatLayers.size(); ++i)
	{
		if (i > 0)
			sb << ',';
		sb << static_cast<std::string>(flatLayers[i].toString());
	}

	sb << ';' << biomeToUse;

	if (!worldFeatures.empty())
	{
		sb << ';';
		int_t featureIdx = 0;
		for (const auto &entry : worldFeatures)
		{
			if (featureIdx++ > 0)
				sb << ',';
			sb << toLowerAscii(entry.first);
			if (!entry.second.empty())
			{
				sb << '(';
				int_t paramIdx = 0;
				for (const auto &param : entry.second)
				{
					if (paramIdx++ > 0)
						sb << ' ';
					sb << param.first << '=' << param.second;
				}
				sb << ')';
			}
		}
	}
	else
	{
		sb << ';';
	}

	return sb.str();
}

bool FlatGeneratorInfo::parseLayerInfo(const std::string &token, int_t currentY, FlatLayerInfo &outLayer)
{
	std::vector<std::string> parts = splitLimit2(token, 'x');
	int_t count = 1;
	int_t meta = 0;

	if (parts.size() == 2)
	{
		if (!parseStrictInt(parts[0], count))
			return false;
		if (currentY + count >= WorldHeight::HEIGHT)
			count = WorldHeight::HEIGHT - currentY;
		if (count < 0)
			count = 0;
	}

	int_t blockId = 0;
	const std::string blockPart = parts.back();
	std::vector<std::string> idMeta = splitLimit2(blockPart, ':');
	if (!parseStrictInt(idMeta[0], blockId))
		return false;
	if (idMeta.size() > 1)
	{
		if (!parseStrictInt(idMeta[1], meta))
			return false;
	}

	if (blockId < 0 || blockId >= Block::BLOCK_REGISTRY_SIZE || Block::blocksList[blockId] == nullptr)
	{
		blockId = 0;
		meta = 0;
	}
	if (meta < 0 || meta > 15)
		meta = 0;

	outLayer = FlatLayerInfo(count, blockId, meta);
	outLayer.setMinY(currentY);
	return true;
}

bool FlatGeneratorInfo::parseLayersFromString(const std::string &str, std::vector<FlatLayerInfo> &outLayers)
{
	if (str.empty())
		return false;

	outLayers.clear();
	const std::vector<std::string> tokens = splitAll(str, ',');
	int_t currentY = 0;

	for (const std::string &tok : tokens)
	{
		FlatLayerInfo layer(1, 0);
		if (!parseLayerInfo(tok, currentY, layer))
			return false;
		outLayers.push_back(layer);
		currentY += layer.getLayerCount();
	}
	return !outLayers.empty();
}

FlatGeneratorInfo FlatGeneratorInfo::createFlatGeneratorFromString(const jstring &str)
{
	const std::string s = str;
	if (s.empty())
		return getDefaultFlatGenerator();

	const std::vector<std::string> sections = splitAll(s, ';');
	const int_t version = (sections.size() == 1) ? 0 : parseIntWithDefault(sections[0], 0);
	if (version < 0 || version > 2)
		return getDefaultFlatGenerator();

	FlatGeneratorInfo info;
	std::size_t idx = (sections.size() == 1) ? 0 : 1;
	std::vector<FlatLayerInfo> layers;
	if (idx >= sections.size() || !parseLayersFromString(sections[idx++], layers) || layers.empty())
		return getDefaultFlatGenerator();

	info.flatLayers = std::move(layers);
	info.func_82645_d();

	int_t biome = (BiomeGenBase::plains != nullptr) ? BiomeGenBase::plains->biomeID : 1;
	if (version > 0 && sections.size() > idx)
	{
		biome = parseIntWithDefault(sections[idx++], biome);
	}
	info.setBiome(biome);

	if (version > 0 && sections.size() > idx)
	{
		const std::string featSection = toLowerAscii(sections[idx++]);
		const std::vector<std::string> featList = splitAll(featSection, ',');
		for (const std::string &featRaw : featList)
		{
			const std::vector<std::string> nameAndParams = splitLimit2(featRaw, '(');
			if (nameAndParams[0].empty())
				continue;

			FeatureParams params;
			if (nameAndParams.size() > 1 && nameAndParams[1].size() > 1 && nameAndParams[1].back() == ')')
			{
				const std::string inner = nameAndParams[1].substr(0, nameAndParams[1].size() - 1);
				const std::vector<std::string> kvTokens = splitAll(inner, ' ');
				for (const std::string &kv : kvTokens)
				{
					const std::vector<std::string> pair = splitLimit2(kv, '=');
					if (pair.size() == 2 && !pair[0].empty())
						params[pair[0]] = pair[1];
				}
			}
			info.worldFeatures[nameAndParams[0]] = std::move(params);
		}
	}
	else
	{
		info.worldFeatures["village"] = FeatureParams();
	}

	return info;
}

FlatGeneratorInfo FlatGeneratorInfo::getDefaultFlatGenerator()
{
	FlatGeneratorInfo info;
	info.setBiome(BiomeGenBase::plains != nullptr ? BiomeGenBase::plains->biomeID : 1);
	info.flatLayers.emplace_back(1, Block::bedrock != nullptr ? Block::bedrock->blockID : 7);
	info.flatLayers.emplace_back(2, Block::dirt != nullptr ? Block::dirt->blockID : 3);
	info.flatLayers.emplace_back(1, Block::grass != nullptr ? Block::grass->blockID : 2);
	info.func_82645_d();
	info.worldFeatures["village"] = FeatureParams();
	return info;
}
