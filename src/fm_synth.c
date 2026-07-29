#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define kSampleRate 44100.0
#define PI 3.14159265358979323846
#define kDelayBufferSize 2048

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
    int   delayWritePos;
    double delayBuffer[kDelayBufferSize];
    double lfoPhase;
    double noteEnvelope;
    double carrierPhase;
    double modPhase;
    double subPhase;
    double currentOctave;
    int   noteOn;
} SynthInstance;

static double sawTooth(double phase) {
    return 2.0 * (phase - floor(phase + 0.5));
}
static double squareWave(double phase) {
    return (phase < 0.5) ? 1.0 : -1.0;
}
static double triWave(double phase) {
    double p = phase - floor(phase);
    return 2.0 * (1.0 - 4.0 * fabs(p - 0.5));
}
static double sineWave(double phase) {
    return sin(2.0 * PI * phase);
}
static double getWave(double phase, int type) {
    switch (type) {
        case 0: return squareWave(phase);
        case 1: return sawTooth(phase);
        case 2: return triWave(phase);
        case 3: default: return sineWave(phase);
    }
}

static OSStatus renderCallback(void *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp *inTimeStamp,
                                UInt32 inBusNumber,
                                UInt32 inNumberFrames,
                                AudioBufferList *ioData) {
    SynthInstance *synth = (SynthInstance *)inRefCon;
    Float32 *left = (Float32 *)ioData->mBuffers[0].mData;
    Float32 *right = (Float32 *)ioData->mBuffers[1].mData;
    double sr = kSampleRate;

    double noteFreq = 220.0 * pow(2.0, synth->currentOctave) * synth->ratio;
    double lfoPhaseInc = synth->lfoRate / sr;
    double modDepth = synth->index / 100.0;

    for (UInt32 i = 0; i < inNumberFrames; i++) {
        double sample = 0.0;
        if (synth->noteOn) {
            synth->lfoPhase += lfoPhaseInc;
            if (synth->lfoPhase > 1.0) synth->lfoPhase -= 1.0;
            double lfoVal = sineWave(synth->lfoPhase) * modDepth;

            synth->carrierPhase += noteFreq / sr;
            if (synth->carrierPhase > 1.0) synth->carrierPhase -= 1.0;
            synth->modPhase += noteFreq * synth->ratio / sr;
            if (synth->modPhase > 1.0) synth->modPhase -= 1.0;

            double modSample = getWave(synth->modPhase, (int)synth->modWave);
            double fmPhase = synth->carrierPhase + modSample * lfoVal;
            double carrierSample = getWave(fmPhase - floor(fmPhase), (int)synth->carrierWave);

            synth->subPhase += (noteFreq / synth->subOctave) / sr;
            if (synth->subPhase > 1.0) synth->subPhase -= 1.0;
            double subSample = getWave(synth->subPhase, 3);

            sample = carrierSample * 0.7 + subSample * (synth->subGain / 150.0)
                   + synth->subSat / 100.0 * (carrierSample * carrierSample);
            sample *= synth->noteEnvelope;
            sample = floor(sample * pow(2.0, synth->bits)) / pow(2.0, synth->bits);

            int delayReadPos = (synth->delayWritePos - (int)(synth->delayTime * sr)
                                + kDelayBufferSize) % kDelayBufferSize;
            synth->delayBuffer[synth->delayWritePos] = sample;
            sample = synth->delayBuffer[delayReadPos];

            synth->noteEnvelope += synth->decay;
            if (synth->noteEnvelope > synth->sustain)
                synth->noteEnvelope = synth->sustain;

            synth->delayWritePos = (synth->delayWritePos + 1) % kDelayBufferSize;
        }
        *left++ = (Float32)sample;
        *right++ = (Float32)sample;
    }
    return noErr;
}

OSStatus AudioComponentEntryPoint(AudioComponentInstance inInstance,
                                        UInt32 inMessage,
                                        const void *inParams) {
    (void)inInstance;
    (void)inMessage;
    (void)inParams;
    return noErr;
}
