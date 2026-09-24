#include "Tessellator.h"

#include <algorithm>
#include <utility>
#include "java/Arithmetic.h"

#include "platform/Log.h"
#include "platform/PlatformTuning.h"
#include <cstring>
#include <stdexcept>

#include "platform/RenderAPI.h"

static int32_t floatToRawIntBits(float f)
{
	int32_t result;
	std::memcpy(&result, &f, sizeof(result));
	return result;
}

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD || defined(PS2_PLATFORM)
static inline void writeFastTerrainVertex(int_t *dst, int_t xBits, int_t yBits, int_t zBits,
	int_t uBits, int_t vBits, int_t color, int_t brightness)
{
	dst[0] = xBits;
	dst[1] = yBits;
	dst[2] = zBits;
	dst[3] = uBits;
	dst[4] = vBits;
	dst[5] = color;
	dst[7] = brightness;
}
#endif

// Endianness is a property of the target, not of the run, but this used to build
// a union and read it back on every setColorRGBA call -- i.e. once per colour
// change inside the chunk meshing loop, several hundred thousand times per
// section build. Resolve it at compile time.


static constexpr bool isLittleEndian()
{
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
	return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || defined(_EE)
	return true;
#else
#error "Unknown byte order; Tessellator colour packing needs it explicitly."
#endif
}

bool Tessellator::convertQuadsToTriangles = PLATFORM_TESSELLATOR_CONVERT_QUADS != 0;

Tessellator Tessellator::instance(PLATFORM_TESSELLATOR_BUFFER_INTS);

Tessellator::Tessellator(int bufferSize) :
	rawBuffer(new int_t[bufferSize]()),
	bufferSize(bufferSize),
	vertexCount(0),
	textureU(0),
	textureV(0),
	brightness(0),
	color(0),
	hasColor(false),
	hasTexture(false),
	hasBrightness(false),
	hasNormals(false),
	rawBufferIndex(0),
	addedVertices(0),
	isColorDisabled(false),
	drawMode(0),
	xOffset(0),
	yOffset(0),
	zOffset(0),
	normal(0),
	isDrawing(false)
{
}

Tessellator::~Tessellator()
{
	delete[] rawBuffer;
}

void Tessellator::ensureRawBufferCapacity(int_t additionalInts)
{
	if (additionalInts <= 0 || rawBufferIndex + additionalInts <= bufferSize)
		return;

	int_t newSize = bufferSize > 0 ? bufferSize : 32;
	const int_t requiredSize = rawBufferIndex + additionalInts;
	while (newSize < requiredSize)
		newSize *= 2;

	int_t *newBuffer = new int_t[newSize]();
	if (rawBufferIndex > 0)
		std::memcpy(newBuffer, rawBuffer, (size_t)rawBufferIndex * sizeof(int_t));
	delete[] rawBuffer;
	rawBuffer = newBuffer;
	bufferSize = newSize;
}

int_t Tessellator::draw()
{
	if (!isDrawing)
	{
		throw std::runtime_error("Not tesselating!");
	}
	isDrawing = false;

	if (vertexCount > 0)
	{
		RenderInterleavedMesh mesh;
		mesh.data = rawBuffer;
		mesh.stride = 32;
		mesh.first = 0;
		mesh.count = vertexCount;
		mesh.primitive = static_cast<RenderPrimitive>((drawMode == 7 && convertQuadsToTriangles) ? 4 : drawMode);
		mesh.hasTexture = hasTexture;
		mesh.texCoordOffset = 12;
		mesh.hasColor = hasColor;
		mesh.colorOffset = 20;
		mesh.hasNormals = hasNormals;
		mesh.normalOffset = 24;
		mesh.hasBrightness = hasBrightness;
		mesh.brightnessOffset = 28;
		(void)renderDrawInterleaved(mesh);
	}

	const int_t bytesDrawn = JavaArithmetic::intMul(rawBufferIndex, 4);
	reset();
	return bytesDrawn;
}



static RenderInterleavedMesh makeTessellatorMesh(const int_t* rawBuffer, int_t vertexCount,
                                                  int_t drawMode, bool convertQuads,
                                                  bool hasTexture, bool hasColor, bool hasNormals,
                                                  bool hasBrightness)
{
    RenderInterleavedMesh mesh;
    mesh.data = rawBuffer;
    mesh.stride = 32;
    mesh.first = 0;
    mesh.count = vertexCount;
    mesh.primitive = static_cast<RenderPrimitive>((drawMode == 7 && convertQuads) ? 4 : drawMode);
    mesh.hasTexture = hasTexture;
    mesh.texCoordOffset = 12;
    mesh.hasColor = hasColor;
    mesh.colorOffset = 20;
    mesh.hasNormals = hasNormals;
    mesh.normalOffset = 24;
    mesh.hasBrightness = hasBrightness;
    mesh.brightnessOffset = 28;
    return mesh;
}

bool Tessellator::capture(RenderCapturedMesh& out, bool append)
{
    // capture() is a terminal operation, just like draw(): once a batch has
    // been started, calling capture must always consume it.  Incremental
    // terrain builders legitimately produce empty steps (air, hidden faces or
    // blocks belonging to the other render pass).  Leaving isDrawing set in
    // that case makes the next step fail with "Already tessellating!".
    if (!isDrawing)
        return false;

    bool captured = false;
    if (vertexCount > 0)
    {
        const RenderInterleavedMesh mesh = makeTessellatorMesh(rawBuffer, vertexCount, drawMode,
            convertQuadsToTriangles, hasTexture, hasColor, hasNormals, hasBrightness);
        captured = renderCaptureInterleaved(mesh, out, append);
    }

    isDrawing = false;
    reset();
    return captured;
}

#if PLATFORM_PERSISTENT_RENDER_MESH || PLATFORM_MODEL_PERSISTENT_MESH
bool Tessellator::finishPersistentMesh(int handle)
{
    if (!isDrawing || vertexCount <= 0)
        return false;
    const RenderInterleavedMesh mesh = makeTessellatorMesh(rawBuffer, vertexCount, drawMode,
        convertQuadsToTriangles, hasTexture, hasColor, hasNormals, hasBrightness);
    if (!renderCompilePersistentMesh(handle, mesh))
        return false;
    isDrawing = false;
    reset();
    return true;
}
#endif

bool Tessellator::finishStaticMesh(RenderStaticMesh& target)
{
    if (!isDrawing)
        return false;

    bool compiled = false;
    if (vertexCount > 0)
    {
        const RenderInterleavedMesh mesh = makeTessellatorMesh(rawBuffer, vertexCount, drawMode,
            convertQuadsToTriangles, hasTexture, hasColor, hasNormals, hasBrightness);
        compiled = renderStaticMeshCompile(target, mesh);
    }

    isDrawing = false;
    reset();
    return compiled;
}


void Tessellator::cancelDrawing()
{
    if (this == &instance)
    {
        for (auto &entry : subTessellators)
            if (entry.second != nullptr && entry.second->isDrawing)
                entry.second->cancelDrawing();
    }
    if (!isDrawing)
        return;
    isDrawing = false;
    reset();
}

Tessellator *Tessellator::getSubTessellator(int_t textureId)
{
    if (textureId <= 0)
        return this;

    std::unique_ptr<Tessellator> &slot = subTessellators[textureId];
    if (!slot)
        slot.reset(new Tessellator(bufferSize));
    Tessellator *sub = slot.get();
    if (!sub->isDrawing)
        sub->startDrawing(drawMode);

    // Copy the current per-vertex state without clearing vertices already emitted
    // to this texture group during the same terrain batch.
    sub->textureU = textureU;
    sub->textureV = textureV;
    sub->brightness = brightness;
    sub->color = color;
    sub->hasColor = hasColor;
    sub->hasTexture = hasTexture;
    sub->hasBrightness = hasBrightness;
    sub->hasNormals = hasNormals;
    sub->normal = normal;
    sub->isColorDisabled = isColorDisabled;
    sub->xOffset = xOffset;
    sub->yOffset = yOffset;
    sub->zOffset = zOffset;
    return sub;
}

bool Tessellator::captureTextureGroups(std::vector<TessellatorTextureMesh> &out, bool append)
{
    if (!append)
        out.clear();

    bool success = true;
    for (auto &entry : subTessellators)
    {
        Tessellator *sub = entry.second.get();
        if (sub == nullptr || !sub->isDrawing)
            continue;

        const bool hadVertices = sub->hasPendingVertices();
        auto existing = out.end();
        if (append)
        {
            existing = std::find_if(out.begin(), out.end(), [&](const TessellatorTextureMesh &group) {
                return group.textureId == entry.first;
            });
        }

        bool captured = false;
        if (existing != out.end())
        {
            captured = sub->capture(existing->mesh, true);
        }
        else
        {
            TessellatorTextureMesh group;
            group.textureId = entry.first;
            captured = sub->capture(group.mesh);
            if (captured)
                out.push_back(std::move(group));
        }

        if (hadVertices && !captured)
            success = false;
    }
    return success;
}

void Tessellator::reset()
{
	vertexCount = 0;
	rawBufferIndex = 0;
	addedVertices = 0;
}

void Tessellator::startDrawingQuads()
{
	startDrawing(7);
}

void Tessellator::startDrawing(int i)
{
    if (this == &instance)
    {
        for (auto &entry : subTessellators)
            if (entry.second != nullptr && entry.second->isDrawing)
                entry.second->cancelDrawing();
    }
	if (isDrawing)
	{
		throw std::runtime_error("Already tesselating!");
	}
	else
	{
		isDrawing = true;
		reset();
		drawMode = i;
		hasNormals = false;
		hasColor = false;
		hasTexture = false;
		hasBrightness = false;
		isColorDisabled = false;
	}
}

void Tessellator::setTextureUV(tess_coord_t d, tess_coord_t d1)
{
	hasTexture = true;
	textureU = d;
	textureV = d1;
}

void Tessellator::setBrightness(int value)
{
	hasBrightness = true;
	brightness = value;
}

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD
int_t Tessellator::packOpaqueColorLegacy(int_t red, int_t green, int_t blue)
{
	red = std::max<int_t>(0, std::min<int_t>(255, red));
	green = std::max<int_t>(0, std::min<int_t>(255, green));
	blue = std::max<int_t>(0, std::min<int_t>(255, blue));
	const uint_t redBits = static_cast<uint_t>(red);
	const uint_t greenBits = static_cast<uint_t>(green);
	const uint_t blueBits = static_cast<uint_t>(blue);
	if (isLittleEndian())
		return JavaArithmetic::intFromBits(0xff000000u | (blueBits << 16) | (greenBits << 8) | redBits);
	return JavaArithmetic::intFromBits((redBits << 24) | (greenBits << 16) | (blueBits << 8) | 0xffu);
}

bool Tessellator::setPackedFaceStateLegacy(int_t packedColor, int_t packedBrightness)
{
	if (isColorDisabled)
		return false;
	hasColor = true;
	hasBrightness = true;
	color = packedColor;
	brightness = packedBrightness;
	return true;
}
#endif

void Tessellator::setColorOpaque_F(float f, float f1, float f2)
{
	setColorOpaque((int)(f * 255.0f), (int)(f1 * 255.0f), (int)(f2 * 255.0f));
}

void Tessellator::setColorRGBA_F(float f, float f1, float f2, float f3)
{
	setColorRGBA((int)(f * 255.0f), (int)(f1 * 255.0f), (int)(f2 * 255.0f), (int)(f3 * 255.0f));
}

void Tessellator::setColorOpaque(int i, int j, int k)
{
	setColorRGBA(i, j, k, 255);
}

void Tessellator::setColorRGBA(int i, int j, int k, int l)
{
	if (isColorDisabled)
	{
		return;
	}
	if (i > 255)
	{
		i = 255;
	}
	if (j > 255)
	{
		j = 255;
	}
	if (k > 255)
	{
		k = 255;
	}
	if (l > 255)
	{
		l = 255;
	}
	if (i < 0)
	{
		i = 0;
	}
	if (j < 0)
	{
		j = 0;
	}
	if (k < 0)
	{
		k = 0;
	}
	if (l < 0)
	{
		l = 0;
	}
	hasColor = true;
	const uint_t red = static_cast<uint_t>(i);
	const uint_t green = static_cast<uint_t>(j);
	const uint_t blue = static_cast<uint_t>(k);
	const uint_t alpha = static_cast<uint_t>(l);
	if (isLittleEndian())
	{
		color = JavaArithmetic::intFromBits((static_cast<uint_t>(alpha) << 24) | (static_cast<uint_t>(blue) << 16) | (static_cast<uint_t>(green) << 8) | static_cast<uint_t>(red));
	}
	else
	{
		color = JavaArithmetic::intFromBits((static_cast<uint_t>(red) << 24) | (static_cast<uint_t>(green) << 16) | (static_cast<uint_t>(blue) << 8) | static_cast<uint_t>(alpha));
	}
}

void Tessellator::addVertexWithUV(tess_coord_t d, tess_coord_t d1, tess_coord_t d2, tess_coord_t d3, tess_coord_t d4)
{
	setTextureUV(d3, d4);
	addVertex(d, d1, d2);
}

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD || defined(PS2_PLATFORM)
bool Tessellator::addAxisAlignedFaceWithUVFast(int_t side, tess_coord_t x, tess_coord_t y, tess_coord_t z,
	tess_coord_t width, tess_coord_t height,
	tess_coord_t u0, tess_coord_t u1, tess_coord_t v0, tess_coord_t v1)
{
	if (!isDrawing || drawMode != 7 || hasNormals || !hasColor || !hasBrightness ||
		side < 0 || side > 5 || width <= (tess_coord_t)0.0f || height <= (tess_coord_t)0.0f)
		return false;

	const bool expandQuads = convertQuadsToTriangles;
	ensureRawBufferCapacity(expandQuads ? 48 : 32);
	hasTexture = true;

	const tess_coord_t x1 = x + ((side <= 3) ? width : (tess_coord_t)1.0f);
	const tess_coord_t y1 = y + ((side >= 2) ? height : (tess_coord_t)1.0f);
	tess_coord_t z1 = z + (tess_coord_t)1.0f;
	if (side <= 1)
		z1 = z + height;
	else if (side >= 4)
		z1 = z + width;

	const int_t x0Bits = floatToRawIntBits((float)(x + xOffset));
	const int_t x1Bits = floatToRawIntBits((float)(x1 + xOffset));
	const int_t y0Bits = floatToRawIntBits((float)(y + yOffset));
	const int_t y1Bits = floatToRawIntBits((float)(y1 + yOffset));
	const int_t z0Bits = floatToRawIntBits((float)(z + zOffset));
	const int_t z1Bits = floatToRawIntBits((float)(z1 + zOffset));
	const int_t u0Bits = floatToRawIntBits((float)u0);
	const int_t u1Bits = floatToRawIntBits((float)u1);
	const int_t v0Bits = floatToRawIntBits((float)v0);
	const int_t v1Bits = floatToRawIntBits((float)v1);

	struct FastVertex
	{
		int_t x;
		int_t y;
		int_t z;
		int_t u;
		int_t v;
	};

	int_t *dst = rawBuffer + rawBufferIndex;
	auto writeVertex = [&](const FastVertex &vertex)
	{
		writeFastTerrainVertex(dst, vertex.x, vertex.y, vertex.z, vertex.u, vertex.v, color, brightness);
		dst += 8;
	};
	auto writeQuad = [&](const FastVertex &a, const FastVertex &b, const FastVertex &c, const FastVertex &d)
	{
		writeVertex(a);
		writeVertex(b);
		writeVertex(c);
		if (expandQuads)
		{
			writeVertex(a);
			writeVertex(c);
		}
		writeVertex(d);
	};

	switch (side)
	{
	case 0:
		writeQuad({x0Bits, y0Bits, z1Bits, u0Bits, v1Bits},
			{x0Bits, y0Bits, z0Bits, u0Bits, v0Bits},
			{x1Bits, y0Bits, z0Bits, u1Bits, v0Bits},
			{x1Bits, y0Bits, z1Bits, u1Bits, v1Bits});
		break;
	case 1:
		writeQuad({x1Bits, y1Bits, z1Bits, u1Bits, v1Bits},
			{x1Bits, y1Bits, z0Bits, u1Bits, v0Bits},
			{x0Bits, y1Bits, z0Bits, u0Bits, v0Bits},
			{x0Bits, y1Bits, z1Bits, u0Bits, v1Bits});
		break;
	case 2:
		writeQuad({x0Bits, y1Bits, z0Bits, u1Bits, v0Bits},
			{x1Bits, y1Bits, z0Bits, u0Bits, v0Bits},
			{x1Bits, y0Bits, z0Bits, u0Bits, v1Bits},
			{x0Bits, y0Bits, z0Bits, u1Bits, v1Bits});
		break;
	case 3:
		writeQuad({x0Bits, y1Bits, z1Bits, u0Bits, v0Bits},
			{x0Bits, y0Bits, z1Bits, u0Bits, v1Bits},
			{x1Bits, y0Bits, z1Bits, u1Bits, v1Bits},
			{x1Bits, y1Bits, z1Bits, u1Bits, v0Bits});
		break;
	case 4:
		writeQuad({x0Bits, y1Bits, z1Bits, u1Bits, v0Bits},
			{x0Bits, y1Bits, z0Bits, u0Bits, v0Bits},
			{x0Bits, y0Bits, z0Bits, u0Bits, v1Bits},
			{x0Bits, y0Bits, z1Bits, u1Bits, v1Bits});
		break;
	case 5:
		writeQuad({x1Bits, y0Bits, z1Bits, u0Bits, v1Bits},
			{x1Bits, y0Bits, z0Bits, u1Bits, v1Bits},
			{x1Bits, y1Bits, z0Bits, u1Bits, v0Bits},
			{x1Bits, y1Bits, z1Bits, u0Bits, v0Bits});
		break;
	default:
		return false;
	}

	addedVertices += 4;
	vertexCount += expandQuads ? 6 : 4;
	rawBufferIndex += expandQuads ? 48 : 32;
	return true;
}

bool Tessellator::addAxisAlignedUnitFaceWithUVFast(int_t side, tess_coord_t x, tess_coord_t y, tess_coord_t z,
	tess_coord_t u0, tess_coord_t u1, tess_coord_t v0, tess_coord_t v1)
{
	return addAxisAlignedUnitFaceWithPackedUVFast(side, x, y, z,
		floatToRawIntBits((float)u0), floatToRawIntBits((float)u1),
		floatToRawIntBits((float)v0), floatToRawIntBits((float)v1));
}

bool Tessellator::addAxisAlignedUnitFaceWithPackedUVFast(int_t side, tess_coord_t x, tess_coord_t y, tess_coord_t z,
	int_t u0Bits, int_t u1Bits, int_t v0Bits, int_t v1Bits)
{
	// This path writes the exact physical layout produced by four generic quad
	// vertices. Other primitive/state combinations stay on the generic writer.
	if (!isDrawing || drawMode != 7 || hasNormals || !hasColor || !hasBrightness ||
		side < 0 || side > 5)
		return false;

	const bool expandQuads = convertQuadsToTriangles;
	ensureRawBufferCapacity(expandQuads ? 48 : 32);
	hasTexture = true;

	const int_t x0Bits = floatToRawIntBits((float)(x + xOffset));
	const int_t x1Bits = floatToRawIntBits((float)(x + (tess_coord_t)1.0f + xOffset));
	const int_t y0Bits = floatToRawIntBits((float)(y + yOffset));
	const int_t y1Bits = floatToRawIntBits((float)(y + (tess_coord_t)1.0f + yOffset));
	const int_t z0Bits = floatToRawIntBits((float)(z + zOffset));
	const int_t z1Bits = floatToRawIntBits((float)(z + (tess_coord_t)1.0f + zOffset));

	struct FastVertex
	{
		int_t x;
		int_t y;
		int_t z;
		int_t u;
		int_t v;
	};

	int_t *dst = rawBuffer + rawBufferIndex;
	auto writeVertex = [&](const FastVertex &vertex)
	{
		writeFastTerrainVertex(dst, vertex.x, vertex.y, vertex.z, vertex.u, vertex.v, color, brightness);
		dst += 8;
	};
	auto writeQuad = [&](const FastVertex &a, const FastVertex &b, const FastVertex &c, const FastVertex &d)
	{
		writeVertex(a);
		writeVertex(b);
		writeVertex(c);
		if (expandQuads)
		{
			writeVertex(a);
			writeVertex(c);
		}
		writeVertex(d);
	};

	switch (side)
	{
	case 0:
		writeQuad(
			{x0Bits, y0Bits, z1Bits, u0Bits, v1Bits},
			{x0Bits, y0Bits, z0Bits, u0Bits, v0Bits},
			{x1Bits, y0Bits, z0Bits, u1Bits, v0Bits},
			{x1Bits, y0Bits, z1Bits, u1Bits, v1Bits});
		break;
	case 1:
		writeQuad(
			{x1Bits, y1Bits, z1Bits, u1Bits, v1Bits},
			{x1Bits, y1Bits, z0Bits, u1Bits, v0Bits},
			{x0Bits, y1Bits, z0Bits, u0Bits, v0Bits},
			{x0Bits, y1Bits, z1Bits, u0Bits, v1Bits});
		break;
	case 2:
		writeQuad(
			{x0Bits, y1Bits, z0Bits, u1Bits, v0Bits},
			{x1Bits, y1Bits, z0Bits, u0Bits, v0Bits},
			{x1Bits, y0Bits, z0Bits, u0Bits, v1Bits},
			{x0Bits, y0Bits, z0Bits, u1Bits, v1Bits});
		break;
	case 3:
		writeQuad(
			{x0Bits, y1Bits, z1Bits, u0Bits, v0Bits},
			{x0Bits, y0Bits, z1Bits, u0Bits, v1Bits},
			{x1Bits, y0Bits, z1Bits, u1Bits, v1Bits},
			{x1Bits, y1Bits, z1Bits, u1Bits, v0Bits});
		break;
	case 4:
		writeQuad(
			{x0Bits, y1Bits, z1Bits, u1Bits, v0Bits},
			{x0Bits, y1Bits, z0Bits, u0Bits, v0Bits},
			{x0Bits, y0Bits, z0Bits, u0Bits, v1Bits},
			{x0Bits, y0Bits, z1Bits, u1Bits, v1Bits});
		break;
	case 5:
		writeQuad(
			{x1Bits, y0Bits, z1Bits, u0Bits, v1Bits},
			{x1Bits, y0Bits, z0Bits, u1Bits, v1Bits},
			{x1Bits, y1Bits, z0Bits, u1Bits, v0Bits},
			{x1Bits, y1Bits, z1Bits, u0Bits, v0Bits});
		break;
	default:
		return false;
	}

	addedVertices += 4;
	vertexCount += expandQuads ? 6 : 4;
	rawBufferIndex += expandQuads ? 48 : 32;
	return true;
}

#endif

void Tessellator::addVertex(tess_coord_t d, tess_coord_t d1, tess_coord_t d2)
{
	const bool expandsQuad = drawMode == 7 && convertQuadsToTriangles && (addedVertices + 1) % 4 == 0;
	ensureRawBufferCapacity(expandsQuad ? 24 : 8);
	addedVertices++;
	if (drawMode == 7 && convertQuadsToTriangles && addedVertices % 4 == 0)
	{
		for (int iter = 0; iter < 2; iter++)
		{
			int j = 8 * (3 - iter);
			if (hasTexture)
			{
				rawBuffer[rawBufferIndex + 3] = rawBuffer[(rawBufferIndex - j) + 3];
				rawBuffer[rawBufferIndex + 4] = rawBuffer[(rawBufferIndex - j) + 4];
			}
			if (hasColor)
			{
				rawBuffer[rawBufferIndex + 5] = rawBuffer[(rawBufferIndex - j) + 5];
			}
			if (hasBrightness)
			{
				rawBuffer[rawBufferIndex + 7] = rawBuffer[(rawBufferIndex - j) + 7];
			}
			rawBuffer[rawBufferIndex + 0] = rawBuffer[(rawBufferIndex - j) + 0];
			rawBuffer[rawBufferIndex + 1] = rawBuffer[(rawBufferIndex - j) + 1];
			rawBuffer[rawBufferIndex + 2] = rawBuffer[(rawBufferIndex - j) + 2];
			vertexCount++;
			rawBufferIndex += 8;
		}
	}
	if (hasTexture)
	{
		rawBuffer[rawBufferIndex + 3] = floatToRawIntBits((float)textureU);
		rawBuffer[rawBufferIndex + 4] = floatToRawIntBits((float)textureV);
	}
	if (hasColor)
	{
		rawBuffer[rawBufferIndex + 5] = color;
	}
	if (hasNormals)
	{
		rawBuffer[rawBufferIndex + 6] = normal;
	}
	if (hasBrightness)
	{
		rawBuffer[rawBufferIndex + 7] = brightness;
	}
	rawBuffer[rawBufferIndex + 0] = floatToRawIntBits((float)(d + xOffset));
	rawBuffer[rawBufferIndex + 1] = floatToRawIntBits((float)(d1 + yOffset));
	rawBuffer[rawBufferIndex + 2] = floatToRawIntBits((float)(d2 + zOffset));
	rawBufferIndex += 8;
	vertexCount++;
}

void Tessellator::setColorOpaque_I(int i)
{
	int j = i >> 16 & 0xFF;
	int k = i >> 8 & 0xFF;
	int l = i & 0xFF;
	setColorOpaque(j, k, l);
}

void Tessellator::setColorRGBA_I(int i, int j)
{
	int k = i >> 16 & 0xFF;
	int l = i >> 8 & 0xFF;
	int i1 = i & 0xFF;
	setColorRGBA(k, l, i1, j);
}

void Tessellator::disableColor()
{
	isColorDisabled = true;
}

void Tessellator::setNormal(float f, float f1, float f2)
{
	if (!isDrawing)
	{
		MC_LOG_WARN("render", "Tessellator::setNormal called outside begin/end");
	}
	hasNormals = true;
	// The & 0xFF masks are vanilla's (`byte0 & 0xff | (byte1 & 0xff) << 8 | ...`)
	// and were dropped in the port. byte_t is int8_t, so without them a negative
	// component sign-extends to 0xFFFFFFxx and ORs 0xFF over the other two lanes:
	// a negative X normal could sign-extend and overwrite the Y/Z lanes.
	// Unsigned so the big-endian branch below can shift into bit 31 without
	// signed overflow; Java's int is two's-complement wrap, C++'s is UB.
	uint_t byte0 = (uint_t)((int_t)(f * 127.0f) & 0xFF);
	uint_t byte1 = (uint_t)((int_t)(f1 * 127.0f) & 0xFF);
	uint_t byte2 = (uint_t)((int_t)(f2 * 127.0f) & 0xFF);
	// This int lands in slot 6 of the vertex and is consumed as three signed
	// bytes starting at that word's lowest address (normalOffset=24), so the
	// packing has to follow the target's byte order -- exactly the reason
	// setColorRGBA already branches on it.
	//
	//   little endian   byte 0 of the word is bits 0-7    -> x in the low byte
	//   big endian      byte 0 of the word is bits 24-31  -> x in the high byte
	//
	// Vanilla only ever emitted the little-endian form. On the Broadway that was
	// read back as (high byte, z, y): X was dropped, Y took Z's value and Z took
	// Y's. Every entity normal on the Wii was wrong before wiigx ever saw it,
	// which is why fixing the backend normal reader and normalization changed
	// nothing visible -- both were correct, and both operated on garbage.
	if (isLittleEndian())
	{
		normal = (int_t)(byte0 | byte1 << 8 | byte2 << 16);
	}
	else
	{
		normal = JavaArithmetic::intFromBits((static_cast<uint_t>(static_cast<ubyte_t>(byte0)) << 24) | (static_cast<uint_t>(static_cast<ubyte_t>(byte1)) << 16) | (static_cast<uint_t>(static_cast<ubyte_t>(byte2)) << 8));
	}
}

void Tessellator::setTranslation(double d, double d1, double d2)
{
	xOffset = (tess_coord_t)d;
	yOffset = (tess_coord_t)d1;
	zOffset = (tess_coord_t)d2;
}

void Tessellator::addTranslation(float f, float f1, float f2)
{
	xOffset += f;
	yOffset += f1;
	zOffset += f2;
}

void Tessellator::setTranslationD(double d, double d1, double d2)
{
	setTranslation(d, d1, d2);
}

void Tessellator::setTranslationF(float f, float f1, float f2)
{
	addTranslation(f, f1, f2);
}
