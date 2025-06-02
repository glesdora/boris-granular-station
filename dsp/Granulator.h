/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#pragma once

#include "RNBO_Common.h"
#include "RNBO_AudioSignal.h"

namespace RNBO {

#define trunc(x) ((Int)(x))

#if defined(__GNUC__) || defined(__clang__)
#define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

class Granulator : public PatcherInterfaceImpl {
public:

    struct voiceState {
        bool isActive;
        bool hasGrainInQueue;
        SampleIndex endOfGrain;
    };

    class RTGrainVoice {
    public:

        RTGrainVoice(Granulator* voicesOwner, int voiceIndex);
        ~RTGrainVoice();

        void process(const SampleValue* const* inputs, Index numInputs, SampleValue* const* outputs, Index numOutputs, Index n);
        void prepareToProcess(number sampleRate, Index maxBlockSize, bool force);

        void setupVoice(SampleIndex trigatindex, const list& v);

        Granulator* getOwner() const;
        Index voice() const;
        Index getIsMuted() const;
        void setIsMuted(Index v);

        void initialize();
        void processDataViewUpdate(DataRefIndex index, MillisecondTime time);
        void allocateDataRefs();

    protected:
        number wrap(number x, number low, number high);

        void processBrain(
            const Sample in1,
            const Sample in2,
            const Sample in3,
            const Sample in4,
            const Sample in5,
            const Sample in6,
            const Sample in7,
            const Sample in8,
            const Sample* in9,
            SampleValue* out1,
            SampleValue* out2,
            Index n
        );

        void signalaccum(
            const SampleValue* in1,
            SampleValue* out,
            Index n
        );

        number position_value = 0;
        number grainsize_value = 0;
        number direction_value = 0;
        number pishift_value = 0;
        number volume_value = 0;
        number panning_value = 0;

        signal zeroBuffer = nullptr;
        signal dummyBuffer = nullptr;
        SampleValue* realtime_grain_out[2];
        bool didAllocateSignals = false;
        Index vs = 0;
        Index maxvs = 0;
        number sr = 44100;
        number invsr = 0.00002267573696;

        SampleIndex triggerindex = -1;
        bool hasGrainInQueue = false;
        SampleIndex endOfGrain = 0;
        bool isMuted = true;
        Index _voiceIndex;

        Float32BufferRef rtaudiobuf;
        Float32BufferRef intrpenvbuf;
        bool play_inc_history = false;
        bool env_inc_history = false;
        SampleIndex envelope_counter_count = 0;
        SampleIndex audio_counter_count = 0;
        number rel_start_pos_history = 0;
        number start_play_pos_history = 0;

        number reverse_offset = 0;

        Granulator* _voiceOwner;

    };

    Granulator();
    ~Granulator();

    Granulator* getTopLevelPatcher();

    void cancelClockEvents();

    template <typename T> void listquicksort(T& arr, T& sortindices, Int l, Int h, bool ascending);
    template <typename T> Int listpartition(T& arr, T& sortindices, Int l, Int h, bool ascending);
    template <typename T> void listswapelements(T& arr, Int a, Int b);
    array<ListNum, 2> listcompare(const list& input1, const list& input2);

    MillisecondTime currenttime();
   
    ParameterValue tonormalized(ParameterIndex index, ParameterValue value);
    ParameterValue fromnormalized(ParameterIndex index, ParameterValue normalizedValue);

    Index getNumMidiInputPorts() const;
    void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length);
    Index getNumMidiOutputPorts() const;

    void process(const SampleValue* const* inputs, Index numInputs, SampleValue* const* outputs, Index numOutputs, Index n);
    void prepareToProcess(number sampleRate, Index maxBlockSize, bool force);

    void setProbingTarget(MessageTag id);
    void setProbingIndex(ProbingIndex);
    Index getProbingChannels(MessageTag outletId) const;

    DataRef* getDataRef(DataRefIndex index);
    DataRefIndex getNumDataRefs() const;
    void fillDataRef(DataRefIndex, DataRef&);
    void zeroDataRef(DataRef& ref);
    void processDataViewUpdate(DataRefIndex index, MillisecondTime time);

    void initialize();

    Index getPatcherSerial() const;

    void getState(PatcherStateInterface&);
    void setState();
    void getPreset(PatcherStateInterface& preset);
    void setPreset(MillisecondTime time, PatcherStateInterface& preset);

    void processTempoEvent(MillisecondTime time, Tempo tempo);
    void processTransportEvent(MillisecondTime time, TransportState state);
    void processBeatTimeEvent(MillisecondTime time, BeatTime beattime);
    void processTimeSignatureEvent(MillisecondTime time, int numerator, int denominator);

    void onSampleRateChanged(double samplerate);

    void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time);
    void processParameterBangEvent(ParameterIndex index, MillisecondTime time);
    void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time);
    
    void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time);
    ParameterValue getParameterValue(ParameterIndex index);
    ConstCharPointer getParameterName(ParameterIndex index) const;
    ConstCharPointer getParameterId(ParameterIndex index) const;
    void getParameterInfo(ParameterIndex index, ParameterInfo* info) const;

    ParameterValue applyJuceSkew(ParameterValue normalizedValue, ParameterValue exp) const;
    ParameterValue applyJuceDeskew(ParameterValue normalizedValue, ParameterValue exp) const;
    ParameterValue applyPitchDenormalization(ParameterValue value, ParameterValue min, ParameterValue max) const;
    ParameterValue applyPitchNormalization(ParameterValue value, ParameterValue min, ParameterValue max) const;
    ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const;
    ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const;
    ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const;

    ParameterIndex getNumSignalInParameters() const;
    ParameterIndex getNumSignalOutParameters() const;
    ParameterIndex getNumParameters() const;
    ParameterIndex getParameterOffset(BaseInterface* subpatcher) const;

    void sendParameter(ParameterIndex index, bool ignoreValue);
    void scheduleParamInit(ParameterIndex index, Index order);

    void processParamInitEvents();
    void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value);
    void processOutletAtCurrentTime(EngineLink*, OutletIndex, ParameterValue);
    void processOutletEvent(EngineLink* sender, OutletIndex index, ParameterValue value, MillisecondTime time);
    void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload);
    void processListMessage(MessageTag tag,MessageTag objectId,MillisecondTime time,const list& payload);
    void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time);

    MessageTagInfo resolveTag(MessageTag tag) const;
    MessageIndex getNumMessages() const;
    const MessageInfo& getMessageInfo(MessageIndex index) const;

    DataRef& getAudioBufferDataRef();
    DataRef& getEnvelopeBufferDataRef();

    void updateVoiceState(int voice, voiceState voicestate);

protected:
    number clip(number v, number inf, number sup);
    number maximum(number x, number y);
    inline number safediv(number num, number denom);

    number samplerate() const;
    Index vectorsize() const;
    number mstosamps(MillisecondTime ms) const;
    MillisecondTime sampstoms(number samps)const;
    number tempo();
    number mstobeats(number ms);
    number beatstohz(number beattime);
    number hztobeats(number hz);
    number tickstohz(number ticks);
    number beattimeatsample(number offset);
    number transportatsample(SampleIndex sampleOffset);

    Index getMaxBlockSize() const;
    number getSampleRate() const;
    bool hasFixedVectorSize() const;
    Index getNumInputChannels() const;
    Index getNumOutputChannels() const;

    void empty_audio_buffer();

    int pickTargetVoice(SampleIndex ti);
    void setGrainProperties(SampleIndex trigatindex);

    number setGrainSize(number len, number rle);
    number setGrainPosition(number posinsamps, number drfinsamps, number leninsamps, number psh, number rpt, number intelligent_offset);
    number setGrainDirection(number frp);
    number setGrainPshift(number psh, number rpt);
    number setGrainVol(number rvo);
    number setGrainPan(number pwi);

    void param_getPresetValue(PatcherStateInterface& preset, Index paramIndex);
    void param_setPresetValue(PatcherStateInterface& preset, Index paramIndex, MillisecondTime time);

    void generate_triggers(bool mut, number len, number den, number cha, number rdl, bool sync, int notetempo, int noterythm,
                            const Sample* sync_n_phasor, const Sample* sync_nd_phasor, const Sample* sync_nt_phasor, Index n);
    void delayRecArrest(number glength, bool scroll, SampleValue* out1, Index n);
    void reverseOffsetHandler(bool frz, number len, SampleValue* out1, Index n);

    void phasor_n_sync_perform(number freq, SampleValue* out, Index n);
    void phasor_nd_sync_perform(number freq, SampleValue* out, Index n);
    void phasor_nt_sync_perform(number freq, SampleValue* out, Index n);
    void phasor_n_sync_dspsetup(bool force);
    void phasor_nd_sync_dspsetup(bool force);
    void phasor_nt_sync_dspsetup(bool force);
    void timevalue_01_sendValue();
    void timevalue_02_sendValue();
    void timevalue_03_sendValue();

    number freerunningphasor_next(number freq, number reset);
    void freerunningphasor_reset();
    void freerunningphasor_dspsetup();

    void recordtilde_01_perform(const Sample* record, number begin, number end, const SampleValue* input1, SampleValue* sync, Index n);
    void recordtilde_02_perform(const Sample* record, number begin, number end, const SampleValue* input1, SampleValue* sync, Index n);

    void stackprotect_perform(Index n);
    bool stackprotect_check();

    void limi_01_perform(const SampleValue* input1, const SampleValue* input2, SampleValue* output1, SampleValue* output2, Index n);
    void limi_01_lookahead_setter(number v);
    void limi_01_preamp_setter(number v);
    void limi_01_postamp_setter(number v);
    void limi_01_threshold_setter(number v);
    number limi_01_dc1_next(number x, number gain);
    void limi_01_dc1_reset();
    void limi_01_dc1_dspsetup();
    number limi_01_dc2_next(number x, number gain);
    void limi_01_dc2_reset();
    void limi_01_dc2_dspsetup();
    number limi_01_dc_next(Index i, number x, number gain);
    void limi_01_dc_reset(Index i);
    void limi_01_dc_dspsetup(Index i);
    void limi_01_reset();
    void limi_01_dspsetup(bool force);

    void dcblock_tilde_01_perform(const Sample* x, number gain, SampleValue* out1, Index n);
    void dcblock_tilde_01_reset();
    void dcblock_tilde_01_dspsetup(bool force);

    Index globaltransport_getSampleOffset(MillisecondTime time);
    number globaltransport_getTempoAtSample(SampleIndex sampleOffset);
    number globaltransport_getStateAtSample(SampleIndex sampleOffset);
    number globaltransport_getState(MillisecondTime time);
    number globaltransport_getBeatTime(MillisecondTime time);
    bool globaltransport_setTempo(MillisecondTime time, number tempo, bool notify);
    number globaltransport_getTempo(MillisecondTime time);
    bool globaltransport_setState(MillisecondTime time, number state, bool notify);
    bool globaltransport_setBeatTime(MillisecondTime time, number beattime, bool notify);
    number globaltransport_getBeatTimeAtSample(SampleIndex sampleOffset);
    array<number, 2> globaltransport_getTimeSignature(MillisecondTime time);
    array<number, 2> globaltransport_getTimeSignatureAtSample(SampleIndex sampleOffset);
    bool globaltransport_setTimeSignature(MillisecondTime time, number numerator, number denominator, bool notify);
    void globaltransport_advance();
    void globaltransport_dspsetup(bool force);

    void allocateDataRefs();
    void sendOutlet(OutletIndex index, ParameterValue value);
    void startup();
    void updateTime(MillisecondTime time);
    void assign_defaults();

    number limi_01_bypass;
    number limi_01_dcblock;
    number limi_01_lookahead;
    number limi_01_preamp;
    number limi_01_postamp;
    number limi_01_threshold;
    number limi_01_release;

    number paramvalues[21];
    number paramlastvalues[21];
    number normalizedparamvalues[21];

    number phasor_01_freq;
    number phasor_02_freq;
    number phasor_03_freq;
    number recordtilde_01_record;
    number recordtilde_01_begin;
    number recordtilde_01_end;
    number recordtilde_01_loop;
    number recordtilde_02_record;
    number recordtilde_02_begin;
    number recordtilde_02_end;
    number recordtilde_02_loop;
    number dcblock_tilde_01_x;
    number dcblock_tilde_01_gain;
    number ctlin_01_input;
    number ctlin_01_controller;
    number ctlin_01_channel;

    MillisecondTime _currentTime;
    SampleIndex audioProcessSampleCount;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue* signals[6];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    SampleValue limi_01_lookaheadBuffers[2][128] = { };
    SampleValue limi_01_gainBuffer[128] = { };
    number limi_01_last;
    int limi_01_lookaheadIndex;
    number limi_01_recover;
    number limi_01_lookaheadInv;
    number limi_01_dc1_xm1;
    number limi_01_dc1_ym1;
    number limi_01_dc2_xm1;
    number limi_01_dc2_ym1;
    bool limi_01_setupDone;
    number codebox_01_mphasor_currentPhase;
    number codebox_01_mphasor_conv;

    voiceState voiceStates[24] = { };

    number gentrggs_syncphase_old;
    number gentrggs_freerunphase_old;
    number codebox_tilde_01_mphasor_currentPhase;
    number codebox_tilde_01_mphasor_conv;
    bool codebox_tilde_01_setupDone;
    signal phasor_01_sigbuf;
    number phasor_01_lastLockedPhase;
    number phasor_01_conv;
    bool phasor_01_setupDone;
    signal phasor_02_sigbuf;
    number phasor_02_lastLockedPhase;
    number phasor_02_conv;
    bool phasor_02_setupDone;
    signal phasor_03_sigbuf;
    number phasor_03_lastLockedPhase;
    number phasor_03_conv;
    bool phasor_03_setupDone;
    number recpointer_at_scroll;
    SampleIndex delrecstop_delaysamps;
    bool delrecstop_record;
    bool delrecstop_scrollhistory;
    SampleIndex delrecstop_count;
    bool delrecstop_inc;
    Float32BufferRef recordtilde_01_buffer;
    SampleIndex recordtilde_01_wIndex;
    number recordtilde_01_lastRecord;
    Float32BufferRef recordtilde_02_buffer;
    SampleIndex recordtilde_02_wIndex;
    number recordtilde_02_lastRecord;
    Float32BufferRef bufferop_01_buffer;
    number dcblock_tilde_01_xm1;
    number dcblock_tilde_01_ym1;
    bool dcblock_tilde_01_setupDone;
    signal feedbackbuffer;
    SampleIndex revoffhndlr_offset;
    bool revoffhndlr_frzhistory;
    SampleIndex revoffhndlr_c;
    bool revoffhndlr_hit;
    int ctlin_01_status;
    int ctlin_01_byte1;
    int ctlin_01_inchan;
    signal globaltransport_tempo;
    bool globaltransport_tempoNeedsReset;
    number globaltransport_lastTempo;
    signal globaltransport_state;
    bool globaltransport_stateNeedsReset;
    number globaltransport_lastState;
    list globaltransport_beatTimeChanges;
    list globaltransport_timeSignatureChanges;
    bool globaltransport_notify;
    bool globaltransport_setupDone;
    number stackprotect_count;
    DataRef borisinrnbo_v01_rtbuf;
    DataRef interpolated_envelope;
    DataRef inter_databuf_01;
    Index _voiceIndex;
    Int _noteNumber;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    RTGrainVoice* rtgrainvoice[24];

    };

//namespace RNBO {
    PatcherInterface* creaternbomatic()
    {
        return new Granulator();
    }

#ifndef RNBO_NO_PATCHERFACTORY

    extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction(PlatformInterface* platformInterface)
#else

    extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction(PlatformInterface* platformInterface)
#endif

    {
        Platform::set(platformInterface);
        return creaternbomatic;
    }

} // end RNBO namespace

