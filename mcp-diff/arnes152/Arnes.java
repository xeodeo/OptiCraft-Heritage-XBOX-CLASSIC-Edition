import net.minecraft.src.*;
import java.util.*;

public class Arnes {
    public static void main(String[] a) {
        long seed = Long.parseLong(a[0]);
        WorldChunkManager wcm = new WorldChunkManager(seed, WorldType.DEFAULT);
        Random r = new Random(seed);
        ChunkPosition p = wcm.findBiomePosition(0, 0, 256, wcm.getBiomesToSpawnIn(), r);
        System.out.println("spawnBiomePos " + (p == null ? "null" : p.x + " " + p.z)
            + " biome " + (p == null ? "-" : wcm.getBiomeGenAt(p.x, p.z).biomeName));
        System.out.println("at00 " + wcm.getBiomeGenAt(0, 0).biomeName);
        StringBuilder sb = new StringBuilder();
        for (int z = -512; z <= 512; z += 32) {
            for (int x = -512; x <= 512; x += 32)
                sb.append((char) ('A' + wcm.getBiomeGenAt(x, z).biomeID));
            sb.append('\n');
        }
        System.out.print(sb);
        List<BiomeGenBase> huts = Arrays.asList(BiomeGenBase.desert, BiomeGenBase.desertHills,
            BiomeGenBase.jungle, BiomeGenBase.jungleHills, BiomeGenBase.swampland);
        for (int cz = -40; cz <= 40; cz++)
            for (int cx = -40; cx <= 40; cx++) {
                int x = cx, z = cz;
                if (x < 0) x -= 31;
                if (z < 0) z -= 31;
                int gx = x / 32, gz = z / 32;
                Random rr = new Random((long) gx * 341873128712L + (long) gz * 132897987541L + seed + 14357617L);
                gx *= 32; gz *= 32;
                gx += rr.nextInt(24); gz += rr.nextInt(24);
                if (cx == gx && cz == gz) {
                    BiomeGenBase b = wcm.getBiomeGenAt(cx * 16 + 8, cz * 16 + 8);
                    if (huts.contains(b))
                        System.out.println("feature chunk " + cx + " " + cz + " " + b.biomeName);
                }
            }
    }
}
