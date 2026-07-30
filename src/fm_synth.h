#ifndef FM_SYNTH_H
#define FM_SYNTH_H

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreMIDI/MIDIServices.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define kSampleRate 44100.0
#define kDelayBufferSize 2048
#define kMaxVoices 16

typedef struct {
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

    double delayBuffer[kDelayBufferSize];
    int   delayWritePos;
    double lfoPhase;
    double envelope;
    double carrierPhase;
    double modPhase;
    double subPhase;
    int   noteOn;
    int   note;
    double velocity;
    double noteStartTime;
    int   isReleased;
} Voice;

typedef struct {
    Voice voices[kMaxVoices];
    int   activeVoiceCount;
    double masterVolume;
    double sampleRate;
    int   notes[128];
} SynthInstance;

typedef enum {
    kParam_Ratio = 0,
    kParam_Index,
    kParam_Drive,
    kParam_Bits,
    kParam_DelayTime,
    kParam_LFORate,
    kParam_Attack,
    kParam_Decay,
    kParam_Sustain,
    kParam_Release,
    kParam_CarrierWave,
    kParam_ModWave,
    kParam_FMEnvDepth,
    kParam_SubGain,
    kParam_SubOctave,
    kParam_SubSat,
    kParam_NoiseGain,
    kParam_NoiseCutoff,
    kParam_NoiseFilterType,
    kParam_NumParameters
} SynthParameter;

double sawTooth(double phase);
double squareWave(double phase);
double triWave(double phase);
double sineWave(double phase);
double getWave(double phase, int type);
void initSynth(SynthInstance *synth);
OSStatus renderAudio(SynthInstance *synth, AudioBufferList *ioData, UInt32 inNumberFrames);
OSStatus handleMIDI(SynthInstance *synth, UInt32 status, UInt32 data1, UInt32 data2);
OSStatus getParameterValue(SynthInstance *synth, AudioUnitParameterID inID, AudioUnitParameterValue *outValue);
OSStatus setParameterValue(SynthInstance *synth, AudioUnitParameterID inID, AudioUnitParameterValue inValue, UInt32 inBufferOffsetInFrames);

#endif