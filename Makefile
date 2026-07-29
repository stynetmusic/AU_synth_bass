CC = clang
CFLAGS = -Wall -O2 -arch arm64
LDFLAGS = -dynamiclib -arch arm64 -Wl,-install_name,@executable_path/AU_synth_bass
FRAMEWORKS = -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework Foundation -framework Cocoa -framework CoreFoundation
LIBS = -lm

TARGET = AU_synth_bass.component/Contents/MacOS/AU_synth_bass
SRC = src/fm_synth.c
HDRS = src/fm_synth.h

BUNDLE = AU_synth_bass.component
PLUGINS_DIR = $(HOME)/Library/Audio/Plug-Ins/Components

all: $(TARGET)

$(TARGET): $(SRC) $(HDRS)
	@mkdir -p $(BUNDLE)/Contents/MacOS
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(FRAMEWORKS) $(LIBS)

clean:
	rm -f $(TARGET)

install: all
	@mkdir -p $(PLUGINS_DIR)
	rm -rf $(PLUGINS_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(PLUGINS_DIR)/
	@echo "Installed to $(PLUGINS_DIR)"
	@echo "Restart your AU host (Ableton, Logic, GarageBand) to scan"

uninstall:
	rm -rf $(PLUGINS_DIR)/$(BUNDLE)

build-and-install: all install

.PHONY: all clean install uninstall build-and-install
