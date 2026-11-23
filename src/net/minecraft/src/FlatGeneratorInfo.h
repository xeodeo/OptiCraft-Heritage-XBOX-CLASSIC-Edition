#pragma once

#include <map>
#include <string>
#include <vector>

#include "FlatLayerInfo.h"
#include "java/Type.h"
#include "java/String.h"

// net.minecraft.src.FlatGeneratorInfo (Java 1.5.2)
class FlatGeneratorInfo
{
public:
	using FeatureParams = std::map<std::string, std::string>;
	using FeatureMap = std::map<std::string, FeatureParams>;

	FlatGeneratorInfo() = default;

	int_t getBiome() const { return biomeToUse; }
	void setBiome(int_t biome) { biomeToUse = biome; }

	FeatureMap &getWorldFeatures() { return worldFeatures; }
	const FeatureMap &getWorldFeatures() const { return worldFeatures; }

	std::vector<FlatLayerInfo> &getFlatLayers() { return flatLayers; }
	const std::vector<FlatLayerInfo> &getFlatLayers() const { return flatLayers; }

	void func_82645_d();
	void updateLayers() { func_82645_d(); }

	jstring toString() const;

	static FlatGeneratorInfo createFlatGeneratorFromString(const jstring &str);
	static FlatGeneratorInfo getDefaultFlatGenerator();

private:
	static bool parseLayerInfo(const std::string &token, int_t currentY, FlatLayerInfo &outLayer);
	static bool parseLayersFromString(const std::string &str, std::vector<FlatLayerInfo> &outLayers);

	std::vector<FlatLayerInfo> flatLayers;
	FeatureMap worldFeatures;
	int_t biomeToUse = 0;
};
