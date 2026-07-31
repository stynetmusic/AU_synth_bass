#include "fm_synth.h"
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreMIDI/MIDIServices.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define kDelayBufferSize 2048
#define kMaxVoices 16

static SynthInstance *gSynth = NULL;
static double gSampleRate = 44100.0;

static double sawTooth(double phase) {
    double p = phase - floor(phase);
    return 2.0 * (p - 0.5);
}

static double squareWave(double phase) {
    double p = phase - floor(phase);
    return (p < 0.5) ? 1.0 : -1.0;
}

static double triWave(double phase) {
    double p = phase - floor(phase);
    return 2.0 * (1.0 - 4.0 * fabs(p - 0.5));
}

static double sineWave(double phase) {
    double p = phase - floor(phase);
    return sin(2.0 * M_PI * p);
}

static double getWave(double phase, int type) {
    switch (type) {
        case 0: return squareWave(phase);
        case 1: return sawTooth(phase);
        case 2: return triWave(phase);
        case 3: default: return sineWave(phase);
    }
}

static void initSynth(SynthInstance *synth) {
    memset(synth, 0, sizeof(SynthInstance));
    synth->masterVolume = 0.8;
    synth->sampleRate = 44100.0;
    synth->activeVoiceCount = 0;
    for (int i = 0; i < 128; i++) {
        synth->notes[i] = 0;
    }
}

static void noteOnVoice(SynthInstance *synth, int note, double velocity) {
    if (synth->notes[note] > 0) return;
    synth->notes[note] = 1;
    
    for (int v = 0; v < kMaxVoices; v++) {
        Voice *voice = &synth->voices[v];
        if (!voice->noteOn) {
            voice->note = note;
            voice->velocity = velocity;
            voice->noteOn = 1;
            voice->isReleased = 0;
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

static void noteOffVoice(SynthInstance *synth, int note) {
    if (synth->notes[note] == 0) return;
    synth->notes[note] = 0;
    
    for (int v = 0; v < kMaxVoices; v++) {
        Voice *voice = &synth->voices[v];
        if (voice->noteOn && voice->note == note) {
            voice->isReleased = 1;
            break;
        }
    }
}

OSStatus AUEntryPoint(AudioComponentInstance inInstance,
                       UInt32 inMessage,
                       const void *inParams) {
    switch (inMessage) {
        case 0: /* kAudioUnitMessage_Initialize */
            if (!gSynth) {
                gSynth = (SynthInstance *)calloc(1, sizeof(SynthInstance));
                if (!gSynth) return kAudioUnitErr_FailedInitialization;
            }
            initSynth(gSynth);
            gSynth->sampleRate = gSampleRate;
            return noErr;
            
        case 1: /* kAudioUnitMessage_Terminate */
            if (gSynth) {
                free(gSynth);
                gSynth = NULL;
            }
            return noErr;
            
        case 2: /* kAudioUnitMessage_SetProperty */
            if (!inParams) return kAudioUnitErr_InvalidPropertyValue;
            return noErr;
            
        case 3: /* kAudioUnitMessage_GetProperty */
            if (!inParams) return kAudioUnitErr_InvalidPropertyValue;
            return noErr;
            
        case 4: /* kAudioUnitMessage_SetRenderCallback */
            return noErr;
            
        default:
            return noErr;
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
    double sr = synth->sampleRate;

    for (UInt32 i = 0; i < inNumberFrames; i++) {
        double sample = 0.0;

        for (int v = 0; v < kMaxVoices; v++) {
            Voice *voice = &synth->voices[v];
            if (!voice->noteOn) continue;

            double noteFreq = 220.0 * pow(2.0, (voice->note - 57) / 12.0);
            double lfoPhaseInc = 3.0 / sr;
            double modDepth = 400.0 / 100.0;

            if (!voice->isReleased) {
                voice->envelope += 1.0 / (0.01 * sr + 0.0001);
                if (voice->envelope > 1.0) voice->envelope = 1.0;
            } else {
                voice->envelope -= 1.0 / (0.1 * sr + 0.0001);
                if (voice->envelope <= 0.0) {
                    voice->envelope = 0.0;
                    voice->noteOn = 0;
                    synth->activeVoiceCount--;
                    continue;
                } else if (voice->envelope >= 0.7) {
                    voice->envelope -= (1.0 - 0.7) / (0.4 * sr + 0.0001);
                    if (voice->envelope < 0.7) voice->envelope = 0.7;
                }
            }

            voice->lfoPhase += lfoPhaseInc;
            if (voice->lfoPhase > 1.0) voice->lfoPhase -= 1.0;
            double lfoVal = sineWave(voice->lfoPhase) * modDepth;

            voice->carrierPhase += noteFreq / sr;
            if (voice->carrierPhase > 1.0) voice->carrierPhase -= 1.0;
            voice->modPhase += noteFreq * 1.0 / sr;
            if (voice->modPhase > 1.0) voice->modPhase -= 1.0;

            double modSample = getWave(voice->modPhase, 3);
            double fmPhase = voice->carrierPhase + modSample * lfoVal;
            double carrierSample = getWave(fmPhase - floor(fmPhase), 1);

            double subFreq = noteFreq / 1.0;
            voice->subPhase += subFreq / sr;
            if (voice->subPhase > 1.0) voice->subPhase -= 1.0;
            double subSample = getWave(voice->subPhase, 3);

            double voiceSample = carrierSample * 0.7
                               + subSample * (100.0 / 150.0)
                               + 30.0 / 100.0 * (carrierSample * carrierSample);
            voiceSample *= voice->envelope * voice->velocity;

            if (voice->bits > 1.0) {
                double bitFactor = pow(2.0, voice->bits);
                voiceSample = floor(voiceSample * bitFactor) / bitFactor;
            }

            int delayReadPos = (voice->delayWritePos - (int)(20.0 * sr * 0.5)
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