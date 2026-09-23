with open(r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\solo_wii_guards.txt", "r", encoding="utf-8") as f:
    lines = f.readlines()

for line in lines:
    path, lno, content = line.split(":", 2)
    # Exclude render files
    if any(r in path for r in ["WorldRenderer", "RenderBlocks", "RenderEngine", "RenderGlobal", "RenderList", "LegacyColorGradePass", "LegacyPanoramaUpload"]):
        continue
    print(f"{path}:{lno}: {content.strip()}")
