import net.minecraft.src.*;
import java.util.*;
public class Paseo {
    public static void main(String[] a) {
        long seed = Long.parseLong(a[0]);
        WorldChunkManager wcm = new WorldChunkManager(seed, WorldType.DEFAULT);
        Random r = new Random(seed);
        ChunkPosition p = wcm.findBiomePosition(0, 0, 256, wcm.getBiomesToSpawnIn(), r);
        int x = p == null ? 0 : p.x, z = p == null ? 0 : p.z;
        int ocean = 0, land = 0;
        for (int i = 0; i < 1000; i++) {
            BiomeGenBase b = wcm.getBiomeGenAt(x, z);
            boolean isOcean = b == BiomeGenBase.ocean || b == BiomeGenBase.frozenOcean;
            if (isOcean) ocean++; else { land++; if (land <= 8) System.out.println("probe " + i + " " + x + " " + z + " " + b.biomeName); }
            x += r.nextInt(64) - r.nextInt(64);
            z += r.nextInt(64) - r.nextInt(64);
        }
        System.out.println("start " + (p == null ? "null" : p.x + "," + p.z) + " ocean " + ocean + " land " + land);
    }
}
