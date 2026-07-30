#include "fm_synth.h"
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreMIDI/MIDIServices.h>
#include <stdio.h>

static SynthInstance *gSynth = NULL;
static double gSampleRate = 44100.0;
static double gGlobalRatio = 1.0;
static double gGlobalIndex = 400.0;
static double gGlobalDrive = 50.0;
static double gGlobalBits = 8.0;
static double gGlobalDelayTime = 20.0;
static double gGlobalLFORate = 3.0;
static double gGlobalAttack = 0.01;
static double gGlobalDecay = 0.4;
static double gGlobalSustain = 0.7;
static double gGlobalRelease = 0.1;
static double gGlobalCarrierWave = 1.0;
static double gGlobalModWave = 3.0;
static double gGlobalFMEnvDepth = 400.0;
static double gGlobalSubGain = 100.0;
static double gGlobalSubOctave = 1.0;
static double gGlobalSubSat = 30.0;
static double gGlobalNoiseGain = 25.0;
static double gGlobalNoiseCutoff = 2000.0;
static int gGlobalNoiseFilterType = 1;

double sawTooth(double phase) {
    double p = phase - floor(phase);
    return 2.0 * (p - 0.5);
}

double squareWave(double phase) {
    double p = phase - floor(phase);
    return (p < 0.5) ? 1.0 : -1.0;
}

double triWave(double phase) {
    double p = phase - floor(phase);
    return 2.0 * (1.0 - 4.0 * fabs(p - 0.5));
}

double sineWave(double phase) {
    double p = phase - floor(phase);
    return sin(2.0 * M_PI * p);
}

double getWave(double phase, int type) {
    switch (type) {
        case 0: return squareWave(phase);
        case 1: return sawTooth(phase);
        case 2: return triWave(phase);
        case 3: default: return sineWave(phase);
    }
}

void initSynth(SynthInstance *synth) {
    memset(synth, 0, sizeof(SynthInstance));
    synth->masterVolume = 0.8;
    synth->sampleRate = 44100.0;
    synth->activeVoiceCount = 0;
    for (int i = 0; i < 128; i++) {
        synth->notes[i] = 0;
    }
    for (int v = 0; v < kMaxVoices; v++) {
        Voice *voice = &synth->voices[v];
        voice->ratio = gGlobalRatio;
        voice->index = gGlobalIndex;
        voice->drive = gGlobalDrive;
        voice->bits = gGlobalBits;
        voice->delayTime = gGlobalDelayTime;
        voice->lfoRate = gGlobalLFORate;
        voice->attack = gGlobalAttack;
        voice->decay = gGlobalDecay;
        voice->sustain = gGlobalSustain;
        voice->release = gGlobalRelease;
        voice->carrierWave = gGlobalCarrierWave;
        voice->modWave = gGlobalModWave;
        voice->fmEnvDepth = gGlobalFMEnvDepth;
        voice->subGain = gGlobalSubGain;
        voice->subOctave = gGlobalSubOctave;
        voice->subSat = gGlobalSubSat;
        voice->noiseGain = gGlobalNoiseGain;
        voice->noiseCutoff = gGlobalNoiseCutoff;
        voice->noiseFilterType = gGlobalNoiseFilterType;
    }
}

OSStatus handleMIDI(SynthInstance *synth, UInt32 status, UInt32 data1, UInt32 data2) {
    UInt32 command = status & 0xF0;
    UInt32 note = data1 & 0x7F;
    UInt32 velocity = data2 & 0x7F;

    if (command == 0x90 && velocity > 0) {
        if (synth->notes[note] == 0) {
            synth->notes[note] = 1;
            for (int v = 0; v < kMaxVoices; v++) {
                if (!synth->voices[v].noteOn) {
                    Voice *voice = &synth->voices[v];
                    voice->note = note;
                    voice->velocity = velocity / 127.0;
                    voice->noteOn = 1;
                    voice->isReleased = 0;
                    voice->noteStartTime = 0.0;
                    voice->lfoPhase = 0.0;
                    voice->envelope = 0.0;
                    voice->carrierPhase = 0.0;
                    voice->modPhase = 0.0;
                    voice->subPhase = 0.0;
                    voice->delayWritePos = 0;
                    synth->activeVoiceCount++;
                    break;
                }
            }
        }
    } else if (command == 0x80 || (command == 0x90 && velocity == 0)) {
        if (synth->notes[note] > 0) {
            synth->notes[note] = 0;
            for (int v = 0; v < kMaxVoices; v++) {
                if (synth->voices[v].noteOn && synth->voices[v].note == note) {
                    synth->voices[v].isReleased = 1;
                    break;
                }
            }
        }
    }

    return noErr;
}

OSStatus getParameterValue(SynthInstance *synth, AudioUnitParameterID inID, AudioUnitParameterValue *outValue) {
    if (!synth || !outValue) return kAudioUnitErr_InvalidPropertyValue;

    switch (inID) {
        case kParam_Ratio: *outValue = gGlobalRatio; break;
        case kParam_Index: *outValue = gGlobalIndex; break;
        case kParam_Drive: *outValue = gGlobalDrive; break;
        case kParam_Bits: *outValue = gGlobalBits; break;
        case kParam_DelayTime: *outValue = gGlobalDelayTime; break;
        case kParam_LFORate: *outValue = gGlobalLFORate; break;
        case kParam_Attack: *outValue = gGlobalAttack; break;
        case kParam_Decay: *outValue = gGlobalDecay; break;
        case kParam_Sustain: *outValue = gGlobalSustain; break;
        case kParam_Release: *outValue = gGlobalRelease; break;
        case kParam_CarrierWave: *outValue = gGlobalCarrierWave; break;
        case kParam_ModWave: *outValue = gGlobalModWave; break;
        case kParam_FMEnvDepth: *outValue = gGlobalFMEnvDepth; break;
        case kParam_SubGain: *outValue = gGlobalSubGain; break;
        case kParam_SubOctave: *outValue = gGlobalSubOctave; break;
        case kParam_SubSat: *outValue = gGlobalSubSat; break;
        case kParam_NoiseGain: *outValue = gGlobalNoiseGain; break;
        case kParam_NoiseCutoff: *outValue = gGlobalNoiseCutoff; break;
        case kParam_NoiseFilterType: *outValue = gGlobalNoiseFilterType; break;
        default: return kAudioUnitErr_InvalidParameter;
    }
    return noErr;
}

OSStatus setParameterValue(SynthInstance *synth, AudioUnitParameterID inID, AudioUnitParameterValue inValue, UInt32 inBufferOffsetInFrames) {
    (void)synth;
    (void)inBufferOffsetInFrames;

    switch (inID) {
        case kParam_Ratio: gGlobalRatio = inValue; break;
        case kParam_Index: gGlobalIndex = inValue; break;
        case kParam_Drive: gGlobalDrive = inValue; break;
        case kParam_Bits: gGlobalBits = inValue; break;
        case kParam_DelayTime: gGlobalDelayTime = inValue; break;
        case kParam_LFORate: gGlobalLFORate = inValue; break;
        case kParam_Attack: gGlobalAttack = inValue; break;
        case kParam_Decay: gGlobalDecay = inValue; break;
        case kParam_Sustain: gGlobalSustain = inValue; break;
        case kParam_Release: gGlobalRelease = inValue; break;
        case kParam_CarrierWave: gGlobalCarrierWave = inValue; break;
        case kParam_ModWave: gGlobalModWave = inValue; break;
        case kParam_FMEnvDepth: gGlobalFMEnvDepth = inValue; break;
        case kParam_SubGain: gGlobalSubGain = inValue; break;
        case kParam_SubOctave: gGlobalSubOctave = inValue; break;
        case kParam_SubSat: gGlobalSubSat = inValue; break;
        case kParam_NoiseGain: gGlobalNoiseGain = inValue; break;
        case kParam_NoiseCutoff: gGlobalNoiseCutoff = inValue; break;
        case kParam_NoiseFilterType: gGlobalNoiseFilterType = (int)inValue; break;
        default: return kAudioUnitErr_InvalidParameter;
    }

    if (synth) {
        for (int v = 0; v < kMaxVoices; v++) {
            Voice *voice = &synth->voices[v];
            voice->ratio = gGlobalRatio;
            voice->index = gGlobalIndex;
            voice->drive = gGlobalDrive;
            voice->bits = gGlobalBits;
            voice->delayTime = gGlobalDelayTime;
            voice->lfoRate = gGlobalLFORate;
            voice->attack = gGlobalAttack;
            voice->decay = gGlobalDecay;
            voice->sustain = gGlobalSustain;
            voice->release = gGlobalRelease;
            voice->carrierWave = gGlobalCarrierWave;
            voice->modWave = gGlobalModWave;
            voice->fmEnvDepth = gGlobalFMEnvDepth;
            voice->subGain = gGlobalSubGain;
            voice->subOctave = gGlobalSubOctave;
            voice->subSat = gGlobalSubSat;
            voice->noiseGain = gGlobalNoiseGain;
            voice->noiseCutoff = gGlobalNoiseCutoff;
            voice->noiseFilterType = gGlobalNoiseFilterType;
        }
    }

    return noErr;
}

OSStatus renderAudio(SynthInstance *synth, AudioBufferList *ioData, UInt32 inNumberFrames) {
    Float32 *left = (Float32 *)ioData->mBuffers[0].mData;
    Float32 *right = (Float32 *)ioData->mBuffers[1].mData;
    double sr = synth->sampleRate;

    for (UInt32 i = 0; i < inNumberFrames; i++) {
        double sample = 0.0;

        for (int v = 0; v < kMaxVoices; v++) {
            Voice *voice = &synth->voices[v];
            if (!voice->noteOn) continue;

            double noteFreq = 220.0 * pow(2.0, (voice->note - 57) / 12.0);
            double lfoPhaseInc = voice->lfoRate / sr;
            double modDepth = voice->index / 100.0;

            if (!voice->isReleased) {
                double attackInc = 1.0 / (voice->attack * sr + 0.0001);
                voice->envelope += attackInc;
                if (voice->envelope > 1.0) voice->envelope = 1.0;
            } else {
                double releaseInc = 1.0 / (voice->release * sr + 0.0001);
                voice->envelope -= releaseInc;
                if (voice->envelope <= 0.0) {
                    voice->envelope = 0.0;
                    voice->noteOn = 0;
                    synth->activeVoiceCount--;
                    continue;
                } else if (voice->envelope >= voice->sustain) {
                    double decayInc = 1.0 / (voice->decay * sr + 0.0001);
                    voice->envelope -= decayInc * (1.0 - voice->sustain);
                    if (voice->envelope < voice->sustain) voice->envelope = voice->sustain;
                }
            }

            voice->lfoPhase += lfoPhaseInc;
            if (voice->lfoPhase > 1.0) voice->lfoPhase -= 1.0;
            double lfoVal = sineWave(voice->lfoPhase) * modDepth;

            voice->carrierPhase += noteFreq / sr;
            if (voice->carrierPhase > 1.0) voice->carrierPhase -= 1.0;
            voice->modPhase += noteFreq * voice->ratio / sr;
            if (voice->modPhase > 1.0) voice->modPhase -= 1.0;

            double modSample = getWave(voice->modPhase, (int)voice->modWave);
            double fmPhase = voice->carrierPhase + modSample * lfoVal;
            double carrierSample = getWave(fmPhase - floor(fmPhase), (int)voice->carrierWave);

            double subFreq = noteFreq / voice->subOctave;
            voice->subPhase += subFreq / sr;
            if (voice->subPhase > 1.0) voice->subPhase -= 1.0;
            double subSample = getWave(voice->subPhase, 3);

            double voiceSample = carrierSample * 0.7
                               + subSample * (voice->subGain / 150.0)
                               + voice->subSat / 100.0 * (carrierSample * carrierSample);
            voiceSample *= voice->envelope * voice->velocity;

            if (voice->bits > 1.0) {
                double bitFactor = pow(2.0, voice->bits);
                voiceSample = floor(voiceSample * bitFactor) / bitFactor;
            }

            int delayReadPos = (voice->delayWritePos - (int)(voice->delayTime * sr * 0.5)
                                + kDelayBufferSize) % kDelayBufferSize;
            voice->delayBuffer[voice->delayWritePos] = voiceSample;
            voiceSample = voice->delayBuffer[delayReadPos] * 0.5 + voiceSample * 0.5;
            voice->delayWritePos = (voice->delayWritePos + 1) % kDelayBufferSize;

            sample += voiceSample;
        }

        sample *= 0.25;
        sample *= synth->masterVolume;

        if (sample > 1.0) sample = 1.0;
        if (sample < -1.0) sample = -1.0;

        *left++ = (Float32)sample;
        *right++ = (Float32)sample;
    }

    return noErr;
}

static OSStatus initializeProc(void *self) {
    SynthInstance *synth = (SynthInstance *)self;
    if (!gSynth) {
        gSynth = (SynthInstance *)calloc(1, sizeof(SynthInstance));
        if (!gSynth) return kAudioUnitErr_FailedInitialization;
    }
    synth = gSynth;
    initSynth(synth);
    synth->sampleRate = gSampleRate;
    return noErr;
}

static OSStatus uninitializeProc(void *self) {
    (void)self;
    if (gSynth) {
        free(gSynth);
        gSynth = NULL;
    }
    return noErr;
}

static OSStatus getPropertyInfoProc(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope,
                                     AudioUnitElement inElement, UInt32 *outDataSize, Boolean *outWritable) {
    (void)self;
    (void)inScope;
    (void)inElement;

    switch (inID) {
        case kAudioUnitProperty_SampleRate:
            *outDataSize = sizeof(double);
            *outWritable = true;
            return noErr;

        case kAudioUnitProperty_ParameterInfo:
            *outDataSize = sizeof(AudioUnitParameterInfo);
            *outWritable = false;
            return noErr;

        case kAudioUnitProperty_ParameterList:
            *outDataSize = kParam_NumParameters * sizeof(AudioUnitParameterID);
            *outWritable = false;
            return noErr;

        default:
            return kAudioUnitErr_PropertyNotInUse;
    }
}

static OSStatus getPropertyProc(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope,
                                 AudioUnitElement inElement, void *outData, UInt32 *ioDataSize) {
    SynthInstance *synth = (SynthInstance *)self;

    switch (inID) {
        case kAudioUnitProperty_SampleRate:
            if (*ioDataSize >= sizeof(double)) {
                *(double *)outData = synth->sampleRate;
                *ioDataSize = sizeof(double);
            }
            return noErr;

        case kAudioUnitProperty_ParameterInfo: {
            if (*ioDataSize < sizeof(AudioUnitParameterInfo)) return kAudioUnitErr_InvalidPropertyValue;

            AudioUnitParameterInfo *params = (AudioUnitParameterInfo *)outData;
            int paramID = (int)inElement;

            memset(params, 0, sizeof(AudioUnitParameterInfo));

            struct { int id; const char *name; float min, max, def; } paramDefs[] = {
                {kParam_Ratio, "Ratio", 0.1f, 20.0f, 1.0f},
                {kParam_Index, "Index", 0.0f, 1000.0f, 400.0f},
                {kParam_Drive, "Drive", 0.0f, 100.0f, 50.0f},
                {kParam_Bits, "Bits", 2.0f, 16.0f, 8.0f},
                {kParam_DelayTime, "Delay", 0.0f, 100.0f, 20.0f},
                {kParam_LFORate, "LFO Rate", 0.1f, 20.0f, 3.0f},
                {kParam_Attack, "Attack", 0.001f, 2.0f, 0.01f},
                {kParam_Decay, "Decay", 0.01f, 5.0f, 0.4f},
                {kParam_Sustain, "Sustain", 0.0f, 1.0f, 0.7f},
                {kParam_Release, "Release", 0.01f, 5.0f, 0.1f},
                {kParam_CarrierWave, "Carrier", 0.0f, 3.0f, 1.0f},
                {kParam_ModWave, "Modulator", 0.0f, 3.0f, 3.0f},
                {kParam_FMEnvDepth, "FM Depth", 0.0f, 1000.0f, 400.0f},
                {kParam_SubGain, "Sub Gain", 0.0f, 150.0f, 100.0f},
                {kParam_SubOctave, "Sub Oct", 0.5f, 2.0f, 1.0f},
                {kParam_SubSat, "Sub Sat", 0.0f, 100.0f, 30.0f},
                {kParam_NoiseGain, "Noise", 0.0f, 100.0f, 25.0f},
                {kParam_NoiseCutoff, "Noise Cutoff", 200.0f, 10000.0f, 2000.0f},
                {kParam_NoiseFilterType, "Filter", 0.0f, 2.0f, 1.0f},
            };

            int numParams = sizeof(paramDefs) / sizeof(paramDefs[0]);
            if (paramID < numParams) {
                strncpy(params->name, paramDefs[paramID].name, 51);
                params->minValue = paramDefs[paramID].min;
                params->maxValue = paramDefs[paramID].max;
                params->defaultValue = paramDefs[paramID].def;
                params->unit = kAudioUnitParameterUnit_Generic;
                params->flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;
            }

            *ioDataSize = sizeof(AudioUnitParameterInfo);
            return noErr;
        }

        case kAudioUnitProperty_ParameterList:
            if (*ioDataSize >= kParam_NumParameters * sizeof(AudioUnitParameterID)) {
                AudioUnitParameterID *list = (AudioUnitParameterID *)outData;
                for (int i = 0; i < kParam_NumParameters; i++) {
                    list[i] = i;
                }
                *ioDataSize = kParam_NumParameters * sizeof(AudioUnitParameterID);
            }
            return noErr;

        default:
            return kAudioUnitErr_PropertyNotInUse;
    }
}

static OSStatus setPropertyProc(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope,
                                 AudioUnitElement inElement, const void *inData, UInt32 inDataSize) {
    SynthInstance *synth = (SynthInstance *)self;

    switch (inID) {
        case kAudioUnitProperty_SampleRate:
            if (inDataSize >= sizeof(double)) {
                synth->sampleRate = *(const double *)inData;
                gSampleRate = synth->sampleRate;
            }
            return noErr;

        case kAudioUnitProperty_ParameterValueStrings:
        case kAudioUnitProperty_SetRenderCallback:
            return noErr;

        default:
            return kAudioUnitErr_PropertyNotInUse;
    }
}

static OSStatus renderProc(void *self, AudioUnitRenderActionFlags *ioActionFlags,
                           const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber,
                           UInt32 inNumberFrames, AudioBufferList *ioData) {
    SynthInstance *synth = (SynthInstance *)self;
    return renderAudio(synth, ioData, inNumberFrames);
}

static OSStatus getParameterProc(void *self, AudioUnitParameterID inID, AudioUnitScope inScope,
                                  AudioUnitElement inElement, AudioUnitParameterValue *outValue) {
    SynthInstance *synth = (SynthInstance *)self;
    return getParameterValue(synth, inID, outValue);
}

static OSStatus setParameterProc(void *self, AudioUnitParameterID inID, AudioUnitScope inScope,
                                  AudioUnitElement inElement, AudioUnitParameterValue inValue,
                                  UInt32 inBufferOffsetInFrames) {
    SynthInstance *synth = (SynthInstance *)self;
    return setParameterValue(synth, inID, inValue, inBufferOffsetInFrames);
}

static AudioComponentMethod lookupProc(SInt16 selector) {
    switch (selector) {
        case kAudioUnitInitializeSelect:
            return (AudioComponentMethod)initializeProc;
        case kAudioUnitUninitializeSelect:
            return (AudioComponentMethod)uninitializeProc;
        case kAudioUnitGetPropertyInfoSelect:
            return (AudioComponentMethod)getPropertyInfoProc;
        case kAudioUnitGetPropertySelect:
            return (AudioComponentMethod)getPropertyProc;
        case kAudioUnitSetPropertySelect:
            return (AudioComponentMethod)setPropertyProc;
        case kAudioUnitRenderSelect:
            return (AudioComponentMethod)renderProc;
        case kAudioUnitGetParameterSelect:
            return (AudioComponentMethod)getParameterProc;
        case kAudioUnitSetParameterSelect:
            return (AudioComponentMethod)setParameterProc;
        default:
            return NULL;
    }
}

static OSStatus openProc(void *self, AudioComponentInstance mInstance) {
    (void)mInstance;
    if (!gSynth) {
        gSynth = (SynthInstance *)calloc(1, sizeof(SynthInstance));
        if (!gSynth) return kAudioUnitErr_FailedInitialization;
    }
    initSynth(gSynth);
    gSynth->sampleRate = gSampleRate;
    return noErr;
}

static OSStatus closeProc(void *self) {
    (void)self;
    if (gSynth) {
        free(gSynth);
        gSynth = NULL;
    }
    return noErr;
}

static AudioComponentPlugInInterface gAUInterface = {
    .Open = openProc,
    .Close = closeProc,
    .Lookup = lookupProc,
    .reserved = NULL
};

AudioComponentPlugInInterface *AUEntryPoint(const AudioComponentDescription *inDesc) {
    (void)inDesc;
    if (!gSynth) {
        gSynth = (SynthInstance *)calloc(1, sizeof(SynthInstance));
        if (gSynth) {
            initSynth(gSynth);
        }
    }
    return &gAUInterface;
}