/* OptiCraft PS3: section meshing on the SPU, in the spirit of 4J's ChunkUpdate
   SPU task (Minecraft.Client/PS3/SPU_Tasks/ChunkUpdate): the PPU sends the
   section's block data with a border, the SPU tesselates the blocks it has code
   for and the PPU renders the rest (4J's e_flag_SPURenderCodeMissing).

   The tile code is not 4J's TileRenderer_SPU but a transcription of OUR PPU
   renderer (SpuAoCube.inl, from RenderBlocks::renderStandardBlockWithAmbientOcclusion),
   so the SPU vertices are bit-for-bit the PPU's and the verify mode can prove it.

   Floats: SPU single precision truncates and has no denormals, so every float
   operation of the AO path goes through sf, which does it in IEEE double and
   rounds to nearest float by hand (innocuous double rounding for + - * /:
   53 >= 2*24+2). */

#include <stdint.h>
#include <spu_intrinsics.h>
#include <cell/dma.h>

#include "SpuChunkMeshJob.h"
#include "ChunkMeshJob.h"
#include "../Common/DmaData.h"

namespace
{
	// ---- IEEE single precision via double ---------------------------------
	// Double to float, round to nearest even, done on the bits: the SPU's frds
	// does not round like the PPU (on the console it gave 0.79999995 where the
	// PPU had 0.8, one colour step off). Our values are normal floats, so
	// rounding the 52-bit mantissa to 23 bits in place leaves a double that is
	// exactly a float, and frds then has nothing left to round. The asm also
	// keeps gcc from narrowing (float)((double)a op (double)b) into the SPU's
	// non-IEEE single precision instruction.
	inline float roundToFloat(double d)
	{
		__asm__ volatile("" : "+r"(d));
		union { double d; unsigned long long u; } c;
		c.d = d;
		c.u += 0x0FFFFFFFull + ((c.u >> 29) & 1ull);
		c.u &= ~0x1FFFFFFFull;
		return (float)c.d;
	}

	struct sf
	{
		float v;
		sf() : v(0.0f) {}
		sf(float x) : v(x) {}
		sf &operator*=(const sf &o) { v = roundToFloat((double)v * (double)o.v); return *this; }
	};
	inline sf operator+(const sf &a, const sf &b) { return sf(roundToFloat((double)a.v + (double)b.v)); }
	inline sf operator-(const sf &a, const sf &b) { return sf(roundToFloat((double)a.v - (double)b.v)); }
	inline sf operator*(const sf &a, const sf &b) { return sf(roundToFloat((double)a.v * (double)b.v)); }
	inline sf operator/(const sf &a, const sf &b) { return sf(roundToFloat((double)a.v / (double)b.v)); }

	// ---- local store --------------------------------------------------------
	SpuChunkMeshInput s_in __attribute__((aligned(128)));
	unsigned short s_ids[SPU_MESH_CELLS] __attribute__((aligned(128)));
	unsigned char s_meta[SPU_MESH_CELLS + 16] __attribute__((aligned(128)));
	unsigned char s_light[SPU_MESH_LIGHT_CELLS] __attribute__((aligned(128)));
	unsigned short s_work[SPU_MESH_MAX_WORK] __attribute__((aligned(128)));
	SpuMeshTextureTable s_tex __attribute__((aligned(128)));
	unsigned int s_texVersion = 0xffffffffu;
	// Packed light per cell, computed on first use (validity by job stamp).
	int s_packed[SPU_MESH_CELLS] __attribute__((aligned(16)));
	unsigned short s_packedStamp[SPU_MESH_CELLS] __attribute__((aligned(16)));
	unsigned short s_stamp = 0;

	// Output staging: 512 vertices (16 KB), flushed to main memory when full.
	const unsigned int kOutVerts = 512;
	unsigned int s_out[kOutVerts * SPU_MESH_VERTEX_INTS] __attribute__((aligned(128)));
	unsigned int s_outCount = 0;      // vertices in s_out
	unsigned int s_outFlushed = 0;    // vertices already in main memory
	bool s_overflow = false;
	unsigned int s_faces = 0;

	inline int cellIndex(int x, int y, int z)
	{
		if (x < 0) x = 0; else if (x > SPU_MESH_SIZE - 1) x = SPU_MESH_SIZE - 1;
		if (y < 0) y = 0; else if (y > SPU_MESH_SIZE - 1) y = SPU_MESH_SIZE - 1;
		if (z < 0) z = 0; else if (z > SPU_MESH_SIZE - 1) z = SPU_MESH_SIZE - 1;
		return (y * SPU_MESH_SIZE + z) * SPU_MESH_SIZE + x;
	}

	inline int idAt(int x, int y, int z)
	{
		const int id = s_ids[cellIndex(x, y, z)];
		return id < SPU_MESH_IDS ? id : 0;
	}

	inline unsigned int flagsAt(int x, int y, int z)
	{
		return s_in.flags[idAt(x, y, z)];
	}

	// Cache coordinates (0..17) to world Y and back: the PPU clamps world Y
	// to 0..255 before reading light (PcLegacySectionCache::snapshotSavedLight).
	inline int clampWorldY(int y)
	{
		const int worldY = s_in.minY + y;
		if (worldY < 0)
			return -s_in.minY;
		if (worldY > 255)
			return 255 - s_in.minY;
		return y;
	}

	inline int savedLight(bool sky, int x, int y, int z)
	{
		y = clampWorldY(y);
		int lx = x + 1, ly = y + 1, lz = z + 1;
		if (lx < 0) lx = 0; else if (lx > SPU_MESH_LIGHT_SIZE - 1) lx = SPU_MESH_LIGHT_SIZE - 1;
		if (ly < 0) ly = 0; else if (ly > SPU_MESH_LIGHT_SIZE - 1) ly = SPU_MESH_LIGHT_SIZE - 1;
		if (lz < 0) lz = 0; else if (lz > SPU_MESH_LIGHT_SIZE - 1) lz = SPU_MESH_LIGHT_SIZE - 1;
		const unsigned int v = s_light[(ly * SPU_MESH_LIGHT_SIZE + lz) * SPU_MESH_LIGHT_SIZE + lx];
		return sky ? (int)(v >> 4) : (int)(v & 15);
	}

	// PcLegacySectionCache::snapshotSkyBlockTypeBrightness.
	inline int skyBlockTypeBrightness(bool sky, int x, int y, int z)
	{
		y = clampWorldY(y);
		if (flagsAt(x, y, z) & SPU_MESH_FLAG_NEIGHBOR_LIGHT)
		{
			int brightness = savedLight(sky, x, y + 1, z);
			const int east = savedLight(sky, x + 1, y, z);
			const int west = savedLight(sky, x - 1, y, z);
			const int south = savedLight(sky, x, y, z + 1);
			const int north = savedLight(sky, x, y, z - 1);
			if (east > brightness) brightness = east;
			if (west > brightness) brightness = west;
			if (south > brightness) brightness = south;
			if (north > brightness) brightness = north;
			return brightness;
		}
		return savedLight(sky, x, y, z);
	}

	// Block::getMixedBrightnessForBlock for a block with lightValue 0.
	inline int packedLight(int x, int y, int z)
	{
		const int cell = cellIndex(x, y, z);
		if (s_packedStamp[cell] != s_stamp)
		{
			s_packed[cell] = (skyBlockTypeBrightness(true, x, y, z) << 20) |
				(skyBlockTypeBrightness(false, x, y, z) << 4);
			s_packedStamp[cell] = s_stamp;
		}
		return s_packed[cell];
	}

	inline int aoBright(int a, int b, int c, int base)
	{
		if (a == 0) a = base;
		if (b == 0) b = base;
		if (c == 0) c = base;
		return ((a + b + c + base) >> 2) & 0x00ff00ff;
	}

	// ---- output -------------------------------------------------------------
	void flushOutput()
	{
		if (s_outCount == 0)
			return;
		DmaData_SPU::putAndWait(s_out, (uintptr_t)(s_in.eaOutput + (unsigned long long)s_outFlushed * 32u),
			s_outCount * 32u);
		s_outFlushed += s_outCount;
		s_outCount = 0;
	}

	inline unsigned int floatBits(float f)
	{
		union { float f; unsigned int u; } c;
		c.f = f;
		return c.u;
	}

	// Tessellator::setColorOpaque_F + setColorRGBA (big-endian RGBA).
	inline unsigned int colorByte(const sf &c)
	{
		int v = (int)roundToFloat((double)c.v * 255.0);
		if (v > 255) v = 255;
		if (v < 0) v = 0;
		return (unsigned int)v;
	}

	struct FaceState
	{
		sf r[4];
		sf g[4];
		sf b[4];
		int br[4];
	};

	// Corner order of every face: top-left, bottom-left, bottom-right, top-right.
	struct Corner
	{
		unsigned char x, y, z, u, v;   // u, v: 0 = low edge, 1 = high edge
	};
	const Corner kFaceCorners[6][4] = {
		{ {0,0,1, 0,1}, {0,0,0, 0,0}, {1,0,0, 1,0}, {1,0,1, 1,1} },   // 0 bottom
		{ {1,1,1, 1,1}, {1,1,0, 1,0}, {0,1,0, 0,0}, {0,1,1, 0,1} },   // 1 top
		{ {0,1,0, 1,0}, {1,1,0, 0,0}, {1,0,0, 0,1}, {0,0,0, 1,1} },   // 2 east (z-)
		{ {0,1,1, 0,0}, {0,0,1, 0,1}, {1,0,1, 1,1}, {1,1,1, 1,0} },   // 3 west (z+)
		{ {0,1,1, 1,0}, {0,1,0, 0,0}, {0,0,0, 0,1}, {0,0,1, 1,1} },   // 4 north (x-)
		{ {1,0,1, 0,1}, {1,0,0, 1,1}, {1,1,0, 1,0}, {1,1,1, 0,0} },   // 5 south (x+)
	};

	// RenderBlocks::render*Face for a unit cube, AO branch, no rotation/flip.
	void emitFace(int side, int tile, int ci, int cj, int ck, const FaceState &s)
	{
		if (s_overflow)
			return;
		if (s_outFlushed + s_outCount + 4 > s_in.outputCapacity)
		{
			s_overflow = true;
			return;
		}
		const int tu = (tile & 0xf) << 4;
		const int tv = tile & 0xf0;
		// ((j + min*16) / 256) and ((j + max*16) - 0.01) / 256, in double.
		const unsigned int u0 = floatBits(roundToFloat((double)tu / 256.0));
		const unsigned int u1 = floatBits(roundToFloat(((double)(tu + 16) - 0.01) / 256.0));
		const unsigned int v0 = floatBits(roundToFloat((double)tv / 256.0));
		const unsigned int v1 = floatBits(roundToFloat(((double)(tv + 16) - 0.01) / 256.0));
		// Section-local position: the worker's translation is minus the origin.
		const int x = ci - 1, y = cj - 1, z = ck - 1;

		for (int c = 0; c < 4; ++c)
		{
			const Corner &corner = kFaceCorners[side][c];
			unsigned int *dst = &s_out[s_outCount * SPU_MESH_VERTEX_INTS];
			dst[0] = floatBits((float)(x + corner.x));
			dst[1] = floatBits((float)(y + corner.y));
			dst[2] = floatBits((float)(z + corner.z));
			dst[3] = corner.u ? u1 : u0;
			dst[4] = corner.v ? v1 : v0;
			dst[5] = (colorByte(s.r[c]) << 24) | (colorByte(s.g[c]) << 16) | (colorByte(s.b[c]) << 8) | 0xffu;
			dst[6] = 0;
			dst[7] = (unsigned int)s.br[c];
			++s_outCount;
		}
		++s_faces;
		if (s_outCount == kOutVerts)
			flushOutput();
	}

	// ---- the block ----------------------------------------------------------
	int textureFor(int kind, int id, int meta, int ci, int cj, int ck, int side)
	{
		if (kind == SPU_MESH_KIND_GRASS)
		{
			// BlockGrass::getBlockTexture.
			if (side == 1) return 0;
			if (side == 0) return 2;
			return (flagsAt(ci, cj + 1, ck) & SPU_MESH_FLAG_SNOW) ? 68 : 3;
		}
		return s_tex.tile[(id * 16 + (meta & 15)) * 6 + side];
	}

	#define AOB(x, y, z) ((flagsAt((x), (y), (z)) & SPU_MESH_FLAG_AO_DARK) ? aoOpaque : sf(1.0f))
	#define AOP(x, y, z) packedLight((x), (y), (z))
	#define CBG(x, y, z) ((flagsAt((x), (y), (z)) & SPU_MESH_FLAG_BLOCKS_GRASS) != 0)
	#define SIDEVIS(x, y, z) ((flagsAt((x), (y), (z)) & SPU_MESH_FLAG_OPAQUE) == 0)
	#define TEX(side) textureFor(kind, id, meta, aoBaseI, aoBaseJ, aoBaseK, (side))
	#define EMIT(side, tile) emitFace((side), (tile), aoBaseI, aoBaseJ, aoBaseK, st)
	#define colorRedTopLeft st.r[0]
	#define colorRedBottomLeft st.r[1]
	#define colorRedBottomRight st.r[2]
	#define colorRedTopRight st.r[3]
	#define colorGreenTopLeft st.g[0]
	#define colorGreenBottomLeft st.g[1]
	#define colorGreenBottomRight st.g[2]
	#define colorGreenTopRight st.g[3]
	#define colorBlueTopLeft st.b[0]
	#define colorBlueBottomLeft st.b[1]
	#define colorBlueBottomRight st.b[2]
	#define colorBlueTopRight st.b[3]
	#define brightnessTopLeft st.br[0]
	#define brightnessBottomLeft st.br[1]
	#define brightnessBottomRight st.br[2]
	#define brightnessTopRight st.br[3]

	// RenderBlocks::renderStandardBlockWithAmbientOcclusion for a full cube.
	// (i, j, k) are cache coordinates (1..16 for the section's own blocks).
	bool spuMeshAoCube(int kind, int id, int meta, int i, int j, int k, const sf &f, const sf &f1, const sf &f2)
	{
		const int aoBaseI = i;
		const int aoBaseJ = j;
		const int aoBaseK = k;
		const sf aoOpaque(s_in.aoLightValueOpaque);
		const int lq = s_in.lightingQuality;
		const bool fancyGrass = s_in.fancyGrass != 0;
		const bool isGrass = kind == SPU_MESH_KIND_GRASS;
		FaceState st;
#include "SpuAoCube.inl"
		return flag;
	}

	#undef AOB
	#undef AOP
	#undef CBG
	#undef SIDEVIS
	#undef TEX
	#undef EMIT
}

bool ChunkMeshJob_run(unsigned long long eaInput)
{
	DmaData_SPU::getAndWait(&s_in, (uintptr_t)eaInput, sizeof(s_in));
	if (s_in.workCount > SPU_MESH_MAX_WORK)
		return false;

	if (s_texVersion != s_in.texturesVersion)
	{
		DmaData_SPU::getAndWait(&s_tex, (uintptr_t)s_in.eaTextures, sizeof(s_tex));
		s_texVersion = s_in.texturesVersion;
	}
	DmaData_SPU::get(s_ids, (uintptr_t)s_in.eaBlockIds, sizeof(s_ids));
	DmaData_SPU::get(s_meta, (uintptr_t)s_in.eaMetadata, DmaData_SPU::roundUpDMASize(SPU_MESH_CELLS));
	DmaData_SPU::get(s_light, (uintptr_t)s_in.eaSavedLight, sizeof(s_light));
	if (s_in.workCount > 0)
		DmaData_SPU::get(s_work, (uintptr_t)s_in.eaWorklist, DmaData_SPU::roundUpDMASize(s_in.workCount * 2));
	DmaData_SPU::wait();

	if (++s_stamp == 0)
	{
		for (int c = 0; c < SPU_MESH_CELLS; ++c)
			s_packedStamp[c] = 0;
		s_stamp = 1;
	}
	s_outCount = 0;
	s_outFlushed = 0;
	s_overflow = false;
	s_faces = 0;
	unsigned int blocks = 0;

	for (unsigned int w = 0; w < s_in.workCount && !s_overflow; ++w)
	{
		const unsigned int cursor = s_work[w];
		const int i = (int)(cursor & 15) + 1;
		const int k = (int)((cursor >> 4) & 15) + 1;
		const int j = (int)((cursor >> 8) & 15) + 1;
		const int cell = (j * SPU_MESH_SIZE + k) * SPU_MESH_SIZE + i;
		const int id = s_ids[cell];
		if (id <= 0 || id >= SPU_MESH_IDS)
			continue;
		const int kind = s_in.kind[id];
		if (kind == SPU_MESH_KIND_NONE)
			continue;
		// The worker only renders a simple cube with an exposed face.
		if ((flagsAt(i, j - 1, k) & flagsAt(i, j + 1, k) & flagsAt(i, j, k - 1) &
			 flagsAt(i, j, k + 1) & flagsAt(i - 1, j, k) & flagsAt(i + 1, j, k) & SPU_MESH_FLAG_OPAQUE) != 0)
			continue;

		// CustomColorizer::getColorMultiplier -> 0xRRGGBB, then / 255.0f.
		const int color = kind == SPU_MESH_KIND_GRASS ? s_in.grassColor[(k - 1) * 16 + (i - 1)] : 0xffffff;
		const sf f = sf((float)(color >> 16 & 0xff)) / sf(255.0f);
		const sf f1 = sf((float)(color >> 8 & 0xff)) / sf(255.0f);
		const sf f2 = sf((float)(color & 0xff)) / sf(255.0f);
		spuMeshAoCube(kind, id, s_meta[cell], i, j, k, f, f1, f2);
		++blocks;
	}
	if (!s_overflow)
		flushOutput();

	s_in.resultVertices = s_overflow ? 0u : s_outFlushed;
	s_in.resultStatus = s_overflow ? SPU_MESH_STATUS_OVERFLOW : SPU_MESH_STATUS_OK;
	s_in.resultBlocks = blocks;
	s_in.resultFaces = s_faces;
	DmaData_SPU::putAndWait(&s_in.resultVertices, (uintptr_t)(eaInput + SPU_MESH_RESULT_OFFSET), 16);
	return true;
}
