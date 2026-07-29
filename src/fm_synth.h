#ifndef FM_SYNTH_H
#define FM_SYNTH_H

#include <AudioUnit/AudioUnit.h>

#define kSampleRate 44100.0
#define kNumFrames 512

typedef struct {
    double ratio;
    double index;
    double drive;
    double bits;
    double lfoRate;
    double attack;
    double decay;
    double sustain;
    double release;
    double carrierWave;
    double modWave;
    double fmEnvDecay;
    double fmEnvDepth;
    double subWave;
    double subOctave;
    double subGain;
    double subSat;
    double noiseGain;
    double noiseCutoff;
    int noiseFilterType;

    double phase;
    double modPhase;
    double subPhase;
    double lfoPhase;
    double envelope;
    double noiseHistory[16];
    int noiseIndex;
} SynthState;

OSStatus renderCallback(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 inBusNumber,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData);

#endif
