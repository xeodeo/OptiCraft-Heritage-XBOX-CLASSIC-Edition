#include "ExtendedBlockStorage.h"

#include <stdexcept>
#include <utility>

#include "Block.h"
#include "platform/PlatformTuning.h"

namespace
{
	constexpr int_t kCurrentBlockRegistrySize =
		(int_t)(sizeof(Block::blocksList) / sizeof(Block::blocksList[0]));
}

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
// The inline normalizeStoredBlockId() in the header hardcodes this bound.
static_assert(Block::BLOCK_REGISTRY_SIZE == 256,
              "ExtendedBlockStorage::normalizeStoredBlockId assumes a 256-entry console registry");
#endif

ExtendedBlockStorage::ExtendedBlockStorage(int_t yBaseIn)
	: yBase(yBaseIn)
	, blockRefCount(0)
	, tickRefCount(0)
	, blockLSBArray((size_t)BLOCK_COUNT, 0)
	, blockMSBArray(nullptr)
	, blockMetadataArray(BLOCK_COUNT, NIBBLE_DEPTH_BITS)
	, blocklightArray(BLOCK_COUNT, NIBBLE_DEPTH_BITS)
	, skylightArray(BLOCK_COUNT, NIBBLE_DEPTH_BITS)
{
}

ExtendedBlockStorage::ExtendedBlockStorage(int_t yBaseIn, std::vector<byte_t> blockLSB,
	std::unique_ptr<NibbleArray> blockMSB, NibbleArray metadata,
	NibbleArray blocklight, NibbleArray skylight)
	: yBase(yBaseIn)
	, blockRefCount(0)
	, tickRefCount(0)
	, blockLSBArray(std::move(blockLSB))
	, blockMSBArray(std::move(blockMSB))
	, blockMetadataArray(std::move(metadata))
	, blocklightArray(std::move(blocklight))
	, skylightArray(std::move(skylight))
{
	validateBlockArraySize(blockLSBArray);
	if (blockMSBArray != nullptr)
		validateNibbleArraySize(*blockMSBArray);
	validateNibbleArraySize(blockMetadataArray);
	validateNibbleArraySize(blocklightArray);
	validateNibbleArraySize(skylightArray);
}

void ExtendedBlockStorage::setExtBlockID(int_t x, int_t y, int_t z, int_t blockId)
{
	blockId = normalizeStoredBlockId(blockId);
	int_t oldBlockId = getExtBlockID(x, y, z);

	if (oldBlockId == 0 && blockId != 0)
	{
		++blockRefCount;
		if (isRandomTickBlock(blockId))
			++tickRefCount;
	}
	else if (oldBlockId != 0 && blockId == 0)
	{
		--blockRefCount;
		if (isRandomTickBlock(oldBlockId))
			--tickRefCount;
	}
	else
	{
		const bool oldNeedsRandomTick = isRandomTickBlock(oldBlockId);
		const bool newNeedsRandomTick = isRandomTickBlock(blockId);
		if (oldNeedsRandomTick && !newNeedsRandomTick)
			--tickRefCount;
		else if (!oldNeedsRandomTick && newNeedsRandomTick)
			++tickRefCount;
	}

	blockLSBArray[(size_t)getStorageIndex(x, y, z)] = (byte_t)(blockId & 0xff);
	if (blockId > 255)
	{
		if (blockMSBArray == nullptr)
			createBlockMSBArray();
		blockMSBArray->set(x, y, z, (blockId & 0xf00) >> 8);
	}
	else if (blockMSBArray != nullptr)
	{
		blockMSBArray->set(x, y, z, 0);
	}
}

bool ExtendedBlockStorage::getIsEmpty() const
{
	return blockRefCount == 0;
}

bool ExtendedBlockStorage::getNeedsRandomTick() const
{
	return tickRefCount > 0;
}

int_t ExtendedBlockStorage::getYLocation() const
{
	return yBase;
}

void ExtendedBlockStorage::func_48708_d()
{
	blockRefCount = 0;
	tickRefCount = 0;

#if PLATFORM_FAST_BLOCK_COUNT_SCAN
	if (blockMSBArray == nullptr)
	{
		// Anvil stores X contiguously; avoid coordinate lookups and strided reads.
		for (byte_t &value : blockLSBArray)
		{
			const int_t blockId = value & 0xff;
			if (blockId == 0)
				continue;
			if (blockId < kCurrentBlockRegistrySize && !isKnownBlockId(blockId))
			{
				value = 0;
				continue;
			}
			++blockRefCount;
			if (isRandomTickBlock(blockId))
				++tickRefCount;
		}
		return;
	}
#endif

	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			for (int_t z = 0; z < 16; ++z)
			{
				int_t blockId = getExtBlockID(x, y, z);
				if (blockId <= 0)
					continue;

				if (blockId < kCurrentBlockRegistrySize && !isKnownBlockId(blockId))
				{
					blockLSBArray[(size_t)getStorageIndex(x, y, z)] = 0;
					if (blockMSBArray != nullptr)
						blockMSBArray->set(x, y, z, 0);
					continue;
				}

				++blockRefCount;
				if (isRandomTickBlock(blockId))
					++tickRefCount;
			}
		}
	}
}

void ExtendedBlockStorage::setBulkBlockCounts(int_t blockCount, int_t randomTickCount)
{
	if (blockCount < 0 || blockCount > BLOCK_COUNT || randomTickCount < 0 || randomTickCount > blockCount)
		throw std::invalid_argument("ExtendedBlockStorage bulk block counts are out of range");
	blockRefCount = blockCount;
	tickRefCount = randomTickCount;
}

void ExtendedBlockStorage::recalculateBlockCounts()
{
	blockRefCount = 0;
	tickRefCount = 0;

	if (blockMSBArray == nullptr)
	{
		for (byte_t value : blockLSBArray)
		{
			const int_t blockId = value & 0xff;
			if (blockId == 0)
				continue;

			++blockRefCount;
			if (isRandomTickBlock(blockId))
				++tickRefCount;
		}
		return;
	}

	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			for (int_t z = 0; z < 16; ++z)
			{
				const int_t blockId = getExtBlockID(x, y, z);
				if (blockId == 0)
					continue;

				++blockRefCount;
				if (isRandomTickBlock(blockId))
					++tickRefCount;
			}
		}
	}
}

void ExtendedBlockStorage::func_48711_e()
{
}

int_t ExtendedBlockStorage::func_48700_f() const
{
	return blockRefCount;
}

std::vector<byte_t> &ExtendedBlockStorage::func_48692_g()
{
	return blockLSBArray;
}

const std::vector<byte_t> &ExtendedBlockStorage::func_48692_g() const
{
	return blockLSBArray;
}

void ExtendedBlockStorage::func_48715_h()
{
	blockMSBArray.reset();
}

NibbleArray *ExtendedBlockStorage::getBlockMSBArray()
{
	return blockMSBArray.get();
}

const NibbleArray *ExtendedBlockStorage::getBlockMSBArray() const
{
	return blockMSBArray.get();
}

NibbleArray &ExtendedBlockStorage::func_48697_j()
{
	return blockMetadataArray;
}

const NibbleArray &ExtendedBlockStorage::func_48697_j() const
{
	return blockMetadataArray;
}

NibbleArray &ExtendedBlockStorage::getBlocklightArray()
{
	return blocklightArray;
}

const NibbleArray &ExtendedBlockStorage::getBlocklightArray() const
{
	return blocklightArray;
}

NibbleArray &ExtendedBlockStorage::getSkylightArray()
{
	return skylightArray;
}

const NibbleArray &ExtendedBlockStorage::getSkylightArray() const
{
	return skylightArray;
}

void ExtendedBlockStorage::setBlockLSBArray(std::vector<byte_t> bytes)
{
	validateBlockArraySize(bytes);
	blockLSBArray = std::move(bytes);
}

void ExtendedBlockStorage::setBlockMSBArray(std::unique_ptr<NibbleArray> array)
{
	if (array != nullptr)
		validateNibbleArraySize(*array);
	blockMSBArray = std::move(array);
}

void ExtendedBlockStorage::setBlockMetadataArray(NibbleArray array)
{
	validateNibbleArraySize(array);
	blockMetadataArray = std::move(array);
}

void ExtendedBlockStorage::setBlocklightArray(NibbleArray array)
{
	validateNibbleArraySize(array);
	blocklightArray = std::move(array);
}

void ExtendedBlockStorage::setSkylightArray(NibbleArray array)
{
	validateNibbleArraySize(array);
	skylightArray = std::move(array);
}

NibbleArray *ExtendedBlockStorage::createBlockMSBArray()
{
	blockMSBArray = std::make_unique<NibbleArray>(BLOCK_COUNT, NIBBLE_DEPTH_BITS);
	return blockMSBArray.get();
}

bool ExtendedBlockStorage::isRandomTickBlock(int_t blockId)
{
	return isKnownBlockId(blockId) && Block::tickOnLoad[blockId];
}

bool ExtendedBlockStorage::isKnownBlockId(int_t blockId)
{
	return blockId > 0
		&& blockId < kCurrentBlockRegistrySize
		&& Block::blocksList[blockId] != nullptr;
}

void ExtendedBlockStorage::validateBlockArraySize(const std::vector<byte_t> &bytes)
{
	if (bytes.size() != (size_t)BLOCK_COUNT)
		throw std::invalid_argument("ExtendedBlockStorage block array must contain 4096 entries");
}

void ExtendedBlockStorage::validateNibbleArraySize(const NibbleArray &array)
{
	if (array.data.size() != (size_t)(BLOCK_COUNT >> 1))
		throw std::invalid_argument("ExtendedBlockStorage nibble array must contain 2048 bytes");
}
