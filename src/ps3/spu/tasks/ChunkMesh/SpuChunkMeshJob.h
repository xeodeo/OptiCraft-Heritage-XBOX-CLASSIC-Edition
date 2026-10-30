#pragma once

// Chunk-section meshing on the SPUs (4J's ChunkUpdate task, OptiCraft flavour).
// Shared between the PPU (src/ps3/spu/Ps3SpuChunkMesh.cpp, gcc 13) and the SPU
// (ChunkMeshJob.cpp, Sony spu-lv2-gcc 4.1, C++03): fixed-width fields and 64-bit
// effective addresses only, no pointers.
//
// The PPU mesh worker hands the SPU the section snapshot it already holds
// (PcLegacySectionCache: 18^3 block ids and metadata, 20^3 saved light, the
// pass-0 work list) plus per-block-id tables. The SPU meshes the blocks whose
// id has an SPU kind -- plain AO cubes and grass, the bulk of the terrain -- in
// work-list order, writing the Tessellator's own 32-byte quad vertices to
// eaOutput. Everything else (kind NONE: 4J's e_flag_SPURenderCodeMissing) is
// left to the PPU worker.

#define PS3_SPU_JOB_CHUNK_MESH        2u

#define SPU_MESH_SIZE                 18
#define SPU_MESH_CELLS                (18 * 18 * 18)
#define SPU_MESH_LIGHT_SIZE           20
#define SPU_MESH_LIGHT_CELLS          (20 * 20 * 20)
#define SPU_MESH_IDS                  256
#define SPU_MESH_MAX_WORK             4096
// Ints per output vertex: x y z u v (float bits), colour, normal, brightness.
#define SPU_MESH_VERTEX_INTS          8

// Per block id: what the SPU does with it.
#define SPU_MESH_KIND_NONE            0   // PPU renders it (no SPU code)
#define SPU_MESH_KIND_CUBE            1   // AO cube, texture from the table
#define SPU_MESH_KIND_GRASS           2   // BlockGrass: snow side, fancy overlay 38

// Per block id flags.
#define SPU_MESH_FLAG_OPAQUE          0x01  // isBlockOpaqueCube
#define SPU_MESH_FLAG_AO_DARK         0x02  // AO: isNormalCube and not glass
#define SPU_MESH_FLAG_BLOCKS_GRASS    0x04  // Block::canBlockGrass
#define SPU_MESH_FLAG_NEIGHBOR_LIGHT  0x08  // Block::useNeighborBrightness
#define SPU_MESH_FLAG_SNOW            0x10  // material snow / builtSnow

#define SPU_MESH_STATUS_OK            0u
#define SPU_MESH_STATUS_OVERFLOW      1u    // output full: the PPU meshes those blocks

// [id][metadata][side] terrain tile; DMA'd once per version and kept in LS.
struct SpuMeshTextureTable
{
	unsigned char tile[SPU_MESH_IDS * 16 * 6];
};

struct SpuChunkMeshInput
{
	unsigned long long eaBlockIds;    // u16[SPU_MESH_CELLS], (y*18+z)*18+x
	unsigned long long eaMetadata;    // u8[SPU_MESH_CELLS] (read 5840 bytes)
	unsigned long long eaSavedLight;  // u8[SPU_MESH_LIGHT_CELLS], sky<<4 | block
	unsigned long long eaWorklist;    // u16[workCount], y<<8 | z<<4 | x
	unsigned long long eaTextures;    // SpuMeshTextureTable
	unsigned long long eaOutput;      // outputCapacity * 32 bytes
	unsigned int workCount;
	unsigned int outputCapacity;      // vertices
	unsigned int texturesVersion;
	int minY;                         // world Y of cache cell 0
	float aoLightValueOpaque;         // RenderBlocks::aoLightValueOpaque
	int lightingQuality;
	int fancyGrass;
	unsigned int pad0;
	// Results, written back by the SPU (16 bytes at offset 80).
	unsigned int resultVertices;
	unsigned int resultStatus;
	unsigned int resultBlocks;
	unsigned int resultFaces;
	unsigned char kind[SPU_MESH_IDS];
	unsigned char flags[SPU_MESH_IDS];
	int grassColor[16 * 16];          // central columns, z*16+x, 0xRRGGBB
};

#define SPU_MESH_RESULT_OFFSET 80

typedef char spu_chunk_mesh_input_size[(sizeof(SpuChunkMeshInput) == 1632) ? 1 : -1];
typedef char spu_mesh_texture_table_size[(sizeof(SpuMeshTextureTable) == 24576) ? 1 : -1];
