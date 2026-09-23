#pragma once

#include <memory>
#include <vector>

#include "java/Type.h"
#include "NibbleArray.h"

// net.minecraft.src.ExtendedBlockStorage
class ExtendedBlockStorage
{
public:
	static constexpr int_t BLOCK_COUNT = 16 * 16 * 16;
	static constexpr int_t NIBBLE_DEPTH_BITS = 4;

	explicit ExtendedBlockStorage(int_t yBase);
	ExtendedBlockStorage(int_t yBase, std::vector<byte_t> blockLSB,
		std::unique_ptr<NibbleArray> blockMSB, NibbleArray metadata,
		NibbleArray blocklight, NibbleArray skylight);

	// The per-block readers and light writers are inline: lighting jobs,
	// collision and mesh building call them for every cell they touch, and as
	// out-of-line functions each was a call around a single nibble or byte load.
	int_t getExtBlockID(int_t x, int_t y, int_t z) const
	{
		int_t blockId = blockLSBArray[(size_t)getStorageIndex(x, y, z)] & 0xff;
		if (blockMSBArray != nullptr)
			blockId |= blockMSBArray->get(x, y, z) << 8;
		return normalizeStoredBlockId(blockId);
	}
	void setExtBlockID(int_t x, int_t y, int_t z, int_t blockId);

	int_t getExtBlockMetadata(int_t x, int_t y, int_t z) const
	{
		return blockMetadataArray.get(x, y, z);
	}
	void setExtBlockMetadata(int_t x, int_t y, int_t z, int_t metadata)
	{
		blockMetadataArray.set(x, y, z, metadata);
	}

	bool getIsEmpty() const;
	bool getNeedsRandomTick() const;
	int_t getYLocation() const;

	void setExtSkylightValue(int_t x, int_t y, int_t z, int_t value)
	{
		skylightArray.set(x, y, z, value);
	}
	int_t getExtSkylightValue(int_t x, int_t y, int_t z) const
	{
		return skylightArray.get(x, y, z);
	}

	void setExtBlocklightValue(int_t x, int_t y, int_t z, int_t value)
	{
		blocklightArray.set(x, y, z, value);
	}
	int_t getExtBlocklightValue(int_t x, int_t y, int_t z) const
	{
		return blocklightArray.get(x, y, z);
	}

	void func_48708_d();
	void recalculateBlockCounts();
	void setBulkBlockCounts(int_t blockCount, int_t randomTickCount);
	void func_48711_e();
	int_t func_48700_f() const;

	std::vector<byte_t> &func_48692_g();
	const std::vector<byte_t> &func_48692_g() const;
	void func_48715_h();

	NibbleArray *getBlockMSBArray();
	const NibbleArray *getBlockMSBArray() const;
	NibbleArray &func_48697_j();
	const NibbleArray &func_48697_j() const;
	NibbleArray &getBlocklightArray();
	const NibbleArray &getBlocklightArray() const;
	NibbleArray &getSkylightArray();
	const NibbleArray &getSkylightArray() const;

	void setBlockLSBArray(std::vector<byte_t> bytes);
	void setBlockMSBArray(std::unique_ptr<NibbleArray> array);
	void setBlockMetadataArray(NibbleArray array);
	void setBlocklightArray(NibbleArray array);
	void setSkylightArray(NibbleArray array);
	NibbleArray *createBlockMSBArray();

private:
	static int_t getStorageIndex(int_t x, int_t y, int_t z)
	{
		return y << 8 | z << 4 | x;
	}
	// Console builds keep a 256-entry block registry (Block::BLOCK_REGISTRY_SIZE)
	// and drop stored ids outside it, matching the check in the .cpp before the
	// readers moved here.
	static int_t normalizeStoredBlockId(int_t blockId)
	{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
		if (blockId < 0 || blockId >= 256)
			return 0;
#endif
		return blockId;
	}
	static bool isRandomTickBlock(int_t blockId);
	static bool isKnownBlockId(int_t blockId);
	static void validateBlockArraySize(const std::vector<byte_t> &bytes);
	static void validateNibbleArraySize(const NibbleArray &array);

	int_t yBase;
	int_t blockRefCount;
	int_t tickRefCount;
	std::vector<byte_t> blockLSBArray;
	std::unique_ptr<NibbleArray> blockMSBArray;
	NibbleArray blockMetadataArray;
	NibbleArray blocklightArray;
	NibbleArray skylightArray;
};
