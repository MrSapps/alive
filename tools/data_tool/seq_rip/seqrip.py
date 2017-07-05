import string

# Opens up each game binary and rips out every XXX.SEQ string

def strings(filename, min=4):
    with open(filename, "rb") as f:           # Python 2.x
        result = ""
        for c in f.read():
            if c in string.printable:
                result += c
                continue
            if len(result) >= min:
                yield result
            result = ""
        if len(result) >= min:  # catch result at EOF
            yield result

def rip_seq_strings(fileName):
    seqs = []
    for s in strings(fileName):
        if s.endswith(".SEQ"):
            seqs.append(s)
    return seqs


def rip_seqs(fileName):
    seqs = rip_seq_strings(fileName)
    seqs.sort()

    thefile = open(fileName + ".txt", 'w')
    for item in seqs:
      thefile.write("\"%s\",\n" % item)

# AbeWin.exe is renamed to AoPc in this case etc
inputs = ['AePc', 'AePcDemo', 'AePsxCd1', 'AePsxCd2', 'AePsxDemo', 'AoPc', 'AoPcDemo', 'AoPsx', 'AoPsxDemo']
for item in inputs:
    rip_seqs(item)
