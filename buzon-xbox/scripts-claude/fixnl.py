import re, sys
# Joins C string literals that a shell heredoc broke ("...text<newline>", args)
# back into "...text\n", args — only on lines tagged probe125.
for p in sys.argv[1:]:
    s = open(p, encoding='utf-8', newline='').read()
    pat = re.compile(r'("probe125", "[^"\r\n]*)\r?\n(")')
    s2, n = pat.subn(lambda m: m.group(1) + '\\n' + m.group(2), s)
    open(p, 'w', encoding='utf-8', newline='').write(s2)
    print(p, n)
