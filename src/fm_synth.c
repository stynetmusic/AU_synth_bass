#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define kSampleRate 44100.0
#define PI 3.14159265358979323846
#define NUM_OSCILLATORS 3

typedef struct {
    double phase;
    double frequency;
    double amplitude;
} OscillatorState;

typedef struct {
    // Parameters (mirroring the web synth)
    double ratio;
    double index;
    double drive;
    double bits;
    double delay;
    double hyper;
    double chorus;
    double phaser;
    double eqGain;
    double comp;
    double reverb;
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
    double timeMix;
    int timeMode;

    // Internal state
    OscillatorState carrier;
    OscillatorState modulator;
    OscillatorState subOsc;
    OscillatorState lfoOsc;
    OscillatorState delayLine[2048];
    int delayWritePos;
    double noteEnvelope;
    double lfoPhase;
    double delayFeedback;
    double delayMix;

    int noteOn;
    int currentNote;
    double currentOctave;
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

static double getWaveform(double phase, int type) {
    switch (type) {
        case 0: return squareWave(phase);
        case 1: return sawTooth(phase);
        case 2: return triWave(phase);
        case 3:
        default: return sineWave(phase);
    }
}

static double noiseBuffer[4096];
static int noiseInitialized = 0;

static void initNoise() {
    if (noiseInitialized) return;
    for (int i = 0; i < 4096; i++) {
        noiseBuffer[i] = (double)rand() / RAND_MAX * 2.0 - 1.0;
    }
    noiseInitialized = 1;
}

static double bitcrush(double sample, double bits) {
    int levels = (int)pow(2.0, bits);
    if (levels < 2) levels = 2;
    return floor(sample * levels / 2.0) * 2.0 / levels;
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

    double carrierWaveForm = synth->carrierWave;
    double modWaveForm = synth->modWave;
    double subWaveForm = synth->subWave;

    double ratio = synth->ratio;
    double fmIndex = synth->index;
    double lfoRate = synth->lfoRate;

    sync_with_lag:
    for (UInt32 i = 0; i < inNumberFrames; i++) {
        double noteFreq = 220.0 * pow(2.0, synth->currentOctave) * ratio;
        if (synth->noteOn) {
            synth->carrier.frequency = noteFreq;
            synth->modulator.frequency = noteFreq * ratio;
            synth->subOsc.frequency = noteFreq / synth->subOctave;
        } else {
            synth->carrier.frequency = 0;
            synth->modulator.frequency = 0;
            synth->subOsc.frequency = 0;
        }

        double sample = 0.0;

        if (synth->noteOn) {
            synth->lfoPhase += lfoRate / sr;
            if (synth->lfoPhase > 1.0) synth->lfoPhase -= 1.0;
            double lfoValue = sineWave(synth->lfoPhase) * (fmIndex / 1000.0);

            synth->carrier.phase += synth->carrier.frequency / sr;
            if (synth->carrier.phase > 1.0) synth->carrier.phase -= 1.0;
            double carrierSample = getWaveform(synth->carrier.phase, (int)carrierWaveForm);

            synth->modulator.phase += synth->modulator.frequency / sr;
            if (synth->modulator.phase > 1.0) synth->modulator.phase -= 1.0;
            double modSample = getWaveform(synth->modulator.phase, (int)modWaveForm);

            double modDepth = lfoValue + (fmIndex / 100.0) * synth->fmEnvDepth;
            double fmPhase = synth->carrier.phase + modSample * modDepth / sr;
            double fmCarriers = getWaveform(fmPhase - floor(fmPhase), (int)carrierWaveForm);

            carrierSample = fmCarriers * (1.0 - synth->drive / 200.0) + carrierSample * (synth->drive / 200.0);

            synth->subOsc.phase += synth->subOsc.frequency / sr;
            if (synth->subOsc.phase > 1.0) synth->subOsc.phase -= 1.0;
            double subSample = getWaveform(synth->subOsc.phase, (int)subWaveForm);

            sample = carrierSample * 0.7 + subSample * (synth->subGain / 150.0);

            double envRate = 1.0 / sr;
            synth->noteEnvelope += envRate;
            if (synth->noteEnvelope > 1.0) synth->noteEnvelope = 1.0;

            sample *= synth->noteEnvelope;

            sample = bitcrush(sample, synth->bits);

            synth->delayWritePos = (synth->delayWritePos + 1) & 2047;
            synth->delayLine[synth->delayWritePos] = sample * (1.0 - synth->delayMix) + synth->delayLine[(synth->delayWritePos - (int)(synth->delay * sr) & 2047)] * synth->delayMix;
            sample = synth->delayLine[synth->delayWritePos];

            initNoise();
            double noise = noiseBuffer[(int)(synth->lfoPhase * 4096) & 4095] * synth->noiseGain / 100.0;
            sample += noise;
        }

        *left++ = (Float32)sample;
        *right++ = (Float32)sample;
    }

    return noErr;
}

static OSStatus setPropertyCallback(void *inRefCon,
                                    AudioUnitUnitID inUnit,
                                    AudioUnitPropertyID inID,
                                    UInt32 inScope,
                                    UInt32 inElement,
                                    const void *inData,
                                    UInt32 inDataSize) {
    SynthInstance *synth = (SynthInstance *)inRefCon;
    if (inID == kAudioUnitProperty_SetRenderCallback) {
        // handled below
    }
    return noErr;
}

static AURenderCallbackStruct renderCallbackStruct;

OSStatus AUEntryPoint(AudioComponentInstance inInstance, UInt32 inMessage, const void *inParam) {
    switch (inMessage) {
        case kAudioUnitMessage_Initialize: {
            UInt32 size = sizeof(SynthInstance);
            SynthInstance *synth = (SynthInstance *)malloc(size);
            memset(synth, 0, size);

            synth->ratio = 1.0;
            synth->index = 400.0;
            synth->drive = 50.0;
            synth->bits = 8.0;
            synth->delay = 20.0;
            synth->hyper = 30.0;
            synth->chorus = 25.0;
            synth->phaser = 40.0;
            synth->eqGain = 4.0;
            synth->comp = 60.0;
            synth->reverb = 15.0;
            synth->lfoRate = 3.0;
            synth->attack = 0.01;
            synth->decay = 0.4;
            synth->sustain = 0.7;
            synth->release = 0.1;
            synth->carrierWave = 1.0;
            synth->modWave = 0.0;
            synth->fmEnvDecay = 0.1;
            synth->fmEnvDepth = 400.0;
            synth->subWave = 3.0;
            synth->subOctave = 1.0;
            synth->subGain = 100.0;
            synth->subSat = 30.0;
            synth->noiseGain = 25.0;
            synth->noiseCutoff = 2000.0;
            synth->noiseFilterType = 0;
            synth->timeMix = 100.0;
            synth->timeMode = 0;
            synth->delayWritePos = 0;
            synth->delayMix = 0.0;
            synth->currentOctave = 1.0;

            // Set render callback
            AURenderCallbackStruct callback;
            callback.inputProc = renderCallback;
            callback.inputProcRefCon = synth;
            AudioUnitSetProperty(inInstance,
                                 kAudioUnitProperty_SetRenderCallback,
                                 kAudioUnitScope_Global,
                                 0,
                                 &callback,
                                 sizeof(callback));

            // Set stream format
            AudioStreamBasicDescription streamFormat;
            memset(&streamFormat, 0, sizeof(streamFormat));
            streamFormat.mSampleRate = kSampleRate;
            streamFormat.mFormatID = kAudioFormatLinearPCM;
            streamFormat.mFormatFlags = kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
            streamFormat.mFramesPerPacket = 1;
            streamFormat.mChannelsPerFrame = 2;
            streamFormat.mBitsPerChannel = 32;
            streamFormat.mBytesPerFrame = 8;
            streamFormat.mBytesPerPacket = 8;
            AudioUnitSetProperty(inInstance,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input,
                                 0,
                                 &streamFormat,
                                 sizeof(streamFormat));
            AudioUnitSetProperty(inInstance,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Output,
                                 0,
                                 &streamFormat,
                                 sizeof(streamFormat));

            *((SynthInstance **)inParam) = synth;
            return noErr;
        }
        case kAudioUnitMessage_Terminate: {
            SynthInstance **synthPtr = (SynthInstance **)inParam;
            if (*synthPtr) free(*synthPtr);
            return noErr;
        }
        case kAudioUnitMessage_Start:
        case kAudioUnitMessage_Stop: {
            SynthInstance **synthPtr = (SynthInstance **)inParam;
            if (*synthPtr) {
                (*synthPtr)->noteOn = (inMessage == kAudioUnitMessage_Start);
            }
            return noErr;
        }
        default:
            return noErr;
    }
}
