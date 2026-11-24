import sys
p = sys.argv[1]
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
old = '''        if (s_probePhaseT == 1)
        {
            setModuleState(HighJump, true);
            setModuleOption(HighJump, 2);
            p->jump();
            PLOG("HIGHJUMP motionY after jump()=%.3f (vanilla 0.42)\\n", p->motionY);
            setModuleState(HighJump, false);
        }
'''.replace('\n', nl)
new = '''        if (s_probePhaseT == 1)
        {
            setModuleState(HighJump, true);
            setModuleOption(HighJump, 2);
            s_py = p->boundingBox->minY;
            s_count0 = 0;
        }
        if (s_probePhaseT >= 2 && s_probePhaseT <= 4)
            mc->gameSettings->keyBindJump->pressed = true;
        if (s_probePhaseT == 5)
            mc->gameSettings->keyBindJump->pressed = false;
        if (s_probePhaseT > 1 && s_probePhaseT < 19)
            s_count0 = std::max(s_count0, (int)((p->boundingBox->minY - s_py) * 100.0));
        if (s_probePhaseT == 19)
        {
            PLOG("HIGHJUMP x3 rise=%.2f blocks (vanilla ~1.25)\\n", s_count0 / 100.0);
            setModuleState(HighJump, false);
        }
'''.replace('\n', nl)
assert s.count(old) == 1
s = s.replace(old, new)
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
