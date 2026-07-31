#ifndef FM_SYNTH_H
#define FM_SYNTH_H

#include <AudioUnit/AudioUnit.h>

#define kDelayBufferSize 2048
#define kMaxVoices 16

typedef struct {
    double delayBuffer[kDelayBufferSize];
    int   delayWritePos;
    double lfoPhase;
    double envelope;
    double carrierPhase;
    double modPhase;
    double subPhase;
    int   note;
    double velocity;
    int   noteOn;
    int   isReleased;
    double ratio;
    double index;
    double drive;
    double bits;
    double delayTime;
    double lfoRate;
    double attack;
    double decay;
    double sustain;
    double release;
    double carrierWave;
    double modWave;
    double fmEnvDepth;
    double subGain;
    double subOctave;
    double subSat;
    double noiseGain;
    double noiseCutoff;
    int   noiseFilterType;
} Voice;

typedef struct {
    Voice voices[kMaxVoices];
    int   activeVoiceCount;
    double masterVolume;
    double sampleRate;
    int   notes[128];
} SynthInstance;

OSStatus AUEntryPoint(AudioComponentInstance inInstance,
                      UInt32 inMessage,
                      const void *inParams);

#endif