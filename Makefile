CC = clang
CFLAGS = -Wall -O2 -arch arm64
LDFLAGS = -dynamiclib -arch arm64 -Wl,-install_name,@executable_path/AU_synth_bass
FRAMEWORKS = -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework Foundation -framework Cocoa -framework CoreFoundation
LIBS = -lm

TARGET_A64 = arm64/AU_synth_bass.component/Contents/MacOS/AU_synth_bass
TARGET_REL = release/AU_synth_bass.component/Contents/MacOS/AU_synth_bass

SRC = src/fm_synth.c
HDRS = src/fm_synth.h

INFO_PLIST = Resources/Info.plist
HTML_RES = Resources/html/index.html

all: $(TARGET_A64) $(TARGET_REL)

# ---- Binary builds ----

$(TARGET_A64): $(SRC) $(HDRS) | arm64/AU_synth_bass.component/Contents/MacOS
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(FRAMEWORKS) $(LIBS)

$(TARGET_REL): $(SRC) $(HDRS) | release/AU_synth_bass.component/Contents/MacOS
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(FRAMEWORKS) $(LIBS)

# ---- Directory creation ----

arm64/AU_synth_bass.component/Contents/MacOS:
	mkdir -p $@

release/AU_synth_bass.component/Contents/MacOS:
	mkdir -p $@

arm64/AU_synth_bass.component/Contents/Info.plist: Resources/Info.plist | arm64/AU_synth_bass.component/Contents
	cp $< $@

release/AU_synth_bass.component/Contents/Info.plist: Resources/Info.plist | release/AU_synth_bass.component/Contents
	cp $< $@

arm64/AU_synth_bass.component/Contents/Resources/html/index.html: Resources/html/index.html | arm64/AU_synth_bass.component/Contents/Resources/html
	cp $< $@

release/AU_synth_bass.component/Contents/Resources/html/index.html: Resources/html/index.html | release/AU_synth_bass.component/Contents/Resources/html
	cp $< $@

arm64/AU_synth_bass.component/Contents/Resources/html:
	mkdir -p $@

release/AU_synth_bass.component/Contents/Resources/html:
	mkdir -p $@

arm64/AU_synth_bass.component/Contents/Resources:
	mkdir -p $@

release/AU_synth_bass.component/Contents/Resources:
	mkdir -p $@

arm64/AU_synth_bass.component/Contents:
	mkdir -p $@

release/AU_synth_bass.component/Contents:
	mkdir -p $@

arm64/AU_synth_bass.component:
	mkdir -p $@

release/AU_synth_bass.component:
	mkdir -p $@

# ---- Phony targets ----

clean:
	rm -rf arm64 release

install: all
	mkdir -p ~/Library/Audio/Plug-Ins/Components
	rm -rf ~/Library/Audio/Plug-Ins/Components/AU_synth_bass.component
	cp -R arm64/AU_synth_bass.component ~/Library/Audio/Plug-Ins/Components/
	@echo "Installed to ~/Library/Audio/Plug-Ins/Components/"

uninstall:
	rm -rf ~/Library/Audio/Plug-Ins/Components/AU_synth_bass.component

.PHONY: all clean install uninstall
