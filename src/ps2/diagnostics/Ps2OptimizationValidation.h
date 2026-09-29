#pragma once

#ifdef PS2_OPTIMIZATION_VALIDATION

namespace Ps2OptimizationValidation
{
void reportAndReset();
void frameEnd(long long frameNs);

void remoteLivingPhysics(bool fullPhysics);
void remoteInterpolationQuery(bool executed);
void multiplayerItemPhysics(bool skippedSettledPhysics);
void skeletonCullDraw();
void particleFastMove();
void particleLayerCapEviction();

void terrainFogCull(int pass);
void terrainClusters(int tested, int rejected, int clipSafe, int guardRisk);
void terrainVu1Submit(int vertices);
void terrainVu1Retry(bool fatal);
void terrainVu0Submit(int vertices);
void translucentQuadRejected();

void enclosedWaterSkip();
void flatWaterFastPath();
void waterMerge(int input, int eligible, int removed);

void lightingDrain(int jobs, bool interactive, bool countExit, bool budgetExit, int queueStart);

void weatherFrame(float rainStrength, int candidates, int rainColumns, int snowColumns,
                  int textureSwitches);
void weatherDrawBegin(int kind);
void weatherDrawEnd(int bytesDrawn);
bool weatherDrawActive();
void weatherBackendBatch(int vertices, bool quads, bool accepted, long queueBefore, long queueAfter,
                         bool textured, bool colored, bool brightness, bool blend, bool alphaTest,
                         int alphaRef, bool depthTest, bool depthWrite, bool cullFace);
void weatherFast3dBegin(int vertices, bool quads);
void weatherTrivialReject(int triangles);
void weatherClip(int inputTriangles, int outputTriangles);
void weatherOffscreen(int triangles);
void weatherBackface(int triangles);
void weatherGsSubmit(int triangles);
void weatherGsAllocationFailure(int triangles);

void meshBuildStarted();
void meshBuildPublished(bool dirtyDuringBuild);
void meshDirtyCoalesced();
void meshDirtyRestarted();
void meshUrgentMarked();
void meshUrgentFinished(bool published);
}

#else

namespace Ps2OptimizationValidation
{
inline void reportAndReset() {}
inline void frameEnd(long long) {}
inline void remoteLivingPhysics(bool) {}
inline void remoteInterpolationQuery(bool) {}
inline void multiplayerItemPhysics(bool) {}
inline void skeletonCullDraw() {}
inline void particleFastMove() {}
inline void particleLayerCapEviction() {}
inline void terrainFogCull(int) {}
inline void terrainClusters(int, int, int, int) {}
inline void terrainVu1Submit(int) {}
inline void terrainVu1Retry(bool) {}
inline void terrainVu0Submit(int) {}
inline void translucentQuadRejected() {}
inline void enclosedWaterSkip() {}
inline void flatWaterFastPath() {}
inline void waterMerge(int, int, int) {}
inline void lightingDrain(int, bool, bool, bool, int) {}
inline void weatherFrame(float, int, int, int, int) {}
inline void weatherDrawBegin(int) {}
inline void weatherDrawEnd(int) {}
inline bool weatherDrawActive() { return false; }
inline void weatherBackendBatch(int, bool, bool, long, long, bool, bool, bool, bool, bool, int, bool, bool, bool) {}
inline void weatherFast3dBegin(int, bool) {}
inline void weatherTrivialReject(int) {}
inline void weatherClip(int, int) {}
inline void weatherOffscreen(int) {}
inline void weatherBackface(int) {}
inline void weatherGsSubmit(int) {}
inline void weatherGsAllocationFailure(int) {}
inline void meshBuildStarted() {}
inline void meshBuildPublished(bool) {}
inline void meshDirtyCoalesced() {}
inline void meshDirtyRestarted() {}
inline void meshUrgentMarked() {}
inline void meshUrgentFinished(bool) {}
}

#endif
