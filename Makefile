CC = clang
CFLAGS = -Wall -O2 -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework Foundation -lm
TARGET = AU_synth_bass.component/Contents/MacOS/AU_synth_bass
SRC = src/fm_synth.c
HDRS = src/fm_synth.h

all: $(TARGET)

AU_synth_bass.component/Contents/MacOS:
	@mkdir -p AU_synth_bass.component/Contents/MacOS

$(TARGET): $(SRC) $(HDRS) | AU_synth_bass.component/Contents/MacOS
	$(CC) $(CFLAGS) -dynamiclib -Wl,-install_name,@executable_path/AU_synth_bass -o $@ $(SRC) -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework Foundation -lm

clean:
	rm -f $(TARGET)

install: all
	cp -R AU_synth_bass.component ~/Library/Audio/PPlug-Ins/

uninstall:
	rm -rf ~/Library/Audio/PPlug-Ins/AU_synth_bass.component

.PHONY: all clean install uninstall
