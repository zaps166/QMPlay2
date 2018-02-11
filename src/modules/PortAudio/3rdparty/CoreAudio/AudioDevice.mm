/*
     File: AudioDevice.cpp
 Adapted from the CAPlayThough example
  Version: 1.2.2

 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.

 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.

 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 Copyright (C) 2013 Apple Inc. All Rights Reserved.
 Copyright (C) 2017,18 René J.V. Bertin All Rights Reserved.

*/

#include "AudioDevice.h"
#import <Cocoa/Cocoa.h>

char *OSTStr(OSType type)
{
    static union OSTStr {
        uint32_t four;
        char str[5];
    } ltype;
    ltype.four = EndianU32_BtoN(type);
    ltype.str[4] = '\0';
    return ltype.str;
}

// the sample rates that can be found in the selections proposed by Audio Midi Setup. Are these representative
// for the devices I have at my disposal, or are they determined by discrete supported values hardcoded into
// CoreAudio or the HAL? Is there any advantage in using one of these rates, as opposed to using a different rate
// on devices that support any rate in an interval?
static Float64 supportedSRateList[] = {6400, 8000, 11025, 12000, 16000, 22050,
                                       24000, 32000, 44100, 48000, 64000, 88200, 96000, 128000, 176400, 192000
                                      };
static UInt32 supportedSRates = sizeof(supportedSRateList) / sizeof(Float64);

#ifdef DEPRECATED_LISTENER_API

OSStatus DefaultListener(AudioDeviceID inDevice, UInt32 inChannel, Boolean forInput,
                         AudioDevicePropertyID inPropertyID,
                         void *inClientData)
{
    UInt32 size;
    Float64 sampleRate;
    AudioDevice *dev = (AudioDevice *) inClientData;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *msg = [NSString stringWithFormat:@"Property %s of device %u changed; data=%p",
                     OSTStr((OSType)inPropertyID), (unsigned int)inDevice, inClientData ];
    AudioObjectPropertyAddress theAddress = { inPropertyID,
                                              forInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                              kAudioObjectPropertyElementMaster
                                            };

    switch (inPropertyID) {
        case kAudioDevicePropertyNominalSampleRate:
            size = sizeof(sampleRate);
            if (AudioObjectGetPropertyData(inPropertyID, &theAddress, 0, NULL, &size, &sampleRate) == noErr
                    && (dev && !dev->listenerSilentFor)) {
                NSLog(@"%@\n\tkAudioDevicePropertyNominalSampleRate=%g\n", msg, sampleRate);
            }
            break;
        case kAudioDevicePropertyActualSampleRate:
            size = sizeof(sampleRate);
            if (AudioObjectGetPropertyData(inPropertyID, &theAddress, 0, NULL, &size, &sampleRate) == noErr && dev) {
                // update the rate we should reset to
                dev->SetInitialNominalSampleRate(sampleRate);
                if (!dev->listenerSilentFor) {
                    NSLog(@"%@\n\tkAudioDevicePropertyActualSampleRate=%g\n", msg, sampleRate);
                }
            }
            break;
        default:
            if ((dev && !dev->listenerSilentFor)) {
                NSLog(@"%@", msg);
            }
            break;
    }
    if (dev && dev->listenerSilentFor) {
        dev->listenerSilentFor -= 1;
    }
    if (pool) {
        [pool drain];
    }
    return noErr;
}

#else   // !DEPRECATED_LISTENER_API

static OSStatus DefaultListener(AudioObjectID inObjectID, UInt32 inNumberProperties,
                                const AudioObjectPropertyAddress propTable[],
                                void *inClientData)
{
    UInt32 size;
    Float64 sampleRate;
    AudioDevice *dev = static_cast<AudioDevice *>(inClientData);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    for (int i = 0 ; i < inNumberProperties ; ++i) {
        NSString *msg = [NSString stringWithFormat:@"#%d Property %s of device %u changed; data=%p", i,
                         OSTStr((OSType)propTable[i].mElement), (unsigned int)inObjectID, inClientData ];
        switch (propTable[i].mElement) {
            case kAudioDevicePropertyNominalSampleRate:
                size = sizeof(sampleRate);
                if (AudioObjectGetPropertyData(inObjectID, &propTable[i], 0, NULL, &size, &sampleRate) == noErr
                        && (dev && !dev->listenerSilentFor)
                   ) {
                    NSLog(@"%@\n\tkAudioDevicePropertyNominalSampleRate=%g\n", msg, sampleRate);
                }
                break;
            case kAudioDevicePropertyActualSampleRate:
                size = sizeof(sampleRate);
                if (AudioObjectGetPropertyData(inObjectID, &propTable[i], 0, NULL, &size, &sampleRate) == noErr && dev) {
                    // update the rate we should reset to
                    dev->SetInitialNominalSampleRate(sampleRate);
                    if (!dev->listenerSilentFor) {
                        NSLog(@"%@\n\tkAudioDevicePropertyActualSampleRate=%g\n", msg, sampleRate);
                    }
                }
                break;
            default:
                if ((dev && !dev->listenerSilentFor)) {
                    NSLog(@"%@", msg);
                }
                break;
        }
    }
    if (dev && dev->listenerSilentFor) {
        dev->listenerSilentFor -= 1;
    }
    if (pool) {
        [pool drain];
    }
    return noErr;
}
#endif

void AudioDevice::Init(AudioPropertyListenerProc lProc = DefaultListener)
{
    if (mID == kAudioDeviceUnknown) {
        return;
    }
    OSStatus err = noErr;

    // getting the device name can be surprisingly slow, so we get and cache it here
    GetName();

    UInt32 propsize = sizeof(Float32);

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertySafetyOffset,
                                              mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                              kAudioObjectPropertyElementMaster
                                            }; // channel
    verify_noerr(AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propsize, &mSafetyOffset));

    propsize = sizeof(UInt32);
    theAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
    verify_noerr(AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propsize, &mBufferSizeFrames));

    listenerProc = lProc;
    listenerSilentFor = 0;
    if (lProc) {
#ifdef DEPRECATED_LISTENER_API
        AudioDeviceAddPropertyListener(mID, 0, false, kAudioDevicePropertyActualSampleRate, lProc, this);
        AudioDeviceAddPropertyListener(mID, 0, false, kAudioDevicePropertyNominalSampleRate, lProc, this);
        AudioDeviceAddPropertyListener(mID, 0, false, kAudioHardwarePropertyDefaultOutputDevice, lProc, this);
#else
        AudioObjectPropertyAddress prop = { kAudioDevicePropertyActualSampleRate,
                                            kAudioObjectPropertyScopeGlobal,
                                            kAudioObjectPropertyElementMaster
                                          };
        if ((err = AudioObjectAddPropertyListener(mID, &prop, lProc, this)) != noErr) {
            NSLog(@"Couldn't register property listener for actual sample rate: %d (%s)", err, OSTStr(err));
        }
        prop.mElement = kAudioDevicePropertyNominalSampleRate;
        if ((err = AudioObjectAddPropertyListener(mID, &prop, lProc, this)) != noErr) {
            NSLog(@"Couldn't register property listener for nominal sample rate: %d (%s)", err, OSTStr(err));
        }
        prop.mElement = kAudioHardwarePropertyDefaultOutputDevice;
        if ((err = AudioObjectAddPropertyListener(mID, &prop, lProc, this)) != noErr) {
            NSLog(@"Couldn't register property listener for selected default device: %d (%s)", err, OSTStr(err));
        }
#endif
    } else {
        NSLog(@"Warning: no CoreAudio event listener has been defined");
    }
    propsize = sizeof(Float64);
    theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    verify_noerr(AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propsize, &currentNominalSR));
    propsize = sizeof(AudioStreamBasicDescription);
    theAddress.mSelector = kAudioDevicePropertyStreamFormat;
    verify_noerr(AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propsize, &mInitialFormat));
    mFormat = mInitialFormat;
    propsize = 0;
    theAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
    // attempt to build a list of the supported sample rates
    if ((err = AudioObjectGetPropertyDataSize(mID, &theAddress, 0, NULL, &propsize)) == noErr) {
        AudioValueRange *list;
        // use a fall-back value of 100 supported rates:
        if (propsize == 0) {
            propsize = 100 * sizeof(AudioValueRange);
        }
        if ((list = (AudioValueRange *) calloc(1, propsize))) {
            theAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
            err = AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propsize, list);
            if (err == noErr) {
                UInt32 i;
                NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
                nominalSampleRates = propsize / sizeof(AudioValueRange);
                NSMutableArray *a = [NSMutableArray arrayWithCapacity:nominalSampleRates];
                minNominalSR = list[0].mMinimum;
                maxNominalSR = list[0].mMaximum;
                // store the returned sample rates in [a] and record the extreme values
                for (i = 0 ; i < nominalSampleRates ; i++) {
                    if (minNominalSR > list[i].mMinimum) {
                        minNominalSR = list[i].mMinimum;
                    }
                    if (maxNominalSR < list[i].mMaximum) {
                        maxNominalSR = list[i].mMaximum;
                    }
                    if (a) {
                        if (list[i].mMinimum != list[i].mMaximum) {
                            UInt32 j;
                            discreteSampleRateList = false;
                            // the 'guessing' case: the device specifies one or more ranges, without
                            // indicating which rates in that range(s) are supported. We assume the
                            // rates that Audio Midi Setup shows.
                            for (j = 0 ; j < supportedSRates ; j++) {
                                if (supportedSRateList[j] >= list[i].mMinimum
                                        && supportedSRateList[j] <= list[i].mMaximum
                                   ) {
                                    [a addObject:[NSNumber numberWithDouble:supportedSRateList[j]]];
                                }
                            }
                        } else {
                            // there's at least one part of the sample rate list that contains discrete
                            // supported values. I don't know if there are devices that "do this" or if they all
                            // either give discrete rates or a single continuous range. So we take the easy
                            // opt-out solution: this only costs a few cycles attempting to match a requested
                            // non-listed rate (with the potential "risk" of matching to a listed integer multiple,
                            // which should not cause any aliasing).
                            discreteSampleRateList = true;
                            // the easy case: the device specifies one or more discrete rates
                            if (![a containsObject:[NSNumber numberWithDouble:list[i].mMinimum]]) {
                                [a addObject:[NSNumber numberWithDouble:list[i].mMinimum]];
                            }
                        }
                    }
                }
                if (a) {
                    // sort the array (should be the case but one never knows)
                    [a sortUsingSelector:@selector(compare:)];
                    // retrieve the number of unique rates:
                    nominalSampleRates = [a count];
                    // now copy the rates into a simple C array for faster access
                    if ((nominalSampleRateList = new Float64[nominalSampleRates])) {
                        for (i = 0 ; i < nominalSampleRates ; i++) {
                            nominalSampleRateList[i] = [[a objectAtIndex:i] doubleValue];
                        }
                    }
                    NSLog(@"Using audio device %u \"%s\", %u sample rates in %lu range(s); [%g,%g] %@; current sample rate %gHz",
                          mID, GetName(), nominalSampleRates, propsize / sizeof(AudioValueRange),
                          minNominalSR, maxNominalSR, (discreteSampleRateList) ? [a description] : @"continuous", currentNominalSR);
                } else {
                    NSLog(@"Using audio device %u \"%s\", %u sample rates in %lu range(s); [%g,%g] %s; current sample rate %gHz",
                          mID, GetName(), nominalSampleRates, propsize / sizeof(AudioValueRange),
                          minNominalSR, maxNominalSR, (discreteSampleRateList) ? "" : "continuous", currentNominalSR);
                }
                // [a] will be flushed down the drain:
                [pool drain];
            }
            free(list);
        }
        mInitialised = true;
    }
}

AudioDevice::AudioDevice()
    : mID(kAudioDeviceUnknown)
    , mForInput(false)
{
    listenerProc = NULL;
}

AudioDevice::AudioDevice(AudioDeviceID devid, bool forInput)
    : mID(devid)
    , mForInput(forInput)
{
    Init(DefaultListener);
}

AudioDevice::AudioDevice(AudioDeviceID devid, bool quick, bool forInput)
    : mID(devid)
    , mForInput(forInput)
{
    if (!quick) {
        Init(DefaultListener);
    }
}

AudioDevice::AudioDevice(AudioDeviceID devid, AudioPropertyListenerProc lProc, bool forInput)
    : mID(devid)
    , mForInput(forInput)
{
    Init(lProc);
}

AudioDevice::~AudioDevice()
{
    if (mID != kAudioDeviceUnknown && mInitialised) {
        OSStatus err;
        AudioDeviceID devId = mID;
        // RJVB 20120902: setting the StreamFormat to the initially read values will set the channel bitdepth to 16??
        // so we reset just the nominal sample rate.
        err = SetNominalSampleRate(mInitialFormat.mSampleRate);
        if (err != noErr) {
            fprintf(stderr, "Cannot reset initial settings for device %u (%s): err %s, %ld\n",
                    (unsigned int) mID, GetName(), OSTStr(err), (long) err);
        }
        if (listenerProc) {
#ifdef DEPRECATED_LISTENER_API
            AudioDeviceRemovePropertyListener(mID, 0, false, kAudioDevicePropertyActualSampleRate, listenerProc);
            AudioDeviceRemovePropertyListener(mID, 0, false, kAudioDevicePropertyNominalSampleRate, listenerProc);
            AudioDeviceRemovePropertyListener(mID, 0, false, kAudioHardwarePropertyDefaultOutputDevice, listenerProc);
#else
            AudioObjectPropertyAddress prop = { kAudioDevicePropertyActualSampleRate,
                                                kAudioObjectPropertyScopeGlobal,
                                                kAudioObjectPropertyElementMaster
                                              };
            verify_noerr(AudioObjectRemovePropertyListener(mID, &prop, listenerProc, this));
            prop.mElement = kAudioDevicePropertyNominalSampleRate;
            verify_noerr(AudioObjectRemovePropertyListener(mID, &prop, listenerProc, this));
            prop.mElement = kAudioHardwarePropertyDefaultOutputDevice;
            verify_noerr(AudioObjectRemovePropertyListener(mID, &prop, listenerProc, this));
#endif
        }
        if (nominalSampleRateList) {
            delete nominalSampleRateList;
        }
        NSLog(@"AudioDevice %s (%u) released", mDevName, devId);
    }
}

void AudioDevice::SetBufferSize(UInt32 size)
{
    UInt32 propsize = sizeof(UInt32);
    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyBufferFrameSize,
                                              mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                              kAudioObjectPropertyElementMaster
                                            }; // channel

    verify_noerr(AudioObjectSetPropertyData(mID, &theAddress, 0, NULL, propsize, &size));
    verify_noerr(AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propsize, &mBufferSizeFrames));
}

OSStatus AudioDevice::NominalSampleRate(Float64 &sampleRate)
{
    UInt32 size = sizeof(Float64);
    OSStatus err;
    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyNominalSampleRate,
                                              mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                              kAudioObjectPropertyElementMaster
                                            };
    err = AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &size, &sampleRate);
    if (err == noErr) {
        currentNominalSR = sampleRate;
    }
    return err;
}

inline Float64 AudioDevice::ClosestNominalSampleRate(Float64 sampleRate)
{
    if (sampleRate > 0) {
#ifndef FORCE_STANDARD_SAMPLERATES
        if (!discreteSampleRateList && sampleRate >= minNominalSR && sampleRate <= maxNominalSR) {
            // the device suggests it supports this exact sample rate; use it.
            listenerSilentFor = 0;
            return sampleRate;
        }
#endif
        if (nominalSampleRateList && sampleRate >= minNominalSR && sampleRate <= maxNominalSR) {
            Float64 minRemainder = 1;
            Float64 closest = 0;
            for (UInt32 i = 0 ; i < nominalSampleRates ; i++) {
                // check if we have a hit:
                if (sampleRate == nominalSampleRateList[i]) {
                    return sampleRate;
                }
                double dec, ent;
                dec = modf(nominalSampleRateList[i] / sampleRate, &ent);
                // if the rate at i is an integer multiple of the requested sample rate.
                if (dec == 0) {
                    listenerSilentFor = 0;
                    return nominalSampleRateList[i];
                } else if ((1 - dec) < minRemainder) {
                    // find the match that is closest to either 1*sampleRate or 2*sampleRate
                    // IOW, the fractional part of nominalSampleRate/sampleRate is closest
                    // either to 0 or to 1 .
                    minRemainder = dec;
                    closest = nominalSampleRateList[i];
                }
            }
            if (closest > 0) {
                listenerSilentFor = 0;
                return closest;
            }
        }
        static bool pass2 = false;
        if (!pass2) {
            Float64 sr = sampleRate;
            int fact = 1;
            // if we're here it's either because there's no list of known supported rates,
            // or we didn't find an integer multiple of the requested rate in the list.
            // scale up as required in steps of 2
            while (sampleRate * fact < minNominalSR && sampleRate * (fact + 1) <= maxNominalSR) {
                fact += 1;
            }
            sampleRate *= fact;
            fact = 1;
            // scale down as required in steps of 2
            while (sampleRate / fact > maxNominalSR && sampleRate / (fact + 1) >= minNominalSR) {
                fact += 1;
            }
            sampleRate /= fact;
            if (sr != sampleRate) {
                pass2 = true;
                // we're now in range, do another pass to find a matching supported rate
                sampleRate = ClosestNominalSampleRate(sampleRate);
                pass2 = false;
                listenerSilentFor = 0;
            } else {
                if (sampleRate > maxNominalSR) {
                    sampleRate = maxNominalSR;
                } else {
                    sampleRate = minNominalSR;
                }
            }
            // note that we really ought to resample the content if we're sending it to a 
            // device running at a lower sample rate!
        }
    }
    return sampleRate;
}

OSStatus AudioDevice::SetNominalSampleRate(Float64 sampleRate, Boolean force)
{
    UInt32 size = sizeof(Float64);
    OSStatus err;
    if (sampleRate <= 0) {
        return paramErr;
    }
    listenerSilentFor = 2;
    Float64 sampleRate2 = ClosestNominalSampleRate(sampleRate);
    NSLog(@"SetNominalSampleRate(%g) setting rate to %gHz", sampleRate, sampleRate2);
    if (sampleRate2 != currentNominalSR || force) {
        AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyNominalSampleRate,
                                                  mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                                  kAudioObjectPropertyElementMaster
                                                };
        err = AudioObjectSetPropertyData(mID, &theAddress, 0, NULL, size, &sampleRate2);
        if (err == noErr) {
            currentNominalSR = sampleRate2;
        } else {
            NSLog(@"Failure setting device \"%s\" to %gHz: %d (%s)", GetName(), sampleRate2, err, OSTStr(err));
        }
    } else {
        err = noErr;
    }
    return err;
}

/*!
    Reset the nominal sample rate to the value found when opening the device
 */
OSStatus AudioDevice::ResetNominalSampleRate(Boolean force)
{
    UInt32 size = sizeof(Float64);
    Float64 sampleRate = mInitialFormat.mSampleRate;
    OSStatus err = noErr;
    if (sampleRate != currentNominalSR || force) {
        listenerSilentFor = 2;
        AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyNominalSampleRate,
                                                  mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                                  kAudioObjectPropertyElementMaster
                                                };
        err = AudioObjectSetPropertyData(mID, &theAddress, 0, NULL, size, &sampleRate);
        if (err == noErr) {
            currentNominalSR = sampleRate;
        }
    }
    return err;
}

OSStatus AudioDevice::SetStreamBasicDescription(AudioStreamBasicDescription *desc)
{
    UInt32 size = sizeof(AudioStreamBasicDescription);
    OSStatus err;
    listenerSilentFor = 1;
    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreamFormat,
                                              mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                              kAudioObjectPropertyElementMaster
                                            };
    err = AudioObjectSetPropertyData(mID, &theAddress, 0, NULL, size, desc);
    if (err == noErr) {
        currentNominalSR = desc->mSampleRate;
    }
    return err;
}

// AudioDeviceGetPropertyInfo() is deprecated, so we wrap AudioObjectGetPropertyDataSize().
OSStatus AudioDevice::GetPropertyDataSize(AudioObjectPropertySelector property, UInt32 *size, AudioObjectPropertyAddress *propertyAddress)
{
    AudioObjectPropertyAddress l_propertyAddress;
    if (!propertyAddress) {
        propertyAddress = &l_propertyAddress;
    }
    propertyAddress->mSelector = property;
    propertyAddress->mScope = (mForInput) ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
    propertyAddress->mElement = kAudioObjectPropertyElementMaster;

    return AudioObjectGetPropertyDataSize(mID, propertyAddress, 0, NULL, size);
}

int AudioDevice::CountChannels()
{
    OSStatus err;
    UInt32 propSize;
    AudioObjectPropertyAddress theAddress;
    int result = 0;

    err = GetPropertyDataSize(kAudioDevicePropertyStreamConfiguration, &propSize, &theAddress);
    if (err) {
        return 0;
    }

    AudioBufferList *buflist = (AudioBufferList *)malloc(propSize);
    err = AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &propSize, buflist);
    if (!err) {
        for (UInt32 i = 0; i < buflist->mNumberBuffers; ++i) {
            result += buflist->mBuffers[i].mNumberChannels;
        }
    }
    free(buflist);
    return result;
}

char *AudioDevice::GetName(char *buf, UInt32 maxlen)
{
    if (!buf) {
        buf = mDevName;
        maxlen = sizeof(mDevName) / sizeof(char);
        if (*buf) {
            return buf;
        }
    }
    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyDeviceName,
                                              mForInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                                              kAudioObjectPropertyElementMaster
                                            }; // channel

    verify_noerr(AudioObjectGetPropertyData(mID, &theAddress, 0, NULL, &maxlen, buf));

    return buf;
}

AudioDevice *AudioDevice::GetDefaultDevice(Boolean forInput, OSStatus &err, AudioDevice *dev)
{
    UInt32 propsize;
    AudioDeviceID defaultDeviceID;

    propsize = sizeof(AudioDeviceID);
    AudioObjectPropertyAddress theAddress = {
        forInput ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &defaultDeviceID);
    if (err == noErr) {
        if (dev) {
            if (dev->ID() != defaultDeviceID) {
                delete dev;
            } else {
                goto bail;
            }
        }
        dev = new AudioDevice(defaultDeviceID, forInput);
    }
bail:
    ;
    return dev;
}

AudioDevice *AudioDevice::GetDevice(AudioDeviceID devId, Boolean forInput, AudioDevice *dev)
{
    if (dev) {
        if (dev->ID() != devId) {
            delete dev;
        } else {
            goto bail;
        }
    }
    dev = new AudioDevice(devId, forInput);
bail:
    ;
    return dev;
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
