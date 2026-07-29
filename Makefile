CC = clang
CFLAGS = -Wall -O2 -arch arm64
LDFLAGS = -dynamiclib -arch arm64 -Wl,-install_name,@executable_path/AU_synth_bass
FRAMEWORKS = -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework Foundation
LIBS = -lm

TARGET = AU_synth_bass.component/Contents/MacOS/AU_synth_bass
SRC = src/fm_synth.c

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p AU_synth_bass.component/Contents/MacOS
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(FRAMEWORKS) $(LIBS)

clean:
	rm -f $(TARGET)

install: all
	cp -R AU_synth_bass.component ~/Library/Audio/PPlug-Ins/

uninstall:
	rm -rf ~/Library/Audio/PPlug-Ins/AU_synth_bass.component

.PHONY: all clean install uninstall
