SOURCE = ./src/vgsweb.cpp
SOURCE += ./src/BufferQueue.cpp
SOURCE += ./vgszero/src/core/vgs0math.c
SOURCE += ./vgszero/src/core/vgstone.c
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Sound/rom_tndtable.c
SOURCE += ./vgszero/src/core/nsf/xgm/player/nsf/nsf.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/CPU/nes_cpu.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Sound/nes_vrc6.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Sound/nes_apu.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Sound/nes_dmc.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Memory/nsf2_vectors.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Memory/nes_bank.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Memory/nes_mem.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Misc/nsf2_irq.cpp
SOURCE += ./vgszero/src/core/nsf/xgm/devices/Audio/rconv.cpp

all: ./tools/bin2var/bin2var ./src/gamepkg.h ./public/index.js
	cd public && python -m http.server 8080

clean:
	-rm -f public/index.js
	-rm -f public/index.wasm
	-rm -f src/gamepkg.h

./tools/bin2var/bin2var:
	cd ./tools/bin2var && make

./src/gamepkg.h: ./game.pkg
	./tools/bin2var/bin2var $< GAMEPKG > $@

./public/index.js: ${SOURCE} src/gamepkg.h
	emcc ${SOURCE} -s WASM=1 -s USE_SDL=2 -O3 -o $@

