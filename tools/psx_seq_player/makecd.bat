:: This requires PSYQ PSX SDK to build along with a few other tools

del main.exe
del main.cpe
ccpsx -g0 -O3 -Xo$80010000 seqplayer.cpp -omain.cpe
cpe2x main.cpe
pause
::exefixup main.exe
del iso_root\main.exe
copy main.exe iso_root\main.exe
mkisofs -xa -o psxiso.iso iso_root
mkpsxiso.exe psxiso.iso psxiso.bin C:\psyq\bin\infoeur.dat
C:
cd C:\Users\paul\Downloads\pcsxr\
pcsxr.exe -nogui -cdfile Z:\psxiso.bin
pause
