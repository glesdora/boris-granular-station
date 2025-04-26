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

#include "RNBO_Common.h"
#include "RNBO_AudioSignal.h"

#include <juce_core/juce_core.h>

namespace RNBO {


#define trunc(x) ((Int)(x))

#if defined(__GNUC__) || defined(__clang__)
#define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

    class rnbomatic : public PatcherInterfaceImpl {
    public:

        class RTGrainVoice : public PatcherInterfaceImpl {

            friend class rnbomatic;

        public:

            RTGrainVoice()
            {
            }
            /*S-mod*/
            ~RTGrainVoice()
            {
                for (int i = 0; i < 2; i++) {
                    free(realtime_grain_out[i]);
                }
                //free(click_01_buf);
                //free(click_02_buf);
                //free(ip_01_sigbuf);
                //free(ip_02_sigbuf);
                //free(ip_03_sigbuf);
                //free(ip_04_sigbuf);
                //free(ip_05_sigbuf);
                //free(ip_06_sigbuf);
                free(zeroBuffer);
                free(dummyBuffer);
            }
            /*E-mod*/
            virtual rnbomatic* getPatcher() const {
                return static_cast<rnbomatic*>(_parentPatcher);
            }

            rnbomatic* getTopLevelPatcher() {
                return this->getPatcher()->getTopLevelPatcher();
            }

            void cancelClockEvents() {}
            //{
            //    getEngine()->flushClockEvents(this, -611950441, false);
            //    getEngine()->flushClockEvents(this, -1584063977, false);
            //}
            //
            template <typename T> inline number dim(T& buffer) {
                return buffer->getSize();
            }

            template <typename T> inline number channels(T& buffer) {
                return buffer->getChannels();
            }

            inline number intnum(const number value) {
                return trunc(value);
            }

            number maximum(number x, number y) {
                return (x < y ? y : x);
            }

            /*inline number safemod(number f, number m) {
                if (m != 0) {
                    Int f_trunc = (Int)(trunc(f));
                    Int m_trunc = (Int)(trunc(m));

                    if (f == f_trunc && m == m_trunc) {
                        f = f_trunc % m_trunc;
                    } else {
                        if (m < 0) {
                            m = -m;
                        }

                        if (f >= m) {
                            if (f >= m * 2.0) {
                                number d = f / m;
                                Int i = (Int)(trunc(d));
                                d = d - i;
                                f = d * m;
                            } else {
                                f -= m;
                            }
                        } else if (f <= -m) {
                            if (f <= -m * 2.0) {
                                number d = f / m;
                                Int i = (Int)(trunc(d));
                                d = d - i;
                                f = d * m;
                            } else {
                                f += m;
                            }
                        }
                    }
                } else {
                    f = 0.0;
                }

                return f;
            }
            */
            number wrap(number x, number low, number high) {
                number lo;
                number hi;

                if (low == high)
                    return low;

                if (low > high) {
                    hi = low;
                    lo = high;
                }
                else {
                    lo = low;
                    hi = high;
                }

                number range = hi - lo;

                if (x >= lo && x < hi)
                    return x;

                if (range <= 0.000000001)
                    return lo;

                long numWraps = (long)(trunc((x - lo) / range));
                numWraps = numWraps - ((x < lo ? 1 : 0));
                number result = x - range * numWraps;

                if (result >= hi)
                    return result - range;
                else
                    return result;
            }

            /*inline number linearinterp(number frac, number x, number y) {
                return x + (y - x) * frac;
            }

            inline number cubicinterp(number a, number w, number x, number y, number z) {
                number a2 = a * a;
                number f0 = z - y - w + x;
                number f1 = w - x - f0;
                number f2 = y - w;
                number f3 = x;
                return f0 * a * a2 + f1 * a2 + f2 * a + f3;
            }

            inline number splineinterp(number a, number w, number x, number y, number z) {
                number a2 = a * a;
                number f0 = -0.5 * w + 1.5 * x - 1.5 * y + 0.5 * z;
                number f1 = w - 2.5 * x + 2 * y - 0.5 * z;
                number f2 = -0.5 * w + 0.5 * y;
                return f0 * a * a2 + f1 * a2 + f2 * a + x;
            }*/

            /*inline number cosT8(number r) {
                number t84 = 56.0;
                number t83 = 1680.0;
                number t82 = 20160.0;
                number t81 = 2.4801587302e-05;
                number t73 = 42.0;
                number t72 = 840.0;
                number t71 = 1.9841269841e-04;

                if (r < 0.785398163397448309615660845819875721 && r > -0.785398163397448309615660845819875721) {
                    number rr = r * r;
                    return 1.0 - rr * t81 * (t82 - rr * (t83 - rr * (t84 - rr)));
                } else if (r > 0.0) {
                    r -= 1.57079632679489661923132169163975144;
                    number rr = r * r;
                    return -r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
                } else {
                    r += 1.57079632679489661923132169163975144;
                    number rr = r * r;
                    return r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
                }
            }

            inline number cosineinterp(number frac, number x, number y) {
                number a2 = (1.0 - this->cosT8(frac * 3.14159265358979323846)) / (number)2.0;
                return x * (1.0 - a2) + y * a2;
            }*/

            /*S-mod*/
            /*template <typename T> inline SampleValue getVirtualSamp(
                T& buffer,
                SampleValue phase,
                Index bufferSize
            ) {
                auto& __buffer = buffer;

                number virtual_index = phase * (bufferSize - 1);
                SampleValue virtual_value;

                SampleIndex index1 = (SampleIndex)virtual_index;
                SampleIndex index2 = index1 + 1;
                number frac = virtual_index - index1;

                if (index2 > bufferSize - 1)
                    index2 = 0;

                auto x = __buffer->getSample(0, index1);
                auto y = __buffer->getSample(0, index2);

                virtual_value = x + (y - x) * frac;

                return virtual_value;
            }*/

            /*template <typename T> inline array<SampleValue, 1 + 1> wave_default(
                T& buffer,
                SampleValue phase,
                SampleValue start,
                SampleValue end,
                int channelOffset
            ) {
                number bufferSize = buffer->getSize();
                const Index bufferChannels = (const Index)(buffer->getChannels());
                constexpr int ___N4 = 1 + 1;
                array<SampleValue, ___N4> out = FIXEDSIZEARRAYINIT(1 + 1);

                if (start < 0)
                    start = 0;

                if (end > bufferSize)
                    end = bufferSize;

                if (end - start <= 0) {
                    start = 0;
                    end = bufferSize;
                }

                number sampleIndex;

                {
                    SampleValue bufferphasetoindex_result;

                    {
                        auto __end = end + 1;
                        auto __start = start;
                        auto __phase = phase;

                        {
                            {
                                {
                                    number size = __end - 1 - __start;
                                    bufferphasetoindex_result = __start + __phase * size;
                                }
                            }
                        }
                    }

                    sampleIndex = bufferphasetoindex_result;
                }

                if (bufferSize == 0 || (3 == 5 && (sampleIndex < start || sampleIndex >= end))) {
                    return out;
                } else {
                    {
                        SampleIndex bufferbindindex_result;

                        {
                            {
                                {
                                    bufferbindindex_result = this->wrap(sampleIndex, start, end);
                                }
                            }
                        }

                        sampleIndex = bufferbindindex_result;
                    }

                    for (Index channel = 0; channel < 1; channel++) {
                        Index channelIndex = (Index)(channel + channelOffset);

                        {
                            Index bufferbindchannel_result;

                            {
                                {
                                    {
                                        bufferbindchannel_result = (bufferChannels == 0 ? 0 : channelIndex % bufferChannels);
                                    }
                                }
                            }

                            channelIndex = bufferbindchannel_result;
                        }

                        SampleValue bufferreadsample_result;

                        {
                            auto& __buffer = buffer;

                            if (sampleIndex < 0.0) {
                                bufferreadsample_result = 0.0;
                            }

                            SampleIndex index1 = (SampleIndex)(trunc(sampleIndex));

                            {
                                number frac = sampleIndex - index1;
                                number wrap = end - 1;
                                SampleIndex index2 = (SampleIndex)(index1 + 1);

                                if (index2 > wrap)
                                    index2 = start;

                                bufferreadsample_result = this->linearinterp(
                                    frac,
                                    __buffer->getSample(channelIndex, index1),
                                    __buffer->getSample(channelIndex, index2)
                                );
                            }
                        }

                        out[(Index)channel] = bufferreadsample_result;
                    }

                    out[1] = sampleIndex - start;
                    return out;
                }
            }

            template <typename T> inline array<SampleValue, 1 + 1> sample_default(T& buffer, SampleValue sampleIndex, Index channelOffset) {
                number bufferSize = buffer->getSize();
                const Index bufferChannels = (const Index)(buffer->getChannels());
                constexpr int ___N3 = 1 + 1;
                array<SampleValue, ___N3> out = FIXEDSIZEARRAYINIT(1 + 1);

                {
                    SampleValue bufferphasetoindex_result;

                    {
                        auto __end = bufferSize;
                        auto __start = 0;
                        auto __phase = sampleIndex;

                        {
                            number size = __end - 1 - __start;
                            bufferphasetoindex_result = __phase * size;
                        }
                    }

                    sampleIndex = bufferphasetoindex_result;
                }

                if (bufferSize == 0 || (5 == 5 && (sampleIndex < 0 || sampleIndex >= bufferSize))) {
                    for (Index i = 0; i < 1 + 1; i++) {
                        out[(Index)i] = 0;
                    }
                } else {
                    for (Index channel = 0; channel < 1; channel++) {
                        Index channelIndex = (Index)(channel + channelOffset);

                        {
                            if (channelIndex >= bufferChannels || channelIndex < 0) {
                                out[(Index)channel] = 0;
                                continue;
                            }
                        }

                        SampleValue bufferreadsample_result;

                        {
                            auto& __buffer = buffer;

                            if (sampleIndex < 0.0) {
                                bufferreadsample_result = 0.0;
                            }

                            SampleIndex index1 = (SampleIndex)(trunc(sampleIndex));

                            {
                                number frac = sampleIndex - index1;
                                number wrap = bufferSize - 1;
                                SampleIndex index2 = (SampleIndex)(index1 + 1);

                                if (index2 > wrap)
                                    index2 = 0;

                                bufferreadsample_result = this->linearinterp(
                                    frac,
                                    __buffer->getSample(channelIndex, index1),
                                    __buffer->getSample(channelIndex, index2)
                                );
                            }
                        }

                        out[(Index)channel] = bufferreadsample_result;
                    }

                    out[1] = sampleIndex;
                }

                return out;
            }*/
            /*E-mod*/
            Index voice() {
                return this->_voiceIndex;
            }

            int notenumber() {
                return this->_noteNumber;
            }

            Index getNumMidiInputPorts() const {
                return 0;
            }

            void processMidiEvent(MillisecondTime, int, ConstByteArray, Index) {}

            Index getNumMidiOutputPorts() const {
                return 0;
            }

            void process(
                const SampleValue* const* inputs,
                Index numInputs,
                SampleValue* const* outputs,
                Index numOutputs,
                Index n
            ) {
                this->vs = n;
                this->updateTime(this->getEngine()->getCurrentTime());
                SampleValue* out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
                SampleValue* out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
                const SampleValue* in2 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
                const SampleValue* recpointer = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);

                if (this->getIsMuted())
                    return;

                //this->click_01_perform(this->graindata[0], n);
                auto trgindx = this->triggerindex;
                this->triggerindex = -1;

                //this->ip_01_perform(this->signals[1], n);
                //this->ip_02_perform(this->signals[2], n);
                //this->ip_03_perform(this->signals[3], n);
                //this->ip_04_perform(this->signals[4], n);
                //this->ip_05_perform(this->signals[5], n);
                //this->ip_06_perform(this->signals[6], n);

                //this->click_02_perform(this->graindata[7], n);    //graindata[7] is all zeros, but starts with 1 if triggered
                //click_02_lastclick = -1;

                //this->sah_tilde_01_perform(in2, this->graindata[7], this->sah_tilde_01_thresh, this->graindata[8], n);

                if (trgindx >= 0)
                    reverse_offset = in2[trgindx];

                this->processCore(
                    trgindx,
                    position_value,
                    grainsize_value,
                    direction_value,
                    pishift_value,
                    volume_value,
                    panning_value,
                    reverse_offset,
                    recpointer,
                    this->realtime_grain_out[0],
                    this->realtime_grain_out[1],
                    n
                );

                this->signaladder_01_perform(this->realtime_grain_out[0], out1, out1, n);
                this->signaladder_02_perform(this->realtime_grain_out[1], out2, out2, n);

                //now, all the audio out is performed, the buffers are filled and ready to be played.
                //if the eog happened, at any samp, I should just mute the voice. Buffer is not needed, just a flag.

                //this->edge_01_perform(this->realtime_grain_out[2], n);
                this->stackprotect_perform(n);
                this->audioProcessSampleCount += this->vs;
            }

            void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
                if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
                    Index i;

                    for (i = 0; i < 2; i++) {
                        this->realtime_grain_out[i] = resizeSignal(this->realtime_grain_out[i], this->maxvs, maxBlockSize);
                    }

                    //this->click_01_buf = resizeSignal(this->click_01_buf, this->maxvs, maxBlockSize);
                    //this->ip_01_sigbuf = resizeSignal(this->ip_01_sigbuf, this->maxvs, maxBlockSize);
                    //this->ip_02_sigbuf = resizeSignal(this->ip_02_sigbuf, this->maxvs, maxBlockSize);
                    //this->ip_03_sigbuf = resizeSignal(this->ip_03_sigbuf, this->maxvs, maxBlockSize);
                    //this->ip_04_sigbuf = resizeSignal(this->ip_04_sigbuf, this->maxvs, maxBlockSize);
                    //this->ip_05_sigbuf = resizeSignal(this->ip_05_sigbuf, this->maxvs, maxBlockSize);
                    //this->ip_06_sigbuf = resizeSignal(this->ip_06_sigbuf, this->maxvs, maxBlockSize);
                    //this->click_02_buf = resizeSignal(this->click_02_buf, this->maxvs, maxBlockSize);
                    this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
                    this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
                    this->didAllocateSignals = true;
                }

                const bool sampleRateChanged = sampleRate != this->sr;
                const bool maxvsChanged = maxBlockSize != this->maxvs;
                const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

                if (sampleRateChanged || maxvsChanged) {
                    this->vs = maxBlockSize;
                    this->maxvs = maxBlockSize;
                    this->sr = sampleRate;
                    this->invsr = 1 / sampleRate;
                }

                //this->ip_01_dspsetup(forceDSPSetup);
                //this->ip_02_dspsetup(forceDSPSetup);
                //this->ip_03_dspsetup(forceDSPSetup);
                //this->ip_04_dspsetup(forceDSPSetup);
                //this->ip_05_dspsetup(forceDSPSetup);
                //this->ip_06_dspsetup(forceDSPSetup);
                //this->gen_01_dspsetup(forceDSPSetup);
                //this->edge_01_dspsetup(forceDSPSetup);

                if (sampleRateChanged)
                    this->onSampleRateChanged(sampleRate);
            }

            void setProbingTarget(MessageTag id) {
                switch (id) {
                default:
                {
                    this->setProbingIndex(-1);
                    break;
                }
                }
            }

            void setProbingIndex(ProbingIndex) {}

            Index getProbingChannels(MessageTag outletId) const {
                RNBO_UNUSED(outletId);
                return 0;
            }

            void setVoiceIndex(Index index) {
                this->_voiceIndex = index;
            }

            void setNoteNumber(Int noteNumber) {
                this->_noteNumber = noteNumber;
            }

            Index getIsMuted() {
                return this->isMuted;
            }

            void setIsMuted(Index v) {
                this->isMuted = v;
            }

            Index getPatcherSerial() const {
                return 0;
            }

            void getState(PatcherStateInterface&) {}

            void setState() {}

            void getPreset(PatcherStateInterface&) {}

            void processTempoEvent(MillisecondTime, Tempo) {}

            void processTransportEvent(MillisecondTime, TransportState) {}

            void processBeatTimeEvent(MillisecondTime, BeatTime) {}

            void onSampleRateChanged(double) {}

            void processTimeSignatureEvent(MillisecondTime, int, int) {}

            void setParameterValue(ParameterIndex, ParameterValue, MillisecondTime) {}

            void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
                this->setParameterValue(index, value, time);
            }

            void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
                this->setParameterValue(index, this->getParameterValue(index), time);
            }

            void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
                this->setParameterValueNormalized(index, value, time);
            }

            ParameterValue getParameterValue(ParameterIndex index) {
                switch (index) {
                default:
                {
                    return 0;
                }
                }
            }

            ParameterValue getPolyParameterValue(PatcherInterface** voices, ParameterIndex index) {
                switch (index) {
                default:
                {
                    return voices[0]->getParameterValue(index);
                }
                }
            }

            void setPolyParameterValue(
                PatcherInterface** voices,
                ParameterIndex index,
                ParameterValue value,
                MillisecondTime time
            ) {
                switch (index) {
                default:
                {
                    for (Index i = 0; i < 24; i++)
                        voices[i]->setParameterValue(index, value, time);
                }
                }
            }

            ParameterIndex getNumSignalInParameters() const {
                return 0;
            }

            ParameterIndex getNumSignalOutParameters() const {
                return 0;
            }

            ParameterIndex getNumParameters() const {
                return 0;
            }

            ConstCharPointer getParameterName(ParameterIndex index) const {
                switch (index) {
                default:
                {
                    return "bogus";
                }
                }
            }

            ConstCharPointer getParameterId(ParameterIndex index) const {
                switch (index) {
                default:
                {
                    return "bogus";
                }
                }
            }

            void getParameterInfo(ParameterIndex, ParameterInfo*) const {}

            void sendParameter(ParameterIndex index, bool ignoreValue) {
                if (this->_voiceIndex == 1)
                    this->getPatcher()->sendParameter(index + this->parameterOffset, ignoreValue);
            }

            void sendPolyParameter(ParameterIndex index, Index voiceIndex, bool ignoreValue) {
                this->getPatcher()->sendParameter(index + this->parameterOffset + voiceIndex - 1, ignoreValue);
            }

            void setParameterOffset(ParameterIndex offset) {
                this->parameterOffset = offset;
            }

            ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
                if (steps == 1) {
                    if (normalizedValue > 0) {
                        normalizedValue = 1.;
                    }
                }
                else {
                    ParameterValue oneStep = (number)1. / (steps - 1);
                    ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
                    normalizedValue = numberOfSteps * oneStep;
                }

                return normalizedValue;
            }

            ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
                switch (index) {
                default:
                {
                    return value;
                }
                }
            }

            ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
                value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                switch (index) {
                default:
                {
                    return value;
                }
                }
            }

            ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
                switch (index) {
                default:
                {
                    return value;
                }
                }
            }

            void scheduleParamInit(ParameterIndex index, Index order) {
                this->getPatcher()->scheduleParamInit(index + this->parameterOffset, order);
            }

            void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {}
            //    RNBO_UNUSED(value);
            //    RNBO_UNUSED(hasValue);
            //    this->updateTime(time);
            //
            //    switch (index) {
            //    case -611950441:
            //        {
            //        /*this->edge_01_onout_bang();*/
            //        this->getPatcher()->updateTime(this->_currentTime);
            //        this->getPatcher()->muteVoice(this->voice());
            //        break;
            //        }
            //    //case -1584063977:
            //    //    {
            //    //    //this->edge_01_offout_bang();
            //    //    break;
            //    //    }
            //    }
            //}

            void processOutletAtCurrentTime(EngineLink*, OutletIndex, ParameterValue) {}

            void processOutletEvent(
                EngineLink* sender,
                OutletIndex index,
                ParameterValue value,
                MillisecondTime time
            ) {
                this->updateTime(time);
                this->processOutletAtCurrentTime(sender, index, value);
            }

            void processNumMessage(MessageTag, MessageTag, MillisecondTime, number) {}

            void processListMessage(MessageTag, MessageTag, MillisecondTime, const list&) {}

            void processBangMessage(MessageTag, MessageTag, MillisecondTime) {}

            MessageTagInfo resolveTag(MessageTag tag) const {
                switch (tag) {

                }

                return nullptr;
            }

            DataRef* getDataRef(DataRefIndex index) {
                switch (index) {
                default:
                {
                    return nullptr;
                }
                }
            }

            DataRefIndex getNumDataRefs() const {
                return 0;
            }

            void fillDataRef(DataRefIndex, DataRef&) {}

            void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
                this->updateTime(time);

                if (index == 0) {
                    this->gen_01_rtbuf = new Float32Buffer(this->getPatcher()->borisinrnbo_v01_rtbuf);
                }

                if (index == 1) {
                    this->gen_01_interpol_env = new Float32Buffer(this->getPatcher()->interpolated_envelope);
                }
            }

            void initialize() {
                this->assign_defaults();
                this->setState();
                this->gen_01_rtbuf = new Float32Buffer(this->getPatcher()->borisinrnbo_v01_rtbuf);
                this->gen_01_interpol_env = new Float32Buffer(this->getPatcher()->interpolated_envelope);
            }

        protected:

            //void eventinlet_01_out1_bang_bang() {
            //    this->trigger_01_input_bang_bang();
            //}

            //void eventinlet_01_out1_number_set(number v) {
            //    this->trigger_01_input_number_set(v);
            //}

            //void edge_01_onout_bang() {
            //    /*this->voice_01_voicebang_bang();*/
            //    this->getPatcher()->updateTime(this->_currentTime);
            //    this->getPatcher()->muteVoice(this->voice());
            //}

            //void edge_01_offout_bang() {}

            number msToSamps(MillisecondTime ms, number sampleRate) {
                return ms * sampleRate * 0.001;
            }

            MillisecondTime sampsToMs(SampleIndex samps) {
                return samps * (this->invsr * 1000);
            }

            Index getMaxBlockSize() const {
                return this->maxvs;
            }

            number getSampleRate() const {
                return this->sr;
            }

            bool hasFixedVectorSize() const {
                return false;
            }

            Index getNumInputChannels() const {
                return 2;
            }

            Index getNumOutputChannels() const {
                return 2;
            }

            void initializeObjects() {
                this->gen_01_history_2_init();
                this->gen_01_history_1_init();      /*S-mod*/
                //this->gen_01_counter_11_init();
                //this->gen_01_counter_48_init();   /*E-mod*/
                //this->ip_01_init();
                //this->ip_02_init();
                //this->ip_03_init();
                //this->ip_04_init();
                //this->ip_05_init();
                //this->ip_06_init();
            }

            void sendOutlet(OutletIndex index, ParameterValue value) {
                this->getEngine()->sendOutlet(this, index, value);
            }

            void startup() {    /*S-mod*/
                this->setIsMuted(1);
            }       /*E-mod*/

            void allocateDataRefs() {
                this->gen_01_rtbuf = this->gen_01_rtbuf->allocateIfNeeded();
                this->gen_01_interpol_env = this->gen_01_interpol_env->allocateIfNeeded();
            }

            void voice_01_mutestatus_set(number) {}

            void voice_01_mutein_list_set(const list& v) {
                if (v[0] == this->voice() || v[0] == 0) {
                    this->voice_01_mutestatus_set(v[1]);
                }
            }

            void voice_01_activevoices_set(number) {}
            //  
            //  void click_02_click_number_set(number v) {
            //      //for (SampleIndex i = (SampleIndex)(this->click_02_lastclick + 1); i < 0 /*this->sampleOffsetIntoNextAudioBuffer*/; i++) {
            //      //    this->click_02_buf[(Index)i] = 0;
            //      //}
            //  
            //      //this->click_02_lastclick = this->sampleOffsetIntoNextAudioBuffer;
                  //this->click_02_lastclick = 0;
            //      this->click_02_buf[0] = 1;
            //  }

              //void click_02_click_bang_bang() {
              //    this->click_02_click_number_set(1);
              //}
              //
              //void trigger_01_out3_bang() {
              //    this->click_02_click_bang_bang();
              //}

              //void ip_06_value_set(number v) {
              //    this->ip_06_value = v;
              //    this->ip_06_fillSigBuf();
              //    this->ip_06_lastValue = v;
              //}

              //void unpack_01_out6_set(number v) {
              //    //this->unpack_01_out6 = v;
              //    this->ip_06_value_set(v);
              //}

              //void ip_05_value_set(number v) {
              //    this->ip_05_value = v;
              //    this->ip_05_fillSigBuf();
              //    this->ip_05_lastValue = v;
              //}

              //void unpack_01_out5_set(number v) {
              //    //this->unpack_01_out5 = v;
              //    this->ip_05_value_set(v);
              //}

              //void ip_04_value_set(number v) {
              //    this->ip_04_value = v;
              //    this->ip_04_fillSigBuf();
              //    this->ip_04_lastValue = v;
              //}

              //void unpack_01_out4_set(number v) {
              //    //this->unpack_01_out4 = v;
              //    this->ip_04_value_set(v);
              //}

              //void ip_03_value_set(number v) {
              //    this->ip_03_value = v;
              //    this->ip_03_fillSigBuf();
              //    this->ip_03_lastValue = v;
              //}

              //void unpack_01_out3_set(number v) {
              //    //this->unpack_01_out3 = v;
              //    this->ip_03_value_set(v);
              //}

              //void ip_02_value_set(number v) {
              //    this->ip_02_value = v;
              //    this->ip_02_fillSigBuf();
              //    this->ip_02_lastValue = v;
              //}

              //void unpack_01_out2_set(number v) {
              //    //this->unpack_01_out2 = v;
              //    this->ip_02_value_set(v);
              //}

              //void ip_01_value_set(number v) {
              //    this->ip_01_value = v;
              //    //this->ip_01_fillSigBuf();
              //    //this->ip_01_lastValue = v;
              //}

              //void unpack_01_out1_set(number v) {
              //    //this->unpack_01_out1 = v;
              //    this->ip_01_value_set(v);
              //}

             /* void unpack_01_input_list_set(const list& v) {
                  if (v->length > 5)
                      this->ip_06_value = v[5];

                  if (v->length > 4)
                      this->ip_05_value = v[4];

                  if (v->length > 3)
                      this->ip_04_value = v[3];

                  if (v->length > 2)
                      this->ip_03_value = v[2];

                  if (v->length > 1)
                      this->ip_02_value = v[1];

                  if (v->length > 0)
                      this->ip_01_value = v[0];
              }*/

              //void trigger_01_out2_set(const list& v) {
              //    this->unpack_01_input_list_set(v);
              //}

              //void click_01_click_number_set(number v) {
              //    //for (SampleIndex i = (SampleIndex)(this->click_01_lastclick + 1); i < 0 /*this->sampleOffsetIntoNextAudioBuffer*/; i++) {
              //    //    this->click_01_buf[(Index)i] = 0;
              //    //}
              //
              //    //this->click_01_lastclick = this->sampleOffsetIntoNextAudioBuffer;
              //    this->click_01_lastclick = 0;
              //    //this->click_01_buf[0] = 1;      //only called with v = 1
              //}

              //void click_01_click_bang_bang() {
              //    this->click_01_click_number_set(1);
              //}

              //void trigger_01_out1_bang() {
              //    this->click_01_click_bang_bang();
              //}

            void initiateVoice(SampleIndex trigatindex, const list& v) {
                //this->trigger_01_out3_bang();           // sample reverse offset at start
                //this->trigger_01_out2_set(v);           // pass grain properties
                //this->trigger_01_out1_bang();           // trigger grain start

                if (v->length > 5) {
                    this->panning_value = v[5];
                    this->volume_value = v[4];
                    this->pishift_value = v[3];
                    this->direction_value = v[2];
                    this->grainsize_value = v[1];
                    this->position_value = v[0];
                }

                this->triggerindex = trigatindex;
            }

            //void eventinlet_01_out1_list_set(const list& v) {
            //    this->initiateVoice(v);
            //}

            //void trigger_01_input_bang_bang() {
            //    this->trigger_01_out3_bang();
            //    list l_2 = list();
            //    this->trigger_01_out2_set(l_2);
            //    this->trigger_01_out1_bang();
            //}

            //void trigger_01_input_number_set(number v) {
            //    this->trigger_01_out3_bang();
            //    list l_2 = list(0);
            //    l_2[0] = v;
            //    this->trigger_01_out2_set(l_2);
            //    this->trigger_01_out1_bang();
            //}

            void voice_01_noteNumber_set(number) {}

            //void eventoutlet_01_in1_number_set(number v) {
            //    this->getPatcher()->updateTime(this->_currentTime);
            //    this->getPatcher()->muteVoice(v);

            //    //this->getPatcher()->p_01_out3_number_set(v);
            //}

            //void voice_01_voicenumber_set(number v) {
            //    this->eventoutlet_01_in1_number_set(v);
            //}

            //void voice_01_voicebang_bang() {
            //    //this->voice_01_noteNumber_set(this->notenumber());
            //    this->voice_01_voicenumber_set(this->voice());
            //}

            void midiouthelper_midiout_set(number) {}

            //  void click_01_perform(SampleValue * out, Index n) {
            //      //auto __click_01_lastclick = this->click_01_lastclick;       // 0 when triggered, -1 otherwise
            //  
            //      //for (SampleIndex i = 0; i <= __click_01_lastclick; i++) {
            //      //    out[(Index)i] = this->click_01_buf[(Index)i];
            //      //}   // if trig, out[0] = click_01_buf[0]
            //  
            //      //for (SampleIndex i = (SampleIndex)(__click_01_lastclick + 1); i < (SampleIndex)(n); i++) {
            //      //    out[(Index)i] = 0;
            //      //}  // the rest is 0
            //  
            //      //__click_01_lastclick = -1;                          
            //      //this->click_01_lastclick = __click_01_lastclick;

            //      out[0] = click_01_lastclick + 1;
                  //click_01_lastclick = -1;
            //  }

            //  void ip_01_perform(SampleValue * out, Index n) {
            //      //auto __ip_01_lastValue = this->ip_01_lastValue;
            //      //auto __ip_01_lastIndex = this->ip_01_lastIndex;
            //      //auto __ip_01_lastIndex = 0;

                  //auto v = this->ip_01_value;
            //  
            //      for (Index i = 0; i < n; i++) {
            //          //out[(Index)i] = ((SampleIndex)(i) >= __ip_01_lastIndex ? __ip_01_lastValue : this->ip_01_sigbuf[(Index)i]);
                  //	out[i] = v;
            //      }
            //  
            //      //__ip_01_lastIndex = 0;
            //      //this->ip_01_lastIndex = __ip_01_lastIndex;
            //  }

            //  void ip_02_perform(SampleValue * out, Index n) {
            //      auto __ip_02_lastValue = this->ip_02_lastValue;
            //      //auto __ip_02_lastIndex = this->ip_02_lastIndex;
                  //auto __ip_02_lastIndex = 0;
            //  
            //      for (Index i = 0; i < n; i++) {
            //          out[(Index)i] = ((SampleIndex)(i) >= __ip_02_lastIndex ? __ip_02_lastValue : this->ip_02_sigbuf[(Index)i]);
            //      }
            //  
            //      //__ip_02_lastIndex = 0;
            //      //this->ip_02_lastIndex = __ip_02_lastIndex;
            //  }
              //
              //void ip_03_perform(SampleValue * out, Index n) {
              //    auto __ip_03_lastValue = this->ip_03_lastValue;
              //    //auto __ip_03_lastIndex = this->ip_03_lastIndex;
              //
              //    for (Index i = 0; i < n; i++) {
              //        out[(Index)i] = ((SampleIndex)(i) >= 0 ? __ip_03_lastValue : this->ip_03_sigbuf[(Index)i]);
              //    }
              //
              //    //__ip_03_lastIndex = 0;
              //    //this->ip_03_lastIndex = __ip_03_lastIndex;
              //}
              //
              //void ip_04_perform(SampleValue * out, Index n) {
              //    auto __ip_04_lastValue = this->ip_04_lastValue;
              //    //auto __ip_04_lastIndex = this->ip_04_lastIndex;
              //
              //    for (Index i = 0; i < n; i++) {
              //        out[(Index)i] = ((SampleIndex)(i) >= 0 ? __ip_04_lastValue : this->ip_04_sigbuf[(Index)i]);
              //    }
              //
              //    //__ip_04_lastIndex = 0;
              //    //this->ip_04_lastIndex = __ip_04_lastIndex;
              //}
              //
              //void ip_05_perform(SampleValue * out, Index n) {
              //    auto __ip_05_lastValue = this->ip_05_lastValue;
              //    //auto __ip_05_lastIndex = this->ip_05_lastIndex;
              //
              //    for (Index i = 0; i < n; i++) {
              //        out[(Index)i] = ((SampleIndex)(i) >= 0 ? __ip_05_lastValue : this->ip_05_sigbuf[(Index)i]);
              //    }
              //
              //    //__ip_05_lastIndex = 0;
              //    //this->ip_05_lastIndex = __ip_05_lastIndex;
              //}
              //
              //void ip_06_perform(SampleValue * out, Index n) {
              //    auto __ip_06_lastValue = this->ip_06_lastValue;
              //    //auto __ip_06_lastIndex = this->ip_06_lastIndex;
              //
              //    for (Index i = 0; i < n; i++) {
              //        out[(Index)i] = ((SampleIndex)(i) >= 0 ? __ip_06_lastValue : this->ip_06_sigbuf[(Index)i]);
              //    }
              //
              //    //__ip_06_lastIndex = 0;
              //    //this->ip_06_lastIndex = __ip_06_lastIndex;
              //}

              //void click_02_perform(SampleValue * out, Index n) {
              //    auto __click_02_lastclick = this->click_02_lastclick;
              //
              //    for (SampleIndex i = 0; i <= __click_02_lastclick; i++) {
              //        out[(Index)i] = this->click_02_buf[(Index)i];
              //    }
              //
              //    for (SampleIndex i = (SampleIndex)(__click_02_lastclick + 1); i < (SampleIndex)(n); i++) {
              //        out[(Index)i] = 0;
              //    }
              //
              //    __click_02_lastclick = -1;
              //    this->click_02_lastclick = __click_02_lastclick;
              //}
            /*
              void sah_tilde_01_perform(
                  const Sample * input,
                  const Sample * trig,
                  number thresh,
                  SampleValue * out,
                  Index n
              ) {
                  RNBO_UNUSED(thresh);

                  for (Index i = 0; i < n; i++) {
                      out[(Index)i] = this->sah_tilde_01_s_next(input[(Index)i], trig[(Index)i], 0);
                  }
              }*/

              /*S-mod*/
              //void __gen_01_perform(
              //    const Sample* in1,     // trigger
              //    const Sample* in2,     // position: 0 - 1
              //    const Sample* in3,     // gsize in samples
              //    const Sample* in4,     // f/r: 1 or -1
              //    const Sample* in5,     // pitch: 0.25 - 4
              //    const Sample* in6,     // volume: 0 - 1
              //    const Sample* in7,     // pan: 0 - 1
              //    const Sample* in8,     // reverse offset in samps
              //    const Sample* in9,     // rec point in samps
              //    SampleValue* out1,
              //    SampleValue* out2,
              //    SampleValue* out3,
              //    Index n
              //) {
              //    auto __audiobuf_incamount_history = this->gen_01_history_2_value;
              //    auto __envbuf_incamount_history = this->gen_01_history_1_value;

              //    auto& rtbuf = this->gen_01_rtbuf;
              //    auto rtbuf_dim = rtbuf->getSize();

              //    auto& envbuf = this->gen_01_interpol_env;
              //    auto envbuf_dim = envbuf->getSize();

              //    number tables_num = 3;

              //    Index i;

              //    for (i = 0; i < n; i++) {
              //        number __trigger = in1[(Index)i];
              //        number __position = (in2[(Index)i] > 1 ? 1 : (in2[(Index)i] < 0 ? 0 : in2[(Index)i]));
              //        number __trgt_samps = this->maximum(in3[(Index)i], 0);
              //        number __f_r = (in4[(Index)i] > 1 ? 1 : (in4[(Index)i] < -1 ? -1 : in4[(Index)i]));
              //        number __pitch = (in5[(Index)i] > 4 ? 4 : (in5[(Index)i] < 0.25 ? 0.25 : in5[(Index)i]));
              //        number __vol = (in6[(Index)i] > 1 ? 1 : (in6[(Index)i] < 0 ? 0 : in6[(Index)i]));
              //        number __pan = (in7[(Index)i] > 1 ? 1 : (in7[(Index)i] < 0 ? 0 : in7[(Index)i]));
              //        number __r_offset = in8[(Index)i];
              //        number __rec_point = in9[(Index)i];

              //        number g_samps = __trgt_samps / __pitch;        // g (grain) refers to the resulting audio,
              //        // while trgt refers to the audio sample before the gen elaboration
              //        // (in this case I only care about the pitch shift, which modifies
              //        //  the length of the grain, while the other params don't change
              //        //  "structural" properties of the grain)

              //        number env_counter_hit = 0;
              //        number env_counter_count = 0;
              //        //envelope counter
              //        {
              //            number carry_flag = 0;

              //            if (__trigger) {
              //                this->gen_01_counter_11_count = 0;
              //            }
              //            else {
              //                this->gen_01_counter_11_count += __envbuf_incamount_history;

              //                if ((__envbuf_incamount_history > 0 && this->gen_01_counter_11_count >= g_samps)) {
              //                    this->gen_01_counter_11_count = 0;
              //                    carry_flag = 1;
              //                }
              //            }

              //            env_counter_hit = carry_flag;
              //            env_counter_count = this->gen_01_counter_11_count;
              //        }

              //        bool env_counter_incr = ((env_counter_hit) ? 0 : 1);
              //        number updated_env_increment = this->gen_01_latch_15_next(env_counter_incr, __trigger + env_counter_hit);     // if just triggered, or counter hit the max, update the value

              //        number pos_in_grain = env_counter_count / g_samps;

              //        out3[(Index)i] = env_counter_hit;                                       // end of grain, used to mute the voice

              //        number envelope_value = this->getVirtualSamp(this->gen_01_interpol_env, pos_in_grain, envbuf_dim);           // check if - 1 or not

              //        bool isforw = __f_r == 1;
              //        number gtimesmps_with_sign = g_samps * __f_r;
              //        number potential_relative_revoffs = __r_offset / rtbuf_dim;
              //        number relative_reverse_offset = (isforw ? 0 : potential_relative_revoffs);
              //        number relative_gsize = __trgt_samps / rtbuf_dim;
              //        auto rel_pos_in_buffer = this->wrap(__rec_point - __position, 0, 1);
              //        number rel_pos_when_triggered = this->gen_01_latch_39_next(rel_pos_in_buffer, __trigger);  //position in buffer (0-1) at the momoment of trigger
              //        number rel_end_of_grain = rel_pos_when_triggered + relative_gsize;

              //        number playstart = (isforw ? rel_pos_when_triggered : rel_end_of_grain);
              //        number playend = (isforw ? rel_end_of_grain : rel_pos_when_triggered);

              //        number play_counter_count = 0;
              //        number play_counter_hit = 0;

              //        {
              //            number carry_flag = 0;

              //            if (__trigger != 0) {
              //                this->gen_01_counter_48_count = 0;
              //            }
              //            else {
              //                this->gen_01_counter_48_count += __audiobuf_incamount_history;

              //                if ((__audiobuf_incamount_history > 0 && this->gen_01_counter_48_count >= g_samps)) {
              //                    this->gen_01_counter_48_count = 0;
              //                    carry_flag = 1;
              //                }
              //            }

              //            play_counter_hit = carry_flag;
              //            play_counter_count = this->gen_01_counter_48_count;
              //        }
              //        number play_counter_incr = ((bool)(play_counter_hit) ? 0 : 1);
              //        number updated_audio_increment = this->gen_01_latch_52_next(play_counter_incr, __trigger + play_counter_hit);       // if just triggered +1, if hit max +0
              //        number progress_in_grain = play_counter_count / g_samps;
              //        number playstart_attrigger = this->gen_01_latch_55_next(playstart, __trigger);      //maybe always equal to playstart
              //        number grain_relsize_wsign = playend - playstart;
              //        number grain_relprogress = progress_in_grain * grain_relsize_wsign;
              //        number playpos = grain_relprogress + playstart_attrigger;

              //        auto offsettedpos = this->wrap(playpos - relative_reverse_offset, 0, 1);
              //        number sample_rtbuf = this->getVirtualSamp(this->gen_01_rtbuf, offsettedpos, rtbuf_dim);
              //        number sample_scaled = sample_rtbuf * envelope_value * __vol;
              //        number outleft = sample_scaled * __pan;
              //        number rpan = 1 - __pan;
              //        number outright = sample_scaled * rpan;
              //        out1[(Index)i] = outleft;
              //        out2[(Index)i] = outright;
              //        __envbuf_incamount_history = updated_env_increment;
              //        __audiobuf_incamount_history = updated_audio_increment;

              //    }

              //    this->gen_01_history_1_value = __envbuf_incamount_history;
              //    this->gen_01_history_2_value = __audiobuf_incamount_history;
              //}
              /*E-mod*/

            void processCore(
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
            ) {
                //auto __audiobuf_incamount_history = this->play_inc_history;
                //auto __envbuf_incamount_history = this->env_inc_history;

                auto& rtbuf = this->gen_01_rtbuf;
                auto rtbuf_dim = rtbuf->getSize();

                auto& envbuf = this->gen_01_interpol_env;
                auto envbuf_dim = envbuf->getSize();

                number __trigger_index = in1;

                if (__trigger_index >= 0) {
                    this->gen_01_counter_11_count = -1;
                }

                bool playsound = (__trigger_index < 0);

                Index i;

                for (i = 0; i < n; i++) {
                    bool triggernow = (__trigger_index == i);

                    number __pan = (in7 > 1 ? 1 : in7 < 0 ? 0 : in7);
                    number __vol = (in6 > 1 ? 1 : (in6 < 0 ? 0 : in6));
                    number __trgt_samps = this->maximum(in3, 0);
                    number __pitch = (in5 > 4 ? 4 : (in5 < 0.25 ? 0.25 : in5));
                    number g_samps = __trgt_samps / __pitch;

                    number env_counter_hit = 0;
                    number env_counter_count = 0;

                    //envelope counter
                    {
                        number carry_flag = 0;

                        if (triggernow) {
                            this->gen_01_counter_11_count = 0;
                            playsound = true;
                        }
                        else if (playsound) {
                            this->gen_01_counter_11_count += this->env_inc_history;

                            if ((this->env_inc_history > 0 && this->gen_01_counter_11_count >= g_samps)) {
                                this->gen_01_counter_11_count = 0;
                                carry_flag = 1;
                            }
                        }

                        env_counter_hit = carry_flag;
                        env_counter_count = this->gen_01_counter_11_count;
                    }

                    bool env_counter_incr = ((env_counter_hit) ? 0 : 1);
					this->env_inc_history = (triggernow + env_counter_hit) ? env_counter_incr : this->env_inc_history;  // if just triggered, or counter hit the max, update the value
                    //number updated_env_increment = this->gen_01_latch_15_next(env_counter_incr, (__trigger == i) + env_counter_hit);     // if just triggered, or counter hit the max, update the value
                    number pos_in_grain = (g_samps == 0. ? 0. : env_counter_count / g_samps);           // why is this necessary?

                    if (env_counter_hit) {
                        //this->getEngine()->scheduleClockEvent(this, -611950441, this->sampsToMs(i) + this->_currentTime);
                        this->getPatcher()->updateTime(this->_currentTime);
                        this->getPatcher()->muteVoice(this->voice());
                    }

                    //number envelope_value = this->getVirtualSamp(this->gen_01_interpol_env, pos_in_grain, envbuf_dim);

                    number envelope_value = 0;
                    {
                        auto& __buffer = gen_01_interpol_env;

                        number virtual_index = pos_in_grain * (envbuf_dim - 1);
                        SampleValue virtual_value;

                        SampleIndex index1 = (SampleIndex)virtual_index;
                        SampleIndex index2 = index1 + 1;
                        number frac = virtual_index - index1;

                        if (index2 > envbuf_dim - 1)
                            index2 = 0;

                        auto x = __buffer->getSample(0, index1);
                        auto y = __buffer->getSample(0, index2);

                        virtual_value = x + (y - x) * frac;

                        envelope_value = virtual_value;
                    }

                    number __f_r = in4;
                    bool isforw = __f_r == 1;
                    number gtimesmps_with_sign = g_samps * __f_r;
                    number __r_offset = in8;
                    number potential_relative_revoffs = __r_offset / rtbuf_dim;
                    number relative_reverse_offset = (isforw ? 0 : potential_relative_revoffs);
                    number relative_gsize = __trgt_samps / rtbuf_dim;
                    number __position = (in2 > 1 ? 1 : in2 < 0 ? 0 : in2);
                    number __rec_point = in9[(Index)i];
                    auto rel_pos_in_buffer = this->wrap(__rec_point - __position, 0, 1);
                    //number rel_pos_when_triggered = this->gen_01_latch_39_next(rel_pos_in_buffer, (__trigger == i));
                    //number rel_pos_when_triggered = (triggernow) ? rel_pos_in_buffer : this->rel_start_pos_history;  //position in buffer (0-1) at the momoment of trigger
					this->rel_start_pos_history = (triggernow) ? rel_pos_in_buffer : this->rel_start_pos_history;  //position in buffer (0-1) at the momoment of trigger
                    number rel_end_of_grain = this->rel_start_pos_history + relative_gsize;

                    number playstart = (isforw ? this->rel_start_pos_history : rel_end_of_grain);
                    number playend = (isforw ? rel_end_of_grain : this->rel_start_pos_history);

                    number play_counter_count = 0;
                    number play_counter_hit = 0;

                    {
                        number carry_flag = 0;

                        if (triggernow) {
                            this->gen_01_counter_48_count = 0;
                        }
                        else {
                            this->gen_01_counter_48_count += play_inc_history;

                            if ((play_inc_history > 0 && this->gen_01_counter_48_count >= g_samps)) {
                                this->gen_01_counter_48_count = 0;
                                carry_flag = 1;
                            }
                        }

                        play_counter_hit = carry_flag;
                        play_counter_count = this->gen_01_counter_48_count;
                    }

                    bool play_counter_incr = ((bool)(play_counter_hit) ? 0 : 1);
					this->play_inc_history = (triggernow + play_counter_hit) ? play_counter_incr : this->play_inc_history;  // if just triggered +1, if hit max +0
                    //number updated_audio_increment = this->gen_01_latch_52_next(play_counter_incr, (__trigger == i) + play_counter_hit);
                    number progress_in_grain = (g_samps == 0. ? 0. : play_counter_count / g_samps);
                    //number playstart_attrigger = this->gen_01_latch_55_next(playstart, (__trigger == i));
                    //number playstart_attrigger = (triggernow) ? playstart : this->start_play_pos_history;
					this->start_play_pos_history = (triggernow) ? playstart : this->start_play_pos_history;
                    number grain_relsize_wsign = playend - playstart;
                    number grain_relprogress = progress_in_grain * grain_relsize_wsign;
                    number playpos = grain_relprogress + this->start_play_pos_history;
                    number sub_67_62 = playpos - relative_reverse_offset;
                    auto offsettedpos = this->wrap(playpos - relative_reverse_offset, 0, 1);

                    //number sample_rtbuf = this->getVirtualSamp(this->gen_01_rtbuf, offsettedpos, rtbuf_dim);

                    number sample_rtbuf = 0;
                    {
                        auto& __buffer = gen_01_rtbuf;

                        number virtual_index = offsettedpos * (rtbuf_dim - 1);
                        SampleValue virtual_value;

                        SampleIndex index1 = (SampleIndex)virtual_index;
                        SampleIndex index2 = index1 + 1;
                        number frac = virtual_index - index1;

                        if (index2 > rtbuf_dim - 1)
                            index2 = 0;

                        auto x = __buffer->getSample(0, index1);
                        auto y = __buffer->getSample(0, index2);

                        virtual_value = x + (y - x) * frac;

                        sample_rtbuf = virtual_value;
                    }

                    number sample_scaled = sample_rtbuf * envelope_value * __vol;

                    number outleft = sample_scaled * __pan;
                    number outright = sample_scaled * (1.0 - __pan);


                    if (!playsound) {
                        out1[(Index)i] = 0;
                        out2[(Index)i] = 0;
                        //__envbuf_incamount_history = updated_env_increment;
                        //__audiobuf_incamount_history = updated_audio_increment;
                        continue;
                    }

                    out2[(Index)i] = outright;
                    out1[(Index)i] = outleft;
                    //__envbuf_incamount_history = updated_env_increment;
                    //__audiobuf_incamount_history = updated_audio_increment;
                }

                //this->env_inc_history = __envbuf_incamount_history;
                //this->play_inc_history = __audiobuf_incamount_history;
            }

            void signaladder_01_perform(
                const SampleValue* in1,
                const SampleValue* in2,
                SampleValue* out,
                Index n
            ) {
                Index i;

                for (i = 0; i < n; i++) {
                    out[(Index)i] = in1[(Index)i] + in2[(Index)i];
                }
            }

            void signaladder_02_perform(
                const SampleValue* in1,
                const SampleValue* in2,
                SampleValue* out,
                Index n
            ) {
                Index i;

                for (i = 0; i < n; i++) {
                    out[(Index)i] = in1[(Index)i] + in2[(Index)i];
                }
            }

            //void edge_01_perform(const SampleValue * input, Index n) {
            //    auto __edge_01_currentState = this->edge_01_currentState;
            //
            //    for (Index i = 0; i < n; i++) {
            //        if (__edge_01_currentState == 1) {
            //            if (input[(Index)i] == 0.) {
            //                __edge_01_currentState = 0;
            //            }
            //        } else {
            //            if (input[(Index)i] != 0.) {
            //                this->getEngine()->scheduleClockEvent(this, -611950441, this->sampsToMs(i) + this->_currentTime);
            //                __edge_01_currentState = 1;
            //            }
            //        }
            //    }
            //
            //    this->edge_01_currentState = __edge_01_currentState;
            //}

            void stackprotect_perform(Index n) {
                RNBO_UNUSED(n);
                auto __stackprotect_count = this->stackprotect_count;
                __stackprotect_count = 0;
                this->stackprotect_count = __stackprotect_count;
            }

            //number gen_01_history_2_getvalue() {
            //    return this->play_inc_history;
            //}
            //
            //void gen_01_history_2_setvalue(number val) {
            //    this->play_inc_history = val;
            //}
            //
            //void gen_01_history_2_reset() {
            //    this->play_inc_history = 0;
            //}
            //
            void gen_01_history_2_init() {
                this->play_inc_history = 0;
            }
            //
            //number gen_01_history_1_getvalue() {
            //    return this->gen_01_history_1_value;
            //}
            //
            //void gen_01_history_1_setvalue(number val) {
            //    this->gen_01_history_1_value = val;
            //}
            //
            //void gen_01_history_1_reset() {
            //    this->gen_01_history_1_value = 0;
            //}

            void gen_01_history_1_init() {
                this->env_inc_history = 0;
            }
            /*S-mod*/
            /*array<number, 3> gen_01_counter_11_next(number a, number reset, number limit) {
                number carry_flag = 0;

                if (reset != 0) {
                    this->gen_01_counter_11_count = 0;
                    this->gen_01_counter_11_carry = 0;
                } else {
                    this->gen_01_counter_11_count += a;

                    if (limit != 0) {
                        if ((a > 0 && this->gen_01_counter_11_count >= limit) || (a < 0 && this->gen_01_counter_11_count <= limit)) {
                            this->gen_01_counter_11_count = 0;
                            this->gen_01_counter_11_carry += 1;
                            carry_flag = 1;
                        }
                    }
                }

                return {this->gen_01_counter_11_count, carry_flag, this->gen_01_counter_11_carry};
            }

            void gen_01_counter_11_init() {
                this->gen_01_counter_11_count = 0;
            }

            void gen_01_counter_11_reset() {
                this->gen_01_counter_11_carry = 0;
                this->gen_01_counter_11_count = 0;
            }*/
            /*E-mod*/
            //number gen_01_latch_15_next(number x, number control) {
            //    if (control != 0.)
            //        this->gen_01_latch_15_value = x;
            //
            //    return this->gen_01_latch_15_value;
            //}

            //void gen_01_latch_15_dspsetup() {
            //    this->gen_01_latch_15_reset();
            //}
            //
            //void gen_01_latch_15_reset() {
            //    this->gen_01_latch_15_value = 0;
            //}

            //number gen_01_latch_18_next(number x, number control) {
            //    if (control != 0.)
            //        this->gen_01_latch_18_value = x;
            //
            //    return this->gen_01_latch_18_value;
            //}
            //
            //void gen_01_latch_18_dspsetup() {
            //    this->gen_01_latch_18_reset();
            //}
            //
            //void gen_01_latch_18_reset() {
            //    this->gen_01_latch_18_value = 0;
            //}
            //
            //number gen_01_latch_39_next(number x, number control) {
            //    if (control != 0.)
            //        this->rel_start_pos_history = x;
            //
            //    return this->rel_start_pos_history;
            //}
            //
            //void gen_01_latch_39_dspsetup() {
            //    this->gen_01_latch_39_reset();
            //}
            //
            //void gen_01_latch_39_reset() {
            //    this->rel_start_pos_history = 0;
            //}
            /*S-mod*/
            /*array<number, 3> gen_01_counter_48_next(number a, number reset, number limit) {
                number carry_flag = 0;

                if (reset != 0) {
                    this->gen_01_counter_48_count = 0;
                    this->gen_01_counter_48_carry = 0;
                } else {
                    this->gen_01_counter_48_count += a;

                    if (limit != 0) {
                        if ((a > 0 && this->gen_01_counter_48_count >= limit) || (a < 0 && this->gen_01_counter_48_count <= limit)) {
                            this->gen_01_counter_48_count = 0;
                            this->gen_01_counter_48_carry += 1;
                            carry_flag = 1;
                        }
                    }
                }

                return {this->gen_01_counter_48_count, carry_flag, this->gen_01_counter_48_carry};
            }

            void gen_01_counter_48_init() {
                this->gen_01_counter_48_count = 0;
            }

            void gen_01_counter_48_reset() {
                this->gen_01_counter_48_carry = 0;
                this->gen_01_counter_48_count = 0;
            }*/
            /*E-mod*/
            //number gen_01_latch_52_next(number x, number control) {
            //    if (control != 0.)
            //        this->gen_01_latch_52_value = x;
            //
            //    return this->gen_01_latch_52_value;
            //}
            //
            //void gen_01_latch_52_dspsetup() {
            //    this->gen_01_latch_52_reset();
            //}
            //
            //void gen_01_latch_52_reset() {
            //    this->gen_01_latch_52_value = 0;
            //}

            //number gen_01_latch_55_next(number x, number control) {
            //    if (control != 0.)
            //        this->start_play_pos_history = x;
            //
            //    return this->start_play_pos_history;
            //}
            //
            //void gen_01_latch_55_dspsetup() {
            //    this->gen_01_latch_55_reset();
            //}
            //
            //void gen_01_latch_55_reset() {
            //    this->start_play_pos_history = 0;
            //}

            //void gen_01_dspsetup(bool force) {
            //    if ((bool)(this->gen_01_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    this->gen_01_setupDone = true;
            //    //this->gen_01_latch_15_dspsetup();
            //    //this->gen_01_latch_18_dspsetup();
            //    //this->gen_01_latch_39_dspsetup();
            //    //this->gen_01_latch_52_dspsetup();
            //    //this->gen_01_latch_55_dspsetup();
            //}

            //void ip_01_init() {
            //    //this->ip_01_lastValue = this->ip_01_value;
            //}

         //   void ip_01_fillSigBuf() {
         //       if ((bool)(this->ip_01_sigbuf)) {
         //           //SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);
            //		//SampleIndex k = 0;

         //           // assuming if the vs is 0 this shouldn't be called (and anyway shouldn't create problems)
         ///*           if (k >= (SampleIndex)(this->vs)) { 
         //               k = (SampleIndex)(this->vs) - 1;
         //           }*/
         //   
         //   //        for (SampleIndex i = (SampleIndex)(this->ip_01_lastIndex); i < k; i++) {
            //			//this->ip_01_sigbuf[(Index)i] = this->ip_01_lastValue;
         //   //        }
         //   
         //           //this->ip_01_lastIndex = k;
         //           //this->ip_01_lastIndex = 0;
         //       }
         //   }

            //void ip_01_dspsetup(bool force) {
            //    if ((bool)(this->ip_01_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    //this->ip_01_lastIndex = 0;
            //    this->ip_01_setupDone = true;
            //}
            //
            //void ip_02_init() {
            //    this->ip_02_lastValue = this->ip_02_value;
            //}
            //
           // void ip_02_fillSigBuf() {
           ////     if ((bool)(this->ip_02_sigbuf)) {
           ////         //SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);
                    ////SampleIndex k = 0;
           //// 
           ////         if (k >= (SampleIndex)(this->vs))
           ////             k = (SampleIndex)(this->vs) - 1;
           //// 
           ////         for (SampleIndex i = (SampleIndex)(this->ip_02_lastIndex); i < k; i++) {
           ////             if (this->ip_02_resetCount > 0) {
           ////                 this->ip_02_sigbuf[(Index)i] = 1;
           ////                 this->ip_02_resetCount--;
           ////             } else {
           ////                 this->ip_02_sigbuf[(Index)i] = this->ip_02_lastValue;
           ////             }
           ////         }
           //// 
           ////         this->ip_02_lastIndex = k;
           ////     }
           // }
            //
            //void ip_02_dspsetup(bool force) {
            //    if ((bool)(this->ip_02_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    //this->ip_02_lastIndex = 0;
            //    this->ip_02_setupDone = true;
            //}
            //
            //void ip_03_init() {
            //    this->ip_03_lastValue = this->ip_03_value;
            //}

            //void ip_03_fillSigBuf() {
           //     if ((bool)(this->ip_03_sigbuf)) {
           //         //SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);
                    //SampleIndex k = 0;
           // 
           //         if (k >= (SampleIndex)(this->vs))
           //             k = (SampleIndex)(this->vs) - 1;
           // 
           //         for (SampleIndex i = (SampleIndex)(this->ip_03_lastIndex); i < k; i++) {
           //             if (this->ip_03_resetCount > 0) {
           //                 this->ip_03_sigbuf[(Index)i] = 1;
           //                 this->ip_03_resetCount--;
           //             } else {
           //                 this->ip_03_sigbuf[(Index)i] = this->ip_03_lastValue;
           //             }
           //         }
           // 
           //         this->ip_03_lastIndex = k;
           //     }
            //}

            //void ip_03_dspsetup(bool force) {
            //    if ((bool)(this->ip_03_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    //this->ip_03_lastIndex = 0;
            //    this->ip_03_setupDone = true;
            //}
            //
            //void ip_04_init() {
            //    this->ip_04_lastValue = this->ip_04_value;
            //}
            //
            //void ip_04_fillSigBuf() {
                ////if ((bool)(this->ip_04_sigbuf)) {
                ////    //SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);
                ////    SampleIndex k = 0;

                ////    if (k >= (SampleIndex)(this->vs))
                ////        k = (SampleIndex)(this->vs) - 1;

                ////    for (SampleIndex i = (SampleIndex)(this->ip_04_lastIndex); i < k; i++) {
                ////        if (this->ip_04_resetCount > 0) {
                ////            this->ip_04_sigbuf[(Index)i] = 1;
                ////            this->ip_04_resetCount--;
                ////        } else {
                ////            this->ip_04_sigbuf[(Index)i] = this->ip_04_lastValue;
                ////        }
                ////    }

                ////    this->ip_04_lastIndex = k;
                ////}
            //}
            //
            //void ip_04_dspsetup(bool force) {
            //    if ((bool)(this->ip_04_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    //this->ip_04_lastIndex = 0;
            //    this->ip_04_setupDone = true;
            //}
            //
            //void ip_05_init() {
            //    this->ip_05_lastValue = this->ip_05_value;
            //}
            //
            //void ip_05_fillSigBuf() {
                //if ((bool)(this->ip_05_sigbuf)) {
                //    //SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);
                //    SampleIndex k = 0;

                //    if (k >= (SampleIndex)(this->vs))
                //        k = (SampleIndex)(this->vs) - 1;

                //    for (SampleIndex i = (SampleIndex)(this->ip_05_lastIndex); i < k; i++) {
                //        if (this->ip_05_resetCount > 0) {
                //            this->ip_05_sigbuf[(Index)i] = 1;
                //            this->ip_05_resetCount--;
                //        } else {
                //            this->ip_05_sigbuf[(Index)i] = this->ip_05_lastValue;
                //        }
                //    }

                //    this->ip_05_lastIndex = k;
                //}
            //}
            //
            //void ip_05_dspsetup(bool force) {
            //    if ((bool)(this->ip_05_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    //this->ip_05_lastIndex = 0;
            //    this->ip_05_setupDone = true;
            //}
            //
            //void ip_06_init() {
            //    this->ip_06_lastValue = this->ip_06_value;
            //}

            //void ip_06_fillSigBuf() {
            //    if ((bool)(this->ip_06_sigbuf)) {
            //        //SampleIndex k = (SampleIndex)(this->sampleOffsetIntoNextAudioBuffer);
            //        //SampleIndex k = 0;
            //
            //        //if (k >= (SampleIndex)(this->vs))
            //        //    k = (SampleIndex)(this->vs) - 1;
            //
            //        //for (SampleIndex i = (SampleIndex)(this->ip_06_lastIndex); i < k; i++) {
            //        //    if (this->ip_06_resetCount > 0) {
            //        //        this->ip_06_sigbuf[(Index)i] = 1;
            //        //        this->ip_06_resetCount--;
            //        //    } else {
            //        //        this->ip_06_sigbuf[(Index)i] = this->ip_06_lastValue;
            //        //    }
            //        //}
            //
            //        //this->ip_06_lastIndex = k;
            //    }
            //}
            //
            //void ip_06_dspsetup(bool force) {
            //    if ((bool)(this->ip_06_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    //this->ip_06_lastIndex = 0;
            //    this->ip_06_setupDone = true;
            //}
            //
            //number sah_tilde_01_s_next(number x, number trig, number thresh) {
            //    if (this->sah_tilde_01_s_prev <= thresh && trig > thresh)
            //        this->sah_tilde_01_s_value = x;
            //
            //    this->sah_tilde_01_s_prev = trig;
            //    return this->sah_tilde_01_s_value;
            //}
            //
            //void sah_tilde_01_s_reset() {
            //    this->sah_tilde_01_s_prev = 0.;
            //    this->sah_tilde_01_s_value = 0.;
            //}

            //void edge_01_dspsetup(bool force) {
            //    if ((bool)(this->edge_01_setupDone) && (bool)(!(bool)(force)))
            //        return;
            //
            //    this->edge_01_setupDone = true;
            //}

            void midiouthelper_sendMidi(number v) {
                this->midiouthelper_midiout_set(v);
            }

            bool stackprotect_check() {
                this->stackprotect_count++;

                if (this->stackprotect_count > 128) {
                    console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
                    return true;
                }

                return false;
            }

            void updateTime(MillisecondTime time) {
                this->_currentTime = time;
                //this->sampleOffsetIntoNextAudioBuffer = (SampleIndex)(rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr)));

                //if (this->sampleOffsetIntoNextAudioBuffer >= (SampleIndex)(this->vs))
                //    this->sampleOffsetIntoNextAudioBuffer = (SampleIndex)(this->vs) - 1;

                //if (this->sampleOffsetIntoNextAudioBuffer < 0)
                //    this->sampleOffsetIntoNextAudioBuffer = 0;
            }

            void assign_defaults()
            {
                //gen_01_in1 = 0;
                //gen_01_in2 = 0;
                //gen_01_in3 = 0;
                //gen_01_in4 = 0;
                //gen_01_in5 = 0;
                //gen_01_in6 = 0;
                //gen_01_in7 = 0;
                //gen_01_in8 = 0;
                //gen_01_in9 = 0;
                //unpack_01_out1 = 0;
                //unpack_01_out2 = 0;
                //unpack_01_out3 = 0;
                //unpack_01_out4 = 0;
                //unpack_01_out5 = 0;
                //unpack_01_out6 = 0;
                position_value = 0;
                //ip_01_impulse = 0;
                grainsize_value = 0;
                //ip_02_impulse = 0;
                direction_value = 0;
                //ip_03_impulse = 0;
                pishift_value = 0;
                //ip_04_impulse = 0;
                volume_value = 0;
                //ip_05_impulse = 0;
                panning_value = 0;
                //ip_06_impulse = 0;
                /*sah_tilde_01_input = 0;
                sah_tilde_01_trig = -1;
                sah_tilde_01_thresh = 0;*/
                voice_01_mute_number = 0;
                _currentTime = 0;
                audioProcessSampleCount = 0;
                //sampleOffsetIntoNextAudioBuffer = 0;
                zeroBuffer = nullptr;
                dummyBuffer = nullptr;
                realtime_grain_out[0] = nullptr;
                realtime_grain_out[1] = nullptr;
                //realtime_grain_out[2] = nullptr;
                //realtime_grain_out[3] = nullptr;
                //realtime_grain_out[4] = nullptr;
                //realtime_grain_out[5] = nullptr;
                //realtime_grain_out[6] = nullptr;
                //realtime_grain_out[7] = nullptr;
                //realtime_grain_out[8] = nullptr;
                //realtime_grain_out[9] = nullptr;
                //realtime_grain_out[10] = nullptr;
                didAllocateSignals = 0;
                vs = 0;
                maxvs = 0;
                sr = 44100;
                invsr = 0.00002267573696;
                triggerindex = -1;
                //click_01_buf = nullptr;
                play_inc_history = 0;
                env_inc_history = 0;
                gen_01_counter_11_carry = 0;
                gen_01_counter_11_count = 0;
                gen_01_latch_15_value = 0;
                gen_01_latch_18_value = 0;
                rel_start_pos_history = 0;
                gen_01_counter_48_carry = 0;
                gen_01_counter_48_count = 0;
                gen_01_latch_52_value = 0;
                start_play_pos_history = 0;
                gen_01_setupDone = false;
                //ip_01_lastIndex = 0;
                //ip_01_lastValue = 0;
                //ip_01_resetCount = 0;
                //ip_01_sigbuf = nullptr;
                //ip_01_setupDone = false;
                ////ip_02_lastIndex = 0;
                //ip_02_lastValue = 0;
                //ip_02_resetCount = 0;
                //ip_02_sigbuf = nullptr;
                //ip_02_setupDone = false;
                //ip_03_lastIndex = 0;
                //ip_03_lastValue = 0;
                //ip_03_resetCount = 0;
                //ip_03_sigbuf = nullptr;
                //ip_03_setupDone = false;
                //ip_04_lastIndex = 0;
                //ip_04_lastValue = 0;
                //ip_04_resetCount = 0;
                //ip_04_sigbuf = nullptr;
                //ip_04_setupDone = false;
                //ip_05_lastIndex = 0;
                //ip_05_lastValue = 0;
                //ip_05_resetCount = 0;
                //ip_05_sigbuf = nullptr;
                //ip_05_setupDone = false;
                //ip_06_lastIndex = 0;
                //ip_06_lastValue = 0;
                //ip_06_resetCount = 0;
                //ip_06_sigbuf = nullptr;
                //ip_06_setupDone = false;
                reverse_offset = 0;
                //sah_tilde_01_s_prev = 0;
                //sah_tilde_01_s_value = 0;
                //click_02_lastclick = -1;
                //click_02_buf = nullptr;
                //edge_01_setupDone = false;
                stackprotect_count = 0;
                _voiceIndex = 0;
                _noteNumber = 0;
                isMuted = 0;
                parameterOffset = 0;
            }

            // member variables

                //number gen_01_in1;
                //number gen_01_in2;
                //number gen_01_in3;
                //number gen_01_in4;
                //number gen_01_in5;
                //number gen_01_in6;
                //number gen_01_in7;
                //number gen_01_in8;
                //number gen_01_in9;
                //number unpack_01_out1;
                //number unpack_01_out2;
                //number unpack_01_out3;
                //number unpack_01_out4;
                //number unpack_01_out5;
                //number unpack_01_out6;
            number position_value;
            //number ip_01_impulse;
            number grainsize_value;
            //number ip_02_impulse;
            number direction_value;
            //number ip_03_impulse;
            number pishift_value;
            //number ip_04_impulse;
            number volume_value;
            //number ip_05_impulse;
            number panning_value;
            //number ip_06_impulse;
            /*number sah_tilde_01_input;
            number sah_tilde_01_trig;
            number sah_tilde_01_thresh;*/
            number voice_01_mute_number;
            MillisecondTime _currentTime;
            SampleIndex audioProcessSampleCount;
            //SampleIndex sampleOffsetIntoNextAudioBuffer;
            signal zeroBuffer;
            signal dummyBuffer;
            SampleValue* realtime_grain_out[2];
            bool didAllocateSignals;
            Index vs;
            Index maxvs;
            number sr;
            number invsr;
            SampleIndex triggerindex;
            //signal click_01_buf;
            int play_inc_history;
            int env_inc_history;
            Float32BufferRef gen_01_rtbuf;
            Float32BufferRef gen_01_interpol_env;
            int gen_01_counter_11_carry;
            number gen_01_counter_11_count;
            number gen_01_latch_15_value;
            number gen_01_latch_18_value;
            number rel_start_pos_history;
            int gen_01_counter_48_carry;
            number gen_01_counter_48_count;
            number gen_01_latch_52_value;
            number start_play_pos_history;
            bool gen_01_setupDone;
            /*SampleIndex ip_01_lastIndex;
            number ip_01_lastValue;
            SampleIndex ip_01_resetCount;
            signal ip_01_sigbuf;
            bool ip_01_setupDone;
            SampleIndex ip_02_lastIndex;
            number ip_02_lastValue;
            SampleIndex ip_02_resetCount;
            signal ip_02_sigbuf;
            bool ip_02_setupDone;
            SampleIndex ip_03_lastIndex;
            number ip_03_lastValue;
            SampleIndex ip_03_resetCount;
            signal ip_03_sigbuf;
            bool ip_03_setupDone;
            SampleIndex ip_04_lastIndex;
            number ip_04_lastValue;
            SampleIndex ip_04_resetCount;
            signal ip_04_sigbuf;
            bool ip_04_setupDone;
            SampleIndex ip_05_lastIndex;
            number ip_05_lastValue;
            SampleIndex ip_05_resetCount;
            signal ip_05_sigbuf;
            bool ip_05_setupDone;
            SampleIndex ip_06_lastIndex;
            number ip_06_lastValue;
            SampleIndex ip_06_resetCount;
            signal ip_06_sigbuf;
            bool ip_06_setupDone;*/

            number reverse_offset;

            //number sah_tilde_01_s_prev;
            //number sah_tilde_01_s_value;
            //SampleIndex click_02_lastclick;
            //signal click_02_buf;
            //number edge_01_currentState;
            //bool edge_01_setupDone;
            number stackprotect_count;
            Index _voiceIndex;
            Int _noteNumber;
            Index isMuted;
            ParameterIndex parameterOffset;

        };

        rnbomatic()
        {
        }

        /*S-mod*/
        ~rnbomatic()
        {
            for (int i = 0; i < 6; i++) {
                free(this->signals[i]);
            }

            free(this->phasor_01_sigbuf);
            free(this->phasor_02_sigbuf);
            free(this->phasor_03_sigbuf);
            free(this->feedbacktilde_01_feedbackbuffer);
            free(this->globaltransport_tempo);
            free(this->globaltransport_state);
            free(this->zeroBuffer);
            free(this->dummyBuffer);

            for (int i = 0; i < 24; i++) {
                delete rtgrainvoice[i];
            }
        }
        /*E-mod*/

        rnbomatic* getTopLevelPatcher() {
            return this;
        }

        void cancelClockEvents()
        {
            getEngine()->flushClockEvents(this, -611950441, false);
            getEngine()->flushClockEvents(this, -1584063977, false);
            getEngine()->flushClockEvents(this, 1821745152, false);
        }

        template <typename T> void listquicksort(T& arr, T& sortindices, Int l, Int h, bool ascending) {
            if (l < h) {
                Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
                this->listquicksort(arr, sortindices, l, p - 1, ascending);
                this->listquicksort(arr, sortindices, p + 1, h, ascending);
            }
        }

        template <typename T> Int listpartition(T& arr, T& sortindices, Int l, Int h, bool ascending) {
            number x = arr[(Index)h];
            Int i = (Int)(l - 1);

            for (Int j = (Int)(l); j <= h - 1; j++) {
                bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
                bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

                if ((bool)(asc) || (bool)(desc)) {
                    i++;
                    this->listswapelements(arr, i, j);
                    this->listswapelements(sortindices, i, j);
                }
            }

            i++;
            this->listswapelements(arr, i, h);
            this->listswapelements(sortindices, i, h);
            return i;
        }

        template <typename T> void listswapelements(T& arr, Int a, Int b) {
            auto tmp = arr[(Index)a];
            arr[(Index)a] = arr[(Index)b];
            arr[(Index)b] = tmp;
        }

        number samplerate() {
            return this->sr;
        }

        Index voice() {
            return this->_voiceIndex;
        }

        number random(number low, number high) {
            number range = high - low;
            return rand01() * range + low;
        }

        number mstosamps(MillisecondTime ms) {
            return ms * this->sr * 0.001;
        }

        number maximum(number x, number y) {
            return (x < y ? y : x);
        }

        inline number safediv(number num, number denom) {
            return (denom == 0.0 ? 0.0 : num / denom);
        }

        number safepow(number base, number exponent) {
            return fixnan(rnbo_pow(base, exponent));
        }

        number scale(
            number x,
            number lowin,
            number hiin,
            number lowout,
            number highout,
            number pow
        ) {
            auto inscale = this->safediv(1., hiin - lowin);
            number outdiff = highout - lowout;
            number value = (x - lowin) * inscale;

            if (pow != 1) {
                if (value > 0)
                    value = this->safepow(value, pow);
                else
                    value = -this->safepow(-value, pow);
            }

            value = value * outdiff + lowout;
            return value;
        }

        MillisecondTime currenttime() {
            return this->_currentTime;
        }

        inline number safemod(number f, number m) {
            if (m != 0) {
                Int f_trunc = (Int)(trunc(f));
                Int m_trunc = (Int)(trunc(m));

                if (f == f_trunc && m == m_trunc) {
                    f = f_trunc % m_trunc;
                }
                else {
                    if (m < 0) {
                        m = -m;
                    }

                    if (f >= m) {
                        if (f >= m * 2.0) {
                            number d = f / m;
                            Int i = (Int)(trunc(d));
                            d = d - i;
                            f = d * m;
                        }
                        else {
                            f -= m;
                        }
                    }
                    else if (f <= -m) {
                        if (f <= -m * 2.0) {
                            number d = f / m;
                            Int i = (Int)(trunc(d));
                            d = d - i;
                            f = d * m;
                        }
                        else {
                            f += m;
                        }
                    }
                }
            }
            else {
                f = 0.0;
            }

            return f;
        }

        array<ListNum, 2> listcompare(const list& input1, const list& input2) {
            array<ListNum, 2> tmp = {};
            tmp[0] = 0;
            tmp[1] = {};
            list longList = (input1->length >= input2->length ? input1 : input2);
            list shortList = (input2->length <= input1->length ? input2 : input1);

            for (Index i = 0; i < shortList->length; i++) {
                if (longList[(Index)i] != shortList[(Index)i])
                    tmp[1]->push(i);
            }

            for (Index i = (Index)(shortList->length); i < longList->length; i++) {
                tmp[1]->push(i);
            }

            if (tmp[1]->length > 0) {
                tmp[0] = 0;
            }
            else {
                tmp[0] = 1;
            }

            return tmp;
        }

        inline number linearinterp(number frac, number x, number y) {
            return x + (y - x) * frac;
        }

        inline number cubicinterp(number a, number w, number x, number y, number z) {
            number a2 = a * a;
            number f0 = z - y - w + x;
            number f1 = w - x - f0;
            number f2 = y - w;
            number f3 = x;
            return f0 * a * a2 + f1 * a2 + f2 * a + f3;
        }

        inline number splineinterp(number a, number w, number x, number y, number z) {
            number a2 = a * a;
            number f0 = -0.5 * w + 1.5 * x - 1.5 * y + 0.5 * z;
            number f1 = w - 2.5 * x + 2 * y - 0.5 * z;
            number f2 = -0.5 * w + 0.5 * y;
            return f0 * a * a2 + f1 * a2 + f2 * a + x;
        }

        inline number cosT8(number r) {
            number t84 = 56.0;
            number t83 = 1680.0;
            number t82 = 20160.0;
            number t81 = 2.4801587302e-05;
            number t73 = 42.0;
            number t72 = 840.0;
            number t71 = 1.9841269841e-04;

            if (r < 0.785398163397448309615660845819875721 && r > -0.785398163397448309615660845819875721) {
                number rr = r * r;
                return 1.0 - rr * t81 * (t82 - rr * (t83 - rr * (t84 - rr)));
            }
            else if (r > 0.0) {
                r -= 1.57079632679489661923132169163975144;
                number rr = r * r;
                return -r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
            }
            else {
                r += 1.57079632679489661923132169163975144;
                number rr = r * r;
                return r * (1.0 - t71 * rr * (t72 - rr * (t73 - rr)));
            }
        }

        inline number cosineinterp(number frac, number x, number y) {
            number a2 = (1.0 - this->cosT8(frac * 3.14159265358979323846)) / (number)2.0;
            return x * (1.0 - a2) + y * a2;
        }

        Index vectorsize() {
            return this->vs;
        }

        number tempo() {
            return this->getTopLevelPatcher()->globaltransport_getTempo(this->currenttime());
        }

        number beatstohz(number beattime) {
            return this->safediv(this->tempo(), beattime * 60);
        }

        number __wrapped_op_round(number in1, number in2) {
            return (in2 == 0 ? 0 : rnbo_fround(in1 * 1 / in2) * in2);
        }

        ParameterValue tonormalized(ParameterIndex index, ParameterValue value) {
            return this->convertToNormalizedParameterValue(index, value);
        }

        number hztobeats(number hz) {
            return this->safediv(this->tempo() * 8, hz * 480.);
        }

        number beattimeatsample(number offset) {
            return this->getTopLevelPatcher()->globaltransport_getBeatTimeAtSample(offset);
        }

        number transportatsample(SampleIndex sampleOffset) {
            return this->getTopLevelPatcher()->globaltransport_getStateAtSample(sampleOffset);
        }

        ParameterValue fromnormalized(ParameterIndex index, ParameterValue normalizedValue) {
            return this->convertFromNormalizedParameterValue(index, normalizedValue);
        }

        number tickstohz(number ticks) {
            return (number)1 / (ticks / (number)480 * this->safediv(60, this->tempo()));
        }

        number mstobeats(number ms) {
            return ms * this->tempo() * 0.008 / (number)480;
        }

        MillisecondTime sampstoms(number samps) {
            return samps * 1000 / this->sr;
        }

        Index getNumMidiInputPorts() const {
            return 1;
        }

        void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length) {
            this->updateTime(time);
            this->ctlin_01_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
        }

        Index getNumMidiOutputPorts() const {
            return 0;
        }

        void process(
            const SampleValue* const* inputs,
            Index numInputs,
            SampleValue* const* outputs,
            Index numOutputs,
            Index n
        ) {
            this->vs = n;
            this->updateTime(this->getEngine()->getCurrentTime());
            SampleValue* out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
            SampleValue* out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
            const SampleValue* in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
            const SampleValue* in2 = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);
            this->phasor_01_perform(this->phasor_01_freq, this->signals[0], n);
            this->phasor_02_perform(this->phasor_02_freq, this->signals[1], n);
            this->phasor_03_perform(this->phasor_03_freq, this->signals[2], n);

            this->codebox_tilde_01_perform(
                this->codebox_tilde_01_in1,
                this->codebox_tilde_01_in2,
                this->codebox_tilde_01_in3,
                this->codebox_tilde_01_in4,
                this->codebox_tilde_01_in5,
                this->codebox_tilde_01_in6,
                this->codebox_tilde_01_in7,
                this->codebox_tilde_01_in8,
                this->signals[0],
                this->signals[1],
                this->signals[2],
                this->signals[3],
                n
            );

            this->edge_02_perform(this->signals[3], n);

            this->codebox_tilde_02_perform(
                this->codebox_tilde_02_in1,
                this->codebox_tilde_02_in2,
                this->signals[3],
                n
            );

            this->dspexpr_04_perform(in1, in2, this->signals[2], n);
            this->dspexpr_03_perform(this->signals[2], this->dspexpr_03_in2, this->signals[1], n);
            this->dspexpr_06_perform(this->signals[1], this->dspexpr_06_in2, this->signals[2], n);

            this->codebox_tilde_03_perform(
                this->codebox_tilde_03_in1,
                this->codebox_tilde_03_in2,
                this->signals[1],
                this->dummyBuffer,
                this->dummyBuffer,
                n
            );

            this->feedbackreader_01_perform(this->signals[0], n);
            this->dspexpr_07_perform(this->signals[0], this->dspexpr_07_in2, this->signals[4], n);
            this->dspexpr_05_perform(this->signals[2], this->signals[4], this->signals[0], n);

            this->recordtilde_01_perform(
                this->signals[3],
                this->recordtilde_01_begin,
                this->recordtilde_01_end,
                this->signals[0],
                this->signals[4],
                n
            );

            this->latch_tilde_01_perform(this->signals[4], this->latch_tilde_01_control, this->signals[2], n);

            //this->p_01_perform(this->signals[1], this->signals[2], this->signals[4], this->signals[5], n);          //!!!!

            {
                ConstSampleArray<2> ins = { signals[1], signals[2] };
                SampleArray<2> outs = { signals[4], signals[5] };

                for (number chan = 0; chan < 2; chan++)
                    zeroSignal(outs[(Index)chan], n);

                for (Index i = 0; i < 24; i++)
                    this->rtgrainvoice[i]->process(ins, 2, outs, 2, n);
            }

            this->dcblock_tilde_01_perform(this->signals[4], this->dcblock_tilde_01_gain, this->signals[2], n);
            this->feedbackwriter_01_perform(this->signals[2], n);
            this->limi_01_perform(this->signals[4], this->signals[5], this->signals[2], this->signals[1], n);
            this->dspexpr_01_perform(this->signals[2], this->dspexpr_01_in2, out1, n);
            this->dspexpr_02_perform(this->signals[1], this->dspexpr_02_in2, out2, n);

            this->recordtilde_02_perform(
                this->signals[3],
                this->recordtilde_02_begin,
                this->recordtilde_02_end,
                this->signals[0],
                this->dummyBuffer,
                n
            );

            this->stackprotect_perform(n);
            this->globaltransport_advance();
            this->audioProcessSampleCount += this->vs;
        }

        void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
            if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
                Index i;

                for (i = 0; i < 6; i++) {
                    this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
                }

                this->phasor_01_sigbuf = resizeSignal(this->phasor_01_sigbuf, this->maxvs, maxBlockSize);
                this->phasor_02_sigbuf = resizeSignal(this->phasor_02_sigbuf, this->maxvs, maxBlockSize);
                this->phasor_03_sigbuf = resizeSignal(this->phasor_03_sigbuf, this->maxvs, maxBlockSize);
                this->feedbacktilde_01_feedbackbuffer = resizeSignal(this->feedbacktilde_01_feedbackbuffer, this->maxvs, maxBlockSize);
                this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
                this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
                this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
                this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
                this->didAllocateSignals = true;
            }

            const bool sampleRateChanged = sampleRate != this->sr;
            const bool maxvsChanged = maxBlockSize != this->maxvs;
            const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

            if (sampleRateChanged || maxvsChanged) {
                this->vs = maxBlockSize;
                this->maxvs = maxBlockSize;
                this->sr = sampleRate;
                this->invsr = 1 / sampleRate;
            }

            this->data_01_dspsetup(forceDSPSetup);
            this->phasor_01_dspsetup(forceDSPSetup);
            this->phasor_02_dspsetup(forceDSPSetup);
            this->phasor_03_dspsetup(forceDSPSetup);
            this->codebox_tilde_01_dspsetup(forceDSPSetup);
            this->edge_02_dspsetup(forceDSPSetup);
            this->data_02_dspsetup(forceDSPSetup);
            this->data_03_dspsetup(forceDSPSetup);
            this->latch_tilde_01_dspsetup(forceDSPSetup);
            this->dcblock_tilde_01_dspsetup(forceDSPSetup);
            this->limi_01_dspsetup(forceDSPSetup);
            this->globaltransport_dspsetup(forceDSPSetup);

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->prepareToProcess(sampleRate, maxBlockSize, force);
            }

            if (sampleRateChanged)
                this->onSampleRateChanged(sampleRate);
        }

        void setProbingTarget(MessageTag id) {
            switch (id) {
            default:
            {
                this->setProbingIndex(-1);
                break;
            }
            }
        }

        void setProbingIndex(ProbingIndex) {}

        Index getProbingChannels(MessageTag outletId) const {
            RNBO_UNUSED(outletId);
            return 0;
        }

        DataRef* getDataRef(DataRefIndex index) {
            switch (index) {
            case 0:
            {
                return addressOf(this->borisinrnbo_v01_rtbuf);
                break;
            }
            case 1:
            {
                return addressOf(this->interpolated_envelope);
                break;
            }
            case 2:
            {
                return addressOf(this->codebox_tilde_01_del_bufferobj);
                break;
            }
            case 3:
            {
                return addressOf(this->inter_databuf_01);
                break;
            }
            default:
            {
                return nullptr;
            }
            }
        }

        DataRefIndex getNumDataRefs() const {
            return 4;
        }

        void fillDataRef(DataRefIndex, DataRef&) {}

        void zeroDataRef(DataRef& ref) {
            ref->setZero();
        }

        void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
            this->updateTime(time);

            if (index == 0) {
                this->recordtilde_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
                this->bufferop_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
                this->data_03_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
                this->data_03_bufferUpdated();
            }

            if (index == 1) {
                this->data_01_buffer = new Float32Buffer(this->interpolated_envelope);
                this->data_01_bufferUpdated();
            }

            if (index == 2) {
                this->codebox_tilde_01_del_buffer = new Float64Buffer(this->codebox_tilde_01_del_bufferobj);
            }

            if (index == 3) {
                this->recordtilde_02_buffer = new Float32Buffer(this->inter_databuf_01);
                this->data_02_buffer = new Float32Buffer(this->inter_databuf_01);
                this->data_02_bufferUpdated();
            }

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->processDataViewUpdate(index, time);
            }
        }

        void initialize() {
            this->borisinrnbo_v01_rtbuf = initDataRef("borisinrnbo_v01_rtbuf", true, nullptr, "data");
            this->interpolated_envelope = initDataRef("interpolated_envelope", false, nullptr, "buffer~");
            this->codebox_tilde_01_del_bufferobj = initDataRef("codebox_tilde_01_del_bufferobj", true, nullptr, "buffer~");
            this->inter_databuf_01 = initDataRef("inter_databuf_01", false, nullptr, "data");
            this->assign_defaults();
            this->setState();
            this->borisinrnbo_v01_rtbuf->setIndex(0);
            this->recordtilde_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->bufferop_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->data_03_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->interpolated_envelope->setIndex(1);
            this->data_01_buffer = new Float32Buffer(this->interpolated_envelope);
            this->codebox_tilde_01_del_bufferobj->setIndex(2);
            this->codebox_tilde_01_del_buffer = new Float64Buffer(this->codebox_tilde_01_del_bufferobj);
            this->inter_databuf_01->setIndex(3);
            this->recordtilde_02_buffer = new Float32Buffer(this->inter_databuf_01);
            this->data_02_buffer = new Float32Buffer(this->inter_databuf_01);
            this->initializeObjects();
            this->allocateDataRefs();
            this->startup();
        }

        Index getIsMuted() {
            return this->isMuted;
        }

        void setIsMuted(Index v) {
            this->isMuted = v;
        }

        Index getPatcherSerial() const {
            return 0;
        }

        void getState(PatcherStateInterface&) {}

        void setState() {
            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[(Index)i] = new RTGrainVoice();
                this->rtgrainvoice[(Index)i]->setEngineAndPatcher(this->getEngine(), this);
                this->rtgrainvoice[(Index)i]->initialize();
                this->rtgrainvoice[(Index)i]->setParameterOffset(this->getParameterOffset(this->rtgrainvoice[0]));
                this->rtgrainvoice[(Index)i]->setVoiceIndex(i + 1);
            }
        }

        void getPreset(PatcherStateInterface& preset) {
            preset["__presetid"] = "rnbo";
            this->param_01_getPresetValue(getSubState(preset, "den"));
            this->param_02_getPresetValue(getSubState(preset, "cha"));
            this->param_03_getPresetValue(getSubState(preset, "rdl"));
            this->param_04_getPresetValue(getSubState(preset, "len"));
            this->param_05_getPresetValue(getSubState(preset, "rle"));
            this->param_06_getPresetValue(getSubState(preset, "psh"));
            this->param_07_getPresetValue(getSubState(preset, "rpt"));
            this->param_08_getPresetValue(getSubState(preset, "env"));
            this->param_09_getPresetValue(getSubState(preset, "frp"));
            this->param_10_getPresetValue(getSubState(preset, "cpo"));
            this->param_11_getPresetValue(getSubState(preset, "drf"));
            this->param_12_getPresetValue(getSubState(preset, "pwi"));
            this->param_13_getPresetValue(getSubState(preset, "rvo"));
            this->param_14_getPresetValue(getSubState(preset, "gai"));
            this->param_15_getPresetValue(getSubState(preset, "fdb"));
            this->param_16_getPresetValue(getSubState(preset, "wet"));
            this->param_17_getPresetValue(getSubState(preset, "mut"));
            this->param_18_getPresetValue(getSubState(preset, "frz"));
            this->param_19_getPresetValue(getSubState(preset, "syc"));
            this->param_20_getPresetValue(getSubState(preset, "tmp"));
            this->param_21_getPresetValue(getSubState(preset, "rtm"));

            for (Index i = 0; i < 24; i++)
                this->rtgrainvoice[i]->getPreset(getSubStateAt(getSubState(preset, "__sps"), "rtgrains", i));
        }

        void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
            this->updateTime(time);
            this->param_14_setPresetValue(getSubState(preset, "gai"));
            this->param_19_setPresetValue(getSubState(preset, "syc"));
            this->param_20_setPresetValue(getSubState(preset, "tmp"));
            this->param_21_setPresetValue(getSubState(preset, "rtm"));
            this->param_17_setPresetValue(getSubState(preset, "mut"));
            this->param_02_setPresetValue(getSubState(preset, "cha"));
            this->param_01_setPresetValue(getSubState(preset, "den"));
            this->param_04_setPresetValue(getSubState(preset, "len"));
            this->param_10_setPresetValue(getSubState(preset, "cpo"));
            this->param_11_setPresetValue(getSubState(preset, "drf"));
            this->param_06_setPresetValue(getSubState(preset, "psh"));
            this->param_03_setPresetValue(getSubState(preset, "rdl"));
            this->param_05_setPresetValue(getSubState(preset, "rle"));
            this->param_07_setPresetValue(getSubState(preset, "rpt"));
            this->param_08_setPresetValue(getSubState(preset, "env"));
            this->param_09_setPresetValue(getSubState(preset, "frp"));
            this->param_13_setPresetValue(getSubState(preset, "rvo"));
            this->param_12_setPresetValue(getSubState(preset, "pwi"));
            this->param_18_setPresetValue(getSubState(preset, "frz"));
            this->param_15_setPresetValue(getSubState(preset, "fdb"));
            this->param_16_setPresetValue(getSubState(preset, "wet"));
        }

        void processTempoEvent(MillisecondTime time, Tempo tempo) {
            this->updateTime(time);

            if (this->globaltransport_setTempo(this->_currentTime, tempo, false)) {
                for (Index i = 0; i < 24; i++) {
                    this->rtgrainvoice[i]->processTempoEvent(time, tempo);
                }

                this->timevalue_01_onTempoChanged(tempo);
                this->timevalue_02_onTempoChanged(tempo);
                this->timevalue_03_onTempoChanged(tempo);
            }
        }

        void processTransportEvent(MillisecondTime time, TransportState state) {
            this->updateTime(time);

            if (this->globaltransport_setState(this->_currentTime, state, false)) {
                for (Index i = 0; i < 24; i++) {
                    this->rtgrainvoice[i]->processTransportEvent(time, state);
                }
            }
        }

        void processBeatTimeEvent(MillisecondTime time, BeatTime beattime) {
            this->updateTime(time);

            if (this->globaltransport_setBeatTime(this->_currentTime, beattime, false)) {
                for (Index i = 0; i < 24; i++) {
                    this->rtgrainvoice[i]->processBeatTimeEvent(time, beattime);
                }
            }
        }

        void onSampleRateChanged(double samplerate) {
            this->timevalue_01_onSampleRateChanged(samplerate);
            this->timevalue_02_onSampleRateChanged(samplerate);
            this->timevalue_03_onSampleRateChanged(samplerate);
        }

        void processTimeSignatureEvent(MillisecondTime time, int numerator, int denominator) {
            this->updateTime(time);

            if (this->globaltransport_setTimeSignature(this->_currentTime, numerator, denominator, false)) {
                for (Index i = 0; i < 24; i++) {
                    this->rtgrainvoice[i]->processTimeSignatureEvent(time, numerator, denominator);
                }

                this->timevalue_01_onTimeSignatureChanged(numerator, denominator);
                this->timevalue_02_onTimeSignatureChanged(numerator, denominator);
                this->timevalue_03_onTimeSignatureChanged(numerator, denominator);
            }
        }

        void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
            this->updateTime(time);

            switch (index) {
            case 0:
            {
                this->param_01_value_set(v);
                break;
            }
            case 1:
            {
                this->param_02_value_set(v);
                break;
            }
            case 2:
            {
                this->param_03_value_set(v);
                break;
            }
            case 3:
            {
                this->param_04_value_set(v);
                break;
            }
            case 4:
            {
                this->param_05_value_set(v);
                break;
            }
            case 5:
            {
                this->param_06_value_set(v);
                break;
            }
            case 6:
            {
                this->param_07_value_set(v);
                break;
            }
            case 7:
            {
                this->param_08_value_set(v);
                break;
            }
            case 8:
            {
                this->param_09_value_set(v);
                break;
            }
            case 9:
            {
                this->param_10_value_set(v);
                break;
            }
            case 10:
            {
                this->param_11_value_set(v);
                break;
            }
            case 11:
            {
                this->param_12_value_set(v);
                break;
            }
            case 12:
            {
                this->param_13_value_set(v);
                break;
            }
            case 13:
            {
                this->param_14_value_set(v);
                break;
            }
            case 14:
            {
                this->param_15_value_set(v);
                break;
            }
            case 15:
            {
                this->param_16_value_set(v);
                break;
            }
            case 16:
            {
                this->param_17_value_set(v);
                break;
            }
            case 17:
            {
                this->param_18_value_set(v);
                break;
            }
            case 18:
            {
                this->param_19_value_set(v);
                break;
            }
            case 19:
            {
                this->param_20_value_set(v);
                break;
            }
            case 20:
            {
                this->param_21_value_set(v);
                break;
            }
            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters())
                    this->rtgrainvoice[0]->setPolyParameterValue((PatcherInterface**)this->rtgrainvoice, index, v, time);

                break;
            }
            }
        }

        void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
            this->setParameterValue(index, value, time);
        }

        void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
            this->setParameterValue(index, this->getParameterValue(index), time);
        }

        void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
            this->setParameterValueNormalized(index, value, time);
        }

        ParameterValue getParameterValue(ParameterIndex index) {
            switch (index) {
            case 0:
            {
                return this->param_01_value;
            }
            case 1:
            {
                return this->param_02_value;
            }
            case 2:
            {
                return this->param_03_value;
            }
            case 3:
            {
                return this->param_04_value;
            }
            case 4:
            {
                return this->param_05_value;
            }
            case 5:
            {
                return this->param_06_value;
            }
            case 6:
            {
                return this->param_07_value;
            }
            case 7:
            {
                return this->param_08_value;
            }
            case 8:
            {
                return this->param_09_value;
            }
            case 9:
            {
                return this->param_10_value;
            }
            case 10:
            {
                return this->param_11_value;
            }
            case 11:
            {
                return this->param_12_value;
            }
            case 12:
            {
                return this->param_13_value;
            }
            case 13:
            {
                return this->param_14_value;
            }
            case 14:
            {
                return this->param_15_value;
            }
            case 15:
            {
                return this->param_16_value;
            }
            case 16:
            {
                return this->param_17_value;
            }
            case 17:
            {
                return this->param_18_value;
            }
            case 18:
            {
                return this->param_19_value;
            }
            case 19:
            {
                return this->param_20_value;
            }
            case 20:
            {
                return this->param_21_value;
            }
            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters())
                    return this->rtgrainvoice[0]->getPolyParameterValue((PatcherInterface**)this->rtgrainvoice, index);

                return 0;
            }
            }
        }

        ParameterIndex getNumSignalInParameters() const {
            return 0;
        }

        ParameterIndex getNumSignalOutParameters() const {
            return 0;
        }

        ParameterIndex getNumParameters() const {
            return 21 + this->rtgrainvoice[0]->getNumParameters();
        }

        ConstCharPointer getParameterName(ParameterIndex index) const {
            switch (index) {
            case 0:
            {
                return "den";
            }
            case 1:
            {
                return "cha";
            }
            case 2:
            {
                return "rdl";
            }
            case 3:
            {
                return "len";
            }
            case 4:
            {
                return "rle";
            }
            case 5:
            {
                return "psh";
            }
            case 6:
            {
                return "rpt";
            }
            case 7:
            {
                return "env";
            }
            case 8:
            {
                return "frp";
            }
            case 9:
            {
                return "cpo";
            }
            case 10:
            {
                return "drf";
            }
            case 11:
            {
                return "pwi";
            }
            case 12:
            {
                return "rvo";
            }
            case 13:
            {
                return "gai";
            }
            case 14:
            {
                return "fdb";
            }
            case 15:
            {
                return "wet";
            }
            case 16:
            {
                return "mut";
            }
            case 17:
            {
                return "frz";
            }
            case 18:
            {
                return "syc";
            }
            case 19:
            {
                return "tmp";
            }
            case 20:
            {
                return "rtm";
            }
            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters()) {
                    {
                        return this->rtgrainvoice[0]->getParameterName(index);
                    }
                }

                return "bogus";
            }
            }
        }

        ConstCharPointer getParameterId(ParameterIndex index) const {
            switch (index) {
            case 0:
            {
                return "den";       // "linear" -> exp handled internally
            }
            case 1:
            {
                return "cha";       // linear
            }
            case 2:
            {
                return "rdl";       // linear
            }
            case 3:
            {
                return "len";       // JUCE skew
            }
            case 4:
            {
                return "rle";       // linear
            }
            case 5:
            {
                return "psh";       // pitch pow
            }
            case 6:
            {
                return "rpt";       // linear
            }
            case 7:
            {
                return "env";       // linear
            }
            case 8:
            {
                return "frp";       // linear
            }
            case 9:
            {
                return "cpo";       // linear
            }
            case 10:
            {
                return "drf";       // linear
            }
            case 11:
            {
                return "pwi";       // linear
            }
            case 12:
            {
                return "rvo";       // linear
            }
            case 13:
            {
                return "gai";       // ??
            }
            case 14:
            {
                return "fdb";       // linear
            }
            case 15:
            {
                return "wet";       // ??
            }
            case 16:
            {
                return "mut";       // tog
            }
            case 17:
            {
                return "frz";       // tog
            }
            case 18:
            {
                return "syc";       // tog
            }
            case 19:
            {
                return "tmp";       // discrete steps but linear
            }
            case 20:
            {
                return "rtm";       // 3tog
            }
            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters()) {
                    {
                        return this->rtgrainvoice[0]->getParameterId(index);
                    }
                }

                return "bogus";
            }
            }
        }

        void getParameterInfo(ParameterIndex index, ParameterInfo* info) const {
            {
                switch (index) {
                case 0:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0.52;
                    info->min = 0.04;
                    info->max = 1;
                    info->exponent = 1;
                    info->steps = 0;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 1:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 100;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 2:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 3:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 200;
                    info->min = 20;
                    info->max = 2000;
                    info->exponent = 0.4203094;
                    info->steps = 201;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 4:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 5:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 1;
                    info->min = 0.25;
                    info->max = 4;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 0;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 6:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 7:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 1;
                    info->min = 0;
                    info->max = 3;
                    info->exponent = 1;
                    info->steps = 0;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 8:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 9:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 1;
                    info->exponent = 1;
                    info->steps = 0;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 10:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 11:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 12:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 13:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 1;
                    info->min = 0;
                    info->max = 1.5;
                    info->exponent = 1;
                    info->steps = 0;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 14:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 100;
                    info->exponent = 1;/*S-mod*/
                    info->steps = 101;  /*E-mod*/
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 15:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 1;
                    info->min = 0;
                    info->max = 1.5;
                    info->exponent = 1;
                    info->steps = 0;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 16:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 1;
                    info->exponent = 1;
                    info->steps = 2;
                    static const char* eVal16[] = { "0", "1" };
                    info->enumValues = eVal16;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 17:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 1;
                    info->exponent = 1;
                    info->steps = 2;
                    static const char* eVal17[] = { "0", "1" };
                    info->enumValues = eVal17;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 18:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 1;
                    info->exponent = 1;
                    info->steps = 2;
                    static const char* eVal18[] = { "0", "1" };
                    info->enumValues = eVal18;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 19:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 3;
                    info->min = 0;
                    info->max = 6;
                    info->exponent = 1;
                    info->steps = 7;
                    static const char* eVal19[] = { "0", "1", "2", "3", "4", "5", "6" };
                    info->enumValues = eVal19;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                case 20:
                {
                    info->type = ParameterTypeNumber;
                    info->initialValue = 0;
                    info->min = 0;
                    info->max = 2;
                    info->exponent = 1;
                    info->steps = 3;
                    static const char* eVal20[] = { "0", "1", "2" };
                    info->enumValues = eVal20;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                default:
                {
                    index -= 21;

                    if (index < this->rtgrainvoice[0]->getNumParameters()) {
                        for (Index i = 0; i < 24; i++) {
                            this->rtgrainvoice[i]->getParameterInfo(index, info);
                        }
                    }

                    break;
                }
                }
            }
        }

        ParameterValue applyJuceDenormalization(ParameterValue value, ParameterValue min, ParameterValue max, ParameterValue exp, int steps) const
        {
            auto interval = 1. / (steps - 1);
            auto snap_val = round((value) / interval) * interval;

            return min + (max - min) * (pow(2.718281828459045, log(snap_val) / exp));
        }

        ParameterValue applyJuceNormalization(ParameterValue value, ParameterValue min, ParameterValue max, ParameterValue exp) const
        {
            return std::pow(((value - min) / (max - min)), exp);
        }

        ParameterValue applyPitchDenormalization(ParameterValue value, ParameterValue min, ParameterValue max) const
        {
            auto semitoneRatio = std::pow(2.0, 1.0 / 12.0);
            auto n = std::round(std::log(value) / std::log(semitoneRatio));

            auto snap_val = std::pow(semitoneRatio, n);
            return min * std::pow(max / min, snap_val);
        }

        ParameterValue applyPitchNormalization(ParameterValue value, ParameterValue min, ParameterValue max) const
        {
            return (std::log(value / min)) / (std::log(max / min));
        }

        void sendParameter(ParameterIndex index, bool ignoreValue) {
            this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
        }

        ParameterIndex getParameterOffset(BaseInterface* subpatcher) const {
            if (subpatcher == this->rtgrainvoice[0])
                return 21;

            return 0;
        }

        ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
            if (steps == 1) {
                if (normalizedValue > 0) {
                    normalizedValue = 1.;
                }
            }
            else {
                ParameterValue oneStep = (number)1. / (steps - 1);
                ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
                normalizedValue = numberOfSteps * oneStep;
            }

            return normalizedValue;
        }

        ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
            switch (index) {
            case 9:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));
                    ParameterValue normalizedValue = (value - 0) / (1 - 0);
                    return normalizedValue;
                }
            }
            case 16:
            case 17:
            case 18:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));
                    ParameterValue normalizedValue = (value - 0) / (1 - 0);

                    {
                        normalizedValue = this->applyStepsToNormalizedParameterValue(normalizedValue, 2);
                    }

                    return normalizedValue;
                }
            }
            case 20:
            {
                {
                    value = (value < 0 ? 0 : (value > 2 ? 2 : value));
                    ParameterValue normalizedValue = (value - 0) / (2 - 0);

                    {
                        normalizedValue = this->applyStepsToNormalizedParameterValue(normalizedValue, 3);
                    }

                    return normalizedValue;
                }
            }
            case 7:
            {
                {
                    value = (value < 0 ? 0 : (value > 3 ? 3 : value));
                    ParameterValue normalizedValue = (value - 0) / (3 - 0);
                    return normalizedValue;
                }
            }
            case 19:
            {
                {
                    value = (value < 0 ? 0 : (value > 6 ? 6 : value));
                    ParameterValue normalizedValue = (value - 0) / (6 - 0);

                    normalizedValue = this->applyStepsToNormalizedParameterValue(normalizedValue, 7);

                    return normalizedValue;
                }
            }
            case 1:
            case 2:
            case 4:
            case 6:
            case 8:
            case 10:
            case 11:
            case 12:
            case 14:
            {
                {
                    value = (value < 0 ? 0 : (value > 100 ? 100 : value));
                    ParameterValue normalizedValue = (value - 0) / (100 - 0);
                    return normalizedValue;
                }
            }
            case 13:
            case 15:
            {
                {
                    value = (value < 0 ? 0 : (value > 1.5 ? 1.5 : value));
                    ParameterValue normalizedValue = (value - 0) / (1.5 - 0);
                    return normalizedValue;
                }
            }
            case 3:
            {
                ParameterInfo pinfo;
                this->getParameterInfo(3, &pinfo);

                value = (value < pinfo.min ? pinfo.min : (value > pinfo.max ? pinfo.max : value));

                return this->applyJuceNormalization(value, pinfo.min, pinfo.max, pinfo.exponent);
            }
            case 0:
            {
                {
                    value = (value < 0.04 ? 0.04 : (value > 1 ? 1 : value));
                    ParameterValue normalizedValue = (value - 0.04) / (1 - 0.04);
                    return normalizedValue;
                }
            }
            case 5:
            {
                ParameterInfo pinfo;
                this->getParameterInfo(5, &pinfo);

                value = (value < pinfo.min ? pinfo.min : (value > pinfo.max ? pinfo.max : value));

                return this->applyPitchNormalization(value, pinfo.min, pinfo.max);
            }
            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters()) {
                    {
                        return this->rtgrainvoice[0]->convertToNormalizedParameterValue(index, value);
                    }
                }

                return value;
            }
            }
        }

        ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
            value = (value < 0 ? 0 : (value > 1 ? 1 : value));

            switch (index) {
            case 9:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        return 0 + value * (1 - 0);
                    }
                }
            }
            case 16:
            case 17:
            case 18:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        value = this->applyStepsToNormalizedParameterValue(value, 2);
                    }

                    {
                        return 0 + value * (1 - 0);
                    }
                }
            }
            case 20:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        value = this->applyStepsToNormalizedParameterValue(value, 3);
                    }

                    {
                        return 0 + value * (2 - 0);
                    }
                }
            }
            case 7:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        return 0 + value * (3 - 0);
                    }
                }
            }
            case 19:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        value = this->applyStepsToNormalizedParameterValue(value, 7);
                    }

                    {
                        return 0 + value * (6 - 0);
                    }
                }
            }
            case 1:
            case 2:
            case 4:
            case 6:
            case 8:
            case 10:
            case 11:
            case 12:
            case 14:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        return round(value * 100);
                    }
                }
            }
            case 13:
            case 15:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        return 0 + value * (1.5 - 0);
                    }
                }
            }
            case 3:
            {
                value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                ParameterInfo pinfo;
                this->getParameterInfo(3, &pinfo);

                return this->applyJuceDenormalization(value, pinfo.min, pinfo.max, pinfo.exponent, pinfo.steps);
            }
            case 0:
            {
                {
                    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                    {
                        return 0.04 + value * (1 - 0.04);
                    }
                }
            }
            case 5:
            {
                value = (value < 0 ? 0 : (value > 1 ? 1 : value));

                ParameterInfo pinfo;
                this->getParameterInfo(5, &pinfo);

                return this->applyPitchDenormalization(value, pinfo.min, pinfo.max);
            }

            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters()) {
                    {
                        return this->rtgrainvoice[0]->convertFromNormalizedParameterValue(index, value);
                    }
                }

                return value;
            }
            }
        }

        ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
            switch (index) {
            case 0:
            {
                return this->param_01_value_constrain(value);
            }
            case 1:
            {
                return this->param_02_value_constrain(value);
            }
            case 2:
            {
                return this->param_03_value_constrain(value);
            }
            case 3:
            {
                return this->param_04_value_constrain(value);
            }
            case 4:
            {
                return this->param_05_value_constrain(value);
            }
            case 5:
            {
                return this->param_06_value_constrain(value);
            }
            case 6:
            {
                return this->param_07_value_constrain(value);
            }
            case 7:
            {
                return this->param_08_value_constrain(value);
            }
            case 8:
            {
                return this->param_09_value_constrain(value);
            }
            case 9:
            {
                return this->param_10_value_constrain(value);
            }
            case 10:
            {
                return this->param_11_value_constrain(value);
            }
            case 11:
            {
                return this->param_12_value_constrain(value);
            }
            case 12:
            {
                return this->param_13_value_constrain(value);
            }
            case 13:
            {
                return this->param_14_value_constrain(value);
            }
            case 14:
            {
                return this->param_15_value_constrain(value);
            }
            case 15:
            {
                return this->param_16_value_constrain(value);
            }
            case 16:
            {
                return this->param_17_value_constrain(value);
            }
            case 17:
            {
                return this->param_18_value_constrain(value);
            }
            case 18:
            {
                return this->param_19_value_constrain(value);
            }
            case 19:
            {
                return this->param_20_value_constrain(value);
            }
            case 20:
            {
                return this->param_21_value_constrain(value);
            }
            default:
            {
                index -= 21;

                if (index < this->rtgrainvoice[0]->getNumParameters()) {
                    {
                        return this->rtgrainvoice[0]->constrainParameterValue(index, value);
                    }
                }

                return value;
            }
            }
        }

        void scheduleParamInit(ParameterIndex index, Index order) {
            this->paramInitIndices->push(index);
            this->paramInitOrder->push(order);
        }

        void processParamInitEvents() {
            this->listquicksort(
                this->paramInitOrder,
                this->paramInitIndices,
                0,
                (int)(this->paramInitOrder->length - 1),
                true
            );

            for (Index i = 0; i < this->paramInitOrder->length; i++) {
                this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
            }
        }

        void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
            RNBO_UNUSED(value);
            RNBO_UNUSED(hasValue);
            this->updateTime(time);

            switch (index) {
            case -611950441:
            {
                this->edge_02_onout_bang();
                break;
            }
            case -1584063977:
            {
                this->edge_02_offout_bang();
                break;
            }
            case 1821745152:
            {
                //this->setGrainProperties();
                break;
            }
            }
        }

        void processOutletAtCurrentTime(EngineLink*, OutletIndex, ParameterValue) {}

        void processOutletEvent(
            EngineLink* sender,
            OutletIndex index,
            ParameterValue value,
            MillisecondTime time
        ) {
            this->updateTime(time);
            this->processOutletAtCurrentTime(sender, index, value);
        }

        void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
            this->updateTime(time);

            switch (tag) {
            case TAG("valin"):
            {
                if (TAG("dial_obj-64") == objectId)
                    this->dial_01_valin_set(payload);

                if (TAG("dial_obj-65") == objectId)
                    this->dial_02_valin_set(payload);

                if (TAG("dial_obj-66") == objectId)
                    this->dial_03_valin_set(payload);

                if (TAG("dial_obj-78") == objectId)
                    this->dial_04_valin_set(payload);

                if (TAG("dial_obj-67") == objectId)
                    this->dial_05_valin_set(payload);

                if (TAG("dial_obj-92") == objectId)
                    this->dial_06_valin_set(payload);

                if (TAG("dial_obj-68") == objectId)
                    this->dial_07_valin_set(payload);

                if (TAG("dial_obj-93") == objectId)
                    this->dial_08_valin_set(payload);

                if (TAG("dial_obj-69") == objectId)
                    this->dial_09_valin_set(payload);

                if (TAG("dial_obj-94") == objectId)
                    this->dial_10_valin_set(payload);

                if (TAG("dial_obj-70") == objectId)
                    this->dial_11_valin_set(payload);

                if (TAG("dial_obj-71") == objectId)
                    this->dial_12_valin_set(payload);

                if (TAG("dial_obj-72") == objectId)
                    this->dial_13_valin_set(payload);

                if (TAG("dial_obj-95") == objectId)
                    this->dial_14_valin_set(payload);

                if (TAG("dial_obj-73") == objectId)
                    this->dial_15_valin_set(payload);

                if (TAG("dial_obj-97") == objectId)
                    this->dial_16_valin_set(payload);

                if (TAG("toggle_obj-100") == objectId)
                    this->toggle_01_valin_set(payload);

                if (TAG("toggle_obj-108") == objectId)
                    this->toggle_02_valin_set(payload);

                if (TAG("toggle_obj-111") == objectId)
                    this->toggle_03_valin_set(payload);

                if (TAG("dial_obj-113") == objectId)
                    this->dial_17_valin_set(payload);

                if (TAG("dial_obj-125") == objectId)
                    this->dial_18_valin_set(payload);

                break;
            }
            }

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->processNumMessage(tag, objectId, time, payload);
            }
        }

        void processListMessage(
            MessageTag tag,
            MessageTag objectId,
            MillisecondTime time,
            const list& payload
        ) {
            RNBO_UNUSED(objectId);
            this->updateTime(time);

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->processListMessage(tag, objectId, time, payload);
            }
        }

        void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
            RNBO_UNUSED(objectId);
            this->updateTime(time);

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->processBangMessage(tag, objectId, time);
            }
        }

        MessageTagInfo resolveTag(MessageTag tag) const {
            switch (tag) {
            case TAG("valout"):
            {
                return "valout";
            }
            case TAG("dial_obj-64"):
            {
                return "dial_obj-64";
            }
            case TAG("dial_obj-65"):
            {
                return "dial_obj-65";
            }
            case TAG("dial_obj-66"):
            {
                return "dial_obj-66";
            }
            case TAG("dial_obj-78"):
            {
                return "dial_obj-78";
            }
            case TAG("dial_obj-67"):
            {
                return "dial_obj-67";
            }
            case TAG("dial_obj-92"):
            {
                return "dial_obj-92";
            }
            case TAG("dial_obj-68"):
            {
                return "dial_obj-68";
            }
            case TAG("dial_obj-93"):
            {
                return "dial_obj-93";
            }
            case TAG("dial_obj-69"):
            {
                return "dial_obj-69";
            }
            case TAG("dial_obj-94"):
            {
                return "dial_obj-94";
            }
            case TAG("dial_obj-70"):
            {
                return "dial_obj-70";
            }
            case TAG("dial_obj-71"):
            {
                return "dial_obj-71";
            }
            case TAG("dial_obj-72"):
            {
                return "dial_obj-72";
            }
            case TAG("dial_obj-95"):
            {
                return "dial_obj-95";
            }
            case TAG("dial_obj-73"):
            {
                return "dial_obj-73";
            }
            case TAG("dial_obj-97"):
            {
                return "dial_obj-97";
            }
            case TAG("toggle_obj-100"):
            {
                return "toggle_obj-100";
            }
            case TAG("toggle_obj-108"):
            {
                return "toggle_obj-108";
            }
            case TAG("toggle_obj-111"):
            {
                return "toggle_obj-111";
            }
            case TAG("dial_obj-113"):
            {
                return "dial_obj-113";
            }
            case TAG("dial_obj-125"):
            {
                return "dial_obj-125";
            }
            case TAG("valin"):
            {
                return "valin";
            }
            }

            auto subpatchResult_0 = this->rtgrainvoice[0]->resolveTag(tag);

            if (subpatchResult_0)
                return subpatchResult_0;

            return "";
        }

        MessageIndex getNumMessages() const {
            return 0;
        }

        const MessageInfo& getMessageInfo(MessageIndex index) const {
            switch (index) {

            }

            return NullMessageInfo;
        }

    protected:

        void param_01_value_set(number v) {
            v = this->param_01_value_constrain(v);
            this->param_01_value = v;
            this->sendParameter(0, false);

            if (this->param_01_value != this->param_01_lastValue) {
                this->getEngine()->presetTouched();
                this->param_01_lastValue = this->param_01_value;
            }

            //this->codebox_01_den_set(v);
            this->codebox_tilde_01_in3_set(v);
        }

        void param_02_value_set(number v) {
            v = this->param_02_value_constrain(v);
            this->param_02_value = v;
            this->sendParameter(1, false);

            {
                this->param_02_normalized_set(this->tonormalized(1, this->param_02_value));
            }

            if (this->param_02_value != this->param_02_lastValue) {
                this->getEngine()->presetTouched();
                this->param_02_lastValue = this->param_02_value;
            }
        }

        void param_03_value_set(number v) {
            v = this->param_03_value_constrain(v);
            this->param_03_value = v;
            this->sendParameter(2, false);

            {
                this->param_03_normalized_set(this->tonormalized(2, this->param_03_value));
            }

            if (this->param_03_value != this->param_03_lastValue) {
                this->getEngine()->presetTouched();
                this->param_03_lastValue = this->param_03_value;
            }
        }

        void param_04_value_set(number v) {
            v = this->param_04_value_constrain(v);
            this->param_04_value = v;
            this->sendParameter(3, false);

            if (this->param_04_value != this->param_04_lastValue) {
                this->getEngine()->presetTouched();
                this->param_04_lastValue = this->param_04_value;
            }

            this->codebox_tilde_03_in2_set(v);
            this->codebox_tilde_02_in1_set(v);
            //this->codebox_01_len_set(v);
            this->codebox_tilde_01_in2_set(v);
        }

        void param_05_value_set(number v) {
            v = this->param_05_value_constrain(v);
            this->param_05_value = v;
            this->sendParameter(4, false);

            //{
            //    this->param_05_normalized_set(this->tonormalized(4, this->param_05_value));
            //}

            if (this->param_05_value != this->param_05_lastValue) {
                this->getEngine()->presetTouched();
                this->param_05_lastValue = this->param_05_value;
            }
        }

        void param_06_value_set(number v) {
            v = this->param_06_value_constrain(v);
            this->param_06_value = v;
            this->sendParameter(5, false);

            if (this->param_06_value != this->param_06_lastValue) {
                this->getEngine()->presetTouched();
                this->param_06_lastValue = this->param_06_value;
            }

            //this->codebox_01_psh_set(v);
        }

        void param_07_value_set(number v) {
            v = this->param_07_value_constrain(v);
            this->param_07_value = v;
            this->sendParameter(6, false);

            //{
            //    this->param_07_normalized_set(this->tonormalized(6, this->param_07_value));
            //}

            if (this->param_07_value != this->param_07_lastValue) {
                this->getEngine()->presetTouched();
                this->param_07_lastValue = this->param_07_value;
            }
        }

        void param_08_value_set(number v) {
            v = this->param_08_value_constrain(v);
            this->param_08_value = v;
            this->sendParameter(7, false);

            if (this->param_08_value != this->param_08_lastValue) {
                this->getEngine()->presetTouched();
                this->param_08_lastValue = this->param_08_value;
            }

            //this->codebox_01_env_set(v);
        }

        void param_09_value_set(number v) {
            v = this->param_09_value_constrain(v);
            this->param_09_value = v;
            this->sendParameter(8, false);

            //{
            //    this->param_09_normalized_set(this->tonormalized(8, this->param_09_value));
            //}

            if (this->param_09_value != this->param_09_lastValue) {
                this->getEngine()->presetTouched();
                this->param_09_lastValue = this->param_09_value;
            }
        }

        void param_10_value_set(number v) {
            v = this->param_10_value_constrain(v);
            this->param_10_value = v;
            this->sendParameter(9, false);

            if (this->param_10_value != this->param_10_lastValue) {
                this->getEngine()->presetTouched();
                this->param_10_lastValue = this->param_10_value;
            }

            //this->codebox_01_cpo_set(v);
        }

        void param_11_value_set(number v) {
            v = this->param_11_value_constrain(v);
            this->param_11_value = v;
            this->sendParameter(10, false);

            //{
            //    this->param_11_normalized_set(this->tonormalized(10, this->param_11_value));
            //}

            if (this->param_11_value != this->param_11_lastValue) {
                this->getEngine()->presetTouched();
                this->param_11_lastValue = this->param_11_value;
            }
        }

        void param_12_value_set(number v) {
            v = this->param_12_value_constrain(v);
            this->param_12_value = v;
            this->sendParameter(11, false);

            //{
            //    this->param_12_normalized_set(this->tonormalized(11, this->param_12_value));
            //}

            if (this->param_12_value != this->param_12_lastValue) {
                this->getEngine()->presetTouched();
                this->param_12_lastValue = this->param_12_value;
            }
        }

        void param_13_value_set(number v) {
            v = this->param_13_value_constrain(v);
            this->param_13_value = v;
            this->sendParameter(12, false);

            //{
            //    this->param_13_normalized_set(this->tonormalized(12, this->param_13_value));
            //}

            if (this->param_13_value != this->param_13_lastValue) {
                this->getEngine()->presetTouched();
                this->param_13_lastValue = this->param_13_value;
            }
        }

        void param_14_value_set(number v) {
            v = this->param_14_value_constrain(v);
            this->param_14_value = v;
            this->sendParameter(13, false);

            if (this->param_14_value != this->param_14_lastValue) {
                this->getEngine()->presetTouched();
                this->param_14_lastValue = this->param_14_value;
            }

            this->dspexpr_06_in2_set(v);
        }

        void param_15_value_set(number v) {
            v = this->param_15_value_constrain(v);
            this->param_15_value = v;
            this->sendParameter(14, false);

            {
                this->param_15_normalized_set(this->tonormalized(14, this->param_15_value));
            }

            if (this->param_15_value != this->param_15_lastValue) {
                this->getEngine()->presetTouched();
                this->param_15_lastValue = this->param_15_value;
            }
        }

        void param_16_value_set(number v) {
            v = this->param_16_value_constrain(v);
            this->param_16_value = v;
            this->sendParameter(15, false);

            if (this->param_16_value != this->param_16_lastValue) {
                this->getEngine()->presetTouched();
                this->param_16_lastValue = this->param_16_value;
            }

            this->dspexpr_02_in2_set(v);
            this->dspexpr_01_in2_set(v);
        }

        void param_17_value_set(number v) {
            v = this->param_17_value_constrain(v);
            this->param_17_value = v;
            this->sendParameter(16, false);

            if (this->param_17_value != this->param_17_lastValue) {
                this->getEngine()->presetTouched();
                this->param_17_lastValue = this->param_17_value;
            }

            this->codebox_tilde_01_in1_set(v);
        }

        void param_18_value_set(number v) {
            v = this->param_18_value_constrain(v);
            this->param_18_value = v;
            this->sendParameter(17, false);

            if (this->param_18_value != this->param_18_lastValue) {
                this->getEngine()->presetTouched();
                this->param_18_lastValue = this->param_18_value;
            }

            this->codebox_tilde_03_in1_set(v);
            this->expr_02_in1_set(v);
            //this->codebox_01_frz_set(v);
        }

        void param_19_value_set(number v) {
            v = this->param_19_value_constrain(v);
            this->param_19_value = v;
            this->sendParameter(18, false);

            if (this->param_19_value != this->param_19_lastValue) {
                this->getEngine()->presetTouched();
                this->param_19_lastValue = this->param_19_value;
            }

            this->codebox_tilde_01_in6_set(v);
        }

        void param_20_value_set(number v) {
            v = this->param_20_value_constrain(v);
            this->param_20_value = v;
            this->sendParameter(19, false);

            if (this->param_20_value != this->param_20_lastValue) {
                this->getEngine()->presetTouched();
                this->param_20_lastValue = this->param_20_value;
            }

            this->codebox_tilde_01_in7_set(v);
        }

        void param_21_value_set(number v) {
            v = this->param_21_value_constrain(v);
            this->param_21_value = v;
            this->sendParameter(20, false);

            if (this->param_21_value != this->param_21_lastValue) {
                this->getEngine()->presetTouched();
                this->param_21_lastValue = this->param_21_value;
            }

            this->codebox_tilde_01_in8_set(v);
        }

        void edge_02_onout_bang() {}

        void edge_02_offout_bang() {
            //this->setGrainProperties();
        }

        void dial_01_valin_set(number v) {
            this->dial_01_value_set(v);
        }

        void dial_02_valin_set(number v) {
            this->dial_02_value_set(v);
        }

        void dial_03_valin_set(number v) {
            this->dial_03_value_set(v);
        }

        void dial_04_valin_set(number v) {
            this->dial_04_value_set(v);
        }

        void dial_05_valin_set(number v) {
            this->dial_05_value_set(v);
        }

        void dial_06_valin_set(number v) {
            this->dial_06_value_set(v);
        }

        void dial_07_valin_set(number v) {
            this->dial_07_value_set(v);
        }

        void dial_08_valin_set(number v) {
            this->dial_08_value_set(v);
        }

        void dial_09_valin_set(number v) {
            this->dial_09_value_set(v);
        }

        void dial_10_valin_set(number v) {
            this->dial_10_value_set(v);
        }

        void dial_11_valin_set(number v) {
            this->dial_11_value_set(v);
        }

        void dial_12_valin_set(number v) {
            this->dial_12_value_set(v);
        }

        void dial_13_valin_set(number v) {
            this->dial_13_value_set(v);
        }

        void dial_14_valin_set(number v) {
            this->dial_14_value_set(v);
        }

        void dial_15_valin_set(number v) {
            this->dial_15_value_set(v);
        }

        void dial_16_valin_set(number v) {
            this->dial_16_value_set(v);
        }

        void toggle_01_valin_set(number v) {
            this->toggle_01_value_number_set(v);
        }

        void toggle_02_valin_set(number v) {
            this->toggle_02_value_number_set(v);
        }

        void toggle_03_valin_set(number v) {
            this->toggle_03_value_number_set(v);
        }

        void dial_17_valin_set(number v) {
            this->dial_17_value_set(v);
        }

        void dial_18_valin_set(number v) {
            this->dial_18_value_set(v);
        }

        number msToSamps(MillisecondTime ms, number sampleRate) {
            return ms * sampleRate * 0.001;
        }

        MillisecondTime sampsToMs(SampleIndex samps) {
            return samps * (this->invsr * 1000);
        }

        Index getMaxBlockSize() const {
            return this->maxvs;
        }

        number getSampleRate() const {
            return this->sr;
        }

        bool hasFixedVectorSize() const {
            return false;
        }

        Index getNumInputChannels() const {
            return 2;
        }

        Index getNumOutputChannels() const {
            return 2;
        }

        void allocateDataRefs() {
            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->allocateDataRefs();
            }

            this->data_03_buffer->requestSize(this->mstosamps(20000), 1);
            this->data_03_buffer->setSampleRate(this->sr);
            this->data_01_buffer->requestSize(100, 1);
            this->data_01_buffer->setSampleRate(this->sr);
            this->recordtilde_01_buffer = this->recordtilde_01_buffer->allocateIfNeeded();
            this->bufferop_01_buffer = this->bufferop_01_buffer->allocateIfNeeded();
            this->data_03_buffer = this->data_03_buffer->allocateIfNeeded();

            if (this->borisinrnbo_v01_rtbuf->hasRequestedSize()) {
                if (this->borisinrnbo_v01_rtbuf->wantsFill())
                    this->zeroDataRef(this->borisinrnbo_v01_rtbuf);

                this->getEngine()->sendDataRefUpdated(0);
            }

            this->data_01_buffer = this->data_01_buffer->allocateIfNeeded();

            if (this->interpolated_envelope->hasRequestedSize()) {
                if (this->interpolated_envelope->wantsFill())
                    this->zeroDataRef(this->interpolated_envelope);

                this->getEngine()->sendDataRefUpdated(1);
            }

            this->codebox_tilde_01_del_buffer = this->codebox_tilde_01_del_buffer->allocateIfNeeded();

            if (this->codebox_tilde_01_del_bufferobj->hasRequestedSize()) {
                if (this->codebox_tilde_01_del_bufferobj->wantsFill())
                    this->zeroDataRef(this->codebox_tilde_01_del_bufferobj);

                this->getEngine()->sendDataRefUpdated(2);
            }

            this->recordtilde_02_buffer = this->recordtilde_02_buffer->allocateIfNeeded();
            this->data_02_buffer = this->data_02_buffer->allocateIfNeeded();

            if (this->inter_databuf_01->hasRequestedSize()) {
                if (this->inter_databuf_01->wantsFill())
                    this->zeroDataRef(this->inter_databuf_01);

                this->getEngine()->sendDataRefUpdated(3);
            }
        }

        void initializeObjects() {
            //this->codebox_01_rdm_init();
            this->codebox_tilde_01_n_subd_init();
            this->codebox_tilde_01_del_init();
            this->codebox_tilde_01_rdm_init();
            this->data_01_init();
            this->codebox_tilde_02_timer_init();
            this->codebox_tilde_02_redge1_init();
            this->codebox_tilde_02_redge0_init();
            this->codebox_tilde_02_trig_init();
            this->codebox_tilde_03_offs_rev_init();
            this->codebox_tilde_03_done_init();
            this->codebox_tilde_03_startrec_init();
            this->data_02_init();
            this->data_03_init();

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->initializeObjects();
            }
        }

        void sendOutlet(OutletIndex index, ParameterValue value) {
            this->getEngine()->sendOutlet(this, index, value);
        }

        void startup() {
            this->updateTime(this->getEngine()->getCurrentTime());
            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->startup();
                //codebox_02_voiceState[i][0] = 0;
                //codebox_02_voiceState[i][1] = 0;

                voice_state[i] = false;
            }

            this->getEngine()->scheduleClockEvent(this, 1821745152, 0 + this->_currentTime);

            this->timevalue_01_sendValue();
            this->timevalue_02_sendValue();
            this->timevalue_03_sendValue();

            {
                this->scheduleParamInit(0, 3);
            }

            {
                this->scheduleParamInit(1, 2);
            }

            {
                this->scheduleParamInit(2, 10);
            }

            {
                this->scheduleParamInit(3, 5);
            }

            {
                this->scheduleParamInit(4, 10);
            }

            {
                this->scheduleParamInit(5, 9);
            }

            {
                this->scheduleParamInit(6, 10);
            }

            {
                this->scheduleParamInit(7, 11);
            }

            {
                this->scheduleParamInit(8, 12);
            }

            {
                this->scheduleParamInit(9, 7);
            }

            {
                this->scheduleParamInit(10, 8);
            }

            {
                this->scheduleParamInit(11, 14);
            }

            {
                this->scheduleParamInit(12, 13);
            }

            {
                this->scheduleParamInit(13, 0);
            }

            {
                this->scheduleParamInit(14, 17);
            }

            {
                this->scheduleParamInit(15, 19);
            }

            {
                this->scheduleParamInit(16, 1);
            }

            {
                this->scheduleParamInit(17, 16);
            }

            {
                this->scheduleParamInit(18, 0);
            }

            {
                this->scheduleParamInit(19, 0);
            }

            {
                this->scheduleParamInit(20, 0);
            }

            this->processParamInitEvents();
        }

        static number param_01_value_constrain(number v) {
            v = (v > 1 ? 1 : (v < 0.04 ? 0.04 : v));
            return v;
        }
        //
        //static number codebox_01_den_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_den_set(number v) {
        //    v = this->codebox_01_den_constrain(v);
        //    this->codebox_01_den = v;
        //}

        void codebox_tilde_01_in3_set(number v) {
            this->codebox_tilde_01_in3 = v;
        }

        static number param_02_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        void codebox_tilde_01_in4_set(number v) {
            this->codebox_tilde_01_in4 = v;
        }

        void param_02_normalized_set(number v) {
            this->codebox_tilde_01_in4_set(v);
        }

        static number param_03_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        void codebox_tilde_01_in5_set(number v) {
            this->codebox_tilde_01_in5 = v;
        }

        void param_03_normalized_set(number v) {
            this->codebox_tilde_01_in5_set(v);
        }

        static number param_04_value_constrain(number v) {
            v = (v > 2000 ? 2000 : (v < 20 ? 20 : v));
            return v;
        }

        void codebox_tilde_03_in2_set(number v) {
            this->codebox_tilde_03_in2 = v;
        }

        void codebox_tilde_02_in1_set(number v) {
            this->codebox_tilde_02_in1 = v;
        }

        //static number codebox_01_len_constrain(number v) {
        //    if (v < 20)
        //        v = 20;
        //
        //    if (v > 2000)
        //        v = 2000;
        //
        //    return v;
        //}

        //void codebox_01_len_set(number v) {
        //    v = this->codebox_01_len_constrain(v);
        //    this->codebox_01_len = v;
        //}

        void codebox_tilde_01_in2_set(number v) {
            this->codebox_tilde_01_in2 = v;
        }

        static number param_05_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_rle_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_rle_set(number v) {
        //    v = this->codebox_01_rle_constrain(v);
        //    this->codebox_01_rle = v;
        //}

        //void param_05_normalized_set(number v) {
        //    this->codebox_01_rle_set(v);
        //}

        static number param_06_value_constrain(number v) {
            v = (v > 4 ? 4 : (v < 0.25 ? 0.25 : v));
            return v;
        }
        //
        //static number codebox_01_psh_constrain(number v) {
        //    if (v < 0.25)
        //        v = 0.25;
        //
        //    if (v > 4)
        //        v = 4;
        //
        //    return v;
        //}

        //void codebox_01_psh_set(number v) {
        //    v = this->codebox_01_psh_constrain(v);
        //    this->codebox_01_psh = v;
        //}

        static number param_07_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_rpt_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_rpt_set(number v) {
        //    v = this->codebox_01_rpt_constrain(v);
        //    this->codebox_01_rpt = v;
        //}

        //void param_07_normalized_set(number v) {
        //    this->codebox_01_rpt_set(v);
        //}

        static number param_08_value_constrain(number v) {
            v = (v > 3 ? 3 : (v < 0 ? 0 : v));
            return v;
        }
        //
        //static number codebox_01_env_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 3)
        //        v = 3;
        //
        //    return v;
        //}

        //void codebox_01_env_set(number v) {
        //    v = this->codebox_01_env_constrain(v);
        //    this->codebox_01_env = v;
        //}

        static number param_09_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_frp_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_frp_set(number v) {
        //    v = this->codebox_01_frp_constrain(v);
        //    this->codebox_01_frp = v;
        //}

        //void param_09_normalized_set(number v) {
        //    //this->codebox_01_frp_set(v);
        //    this->codebox_01_frp = v;
        //}

        static number param_10_value_constrain(number v) {
            v = (v > 1 ? 1 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_cpo_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_cpo_set(number v) {
        //    v = this->codebox_01_cpo_constrain(v);
        //    this->codebox_01_cpo = v;
        //}

        static number param_11_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_drf_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_drf_set(number v) {
        //    v = this->codebox_01_drf_constrain(v);
        //    this->codebox_01_drf = v;
        //}

        //void param_11_normalized_set(number v) {
        //    this->codebox_01_drf_set(v);
        //}

        static number param_12_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_pwi_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_pwi_set(number v) {
        //    v = this->codebox_01_pwi_constrain(v);
        //    this->codebox_01_pwi = v;
        //}

        //void param_12_normalized_set(number v) {
        //    this->codebox_01_pwi_set(v);
        //}

        static number param_13_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        //static number codebox_01_rvo_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_rvo_set(number v) {
        //    v = this->codebox_01_rvo_constrain(v);
        //    this->codebox_01_rvo = v;
        //}

        //void param_13_normalized_set(number v) {
        //    this->codebox_01_rvo_set(v);
        //}

        static number param_14_value_constrain(number v) {
            v = (v > 1.5 ? 1.5 : (v < 0 ? 0 : v));
            return v;
        }

        void dspexpr_06_in2_set(number v) {
            this->dspexpr_06_in2 = v;
        }

        static number param_15_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        void dspexpr_07_in2_set(number v) {
            this->dspexpr_07_in2 = v;
        }

        void param_15_normalized_set(number v) {
            this->dspexpr_07_in2_set(v);
        }

        static number param_16_value_constrain(number v) {
            v = (v > 1.5 ? 1.5 : (v < 0 ? 0 : v));
            return v;
        }

        void dspexpr_02_in2_set(number v) {
            this->dspexpr_02_in2 = v;
        }

        void dspexpr_01_in2_set(number v) {
            this->dspexpr_01_in2 = v;
        }

        static number param_17_value_constrain(number v) {
            v = (v > 1 ? 1 : (v < 0 ? 0 : v));

            {
                number oneStep = (number)1 / (number)1;
                number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
                number numberOfSteps = rnbo_fround(v * oneStepInv * 1 / (number)1) * 1;
                v = numberOfSteps * oneStep;
            }

            return v;
        }

        void codebox_tilde_01_in1_set(number v) {
            this->codebox_tilde_01_in1 = v;
        }

        static number param_18_value_constrain(number v) {
            v = (v > 1 ? 1 : (v < 0 ? 0 : v));

            {
                number oneStep = (number)1 / (number)1;
                number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
                number numberOfSteps = rnbo_fround(v * oneStepInv * 1 / (number)1) * 1;
                v = numberOfSteps * oneStep;
            }

            return v;
        }

        void codebox_tilde_03_in1_set(number v) {
            this->codebox_tilde_03_in1 = v;
        }
        /*S-mod*/
        void bufferop_01_trigger_bang() {
            auto& buffer = this->bufferop_01_buffer;
            //SampleIndex bufsize = (SampleIndex)(this->bufferop_01_buffer->getSize());
            //number bufsizeDiv = bufsize - 1;

            //for (number channel = 0; channel < buffer->getChannels(); channel++) {
            //    number doIt = this->bufferop_01_channels->length == 0 || (bool)(this->bufferop_01_channels->includes(channel + 1));

            //    if ((bool)(doIt)) {
            //        for (SampleIndex index = 0; index < bufsize; index++) {
            //            number x = index / bufsizeDiv;
            //            number value = buffer->getSample(channel, index);
            //            value = 0;
            //            buffer->setSample(channel, index, value);
            //        }
            //    }
            //}
            buffer->setZero();
            buffer->setTouched(true);
        }
        /*E-mod*/
        void select_01_match1_bang() {
            this->bufferop_01_trigger_bang();
        }

        void select_01_nomatch_number_set(number) {}

        void select_01_input_number_set(number v) {
            if (v == this->select_01_test1)
                this->select_01_match1_bang();
            else
                this->select_01_nomatch_number_set(v);
        }

        void trigger_02_out3_set(number v) {
            this->select_01_input_number_set(v);
        }

        void latch_tilde_01_control_set(number v) {
            this->latch_tilde_01_control = v;
        }

        void trigger_02_out2_set(number v) {
            this->latch_tilde_01_control_set(v);
        }

        void codebox_tilde_02_in2_set(number v) {
            this->codebox_tilde_02_in2 = v;
        }

        void trigger_02_out1_set(number v) {
            this->codebox_tilde_02_in2_set(v);
        }

        void trigger_02_input_number_set(number v) {
            this->trigger_02_out3_set(trunc(v));
            this->trigger_02_out2_set(trunc(v));
            this->trigger_02_out1_set(trunc(v));
        }

        void expr_02_out1_set(number v) {
            this->expr_02_out1 = v;
            this->trigger_02_input_number_set(this->expr_02_out1);
        }

        void expr_02_in1_set(number in1) {
            this->expr_02_in1 = in1;
            this->expr_02_out1_set(this->expr_02_in1 == 0);//#map:!_obj-60:1
        }

        //static number codebox_01_frz_constrain(number v) {
        //    if (v < 0)
        //        v = 0;
        //
        //    if (v > 1)
        //        v = 1;
        //
        //    return v;
        //}

        //void codebox_01_frz_set(number v) {
        //    v = this->codebox_01_frz_constrain(v);
        //    this->codebox_01_frz = v;
        //}

        static number param_19_value_constrain(number v) {
            v = (v > 1 ? 1 : (v < 0 ? 0 : v));

            {
                number oneStep = (number)1 / (number)1;
                number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
                number numberOfSteps = rnbo_fround(v * oneStepInv * 1 / (number)1) * 1;
                v = numberOfSteps * oneStep;
            }

            return v;
        }

        void codebox_tilde_01_in6_set(number v) {
            this->codebox_tilde_01_in6 = v;
        }

        static number param_20_value_constrain(number v) {
            v = (v > 6 ? 6 : (v < 0 ? 0 : v));

            {
                number oneStep = (number)6 / (number)6;
                number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
                number numberOfSteps = rnbo_fround(v * oneStepInv * 1 / (number)1) * 1;
                v = numberOfSteps * oneStep;
            }

            return v;
        }

        void codebox_tilde_01_in7_set(number v) {
            this->codebox_tilde_01_in7 = v;
        }

        static number param_21_value_constrain(number v) {
            v = (v > 2 ? 2 : (v < 0 ? 0 : v));

            {
                number oneStep = (number)2 / (number)2;
                number oneStepInv = (oneStep != 0 ? (number)1 / oneStep : 0);
                number numberOfSteps = rnbo_fround(v * oneStepInv * 1 / (number)1) * 1;
                v = numberOfSteps * oneStep;
            }

            return v;
        }

        void codebox_tilde_01_in8_set(number v) {
            this->codebox_tilde_01_in8 = v;
        }

        void p_01_voicestatus_set(const list& v) {
            for (Index i = 0; i < 24; i++) {
                if (i + 1 == this->p_01_target || 0 == this->p_01_target) {
                    this->rtgrainvoice[i]->updateTime(this->_currentTime);
                }
            }

            for (Index i = 0; i < 24; i++) {
                if (i + 1 == this->p_01_target || 0 == this->p_01_target) {
                    this->rtgrainvoice[i]->voice_01_mutein_list_set(v);
                }
            }
        }

        void p_01_activevoices_set(number v) {
            for (Index i = 0; i < 24; i++) {
                if (i + 1 == this->p_01_target || 0 == this->p_01_target) {
                    this->rtgrainvoice[i]->updateTime(this->_currentTime);
                }
            }

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->voice_01_activevoices_set(v);
            }
        }

        void p_01_mute_set(const list& v) {
            Index voiceNumber = (Index)(v[0]);
            Index muteState = (Index)(v[1]);

            if (voiceNumber == 0) {
                for (Index i = 0; i < 24; i++) {
                    this->rtgrainvoice[(Index)i]->setIsMuted(muteState);
                }
            }
            else {
                Index subpatcherIndex = voiceNumber - 1;

                if (subpatcherIndex >= 0 && subpatcherIndex < 24) {
                    this->rtgrainvoice[(Index)subpatcherIndex]->setIsMuted(muteState);
                }
            }

            list tmp = { v[0], v[1] };
            this->p_01_voicestatus_set(tmp);
            this->p_01_activevoices_set(this->p_01_calcActiveVoices());
        }

        void setVoiceMuteState(const list& v) {
            this->codebox_02_out2 = jsCreateListCopy(v);
            this->p_01_mute_set(this->codebox_02_out2);
        }

        //void setTarget(number v) {
        //    this->p_01_target = v;
        //}

        void selectTarget(number targetindex) {
            //this->codebox_02_out3 = v;
            //this->p_01_target_set(this->codebox_02_out3);

            //p_01_target_set(targetindex);
        }
        //void p_01_in1_list_set(const list& v) {
        //
        //	// this is only called when I'm selecting a specific target, so I'm not concerned about 
        //	// the 0 case (target all)
        //    /*for (Index i = 0; i < 24; i++) {
        //        if (i + 1 == this->p_01_target || 0 == this->p_01_target) {
        //            this->p_01[i]->updateTime(this->_currentTime);
        //        }
        //    }
        //
        //    for (Index i = 0; i < 24; i++) {
        //        if (i + 1 == this->p_01_target || 0 == this->p_01_target) {
        //            this->p_01[i]->eventinlet_01_out1_list_set(v);
        //        }
        //    }*/
        //	Index tindex = this->p_01_target - 1;
        //
        //    this->rtgrainvoice[tindex]->updateTime(this->_currentTime);
        //	this->rtgrainvoice[tindex]->initiateVoice(v);
        //}
        //
        //void sendGrainPropsToTarget(SampleIndex trigatindex, const list& grainProps) {
        //    /*this->codebox_02_out1 = jsCreateListCopy(v);
        //    this->p_01_in1_list_set(this->codebox_02_out1);*/
        //
        //	//this->p_01_in1_list_set(grainProps);
        //
        //    Index tindex = this->p_01_target - 1;
        //
        //    this->rtgrainvoice[tindex]->updateTime(this->_currentTime);
        //    this->rtgrainvoice[tindex]->initiateVoice(trigatindex, grainProps);
        //}

        void triggerGrain(SampleIndex trigatindex, const list& grainprops) {
            /*this->codebox_02_in1 = jsCreateListCopy(in1);
            list cv = this->codebox_02_in1;*/

            //if (cv->length == 6) {
            //    this->codebox_02_pos = cv[0];//#map:_###_obj_###_:58
            //    this->codebox_02_len = cv[1];//#map:_###_obj_###_:59
            //    this->codebox_02_f_r = cv[2];//#map:_###_obj_###_:60
            //    this->codebox_02_ptc = cv[3];//#map:_###_obj_###_:61
            //    this->codebox_02_vol = cv[4];//#map:_###_obj_###_:62
            //    this->codebox_02_pan = cv[5];//#map:_###_obj_###_:63
            //    this->codebox_02_assignvoice();//#map:_###_obj_###_:65
            //}//#map:codebox_obj-19:56

            int target = this->findtargetvoice();

            if (target >= 0) {
                this->p_01_target = target;
                this->setVoiceMuteState({ target, 0 });
                /*this->sendGrainPropsToTarget(trigatindex, {grainprops});*/

                this->rtgrainvoice[this->p_01_target - 1]->initiateVoice(trigatindex, grainprops);
            }
        }

        //void codebox_01_out1_set(const list& v) {
        //    //this->codebox_01_out1 = jsCreateListCopy(v);
        //	this->codebox_01_out1 = v;
        //    this->codebox_02_in1_set(this->codebox_01_out1);
        //}

        void setGrainProperties(SampleIndex trigatindex) {
            number len = getParameterValue(3);
            number rle = getParameterNormalized(4);
            number psh = getParameterValue(5);
            number rpt = getParameterNormalized(6);
            number frp = getParameterNormalized(8);
            number cpo = getParameterValue(9);
            number drf = getParameterNormalized(10);
            number pwi = getParameterNormalized(11);
            number rvo = getParameterNormalized(12);
            number frz = getParameterValue(17);

            auto cpo_in_samps = rnbo_ceil(cpo * this->samplerate() * 10);
            auto drf_in_samps = rnbo_ceil(drf * drf * this->samplerate() * 10);
            auto len_in_samps = this->mstosamps(this->setGrainSize(len, rle));

            auto intelligent_offset = 0;

            if (!frz) {
                intelligent_offset = len_in_samps;
            }

            /*this->codebox_01_out1_set({*/
            this->triggerGrain(
                trigatindex,
                {
                this->setGrainPosition(cpo_in_samps, drf_in_samps, len_in_samps, psh, rpt, intelligent_offset),
                len_in_samps,
                this->setGrainDirection(frp),
                this->setGrainPshift(psh, rpt),
                this->setGrainVol(rvo),
                this->setGrainPan(pwi)
                });
        }

        void dial_01_output_set(number v) {
            this->param_01_value_set(v);
        }

        void dial_01_value_set(number v) {
            this->dial_01_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0.04, 0.04 + 96 - 1, 1), 1) * 0.01;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-64"), v, this->_currentTime);
            this->dial_01_output_set(value);
        }

        void dial_02_output_set(number v) {
            this->param_02_value_set(v);
        }

        void dial_02_value_set(number v) {
            this->dial_02_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-65"), v, this->_currentTime);
            this->dial_02_output_set(value);
        }

        void dial_03_output_set(number v) {
            this->param_03_value_set(v);
        }

        void dial_03_value_set(number v) {
            this->dial_03_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-66"), v, this->_currentTime);
            this->dial_03_output_set(value);
        }

        void dial_04_output_set(number v) {
            this->param_04_value_set(v);
        }

        void dial_04_value_set(number v) {
            this->dial_04_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 20, 20 + 1980 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-78"), v, this->_currentTime);
            this->dial_04_output_set(value);
        }

        void dial_05_output_set(number v) {
            this->param_05_value_set(v);
        }

        void dial_05_value_set(number v) {
            this->dial_05_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-67"), v, this->_currentTime);
            this->dial_05_output_set(value);
        }

        void dial_06_output_set(number v) {
            this->param_06_value_set(v);
        }

        void dial_06_value_set(number v) {
            this->dial_06_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0.25, 0.25 + 75 - 1, 1), 1) * 0.05;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-92"), v, this->_currentTime);
            this->dial_06_output_set(value);
        }

        void dial_07_output_set(number v) {
            this->param_07_value_set(v);
        }

        void dial_07_value_set(number v) {
            this->dial_07_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-68"), v, this->_currentTime);
            this->dial_07_output_set(value);
        }

        void dial_08_output_set(number v) {
            this->param_08_value_set(v);
        }

        void dial_08_value_set(number v) {
            this->dial_08_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 30 - 1, 1), 1) * 0.1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-93"), v, this->_currentTime);
            this->dial_08_output_set(value);
        }

        void dial_09_output_set(number v) {
            this->param_09_value_set(v);
        }

        void dial_09_value_set(number v) {
            this->dial_09_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-69"), v, this->_currentTime);
            this->dial_09_output_set(value);
        }

        void dial_10_output_set(number v) {
            this->param_10_value_set(v);
        }

        void dial_10_value_set(number v) {
            this->dial_10_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 0.01;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-94"), v, this->_currentTime);
            this->dial_10_output_set(value);
        }

        void dial_11_output_set(number v) {
            this->param_11_value_set(v);
        }

        void dial_11_value_set(number v) {
            this->dial_11_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-70"), v, this->_currentTime);
            this->dial_11_output_set(value);
        }

        void dial_12_output_set(number v) {
            this->param_12_value_set(v);
        }

        void dial_12_value_set(number v) {
            this->dial_12_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-71"), v, this->_currentTime);
            this->dial_12_output_set(value);
        }

        void dial_13_output_set(number v) {
            this->param_13_value_set(v);
        }

        void dial_13_value_set(number v) {
            this->dial_13_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-72"), v, this->_currentTime);
            this->dial_13_output_set(value);
        }

        void dial_14_output_set(number v) {
            this->param_14_value_set(v);
        }

        void dial_14_value_set(number v) {
            this->dial_14_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 150 - 1, 1), 1) * 0.01;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-95"), v, this->_currentTime);
            this->dial_14_output_set(value);
        }

        void dial_15_output_set(number v) {
            this->param_15_value_set(v);
        }

        void dial_15_value_set(number v) {
            this->dial_15_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 100 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-73"), v, this->_currentTime);
            this->dial_15_output_set(value);
        }

        void dial_16_output_set(number v) {
            this->param_16_value_set(v);
        }

        void dial_16_value_set(number v) {
            this->dial_16_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 150 - 1, 1), 1) * 0.01;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-97"), v, this->_currentTime);
            this->dial_16_output_set(value);
        }

        void toggle_01_output_set(number v) {
            this->param_17_value_set(v);
        }

        void toggle_01_value_number_set(number v) {
            this->toggle_01_value_number_setter(v);
            v = this->toggle_01_value_number;
            this->getEngine()->sendNumMessage(TAG("valout"), TAG("toggle_obj-100"), v, this->_currentTime);
            this->toggle_01_output_set(v);
        }

        void toggle_02_output_set(number v) {
            this->param_18_value_set(v);
        }

        void toggle_02_value_number_set(number v) {
            this->toggle_02_value_number_setter(v);
            v = this->toggle_02_value_number;
            this->getEngine()->sendNumMessage(TAG("valout"), TAG("toggle_obj-108"), v, this->_currentTime);
            this->toggle_02_output_set(v);
        }

        void toggle_03_output_set(number v) {
            this->param_19_value_set(v);
        }

        void toggle_03_value_number_set(number v) {
            this->toggle_03_value_number_setter(v);
            v = this->toggle_03_value_number;
            this->getEngine()->sendNumMessage(TAG("valout"), TAG("toggle_obj-111"), v, this->_currentTime);
            this->toggle_03_output_set(v);
        }

        void dial_17_output_set(number v) {
            this->param_20_value_set(v);
        }

        void dial_17_value_set(number v) {
            this->dial_17_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 7 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-113"), v, this->_currentTime);
            this->dial_17_output_set(value);
        }

        void dial_18_output_set(number v) {
            this->param_21_value_set(v);
        }

        void dial_18_value_set(number v) {
            this->dial_18_value = v;
            number value;

            {
                value = this->__wrapped_op_round(this->scale(v, 0, 128, 0, 0 + 3 - 1, 1), 1) * 1;
            }

            this->getEngine()->sendNumMessage(TAG("valout"), TAG("dial_obj-125"), v, this->_currentTime);
            this->dial_18_output_set(value);
        }

        void phasor_01_freq_set(number v) {
            this->phasor_01_freq = v;
        }

        void timevalue_01_out_set(number v) {
            this->phasor_01_freq_set(v);
        }

        void phasor_02_freq_set(number v) {
            this->phasor_02_freq = v;
        }

        void timevalue_02_out_set(number v) {
            this->phasor_02_freq_set(v);
        }

        void phasor_03_freq_set(number v) {
            this->phasor_03_freq = v;
        }

        void timevalue_03_out_set(number v) {
            this->phasor_03_freq_set(v);
        }

        void ctlin_01_outchannel_set(number) {}

        void ctlin_01_outcontroller_set(number) {}

        void fromnormalized_01_output_set(number v) {
            this->param_17_value_set(v);
        }

        void fromnormalized_01_input_set(number v) {
            this->fromnormalized_01_output_set(this->fromnormalized(16, v));
        }

        void expr_01_out1_set(number v) {
            this->expr_01_out1 = v;
            this->fromnormalized_01_input_set(this->expr_01_out1);
        }

        void expr_01_in1_set(number in1) {
            this->expr_01_in1 = in1;
            this->expr_01_out1_set(this->expr_01_in1 * this->expr_01_in2);//#map:expr_01:1
        }

        void ctlin_01_value_set(number v) {
            this->expr_01_in1_set(v);
        }

        void ctlin_01_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
            RNBO_UNUSED(length);
            RNBO_UNUSED(port);

            if (status == 0xB0 && (channel == this->ctlin_01_channel || this->ctlin_01_channel == -1) && (data[1] == this->ctlin_01_controller || this->ctlin_01_controller == -1)) {
                this->ctlin_01_outchannel_set(channel);
                this->ctlin_01_outcontroller_set(data[1]);
                this->ctlin_01_value_set(data[2]);
                this->ctlin_01_status = 0;
            }
        }

        void muteVoice(number voicenumber) {
            //this->codebox_02_in2 = voicenumber;
            //this->codebox_02_voiceState[(Index)voiceIndex][0] = 0;//#map:_###_obj_###_:22
            //this->codebox_02_voiceState[(Index)voiceIndex][1] = this->currenttime();//#map:_###_obj_###_:23

            Index voiceIndex = voicenumber - 1;

            voice_state[voiceIndex] = false;
            this->setVoiceMuteState({ voicenumber, 1 });
        }
        //
        //void p_01_out3_number_set(number v) {
        //    this->muteVoice(v);
        //}

        void phasor_01_perform(number freq, SampleValue* out, Index n) {
            auto __phasor_01_lastLockedPhase = this->phasor_01_lastLockedPhase;
            auto quantum = this->hztobeats(freq);
            auto tempo = this->tempo();

            if (quantum > 0 && tempo > 0) {
                auto samplerate = this->samplerate();
                auto beattime = this->beattimeatsample(0);
                number offset = fmod(beattime, quantum);
                number nextJump = quantum - offset;
                number nextJumpInSamples = rnbo_fround(nextJump * 60 * samplerate / tempo * 1 / (number)1) * 1;
                number inc = tempo / (number)60 / samplerate;
                number oneoverquantum = (number)1 / quantum;

                for (Index i = 0; i < n; i++) {
                    if ((bool)(this->transportatsample(i))) {
                        out[(Index)i] = __phasor_01_lastLockedPhase = offset * oneoverquantum;
                        offset += inc;
                        nextJumpInSamples--;

                        if (nextJumpInSamples <= 0) {
                            offset -= quantum;
                            nextJumpInSamples = rnbo_fround(quantum * 60 * samplerate / tempo * 1 / (number)1) * 1;
                        }
                    }
                    else {
                        out[(Index)i] = __phasor_01_lastLockedPhase;
                    }
                }
            }
            else {
                for (Index i = 0; i < n; i++) {
                    out[(Index)i] = 0;
                }
            }

            this->phasor_01_lastLockedPhase = __phasor_01_lastLockedPhase;
        }

        void phasor_02_perform(number freq, SampleValue* out, Index n) {
            auto __phasor_02_lastLockedPhase = this->phasor_02_lastLockedPhase;
            auto quantum = this->hztobeats(freq);
            auto tempo = this->tempo();

            if (quantum > 0 && tempo > 0) {
                auto samplerate = this->samplerate();
                auto beattime = this->beattimeatsample(0);
                number offset = fmod(beattime, quantum);
                number nextJump = quantum - offset;
                number nextJumpInSamples = rnbo_fround(nextJump * 60 * samplerate / tempo * 1 / (number)1) * 1;
                number inc = tempo / (number)60 / samplerate;
                number oneoverquantum = (number)1 / quantum;

                for (Index i = 0; i < n; i++) {
                    if ((bool)(this->transportatsample(i))) {
                        out[(Index)i] = __phasor_02_lastLockedPhase = offset * oneoverquantum;
                        offset += inc;
                        nextJumpInSamples--;

                        if (nextJumpInSamples <= 0) {
                            offset -= quantum;
                            nextJumpInSamples = rnbo_fround(quantum * 60 * samplerate / tempo * 1 / (number)1) * 1;
                        }
                    }
                    else {
                        out[(Index)i] = __phasor_02_lastLockedPhase;
                    }
                }
            }
            else {
                for (Index i = 0; i < n; i++) {
                    out[(Index)i] = 0;
                }
            }

            this->phasor_02_lastLockedPhase = __phasor_02_lastLockedPhase;
        }

        void phasor_03_perform(number freq, SampleValue* out, Index n) {
            auto __phasor_03_lastLockedPhase = this->phasor_03_lastLockedPhase;
            auto quantum = this->hztobeats(freq);
            auto tempo = this->tempo();

            if (quantum > 0 && tempo > 0) {
                auto samplerate = this->samplerate();
                auto beattime = this->beattimeatsample(0);
                number offset = fmod(beattime, quantum);
                number nextJump = quantum - offset;
                number nextJumpInSamples = rnbo_fround(nextJump * 60 * samplerate / tempo * 1 / (number)1) * 1;
                number inc = tempo / (number)60 / samplerate;
                number oneoverquantum = (number)1 / quantum;

                for (Index i = 0; i < n; i++) {
                    if ((bool)(this->transportatsample(i))) {
                        out[(Index)i] = __phasor_03_lastLockedPhase = offset * oneoverquantum;
                        offset += inc;
                        nextJumpInSamples--;

                        if (nextJumpInSamples <= 0) {
                            offset -= quantum;
                            nextJumpInSamples = rnbo_fround(quantum * 60 * samplerate / tempo * 1 / (number)1) * 1;
                        }
                    }
                    else {
                        out[(Index)i] = __phasor_03_lastLockedPhase;
                    }
                }
            }
            else {
                for (Index i = 0; i < n; i++) {
                    out[(Index)i] = 0;
                }
            }

            this->phasor_03_lastLockedPhase = __phasor_03_lastLockedPhase;
        }
        /*S-mod*/
        void codebox_tilde_01_perform(
            number in1,
            number in2,
            number in3,
            number in4,
            number in5,
            number in6,
            number in7,
            number in8,
            const Sample* in9,
            const Sample* in10,
            const Sample* in11,
            SampleValue* out1,
            Index n
        ) {
            auto __codebox_tilde_01_old_sub_phase = this->codebox_tilde_01_old_sub_phase;
            auto __codebox_tilde_01_rdmdel = this->codebox_tilde_01_rdmdel;
            auto __codebox_tilde_01_oldphas = this->codebox_tilde_01_oldphas;
            auto __codebox_tilde_01_newphas = this->codebox_tilde_01_newphas;
            number temposel = in7;
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = 0;
                number mut = in1;
                number len = in2;
                number den = in3;
                number cha = in4;
                number rdl = in5;
                number syc = in6;
                number notevaluesel = in8;
                number sync_n_phase = in9[(Index)i];
                number sync_nd_phase = in10[(Index)i];
                number sync_nt_phase = in11[(Index)i];
                number maxdelay = 0;
                number frq = 0;

                if (syc == 0) {
                    frq = (-0.60651 + 41.4268 * rnbo_exp(-0.001 * len)) * den * (1 - mut);//#map:_###_obj_###_:28
                    __codebox_tilde_01_newphas = this->codebox_tilde_01_mphasor_next(frq, -1);//#map:_###_obj_###_:30

                    if (frq < ((2 * this->samplerate() == 0. ? 0. : (number)1 / (2 * this->samplerate())))) {
                        maxdelay = rdl * 2 * this->samplerate();//#map:_###_obj_###_:33
                    }
                    else {
                        maxdelay = ((frq == 0. ? 0. : rdl / frq)) * this->samplerate();//#map:_###_obj_###_:36
                    }//#map:_###_obj_###_:32

                    if (__codebox_tilde_01_newphas < __codebox_tilde_01_oldphas) {
                        number r = this->codebox_tilde_01_rdm_next() / (number)2 + 0.5;
                        number bangin = r <= cha;
                        r = this->codebox_tilde_01_rdm_next() / (number)2 + 0.5;//#map:_###_obj_###_:43
                        __codebox_tilde_01_rdmdel = r * maxdelay;//#map:_###_obj_###_:44
                        out1[(Index)i] = this->codebox_tilde_01_del_next(bangin, __codebox_tilde_01_rdmdel);//#map:_###_obj_###_:46
                    }//#map:_###_obj_###_:39

                    out1[(Index)i] = this->codebox_tilde_01_del_next(0, __codebox_tilde_01_rdmdel);//#map:_###_obj_###_:49
                    __codebox_tilde_01_oldphas = __codebox_tilde_01_newphas;//#map:_###_obj_###_:51
                }
                else {
                    number div;
                    div = 1 << (6 - (int)temposel);
                    frq = this->beatstohz((div == 0. ? 0. : (number)4 * (1 - mut) / div));                  // -mod: if mute frq is 0

                    if (frq < ((2 * this->samplerate() == 0. ? 0. : (number)1 / (2 * this->samplerate())))) {
                        maxdelay = rdl * 2 * this->samplerate();//#map:_###_obj_###_:59
                    }
                    else {
                        maxdelay = ((frq == 0. ? 0. : rdl / frq)) * this->samplerate();//#map:_###_obj_###_:62
                    }//#map:_###_obj_###_:58

                    number new_sub_phase = 0;

                    // to add: on change of notevaluesel, old_sub_phase should be reset

                    switch ((int)notevaluesel) {
                    case 0:
                    {
                        new_sub_phase = fmod(sync_n_phase * div, 1);
                        break;
                    }
                    case 1:
                    {
                        new_sub_phase = fmod(sync_nd_phase * div, 1);
                        break;
                    }
                    case 2:
                    {
                        new_sub_phase = fmod(sync_nt_phase * div, 1);
                        break;
                    }
                    }

                    if (!mut) {
                        if (new_sub_phase < __codebox_tilde_01_old_sub_phase) {
                            number r = this->codebox_tilde_01_rdm_next() / (number)2 + 0.5;
                            number bangin = r <= cha;
                            r = this->codebox_tilde_01_rdm_next() / (number)2 + 0.5;//#map:_###_obj_###_:85
                            __codebox_tilde_01_rdmdel = r * maxdelay;//#map:_###_obj_###_:86
                            out1[(Index)i] = this->codebox_tilde_01_del_next(bangin, __codebox_tilde_01_rdmdel);//#map:_###_obj_###_:88
                        }//#map:_###_obj_###_:81
                    }

                    __codebox_tilde_01_old_sub_phase = new_sub_phase;//#map:_###_obj_###_:93

                }//#map:_###_obj_###_:27
            }

            this->codebox_tilde_01_newphas = __codebox_tilde_01_newphas;
            this->codebox_tilde_01_oldphas = __codebox_tilde_01_oldphas;
            this->codebox_tilde_01_rdmdel = __codebox_tilde_01_rdmdel;
            this->codebox_tilde_01_old_sub_phase = __codebox_tilde_01_old_sub_phase;
        }
        /*E-mod*/
        void edge_02_perform(const SampleValue* input, Index n) {
            auto __edge_02_currentState = this->edge_02_currentState;

            for (Index i = 0; i < n; i++) {
                if (__edge_02_currentState == 1) {
                    if (input[(Index)i] == 0.) {
                        //this->getEngine()->scheduleClockEvent(this, -1584063977, this->sampsToMs(i) + this->_currentTime);;
                        this->setGrainProperties(i);
                        __edge_02_currentState = 0;
                    }
                }
                else {
                    if (input[(Index)i] != 0.) {
                        //this->getEngine()->scheduleClockEvent(this, -611950441, this->sampsToMs(i) + this->_currentTime);;
                        __edge_02_currentState = 1;
                    }
                }
            }

            this->edge_02_currentState = __edge_02_currentState;
        }

        void codebox_tilde_02_perform(number in1, number in2, SampleValue* out1, Index n) {
            auto __codebox_tilde_02_stoppin_time = this->codebox_tilde_02_stoppin_time;
            auto __codebox_tilde_02_gonow = this->codebox_tilde_02_gonow;
            auto __codebox_tilde_02_record = this->codebox_tilde_02_record;
            auto __codebox_tilde_02_len = this->codebox_tilde_02_len;
            Index i;

            for (i = 0; i < n; i++) {
                __codebox_tilde_02_len = in1;//#map:_###_obj_###_:3
                number r1 = this->codebox_tilde_02_redge1_next(in2);
                number r0 = this->codebox_tilde_02_redge0_next(!(bool)(in2));

                if ((bool)(r1)) {
                    __codebox_tilde_02_record = true;//#map:_###_obj_###_:19
                }
                else if ((bool)(r0)) {
                    __codebox_tilde_02_gonow = 0;//#map:_###_obj_###_:22
                    this->codebox_tilde_02_timer_reset();//#map:_###_obj_###_:23
                    __codebox_tilde_02_stoppin_time = this->mstosamps(__codebox_tilde_02_len);//#map:_###_obj_###_:24
                }//#map:_###_obj_###_:21//#map:_###_obj_###_:19

                if ((bool)(!(bool)(in2)) && (bool)(!(bool)(__codebox_tilde_02_gonow))) {
                    __codebox_tilde_02_gonow = this->codebox_tilde_02_timer_next(1, 0, __codebox_tilde_02_stoppin_time)[1];//#map:_###_obj_###_:28
                }//#map:_###_obj_###_:27

                if ((bool)(this->codebox_tilde_02_trig_next(__codebox_tilde_02_gonow)))
                    __codebox_tilde_02_record = false;//#map:_###_obj_###_:31;//#map:_###_obj_###_:31

                out1[(Index)i] = __codebox_tilde_02_record;//#map:_###_obj_###_:33
            }

            this->codebox_tilde_02_len = __codebox_tilde_02_len;
            this->codebox_tilde_02_record = __codebox_tilde_02_record;
            this->codebox_tilde_02_gonow = __codebox_tilde_02_gonow;
            this->codebox_tilde_02_stoppin_time = __codebox_tilde_02_stoppin_time;
        }

        void dspexpr_04_perform(const Sample* in1, const Sample* in2, SampleValue* out1, Index n) {
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
            }
        }

        void dspexpr_03_perform(const Sample* in1, number in2, SampleValue* out1, Index n) {
            RNBO_UNUSED(in2);
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] * 0.71;//#map:_###_obj_###_:1
            }
        }

        void dspexpr_06_perform(const Sample* in1, number in2, SampleValue* out1, Index n) {
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
            }
        }

        void codebox_tilde_03_perform(
            number in1,
            number in2,
            SampleValue* out1,
            SampleValue* out2,
            SampleValue* out3,
            Index n
        ) {
            auto __codebox_tilde_03_offs = this->codebox_tilde_03_offs;
            auto __codebox_tilde_03_reached = this->codebox_tilde_03_reached;
            auto __codebox_tilde_03_c = this->codebox_tilde_03_c;
            auto __codebox_tilde_03_len = this->codebox_tilde_03_len;
            auto __codebox_tilde_03_frz = this->codebox_tilde_03_frz;
            Index i;

            for (i = 0; i < n; i++) {
                __codebox_tilde_03_frz = in1;//#map:_###_obj_###_:4
                __codebox_tilde_03_len = this->mstosamps(in2);//#map:_###_obj_###_:6

                if ((bool)(this->codebox_tilde_03_startrec_next(!(bool)(__codebox_tilde_03_frz)))) {
                    this->codebox_tilde_03_offs_rev_reset();//#map:_###_obj_###_:19
                    __codebox_tilde_03_c = 0;//#map:_###_obj_###_:20
                    __codebox_tilde_03_reached = 0;//#map:_###_obj_###_:21
                }//#map:_###_obj_###_:18

                if ((bool)(!(bool)(__codebox_tilde_03_frz))) {
                    __codebox_tilde_03_offs = __codebox_tilde_03_len;//#map:_###_obj_###_:25
                }
                else {
                    if ((bool)(!(bool)(__codebox_tilde_03_reached))) {
                        this->codebox_tilde_03_counterstate = this->codebox_tilde_03_offs_rev_next(1, 0, __codebox_tilde_03_offs);//#map:_###_obj_###_:32
                        __codebox_tilde_03_reached = this->codebox_tilde_03_counterstate[1];//#map:_###_obj_###_:33

                        if ((bool)(!(bool)(__codebox_tilde_03_reached))) {
                            __codebox_tilde_03_c = this->codebox_tilde_03_counterstate[0];//#map:_###_obj_###_:36
                        }//#map:_###_obj_###_:35
                    }//#map:_###_obj_###_:31
                }//#map:_###_obj_###_:24

                out3[(Index)i] = this->codebox_tilde_03_done_next(__codebox_tilde_03_reached);//#map:_###_obj_###_:44
                out2[(Index)i] = __codebox_tilde_03_offs;//#map:_###_obj_###_:45
                out1[(Index)i] = __codebox_tilde_03_offs - __codebox_tilde_03_c;//#map:_###_obj_###_:46
            }

            this->codebox_tilde_03_frz = __codebox_tilde_03_frz;
            this->codebox_tilde_03_len = __codebox_tilde_03_len;
            this->codebox_tilde_03_c = __codebox_tilde_03_c;
            this->codebox_tilde_03_reached = __codebox_tilde_03_reached;
            this->codebox_tilde_03_offs = __codebox_tilde_03_offs;
        }

        void feedbackreader_01_perform(SampleValue* output, Index n) {
            auto& buffer = this->feedbacktilde_01_feedbackbuffer;

            for (Index i = 0; i < n; i++) {
                output[(Index)i] = buffer[(Index)i];
            }
        }

        void dspexpr_07_perform(const Sample* in1, number in2, SampleValue* out1, Index n) {
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
            }
        }

        void dspexpr_05_perform(const Sample* in1, const Sample* in2, SampleValue* out1, Index n) {
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] + in2[(Index)i];//#map:_###_obj_###_:1
            }
        }

        void recordtilde_01_perform(
            const Sample* record,
            number begin,
            number end,
            const SampleValue* input1,
            SampleValue* sync,
            Index n
        ) {
            RNBO_UNUSED(input1);
            RNBO_UNUSED(end);
            RNBO_UNUSED(begin);
            auto __recordtilde_01_loop = this->recordtilde_01_loop;
            auto __recordtilde_01_wIndex = this->recordtilde_01_wIndex;
            auto __recordtilde_01_lastRecord = this->recordtilde_01_lastRecord;
            auto __recordtilde_01_buffer = this->recordtilde_01_buffer;
            ConstSampleArray<1> input = { input1 };
            number bufferSize = __recordtilde_01_buffer->getSize();
            number srInv = (number)1 / this->samplerate();

            if (bufferSize > 0) {
                number maxChannel = __recordtilde_01_buffer->getChannels();
                number touched = false;

                for (Index i = 0; i < n; i++) {
                    number loopBegin = 0;
                    number loopEnd = bufferSize;

                    if (loopEnd > loopBegin) {
                        {
                            if ((bool)(record[(Index)i]) && __recordtilde_01_lastRecord != record[(Index)i] && __recordtilde_01_wIndex >= loopEnd) {
                                __recordtilde_01_wIndex = loopBegin;
                            }
                        }

                        if (record[(Index)i] != 0 && __recordtilde_01_wIndex < loopEnd) {
                            for (number channel = 0; channel < 1; channel++) {
                                number effectiveChannel = channel + 0;

                                if (effectiveChannel < maxChannel) {
                                    __recordtilde_01_buffer->setSample(channel, __recordtilde_01_wIndex, input[(Index)channel][(Index)i]);
                                    touched = true;
                                }
                                else
                                    break;
                            }

                            __recordtilde_01_wIndex++;

                            if ((bool)(__recordtilde_01_loop) && __recordtilde_01_wIndex >= loopEnd) {
                                __recordtilde_01_wIndex = loopBegin;
                            }

                            {
                                sync[(Index)i] = this->recordtilde_01_calcSync(__recordtilde_01_wIndex, loopBegin, loopEnd - loopBegin, bufferSize, srInv);
                            }
                        }
                        else {
                            sync[(Index)i] = 0;
                        }
                    }
                }

                if ((bool)(touched)) {
                    __recordtilde_01_buffer->setTouched(true);
                    __recordtilde_01_buffer->setSampleRate(this->samplerate());
                }
            }

            this->recordtilde_01_wIndex = __recordtilde_01_wIndex;
        }

        void latch_tilde_01_perform(const Sample* x, number control, SampleValue* out1, Index n) {
            auto __latch_tilde_01_value = this->latch_tilde_01_value;
            Index i;

            for (i = 0; i < n; i++) {
                if (control != 0.)
                    __latch_tilde_01_value = x[(Index)i];

                out1[(Index)i] = __latch_tilde_01_value;
            }

            this->latch_tilde_01_value = __latch_tilde_01_value;
        }

        //void p_01_perform(
        //    const SampleValue * in2,
        //    const SampleValue * in3,
        //    SampleValue * out1,
        //    SampleValue * out2,
        //    Index n
        //) {
        //    ConstSampleArray<2> ins = {in2, in3};
        //    SampleArray<2> outs = {out1, out2};
        //
        //    for (number chan = 0; chan < 2; chan++)
        //        zeroSignal(outs[(Index)chan], n);
        //
        //    for (Index i = 0; i < 24; i++)
        //        this->rtgrainvoice[i]->process(ins, 2, outs, 2, n);
        //}

        void dcblock_tilde_01_perform(const Sample* x, number gain, SampleValue* out1, Index n) {
            RNBO_UNUSED(gain);
            auto __dcblock_tilde_01_ym1 = this->dcblock_tilde_01_ym1;
            auto __dcblock_tilde_01_xm1 = this->dcblock_tilde_01_xm1;
            Index i;

            for (i = 0; i < n; i++) {
                number y = x[(Index)i] - __dcblock_tilde_01_xm1 + __dcblock_tilde_01_ym1 * 0.9997;
                __dcblock_tilde_01_xm1 = x[(Index)i];
                __dcblock_tilde_01_ym1 = y;
                out1[(Index)i] = y;
            }

            this->dcblock_tilde_01_xm1 = __dcblock_tilde_01_xm1;
            this->dcblock_tilde_01_ym1 = __dcblock_tilde_01_ym1;
        }

        void feedbackwriter_01_perform(const SampleValue* input, Index n) {
            auto& buffer = this->feedbacktilde_01_feedbackbuffer;

            for (Index i = 0; i < n; i++) {
                buffer[(Index)i] = input[(Index)i];
            }
        }

        void limi_01_perform(
            const SampleValue* input1,
            const SampleValue* input2,
            SampleValue* output1,
            SampleValue* output2,
            Index n
        ) {
            RNBO_UNUSED(output2);
            RNBO_UNUSED(output1);
            RNBO_UNUSED(input2);
            RNBO_UNUSED(input1);
            auto __limi_01_lookaheadInv = this->limi_01_lookaheadInv;
            auto __limi_01_threshold = this->limi_01_threshold;
            auto __limi_01_lookahead = this->limi_01_lookahead;
            auto __limi_01_recover = this->limi_01_recover;
            auto __limi_01_last = this->limi_01_last;
            auto __limi_01_postamp = this->limi_01_postamp;
            auto __limi_01_lookaheadIndex = this->limi_01_lookaheadIndex;
            auto __limi_01_preamp = this->limi_01_preamp;
            auto __limi_01_dcblock = this->limi_01_dcblock;
            auto __limi_01_bypass = this->limi_01_bypass;
            ConstSampleArray<2> input = { input1, input2 };
            SampleArray<2> output = { output1, output2 };

            if ((bool)(__limi_01_bypass)) {
                for (Index i = 0; i < n; i++) {
                    for (Index j = 0; j < 2; j++) {
                        output[(Index)j][(Index)i] = input[(Index)j][(Index)i];
                    }
                }
            }
            else {
                number v;

                for (Index i = 0; i < n; i++) {
                    number hotSample = 0;

                    for (Index j = 0; j < 2; j++) {
                        auto smps = input[(Index)j];
                        v = ((bool)(__limi_01_dcblock) ? this->limi_01_dc_next(j, smps[(Index)i], 0.9997) : smps[(Index)i]);
                        v *= __limi_01_preamp;
                        this->limi_01_lookaheadBuffers[(Index)j][__limi_01_lookaheadIndex] = v * __limi_01_postamp;
                        v = rnbo_fabs(v);

                        if (v > hotSample)
                            hotSample = v;
                    }

                    {
                        if (__limi_01_last > 0.01)
                            v = __limi_01_last + __limi_01_recover * __limi_01_last;
                        else
                            v = __limi_01_last + __limi_01_recover;
                    }

                    if (v > 1)
                        v = 1;

                    this->limi_01_gainBuffer[__limi_01_lookaheadIndex] = v;
                    int lookaheadPlayback = (int)(__limi_01_lookaheadIndex - (int)(__limi_01_lookahead));

                    if (lookaheadPlayback < 0)
                        lookaheadPlayback += (int)(__limi_01_lookahead);

                    if (hotSample * v > __limi_01_threshold) {
                        number newgain;
                        number curgain = __limi_01_threshold / hotSample;
                        number inc = __limi_01_threshold - curgain;
                        number acc = 0.0;
                        number flag = 0;

                        for (Index j = 0; flag == 0 && j < (Index)(__limi_01_lookahead); j++) {
                            int k = (int)(__limi_01_lookaheadIndex - (int)(j));

                            if (k < 0)
                                k += (int)(__limi_01_lookahead);

                            {
                                newgain = curgain + inc * (acc * acc);
                            }

                            if (newgain < this->limi_01_gainBuffer[(Index)k])
                                this->limi_01_gainBuffer[(Index)k] = newgain;
                            else
                                flag = 1;

                            acc = acc + __limi_01_lookaheadInv;
                        }
                    }

                    for (Index j = 0; j < 2; j++) {
                        output[(Index)j][(Index)i] = this->limi_01_lookaheadBuffers[(Index)j][(Index)lookaheadPlayback] * this->limi_01_gainBuffer[(Index)lookaheadPlayback];
                    }

                    __limi_01_last = this->limi_01_gainBuffer[__limi_01_lookaheadIndex];
                    __limi_01_lookaheadIndex++;

                    if (__limi_01_lookaheadIndex >= __limi_01_lookahead)
                        __limi_01_lookaheadIndex = 0;
                }
            }

            this->limi_01_lookaheadIndex = __limi_01_lookaheadIndex;
            this->limi_01_last = __limi_01_last;
        }

        void dspexpr_01_perform(const Sample* in1, number in2, SampleValue* out1, Index n) {
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
            }
        }

        void dspexpr_02_perform(const Sample* in1, number in2, SampleValue* out1, Index n) {
            Index i;

            for (i = 0; i < n; i++) {
                out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
            }
        }

        void recordtilde_02_perform(
            const Sample* record,
            number begin,
            number end,
            const SampleValue* input1,
            SampleValue* sync,
            Index n
        ) {
            RNBO_UNUSED(input1);
            RNBO_UNUSED(end);
            RNBO_UNUSED(begin);
            auto __recordtilde_02_loop = this->recordtilde_02_loop;
            auto __recordtilde_02_wIndex = this->recordtilde_02_wIndex;
            auto __recordtilde_02_lastRecord = this->recordtilde_02_lastRecord;
            auto __recordtilde_02_buffer = this->recordtilde_02_buffer;
            ConstSampleArray<1> input = { input1 };
            number bufferSize = __recordtilde_02_buffer->getSize();
            number srInv = (number)1 / this->samplerate();

            /* recordtilde_02_perform records the input signal to the intermediate buffer, which i only use
            * for visualisation (the AudioVisualiserComponent and its mirror_buffer, at every block read from this
            * buffer).
            * Therefore, I don't need the untouched signal, but I could downsample it for performance reasons.
            */

            /* I can try just by taking a samp every 16 or so, which should enable me to output between 8 (at 128)
            * and 128 (at 2048) samples per block. The data buffer should be vecsize()/16. Should I also set the samplerate to
            * samplerate()/16 or can I just ignore it?
            */

            if (bufferSize > 0) {
                number maxChannel = __recordtilde_02_buffer->getChannels();
                number touched = false;

                for (Index i = 0; i < n; i++) {
                    number loopBegin = 0;
                    number loopEnd = bufferSize;

                    if (loopEnd > loopBegin) {
                        {
                            if ((bool)(record[(Index)i]) && __recordtilde_02_lastRecord != record[(Index)i] && __recordtilde_02_wIndex >= loopEnd) {
                                __recordtilde_02_wIndex = loopBegin;
                            }
                        }

                        if (record[(Index)i] != 0 && __recordtilde_02_wIndex < loopEnd) {
                            for (number channel = 0; channel < 1; channel++) {
                                //int channel = 0;
                                number effectiveChannel = channel + 0;

                                if (effectiveChannel < maxChannel) {
                                    __recordtilde_02_buffer->setSample(channel, __recordtilde_02_wIndex, input[(Index)channel][(Index)i]);
                                    touched = true;
                                    //if (!__recordtilde_02_wIndex % 16) {
                                    //    recordtilde_02_buffer->setSample(0, __recordtilde_02_wIndex / 16, input[0][i]);
                                    //    touched = true;
                                    //}
                                }
                                else
                                    break;
                            }

                            __recordtilde_02_wIndex++;

                            if ((bool)(__recordtilde_02_loop) && __recordtilde_02_wIndex >= loopEnd) {
                                __recordtilde_02_wIndex = loopBegin;
                            }
                        }
                        else {
                            sync[(Index)i] = 0;
                        }
                    }
                }

                if ((bool)(touched)) {
                    __recordtilde_02_buffer->setTouched(true);
                    __recordtilde_02_buffer->setSampleRate(this->samplerate());
                }
            }

            this->recordtilde_02_wIndex = __recordtilde_02_wIndex;
        }

        void stackprotect_perform(Index n) {
            RNBO_UNUSED(n);
            auto __stackprotect_count = this->stackprotect_count;
            __stackprotect_count = 0;
            this->stackprotect_count = __stackprotect_count;
        }

        void data_01_srout_set(number) {}

        void data_01_chanout_set(number) {}

        void data_01_sizeout_set(number v) {
            this->data_01_sizeout = v;
        }

        void data_02_srout_set(number) {}

        void data_02_chanout_set(number) {}

        void data_02_sizeout_set(number v) {
            this->data_02_sizeout = v;
        }

        void data_03_srout_set(number) {}

        void data_03_chanout_set(number) {}

        void data_03_sizeout_set(number v) {
            this->data_03_sizeout = v;
        }

        void limi_01_lookahead_setter(number v) {
            this->limi_01_lookahead = (v > 128 ? 128 : (v < 0 ? 0 : v));
            this->limi_01_lookaheadInv = (number)1 / this->limi_01_lookahead;
        }

        void limi_01_preamp_setter(number v) {
            this->limi_01_preamp = rnbo_pow(10., v * 0.05);
        }

        void limi_01_postamp_setter(number v) {
            this->limi_01_postamp = rnbo_pow(10., v * 0.05);
        }

        void limi_01_threshold_setter(number v) {
            this->limi_01_threshold = rnbo_pow(10., v * 0.05);
        }

        void toggle_01_value_number_setter(number v) {
            this->toggle_01_value_number = (v != 0 ? 1 : 0);
        }

        void toggle_02_value_number_setter(number v) {
            this->toggle_02_value_number = (v != 0 ? 1 : 0);
        }

        void toggle_03_value_number_setter(number v) {
            this->toggle_03_value_number = (v != 0 ? 1 : 0);
        }

        void edge_02_dspsetup(bool force) {
            if ((bool)(this->edge_02_setupDone) && (bool)(!(bool)(force)))
                return;

            this->edge_02_setupDone = true;
        }

        number limi_01_dc1_next(number x, number gain) {
            number y = x - this->limi_01_dc1_xm1 + this->limi_01_dc1_ym1 * gain;
            this->limi_01_dc1_xm1 = x;
            this->limi_01_dc1_ym1 = y;
            return y;
        }

        void limi_01_dc1_reset() {
            this->limi_01_dc1_xm1 = 0;
            this->limi_01_dc1_ym1 = 0;
        }

        void limi_01_dc1_dspsetup() {
            this->limi_01_dc1_reset();
        }

        number limi_01_dc2_next(number x, number gain) {
            number y = x - this->limi_01_dc2_xm1 + this->limi_01_dc2_ym1 * gain;
            this->limi_01_dc2_xm1 = x;
            this->limi_01_dc2_ym1 = y;
            return y;
        }

        void limi_01_dc2_reset() {
            this->limi_01_dc2_xm1 = 0;
            this->limi_01_dc2_ym1 = 0;
        }

        void limi_01_dc2_dspsetup() {
            this->limi_01_dc2_reset();
        }

        number limi_01_dc_next(Index i, number x, number gain) {
            switch ((int)i) {
            case 0:
            {
                return this->limi_01_dc1_next(x, gain);
            }
            default:
            {
                return this->limi_01_dc2_next(x, gain);
            }
            }

            return 0;
        }

        void limi_01_dc_reset(Index i) {
            switch ((int)i) {
            case 0:
            {
                return this->limi_01_dc1_reset();
            }
            default:
            {
                return this->limi_01_dc2_reset();
            }
            }
        }

        void limi_01_dc_dspsetup(Index i) {
            switch ((int)i) {
            case 0:
            {
                return this->limi_01_dc1_dspsetup();
            }
            default:
            {
                return this->limi_01_dc2_dspsetup();
            }
            }
        }

        void limi_01_reset() {
            this->limi_01_recover = (number)1000 / (this->limi_01_release * this->samplerate());

            {
                this->limi_01_recover *= 0.707;
            }
        }

        void limi_01_dspsetup(bool force) {
            if ((bool)(this->limi_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->limi_01_reset();
            this->limi_01_setupDone = true;
            this->limi_01_dc1_dspsetup();
            this->limi_01_dc2_dspsetup();
        }

        number codebox_01_mphasor_next(number freq, number reset) {
            {
                {
                    if (reset >= 0.)
                        this->codebox_01_mphasor_currentPhase = reset;
                }
            }

            number pincr = freq * this->codebox_01_mphasor_conv;

            if (this->codebox_01_mphasor_currentPhase < 0.)
                this->codebox_01_mphasor_currentPhase = 1. + this->codebox_01_mphasor_currentPhase;

            if (this->codebox_01_mphasor_currentPhase > 1.)
                this->codebox_01_mphasor_currentPhase = this->codebox_01_mphasor_currentPhase - 1.;

            number tmp = this->codebox_01_mphasor_currentPhase;
            this->codebox_01_mphasor_currentPhase += pincr;
            return tmp;
        }

        void codebox_01_mphasor_reset() {
            this->codebox_01_mphasor_currentPhase = 0;
        }

        void codebox_01_mphasor_dspsetup() {
            this->codebox_01_mphasor_conv = (this->sr == 0. ? 0. : (number)1 / this->sr);
        }

        //void codebox_01_rdm_reset() {
        //    xoshiro_reset(
        //        systemticks() + this->voice() + this->random(0, 10000),
        //        this->codebox_01_rdm_state
        //    );
        //}

        //void codebox_01_rdm_init() {
        //    this->codebox_01_rdm_reset();
        //}

        //void codebox_01_rdm_seed(number v) {
        //    xoshiro_reset(v, this->codebox_01_rdm_state);
        //}

        //number codebox_01_rdm_next() {
        //    return xoshiro_next(this->codebox_01_rdm_state);
        //}

        number setGrainSize(number len, number rle)
        {
            number newlen = (1 - (rand01() * 0.5 + 0.5) * rle) * len;
            return newlen;//#map:_###_obj_###_:77
        }

        number clip(number v, number inf, number sup) /*#map:_###_obj_###_:28*/
        {
            if (v < inf)
                v = inf;
            else if (v > sup)
                v = sup;

            return v;
        }

        number setGrainPosition(number posinsamps, number drfinsamps, number leninsamps, number psh, number rpt, number intelligent_offset)
        {
            number r_marg = rnbo_ceil(this->clip(
                leninsamps * (psh * (1 + rpt) - 1),
                1,
                3 * leninsamps
            ));

            number pos = rnbo_ceil(this->clip(
                posinsamps + rand01() * drfinsamps,
                this->maximum(r_marg, leninsamps - intelligent_offset),
                this->samplerate() * 10
            ));

            pos = this->scale(pos, 0, this->samplerate() * 10, 0, 0.5, 1);//#map:_###_obj_###_:39
            return pos;//#map:_###_obj_###_:41
        }

        number setGrainDirection(number frp) // return -1 or 1
        {
            number r = rand01();

            number fr = 1;
            if (frp)
                fr = (r > frp) * 2 - 1;

            return fr;
        }

        number setGrainPshift(number psh, number rpt)
        {
            number ptc = rnbo_pow(2, rand01() * rpt) * psh;
            return ptc;
        }

        number setGrainVol(number rvo)
        {
            number r = rand01();
            number vol = 1 - r * rvo;
            return vol;
        }

        number setGrainPan(number pwi)
        {
            number pan = 0.5 * (1 + rand01() * pwi);
            return pan;
        }

        number p_01_calcActiveVoices() {
            {
                number activeVoices = 0;

                for (Index i = 0; i < 24; i++) {
                    if ((bool)(!(bool)(this->rtgrainvoice[(Index)i]->getIsMuted())))
                        activeVoices++;
                }

                return activeVoices;
            }
        }

        //void codebox_02_sendnoteon(number target) /*#map:_###_obj_###_:28*/
        //{
        //    this->codebox_02_out2_set({target, 0});//#map:_###_obj_###_:30
        //    this->codebox_02_out3_set(target);//#map:_###_obj_###_:32
        //
        //    this->codebox_02_out1_set({
        //        this->codebox_02_pos,
        //        this->codebox_02_len,
        //        this->codebox_02_f_r,
        //        this->codebox_02_ptc,
        //        this->codebox_02_vol,
        //        this->codebox_02_pan
        //    });//#map:_###_obj_###_:35
        //}

        //void codebox_02_assignvoice()
        //{
        //    for (int i = 0; i < 24; i++) {
        //        bool candidate_state = this->codebox_02_voiceState[i][0];
        //
        //        if (candidate_state == 0) {
        //            number target = i + 1;
        //            this->codebox_02_voiceState[i][0] = 1;
        //            this->codebox_02_voiceState[i][1] = this->currenttime();
        //            this->codebox_02_sendnoteon(target);
        //            break;
        //        }
        //    }
        //}

        int findtargetvoice()
        {
            for (int i = 0; i < 24; i++) {
                bool candidate_state = voice_state[i];

                if (candidate_state == 0) {
                    int target = i + 1;
                    voice_state[i] = true;
                    return target;
                }
            }

            return -1;
        }

        void codebox_tilde_01_n_subd_rebuild() {
            number i; number j; number c; number totaltime; number totalsteps;

            if (this->codebox_tilde_01_n_subd_pattern->length == 0) {
                this->codebox_tilde_01_n_subd_p2length = 0;
                this->codebox_tilde_01_n_subd_patternstep = 0;
                this->codebox_tilde_01_n_subd_patternphase = 0;
                return;
            }

            totaltime = 0;

            for (i = 0; i < this->codebox_tilde_01_n_subd_pattern->length; i++) {
                totaltime += this->codebox_tilde_01_n_subd_pattern[(Index)i];
            }

            i = this->safemod(totaltime, this->codebox_tilde_01_n_subd_div);
            totalsteps = (i == 0 ? totaltime : totaltime + (this->codebox_tilde_01_n_subd_div - i));

            if (this->codebox_tilde_01_n_subd_p2length != totalsteps) {
                this->codebox_tilde_01_n_subd_p2length = totalsteps;
            }

            this->codebox_tilde_01_n_subd_p2 = {};
            this->codebox_tilde_01_n_subd_synco = {};
            this->codebox_tilde_01_n_subd_p2prob = {};

            for (number i = 0; i < totalsteps; i++) {
                this->codebox_tilde_01_n_subd_p2->push(1);
                this->codebox_tilde_01_n_subd_synco->push(0);
                this->codebox_tilde_01_n_subd_p2prob->push(1.0);
            }

            totaltime = 0;
            c = 0;

            for (i = 0; i < this->codebox_tilde_01_n_subd_pattern->length; i++) {
                number nextvalue = (number)(this->codebox_tilde_01_n_subd_pattern[(Index)i]);
                number nextstart = (number)(this->safemod(totaltime, this->codebox_tilde_01_n_subd_div));
                number pv;

                if (this->codebox_tilde_01_n_subd_prob->length > 0) {
                    number ps = (number)(this->safemod(i, this->codebox_tilde_01_n_subd_prob->length));
                    pv = this->codebox_tilde_01_n_subd_prob[(Index)ps];
                }
                else {
                    pv = 1.0;
                }

                for (j = 0; j < nextvalue; j++) {
                    this->codebox_tilde_01_n_subd_p2[(Index)c] = nextvalue;

                    if (nextvalue > 1)
                        this->codebox_tilde_01_n_subd_synco[(Index)c] = nextstart;
                    else
                        this->codebox_tilde_01_n_subd_synco[(Index)c] = 0;

                    if (j == 0)
                        this->codebox_tilde_01_n_subd_p2prob[(Index)c] = pv;
                    else
                        this->codebox_tilde_01_n_subd_p2prob[(Index)c] = -1;

                    c++;
                }

                totaltime += nextvalue;
            }

            for (j = 0; c < totalsteps; c++, j++) {
                number pv = 1.0;
                this->codebox_tilde_01_n_subd_p2[(Index)c] = totalsteps - totaltime;
                this->codebox_tilde_01_n_subd_synco[(Index)c] = this->safemod(totaltime, this->codebox_tilde_01_n_subd_div);

                if (j == 0) {
                    if (this->codebox_tilde_01_n_subd_prob->length > 0) {
                        number ps = (number)(this->safemod(i, this->codebox_tilde_01_n_subd_prob->length));
                        pv = this->codebox_tilde_01_n_subd_prob[(Index)ps];
                    }
                    else {
                        pv = 1.0;
                    }
                }
                else {
                    pv = -1.0;
                }

                this->codebox_tilde_01_n_subd_p2prob[(Index)c] = pv;
            }
        }

        void codebox_tilde_01_n_subd_reset() {
            this->codebox_tilde_01_n_subd_outstep = 0;
            this->codebox_tilde_01_n_subd_posstep = 0;
            this->codebox_tilde_01_n_subd_probstep = 0;
            this->codebox_tilde_01_n_subd_lastoutstep = 0;
            this->codebox_tilde_01_n_subd_div = 1;
            this->codebox_tilde_01_n_subd_nextdiv = 1;
            this->codebox_tilde_01_n_subd_resolveSync();
            this->codebox_tilde_01_n_subd_playing = this->codebox_tilde_01_n_subd_isPlaying(0);
            this->codebox_tilde_01_n_subd_patternstep = 0;
        }

        void codebox_tilde_01_n_subd_dspsetup() {
            this->codebox_tilde_01_n_subd_reset();
            this->codebox_tilde_01_n_subd_rebuild();
        }

        void codebox_tilde_01_n_subd_init() {
            this->codebox_tilde_01_n_subd_reset();
            this->codebox_tilde_01_n_subd_rebuild();
        }

        void codebox_tilde_01_n_subd_setProb(list prob) {
            if ((bool)(!(bool)(this->listcompare(this->codebox_tilde_01_n_subd_prob, prob)[0]))) {
                this->codebox_tilde_01_n_subd_prob = {};

                if (prob->length > 0) {
                    for (number i = 0; i < prob->length; i++) {
                        this->codebox_tilde_01_n_subd_prob->push(
                            (prob[(Index)i] > 1.0 ? 1.0 : (prob[(Index)i] < 0.0 ? 0.0 : prob[(Index)i]))
                        );
                    }
                }

                this->codebox_tilde_01_n_subd_rebuild();
            }
        }

        void codebox_tilde_01_n_subd_setPattern(list pattern) {
            if ((bool)(!(bool)(this->listcompare(this->codebox_tilde_01_n_subd_pattern, pattern)[0]))) {
                this->codebox_tilde_01_n_subd_pattern = {};

                if (pattern->length > 0) {
                    for (number i = 0; i < pattern->length; i++) {
                        this->codebox_tilde_01_n_subd_pattern->push(
                            (pattern[(Index)i] > this->codebox_tilde_01_n_subd_div ? this->codebox_tilde_01_n_subd_div : (pattern[(Index)i] < 1 ? 1 : pattern[(Index)i]))
                        );
                    }
                }

                this->codebox_tilde_01_n_subd_patternstep = 0;
                this->codebox_tilde_01_n_subd_rebuild();
            }
        }

        void codebox_tilde_01_n_subd_setDiv(int div) {
            if (div != this->codebox_tilde_01_n_subd_nextdiv) {
                this->codebox_tilde_01_n_subd_nextdiv = div;
            }
        }

        number codebox_tilde_01_n_subd_getPatternNumerator(number step) {
            number value = (number)(this->codebox_tilde_01_n_subd_p2[(Index)(this->codebox_tilde_01_n_subd_patternstep + step)]);
            this->codebox_tilde_01_n_subd_patternphase = this->codebox_tilde_01_n_subd_synco[(Index)(this->codebox_tilde_01_n_subd_patternstep + step)];

            if (step == this->codebox_tilde_01_n_subd_div - 1) {
                if (this->codebox_tilde_01_n_subd_patternstep + this->codebox_tilde_01_n_subd_div >= this->codebox_tilde_01_n_subd_p2length) {
                    this->codebox_tilde_01_n_subd_patternstep = 0;
                }
                else {
                    this->codebox_tilde_01_n_subd_patternstep += this->codebox_tilde_01_n_subd_div;
                }
            }

            return value;
        }

        bool codebox_tilde_01_n_subd_getPatternProbability(number step, bool playing) {
            number pv = (number)(this->codebox_tilde_01_n_subd_p2prob[(Index)(this->codebox_tilde_01_n_subd_patternstep + step)]);

            if (pv == -1) {
                return playing;
            }

            if (pv == 0) {
                return false;
            }

            if (pv == 1) {
                return true;
            }

            return this->random(0, 1) < pv;
        }

        bool codebox_tilde_01_n_subd_isPlaying(number step) {
            bool result = false;
            bool decided = false;
            number pv;

            if (this->codebox_tilde_01_n_subd_prob->length > 0 && (bool)(!(bool)(this->codebox_tilde_01_n_subd_lockprob))) {
                number ps = (number)(this->codebox_tilde_01_n_subd_probstep);

                if (ps >= this->codebox_tilde_01_n_subd_prob->length) {
                    ps = 0;
                }

                pv = this->codebox_tilde_01_n_subd_prob[(Index)ps];
                result = this->random(0, 1) < pv;
                decided = true;
            }

            if (this->codebox_tilde_01_n_subd_pattern->length > 0) {
                if ((bool)(!(bool)(decided))) {
                    result = this->codebox_tilde_01_n_subd_getPatternProbability(step, this->codebox_tilde_01_n_subd_playing);
                }

                this->codebox_tilde_01_n_subd_num = this->codebox_tilde_01_n_subd_getPatternNumerator(step);
            }
            else {
                this->codebox_tilde_01_n_subd_num = 1;

                if ((bool)(this->codebox_tilde_01_n_subd_prob->length) && (bool)(!(bool)(decided))) {
                    number index = (number)(this->safemod(step, this->codebox_tilde_01_n_subd_prob->length));
                    pv = this->codebox_tilde_01_n_subd_prob[(Index)index];

                    if (pv == 0) {
                        return false;
                    }

                    if (pv == 1) {
                        return true;
                    }

                    return this->random(0, 1) < pv;
                }
                else if ((bool)(!(bool)(decided))) {
                    result = true;
                }
            }

            return result;
        }

        bool codebox_tilde_01_n_subd_detectReset(number v, number p, number p2) {
            if (p2 < p && v < p && p - v > 0.25) {
                return true;
            }

            if (p2 > p && v > p && v - p > 0.25) {
                return true;
            }

            return false;
        }

        void codebox_tilde_01_n_subd_resolveSync() {
            if (this->codebox_tilde_01_n_subd_nextdiv != this->codebox_tilde_01_n_subd_div) {
                this->codebox_tilde_01_n_subd_div = this->codebox_tilde_01_n_subd_nextdiv;

                if (this->codebox_tilde_01_n_subd_pattern->length > 0) {
                    this->codebox_tilde_01_n_subd_rebuild();
                }
            }
        }

        array<number, 2> codebox_tilde_01_n_subd_next(
            number x,
            number divarg,
            Int syncupdate,
            list prob,
            list pattern,
            bool lockprob,
            bool silentmode
        ) {
            this->codebox_tilde_01_n_subd_lockprob = lockprob;
            this->codebox_tilde_01_n_subd_setProb(prob);
            this->codebox_tilde_01_n_subd_setPattern(pattern);
            divarg = (divarg > 16384 ? 16384 : (divarg < 1 ? 1 : divarg));
            this->codebox_tilde_01_n_subd_setDiv(divarg);

            if (syncupdate == 0) {
                this->codebox_tilde_01_n_subd_resolveSync();
            }

            number div = (number)(this->codebox_tilde_01_n_subd_div);

            if (this->codebox_tilde_01_n_subd_posstep >= div) {
                this->codebox_tilde_01_n_subd_posstep = 0;
            }

            bool steppedalready = false;
            number val = (number)(x);

            if ((bool)(this->codebox_tilde_01_n_subd_detectReset(
                val,
                this->codebox_tilde_01_n_subd_prev,
                this->codebox_tilde_01_n_subd_prev2
            ))) {
                this->codebox_tilde_01_n_subd_posstep = 0;

                if (syncupdate > 0) {
                    this->codebox_tilde_01_n_subd_resolveSync();
                    div = this->codebox_tilde_01_n_subd_div;
                }

                if ((bool)(this->codebox_tilde_01_n_subd_prob->length) && (bool)(!(bool)(this->codebox_tilde_01_n_subd_lockprob))) {
                    if (this->codebox_tilde_01_n_subd_probstep >= this->codebox_tilde_01_n_subd_prob->length) {
                        this->codebox_tilde_01_n_subd_probstep = 0;
                    }

                    steppedalready = true;
                }

                this->codebox_tilde_01_n_subd_playing = this->codebox_tilde_01_n_subd_isPlaying(this->codebox_tilde_01_n_subd_posstep);
            }

            this->codebox_tilde_01_n_subd_prev2 = this->codebox_tilde_01_n_subd_prev;
            this->codebox_tilde_01_n_subd_prev = val;
            val *= div;
            this->codebox_tilde_01_n_subd_nextposstep = trunc(val);
            val -= this->codebox_tilde_01_n_subd_patternphase;
            val /= this->codebox_tilde_01_n_subd_num;
            this->codebox_tilde_01_n_subd_nextoutstep = trunc(val);

            if (this->codebox_tilde_01_n_subd_nextoutstep != this->codebox_tilde_01_n_subd_outstep) {
                if (syncupdate == 1) {
                    this->codebox_tilde_01_n_subd_resolveSync();
                    div = this->codebox_tilde_01_n_subd_div;
                }

                this->codebox_tilde_01_n_subd_outstep = this->codebox_tilde_01_n_subd_nextoutstep;

                if ((bool)(this->codebox_tilde_01_n_subd_prob->length) && (bool)(!(bool)(this->codebox_tilde_01_n_subd_lockprob)) && (bool)(!(bool)(steppedalready))) {
                    this->codebox_tilde_01_n_subd_probstep += 1;

                    if (this->codebox_tilde_01_n_subd_probstep >= this->codebox_tilde_01_n_subd_prob->length) {
                        this->codebox_tilde_01_n_subd_probstep = 0;
                    }
                }
            }

            if (this->codebox_tilde_01_n_subd_nextposstep != this->codebox_tilde_01_n_subd_posstep) {
                this->codebox_tilde_01_n_subd_posstep = this->codebox_tilde_01_n_subd_nextposstep;
                this->codebox_tilde_01_n_subd_playing = this->codebox_tilde_01_n_subd_isPlaying(this->codebox_tilde_01_n_subd_posstep);
            }

            number out1; number out2;

            if ((bool)(this->codebox_tilde_01_n_subd_playing)) {
                out1 = val - this->codebox_tilde_01_n_subd_nextoutstep;
                out2 = this->codebox_tilde_01_n_subd_nextoutstep;
                this->codebox_tilde_01_n_subd_lastoutstep = this->codebox_tilde_01_n_subd_nextoutstep;
            }
            else {
                out1 = 0.0;
                out2 = (silentmode == 0 ? this->codebox_tilde_01_n_subd_lastoutstep : -1);
            }

            return { out1, out2 };
        }

        number codebox_tilde_01_mphasor_next(number freq, number reset) {
            {
                {
                    if (reset >= 0.)
                        this->codebox_tilde_01_mphasor_currentPhase = reset;
                }
            }

            number pincr = freq * this->codebox_tilde_01_mphasor_conv;

            if (this->codebox_tilde_01_mphasor_currentPhase < 0.)
                this->codebox_tilde_01_mphasor_currentPhase = 1. + this->codebox_tilde_01_mphasor_currentPhase;

            if (this->codebox_tilde_01_mphasor_currentPhase > 1.)
                this->codebox_tilde_01_mphasor_currentPhase = this->codebox_tilde_01_mphasor_currentPhase - 1.;

            number tmp = this->codebox_tilde_01_mphasor_currentPhase;
            this->codebox_tilde_01_mphasor_currentPhase += pincr;
            return tmp;
        }

        void codebox_tilde_01_mphasor_reset() {
            this->codebox_tilde_01_mphasor_currentPhase = 0;
        }

        void codebox_tilde_01_mphasor_dspsetup() {
            this->codebox_tilde_01_mphasor_conv = (this->sr == 0. ? 0. : (number)1 / this->sr);
        }

        void codebox_tilde_01_del_step() {
            this->codebox_tilde_01_del_reader++;

            if (this->codebox_tilde_01_del_reader >= (int)(this->codebox_tilde_01_del_buffer->getSize()))
                this->codebox_tilde_01_del_reader = 0;
        }

        number codebox_tilde_01_del_read(number size, Int interp) {
            if (interp == 0) {
                number r = (int)(this->codebox_tilde_01_del_buffer->getSize()) + this->codebox_tilde_01_del_reader - ((size > this->codebox_tilde_01_del__maxdelay ? this->codebox_tilde_01_del__maxdelay : (size < (this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer) ? this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer : size)));
                long index1 = (long)(rnbo_floor(r));
                number frac = r - index1;
                long index2 = (long)(index1 + 1);

                return this->linearinterp(frac, this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index1 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index2 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ));
            }
            else if (interp == 1) {
                number r = (int)(this->codebox_tilde_01_del_buffer->getSize()) + this->codebox_tilde_01_del_reader - ((size > this->codebox_tilde_01_del__maxdelay ? this->codebox_tilde_01_del__maxdelay : (size < (1 + this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer) ? 1 + this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer : size)));
                long index1 = (long)(rnbo_floor(r));
                number frac = r - index1;
                Index index2 = (Index)(index1 + 1);
                Index index3 = (Index)(index2 + 1);
                Index index4 = (Index)(index3 + 1);

                return this->cubicinterp(frac, this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index1 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index2 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index3 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index4 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ));
            }
            else if (interp == 2) {
                number r = (int)(this->codebox_tilde_01_del_buffer->getSize()) + this->codebox_tilde_01_del_reader - ((size > this->codebox_tilde_01_del__maxdelay ? this->codebox_tilde_01_del__maxdelay : (size < (1 + this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer) ? 1 + this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer : size)));
                long index1 = (long)(rnbo_floor(r));
                number frac = r - index1;
                Index index2 = (Index)(index1 + 1);
                Index index3 = (Index)(index2 + 1);
                Index index4 = (Index)(index3 + 1);

                return this->splineinterp(frac, this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index1 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index2 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index3 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index4 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ));
            }
            else if (interp == 3) {
                number r = (int)(this->codebox_tilde_01_del_buffer->getSize()) + this->codebox_tilde_01_del_reader - ((size > this->codebox_tilde_01_del__maxdelay ? this->codebox_tilde_01_del__maxdelay : (size < (this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer) ? this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer : size)));
                long index1 = (long)(rnbo_floor(r));
                number frac = r - index1;
                Index index2 = (Index)(index1 + 1);

                return this->cosineinterp(frac, this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index1 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ), this->codebox_tilde_01_del_buffer->getSample(
                    0,
                    (Index)((BinOpInt)((BinOpInt)index2 & (BinOpInt)this->codebox_tilde_01_del_wrap))
                ));
            }

            number r = (int)(this->codebox_tilde_01_del_buffer->getSize()) + this->codebox_tilde_01_del_reader - ((size > this->codebox_tilde_01_del__maxdelay ? this->codebox_tilde_01_del__maxdelay : (size < (this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer) ? this->codebox_tilde_01_del_reader != this->codebox_tilde_01_del_writer : size)));
            long index1 = (long)(rnbo_floor(r));

            return this->codebox_tilde_01_del_buffer->getSample(
                0,
                (Index)((BinOpInt)((BinOpInt)index1 & (BinOpInt)this->codebox_tilde_01_del_wrap))
            );
        }

        void codebox_tilde_01_del_write(number v) {
            this->codebox_tilde_01_del_writer = this->codebox_tilde_01_del_reader;
            this->codebox_tilde_01_del_buffer[(Index)this->codebox_tilde_01_del_writer] = v;
        }

        number codebox_tilde_01_del_next(number v, int size) {
            number effectiveSize = (size == -1 ? this->codebox_tilde_01_del__maxdelay : size);
            number val = this->codebox_tilde_01_del_read(effectiveSize, 0);
            this->codebox_tilde_01_del_write(v);
            this->codebox_tilde_01_del_step();
            return val;
        }

        array<Index, 2> codebox_tilde_01_del_calcSizeInSamples() {
            number sizeInSamples = 0;
            Index allocatedSizeInSamples = 0;

            {
                sizeInSamples = this->codebox_tilde_01_del_evaluateSizeExpr(this->samplerate(), this->vectorsize());
                this->codebox_tilde_01_del_sizemode = 0;
            }

            sizeInSamples = rnbo_floor(sizeInSamples);
            sizeInSamples = this->maximum(sizeInSamples, 2);
            allocatedSizeInSamples = (Index)(sizeInSamples);
            allocatedSizeInSamples = nextpoweroftwo(allocatedSizeInSamples);
            return { sizeInSamples, allocatedSizeInSamples };
        }

        void codebox_tilde_01_del_init() {
            auto result = this->codebox_tilde_01_del_calcSizeInSamples();
            this->codebox_tilde_01_del__maxdelay = result[0];
            Index requestedSizeInSamples = (Index)(result[1]);
            this->codebox_tilde_01_del_buffer->requestSize(requestedSizeInSamples, 1);
            this->codebox_tilde_01_del_wrap = requestedSizeInSamples - 1;
        }

        void codebox_tilde_01_del_clear() {
            this->codebox_tilde_01_del_buffer->setZero();
        }

        void codebox_tilde_01_del_reset() {
            auto result = this->codebox_tilde_01_del_calcSizeInSamples();
            this->codebox_tilde_01_del__maxdelay = result[0];
            Index allocatedSizeInSamples = (Index)(result[1]);
            this->codebox_tilde_01_del_buffer->setSize(allocatedSizeInSamples);
            updateDataRef(this, this->codebox_tilde_01_del_buffer);
            this->codebox_tilde_01_del_wrap = this->codebox_tilde_01_del_buffer->getSize() - 1;
            this->codebox_tilde_01_del_clear();

            if (this->codebox_tilde_01_del_reader >= this->codebox_tilde_01_del__maxdelay || this->codebox_tilde_01_del_writer >= this->codebox_tilde_01_del__maxdelay) {
                this->codebox_tilde_01_del_reader = 0;
                this->codebox_tilde_01_del_writer = 0;
            }
        }

        void codebox_tilde_01_del_dspsetup() {
            this->codebox_tilde_01_del_reset();
        }

        number codebox_tilde_01_del_evaluateSizeExpr(number samplerate, number vectorsize) {
            RNBO_UNUSED(vectorsize);
            RNBO_UNUSED(samplerate);
            return 96000;
        }

        number codebox_tilde_01_del_size() {
            return this->codebox_tilde_01_del__maxdelay;
        }

        void codebox_tilde_01_rdm_reset() {
            xoshiro_reset(
                systemticks() + this->voice() + this->random(0, 10000),
                this->codebox_tilde_01_rdm_state
            );
        }

        void codebox_tilde_01_rdm_init() {
            this->codebox_tilde_01_rdm_reset();
        }

        void codebox_tilde_01_rdm_seed(number v) {
            xoshiro_reset(v, this->codebox_tilde_01_rdm_state);
        }

        number codebox_tilde_01_rdm_next() {
            return xoshiro_next(this->codebox_tilde_01_rdm_state);
        }

        void codebox_tilde_01_dspsetup(bool force) {
            if ((bool)(this->codebox_tilde_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->codebox_tilde_01_setupDone = true;
            this->codebox_tilde_01_n_subd_dspsetup();
            this->codebox_tilde_01_mphasor_dspsetup();
            this->codebox_tilde_01_del_dspsetup();
        }

        void param_01_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_01_value;
        }

        void param_01_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_01_value_set(preset["value"]);
        }

        void dial_01_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_01_value;
        }

        void dial_01_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_01_value_set(preset["value"]);
        }

        void param_02_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_02_value;
        }

        void param_02_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_02_value_set(preset["value"]);
        }

        void dial_02_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_02_value;
        }

        void dial_02_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_02_value_set(preset["value"]);
        }

        void param_03_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_03_value;
        }

        void param_03_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_03_value_set(preset["value"]);
        }

        void dial_03_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_03_value;
        }

        void dial_03_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_03_value_set(preset["value"]);
        }

        void param_04_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_04_value;
        }

        void param_04_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_04_value_set(preset["value"]);
        }

        void dial_04_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_04_value;
        }

        void dial_04_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_04_value_set(preset["value"]);
        }

        void param_05_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_05_value;
        }

        void param_05_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_05_value_set(preset["value"]);
        }

        void dial_05_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_05_value;
        }

        void dial_05_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_05_value_set(preset["value"]);
        }

        void data_01_init() {
            this->data_01_buffer->setWantsFill(true);
        }

        Index data_01_evaluateSizeExpr(number samplerate, number vectorsize) {
            RNBO_UNUSED(vectorsize);
            RNBO_UNUSED(samplerate);
            number size = 0;
            return (Index)(size);
        }

        void data_01_dspsetup(bool force) {
            if ((bool)(this->data_01_setupDone) && (bool)(!(bool)(force)))
                return;

            if (this->data_01_sizemode == 2) {
                this->data_01_buffer = this->data_01_buffer->setSize((Index)(this->mstosamps(this->data_01_sizems)));
                updateDataRef(this, this->data_01_buffer);
            }
            else if (this->data_01_sizemode == 3) {
                this->data_01_buffer = this->data_01_buffer->setSize(this->data_01_evaluateSizeExpr(this->samplerate(), this->vectorsize()));
                updateDataRef(this, this->data_01_buffer);
            }

            this->data_01_setupDone = true;
        }

        void data_01_bufferUpdated() {
            this->data_01_report();
        }

        void data_01_report() {
            this->data_01_srout_set(this->data_01_buffer->getSampleRate());
            this->data_01_chanout_set(this->data_01_buffer->getChannels());
            this->data_01_sizeout_set(this->data_01_buffer->getSize());
        }

        void phasor_01_dspsetup(bool force) {
            if ((bool)(this->phasor_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->phasor_01_conv = (number)1 / this->samplerate();
            this->phasor_01_setupDone = true;
        }

        void param_06_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_06_value;
        }

        void param_06_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_06_value_set(preset["value"]);
        }

        void dial_06_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_06_value;
        }

        void dial_06_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_06_value_set(preset["value"]);
        }

        void param_07_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_07_value;
        }

        void param_07_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_07_value_set(preset["value"]);
        }

        void dial_07_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_07_value;
        }

        void dial_07_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_07_value_set(preset["value"]);
        }

        void phasor_02_dspsetup(bool force) {
            if ((bool)(this->phasor_02_setupDone) && (bool)(!(bool)(force)))
                return;

            this->phasor_02_conv = (number)1 / this->samplerate();
            this->phasor_02_setupDone = true;
        }

        void param_08_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_08_value;
        }

        void param_08_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_08_value_set(preset["value"]);
        }

        void dial_08_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_08_value;
        }

        void dial_08_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_08_value_set(preset["value"]);
        }

        void param_09_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_09_value;
        }

        void param_09_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_09_value_set(preset["value"]);
        }

        void dial_09_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_09_value;
        }

        void dial_09_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_09_value_set(preset["value"]);
        }

        void phasor_03_dspsetup(bool force) {
            if ((bool)(this->phasor_03_setupDone) && (bool)(!(bool)(force)))
                return;

            this->phasor_03_conv = (number)1 / this->samplerate();
            this->phasor_03_setupDone = true;
        }

        void param_10_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_10_value;
        }

        void param_10_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_10_value_set(preset["value"]);
        }

        void dial_10_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_10_value;
        }

        void dial_10_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_10_value_set(preset["value"]);
        }

        void param_11_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_11_value;
        }

        void param_11_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_11_value_set(preset["value"]);
        }

        void dial_11_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_11_value;
        }

        void dial_11_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_11_value_set(preset["value"]);
        }

        void param_12_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_12_value;
        }

        void param_12_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_12_value_set(preset["value"]);
        }

        void dial_12_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_12_value;
        }

        void dial_12_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_12_value_set(preset["value"]);
        }

        void param_13_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_13_value;
        }

        void param_13_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_13_value_set(preset["value"]);
        }

        void dial_13_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_13_value;
        }

        void dial_13_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_13_value_set(preset["value"]);
        }

        void param_14_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_14_value;
        }

        void param_14_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_14_value_set(preset["value"]);
        }

        void dial_14_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_14_value;
        }

        void dial_14_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_14_value_set(preset["value"]);
        }

        void latch_tilde_01_reset() {
            this->latch_tilde_01_value = 0;
        }

        void latch_tilde_01_dspsetup(bool force) {
            if ((bool)(this->latch_tilde_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->latch_tilde_01_reset();
            this->latch_tilde_01_setupDone = true;
        }

        array<number, 3> codebox_tilde_02_timer_next(number a, number reset, number limit) {
            number carry_flag = 0;

            if (reset != 0) {
                this->codebox_tilde_02_timer_count = 0;
                this->codebox_tilde_02_timer_carry = 0;
            }
            else {
                this->codebox_tilde_02_timer_count += a;

                if (limit != 0) {
                    if ((a > 0 && this->codebox_tilde_02_timer_count >= limit) || (a < 0 && this->codebox_tilde_02_timer_count <= limit)) {
                        this->codebox_tilde_02_timer_count = 0;
                        this->codebox_tilde_02_timer_carry += 1;
                        carry_flag = 1;
                    }
                }
            }

            return {
                this->codebox_tilde_02_timer_count,
                carry_flag,
                this->codebox_tilde_02_timer_carry
            };
        }

        void codebox_tilde_02_timer_init() {
            this->codebox_tilde_02_timer_count = 0;
        }

        void codebox_tilde_02_timer_reset() {
            this->codebox_tilde_02_timer_carry = 0;
            this->codebox_tilde_02_timer_count = 0;
        }

        number codebox_tilde_02_redge1_next(number val) {
            if ((0 == 0 && val <= 0) || (0 == 1 && val > 0)) {
                this->codebox_tilde_02_redge1_active = false;
            }
            else if ((bool)(!(bool)(this->codebox_tilde_02_redge1_active))) {
                this->codebox_tilde_02_redge1_active = true;
                return 1.0;
            }

            return 0.0;
        }

        void codebox_tilde_02_redge1_init() {}

        void codebox_tilde_02_redge1_reset() {
            this->codebox_tilde_02_redge1_active = false;
        }

        number codebox_tilde_02_redge0_next(number val) {
            if ((0 == 0 && val <= 0) || (0 == 1 && val > 0)) {
                this->codebox_tilde_02_redge0_active = false;
            }
            else if ((bool)(!(bool)(this->codebox_tilde_02_redge0_active))) {
                this->codebox_tilde_02_redge0_active = true;
                return 1.0;
            }

            return 0.0;
        }

        void codebox_tilde_02_redge0_init() {}

        void codebox_tilde_02_redge0_reset() {
            this->codebox_tilde_02_redge0_active = false;
        }

        number codebox_tilde_02_trig_next(number val) {
            if ((0 == 0 && val <= 0) || (0 == 1 && val > 0)) {
                this->codebox_tilde_02_trig_active = false;
            }
            else if ((bool)(!(bool)(this->codebox_tilde_02_trig_active))) {
                this->codebox_tilde_02_trig_active = true;
                return 1.0;
            }

            return 0.0;
        }

        void codebox_tilde_02_trig_init() {}

        void codebox_tilde_02_trig_reset() {
            this->codebox_tilde_02_trig_active = false;
        }

        number recordtilde_01_calcSync(
            number writeIndex,
            number loopMin,
            number loopLength,
            SampleIndex bufferLength,
            number srInv
        ) {
            RNBO_UNUSED(srInv);
            RNBO_UNUSED(bufferLength);

            {
                return (writeIndex - loopMin) / loopLength;
            }
        }

        void param_15_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_15_value;
        }

        void param_15_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_15_value_set(preset["value"]);
        }

        void dial_15_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_15_value;
        }

        void dial_15_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_15_value_set(preset["value"]);
        }

        void param_16_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_16_value;
        }

        void param_16_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_16_value_set(preset["value"]);
        }

        void dial_16_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_16_value;
        }

        void dial_16_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_16_value_set(preset["value"]);
        }

        void toggle_01_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->toggle_01_value_number;
        }

        void toggle_01_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->toggle_01_value_number_set(preset["value"]);
        }

        void param_17_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_17_value;
        }

        void param_17_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_17_value_set(preset["value"]);
        }

        void param_18_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_18_value;
        }

        void param_18_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_18_value_set(preset["value"]);
        }

        void toggle_02_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->toggle_02_value_number;
        }

        void toggle_02_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->toggle_02_value_number_set(preset["value"]);
        }

        void toggle_03_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->toggle_03_value_number;
        }

        void toggle_03_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->toggle_03_value_number_set(preset["value"]);
        }

        void param_19_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_19_value;
        }

        void param_19_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_19_value_set(preset["value"]);
        }

        void dial_17_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_17_value;
        }

        void dial_17_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_17_value_set(preset["value"]);
        }

        void param_20_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_20_value;
        }

        void param_20_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_20_value_set(preset["value"]);
        }

        void dial_18_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->dial_18_value;
        }

        void dial_18_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->dial_18_value_set(preset["value"]);
        }

        void param_21_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_21_value;
        }

        void param_21_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_21_value_set(preset["value"]);
        }

        void dcblock_tilde_01_reset() {
            this->dcblock_tilde_01_xm1 = 0;
            this->dcblock_tilde_01_ym1 = 0;
        }

        void dcblock_tilde_01_dspsetup(bool force) {
            if ((bool)(this->dcblock_tilde_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->dcblock_tilde_01_reset();
            this->dcblock_tilde_01_setupDone = true;
        }

        array<number, 3> codebox_tilde_03_offs_rev_next(number a, number reset, number limit) {
            number carry_flag = 0;

            if (reset != 0) {
                this->codebox_tilde_03_offs_rev_count = 0;
                this->codebox_tilde_03_offs_rev_carry = 0;
            }
            else {
                this->codebox_tilde_03_offs_rev_count += a;

                if (limit != 0) {
                    if ((a > 0 && this->codebox_tilde_03_offs_rev_count >= limit) || (a < 0 && this->codebox_tilde_03_offs_rev_count <= limit)) {
                        this->codebox_tilde_03_offs_rev_count = 0;
                        this->codebox_tilde_03_offs_rev_carry += 1;
                        carry_flag = 1;
                    }
                }
            }

            return {
                this->codebox_tilde_03_offs_rev_count,
                carry_flag,
                this->codebox_tilde_03_offs_rev_carry
            };
        }

        void codebox_tilde_03_offs_rev_init() {
            this->codebox_tilde_03_offs_rev_count = 0;
        }

        void codebox_tilde_03_offs_rev_reset() {
            this->codebox_tilde_03_offs_rev_carry = 0;
            this->codebox_tilde_03_offs_rev_count = 0;
        }

        number codebox_tilde_03_done_next(number val) {
            if ((0 == 0 && val <= 0) || (0 == 1 && val > 0)) {
                this->codebox_tilde_03_done_active = false;
            }
            else if ((bool)(!(bool)(this->codebox_tilde_03_done_active))) {
                this->codebox_tilde_03_done_active = true;
                return 1.0;
            }

            return 0.0;
        }

        void codebox_tilde_03_done_init() {}

        void codebox_tilde_03_done_reset() {
            this->codebox_tilde_03_done_active = false;
        }

        number codebox_tilde_03_startrec_next(number val) {
            if ((0 == 0 && val <= 0) || (0 == 1 && val > 0)) {
                this->codebox_tilde_03_startrec_active = false;
            }
            else if ((bool)(!(bool)(this->codebox_tilde_03_startrec_active))) {
                this->codebox_tilde_03_startrec_active = true;
                return 1.0;
            }

            return 0.0;
        }

        void codebox_tilde_03_startrec_init() {}

        void codebox_tilde_03_startrec_reset() {
            this->codebox_tilde_03_startrec_active = false;
        }

        void data_02_init() {
            {
                this->data_02_buffer->requestSize(
                    this->data_02_evaluateSizeExpr(this->samplerate(), this->vectorsize()),
                    this->data_02_channels
                );
            }

            this->data_02_buffer->setWantsFill(true);
        }

        Index data_02_evaluateSizeExpr(number samplerate, number vectorsize) {
            RNBO_UNUSED(samplerate);
            number size = 0;

            {
                size = vectorsize;
            }

            return (Index)(size);
        }

        void data_02_dspsetup(bool force) {
            if ((bool)(this->data_02_setupDone) && (bool)(!(bool)(force)))
                return;

            if (this->data_02_sizemode == 2) {
                this->data_02_buffer = this->data_02_buffer->setSize((Index)(this->mstosamps(this->data_02_sizems)));
                updateDataRef(this, this->data_02_buffer);
            }
            else if (this->data_02_sizemode == 3) {
                this->data_02_buffer = this->data_02_buffer->setSize(this->data_02_evaluateSizeExpr(this->samplerate(), this->vectorsize()));
                updateDataRef(this, this->data_02_buffer);
            }

            this->data_02_setupDone = true;
        }

        void data_02_bufferUpdated() {
            this->data_02_report();
        }

        void data_02_report() {
            this->data_02_srout_set(this->data_02_buffer->getSampleRate());
            this->data_02_chanout_set(this->data_02_buffer->getChannels());
            this->data_02_sizeout_set(this->data_02_buffer->getSize());
        }

        void data_03_init() {
            this->data_03_buffer->setWantsFill(true);
        }

        Index data_03_evaluateSizeExpr(number samplerate, number vectorsize) {
            RNBO_UNUSED(vectorsize);
            RNBO_UNUSED(samplerate);
            number size = 0;
            return (Index)(size);
        }

        void data_03_dspsetup(bool force) {
            if ((bool)(this->data_03_setupDone) && (bool)(!(bool)(force)))
                return;

            if (this->data_03_sizemode == 2) {
                this->data_03_buffer = this->data_03_buffer->setSize((Index)(this->mstosamps(this->data_03_sizems)));
                updateDataRef(this, this->data_03_buffer);
            }
            else if (this->data_03_sizemode == 3) {
                this->data_03_buffer = this->data_03_buffer->setSize(this->data_03_evaluateSizeExpr(this->samplerate(), this->vectorsize()));
                updateDataRef(this, this->data_03_buffer);
            }

            this->data_03_setupDone = true;
        }

        void data_03_bufferUpdated() {
            this->data_03_report();
        }

        void data_03_report() {
            this->data_03_srout_set(this->data_03_buffer->getSampleRate());
            this->data_03_chanout_set(this->data_03_buffer->getChannels());
            this->data_03_sizeout_set(this->data_03_buffer->getSize());
        }

        void timevalue_01_sendValue() {
            {
                {
                    {
                        {
                            {
                                {
                                    this->timevalue_01_out_set(this->tickstohz(1920));
                                }
                            }
                        }
                    }
                }
            }
        }

        void timevalue_01_onTempoChanged(number tempo) {
            RNBO_UNUSED(tempo);

            {
                this->timevalue_01_sendValue();
            }
        }

        void timevalue_01_onSampleRateChanged(number) {}

        void timevalue_01_onTimeSignatureChanged(number, number) {}

        void timevalue_02_sendValue() {
            {
                {
                    {
                        {
                            {
                                {
                                    this->timevalue_02_out_set(this->tickstohz(2880));
                                }
                            }
                        }
                    }
                }
            }
        }

        void timevalue_02_onTempoChanged(number tempo) {
            RNBO_UNUSED(tempo);

            {
                this->timevalue_02_sendValue();
            }
        }

        void timevalue_02_onSampleRateChanged(number) {}

        void timevalue_02_onTimeSignatureChanged(number, number) {}

        void timevalue_03_sendValue() {
            {
                {
                    {
                        {
                            {
                                {
                                    this->timevalue_03_out_set(this->tickstohz(1280));
                                }
                            }
                        }
                    }
                }
            }
        }

        void timevalue_03_onTempoChanged(number tempo) {
            RNBO_UNUSED(tempo);

            {
                this->timevalue_03_sendValue();
            }
        }

        void timevalue_03_onSampleRateChanged(number) {}

        void timevalue_03_onTimeSignatureChanged(number, number) {}

        Index globaltransport_getSampleOffset(MillisecondTime time) {
            return this->mstosamps(this->maximum(0, time - this->getEngine()->getCurrentTime()));
        }

        number globaltransport_getTempoAtSample(SampleIndex sampleOffset) {
            return (sampleOffset >= 0 && sampleOffset < this->vs ? this->globaltransport_tempo[(Index)sampleOffset] : this->globaltransport_lastTempo);
        }

        number globaltransport_getStateAtSample(SampleIndex sampleOffset) {
            return (sampleOffset >= 0 && sampleOffset < this->vs ? this->globaltransport_state[(Index)sampleOffset] : this->globaltransport_lastState);
        }

        number globaltransport_getState(MillisecondTime time) {
            return this->globaltransport_getStateAtSample(this->globaltransport_getSampleOffset(time));
        }

        number globaltransport_getBeatTime(MillisecondTime time) {
            number i = 2;

            while (i < this->globaltransport_beatTimeChanges->length && this->globaltransport_beatTimeChanges[(Index)(i + 1)] <= time) {
                i += 2;
            }

            i -= 2;
            number beatTimeBase = this->globaltransport_beatTimeChanges[(Index)i];

            if (this->globaltransport_getState(time) == 0)
                return beatTimeBase;

            number beatTimeBaseMsTime = this->globaltransport_beatTimeChanges[(Index)(i + 1)];
            number diff = time - beatTimeBaseMsTime;
            return beatTimeBase + this->mstobeats(diff);
        }

        bool globaltransport_setTempo(MillisecondTime time, number tempo, bool notify) {
            if ((bool)(notify)) {
                this->processTempoEvent(time, tempo);
                this->globaltransport_notify = true;
            }
            else {
                Index offset = (Index)(this->globaltransport_getSampleOffset(time));

                if (this->globaltransport_getTempoAtSample(offset) != tempo) {
                    this->globaltransport_beatTimeChanges->push(this->globaltransport_getBeatTime(time));
                    this->globaltransport_beatTimeChanges->push(time);
                    fillSignal(this->globaltransport_tempo, this->vs, tempo, offset);
                    this->globaltransport_lastTempo = tempo;
                    this->globaltransport_tempoNeedsReset = true;
                    return true;
                }
            }

            return false;
        }

        number globaltransport_getTempo(MillisecondTime time) {
            return this->globaltransport_getTempoAtSample(this->globaltransport_getSampleOffset(time));
        }

        bool globaltransport_setState(MillisecondTime time, number state, bool notify) {
            if ((bool)(notify)) {
                this->processTransportEvent(time, TransportState(state));
                this->globaltransport_notify = true;
            }
            else {
                Index offset = (Index)(this->globaltransport_getSampleOffset(time));

                if (this->globaltransport_getStateAtSample(offset) != state) {
                    fillSignal(this->globaltransport_state, this->vs, state, offset);
                    this->globaltransport_lastState = TransportState(state);
                    this->globaltransport_stateNeedsReset = true;

                    if (state == 0) {
                        this->globaltransport_beatTimeChanges->push(this->globaltransport_getBeatTime(time));
                        this->globaltransport_beatTimeChanges->push(time);
                    }

                    return true;
                }
            }

            return false;
        }

        bool globaltransport_setBeatTime(MillisecondTime time, number beattime, bool notify) {
            if ((bool)(notify)) {
                this->processBeatTimeEvent(time, beattime);
                this->globaltransport_notify = true;
                return false;
            }
            else {
                bool beatTimeHasChanged = false;
                float oldBeatTime = (float)(this->globaltransport_getBeatTime(time));
                float newBeatTime = (float)(beattime);

                if (oldBeatTime != newBeatTime) {
                    beatTimeHasChanged = true;
                }

                this->globaltransport_beatTimeChanges->push(beattime);
                this->globaltransport_beatTimeChanges->push(time);
                return beatTimeHasChanged;
            }
        }

        number globaltransport_getBeatTimeAtSample(SampleIndex sampleOffset) {
            auto msOffset = this->sampstoms(sampleOffset);
            return this->globaltransport_getBeatTime(this->getEngine()->getCurrentTime() + msOffset);
        }

        array<number, 2> globaltransport_getTimeSignature(MillisecondTime time) {
            number i = 3;

            while (i < this->globaltransport_timeSignatureChanges->length && this->globaltransport_timeSignatureChanges[(Index)(i + 2)] <= time) {
                i += 3;
            }

            i -= 3;

            return {
                this->globaltransport_timeSignatureChanges[(Index)i],
                this->globaltransport_timeSignatureChanges[(Index)(i + 1)]
            };
        }

        array<number, 2> globaltransport_getTimeSignatureAtSample(SampleIndex sampleOffset) {
            auto msOffset = this->sampstoms(sampleOffset);
            return this->globaltransport_getTimeSignature(this->getEngine()->getCurrentTime() + msOffset);
        }

        bool globaltransport_setTimeSignature(MillisecondTime time, number numerator, number denominator, bool notify) {
            if ((bool)(notify)) {
                this->processTimeSignatureEvent(time, (int)(numerator), (int)(denominator));
                this->globaltransport_notify = true;
            }
            else {
                array<number, 2> currentSig = this->globaltransport_getTimeSignature(time);

                if (currentSig[0] != numerator || currentSig[1] != denominator) {
                    this->globaltransport_timeSignatureChanges->push(numerator);
                    this->globaltransport_timeSignatureChanges->push(denominator);
                    this->globaltransport_timeSignatureChanges->push(time);
                    return true;
                }
            }

            return false;
        }

        void globaltransport_advance() {
            if ((bool)(this->globaltransport_tempoNeedsReset)) {
                fillSignal(this->globaltransport_tempo, this->vs, this->globaltransport_lastTempo);
                this->globaltransport_tempoNeedsReset = false;

                if ((bool)(this->globaltransport_notify)) {
                    this->getEngine()->sendTempoEvent(this->globaltransport_lastTempo);
                }
            }

            if ((bool)(this->globaltransport_stateNeedsReset)) {
                fillSignal(this->globaltransport_state, this->vs, this->globaltransport_lastState);
                this->globaltransport_stateNeedsReset = false;

                if ((bool)(this->globaltransport_notify)) {
                    this->getEngine()->sendTransportEvent(TransportState(this->globaltransport_lastState));
                }
            }

            if (this->globaltransport_beatTimeChanges->length > 2) {
                this->globaltransport_beatTimeChanges[0] = this->globaltransport_beatTimeChanges[(Index)(this->globaltransport_beatTimeChanges->length - 2)];
                this->globaltransport_beatTimeChanges[1] = this->globaltransport_beatTimeChanges[(Index)(this->globaltransport_beatTimeChanges->length - 1)];
                this->globaltransport_beatTimeChanges->length = 2;

                if ((bool)(this->globaltransport_notify)) {
                    this->getEngine()->sendBeatTimeEvent(this->globaltransport_beatTimeChanges[0]);
                }
            }

            if (this->globaltransport_timeSignatureChanges->length > 3) {
                this->globaltransport_timeSignatureChanges[0] = this->globaltransport_timeSignatureChanges[(Index)(this->globaltransport_timeSignatureChanges->length - 3)];
                this->globaltransport_timeSignatureChanges[1] = this->globaltransport_timeSignatureChanges[(Index)(this->globaltransport_timeSignatureChanges->length - 2)];
                this->globaltransport_timeSignatureChanges[2] = this->globaltransport_timeSignatureChanges[(Index)(this->globaltransport_timeSignatureChanges->length - 1)];
                this->globaltransport_timeSignatureChanges->length = 3;

                if ((bool)(this->globaltransport_notify)) {
                    this->getEngine()->sendTimeSignatureEvent(
                        (int)(this->globaltransport_timeSignatureChanges[0]),
                        (int)(this->globaltransport_timeSignatureChanges[1])
                    );
                }
            }

            this->globaltransport_notify = false;
        }

        void globaltransport_dspsetup(bool force) {
            if ((bool)(this->globaltransport_setupDone) && (bool)(!(bool)(force)))
                return;

            fillSignal(this->globaltransport_tempo, this->vs, this->globaltransport_lastTempo);
            this->globaltransport_tempoNeedsReset = false;
            fillSignal(this->globaltransport_state, this->vs, this->globaltransport_lastState);
            this->globaltransport_stateNeedsReset = false;
            this->globaltransport_setupDone = true;
        }

        bool stackprotect_check() {
            this->stackprotect_count++;

            if (this->stackprotect_count > 128) {
                console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
                return true;
            }

            return false;
        }

        void updateTime(MillisecondTime time) {
            this->_currentTime = time;
            //this->sampleOffsetIntoNextAudioBuffer = (SampleIndex)(rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr)));

            //if (this->sampleOffsetIntoNextAudioBuffer >= (SampleIndex)(this->vs))
            //    this->sampleOffsetIntoNextAudioBuffer = (SampleIndex)(this->vs) - 1;

            //if (this->sampleOffsetIntoNextAudioBuffer < 0)
            //    this->sampleOffsetIntoNextAudioBuffer = 0;
        }

        void assign_defaults()
        {
            limi_01_bypass = 0;
            limi_01_dcblock = 0;
            limi_01_lookahead = 100;
            limi_01_lookahead_setter(limi_01_lookahead);
            limi_01_preamp = 0;
            limi_01_preamp_setter(limi_01_preamp);
            limi_01_postamp = 0;
            limi_01_postamp_setter(limi_01_postamp);
            limi_01_threshold = 0;
            limi_01_threshold_setter(limi_01_threshold);
            limi_01_release = 1000;
            dspexpr_01_in1 = 0;
            dspexpr_01_in2 = 1;
            //codebox_01_len = 500;
            //codebox_01_den = 1;
            //codebox_01_cpo = 0.5;
            //codebox_01_drf = 0;
            //codebox_01_rvo = 0;
            //codebox_01_pwi = 0.5;
            //codebox_01_frp = 0;
            //codebox_01_psh = 1;
            //codebox_01_rpt = 0;
            //codebox_01_env = 1;
            //codebox_01_frz = 0;
            //codebox_01_rle = 0;
            p_01_target = 0;
            //codebox_02_in2 = 0;
            //codebox_02_out3 = 0;
            codebox_tilde_01_in1 = 0;
            codebox_tilde_01_in2 = 0;
            codebox_tilde_01_in3 = 0;
            codebox_tilde_01_in4 = 0;
            codebox_tilde_01_in5 = 0;
            codebox_tilde_01_in6 = 0;
            codebox_tilde_01_in7 = 0;
            codebox_tilde_01_in8 = 0;
            codebox_tilde_01_in9 = 0;
            codebox_tilde_01_in10 = 0;
            codebox_tilde_01_in11 = 0;
            param_01_value = 0.52;
            dial_01_value = 0;
            param_02_value = 100;
            dial_02_value = 0;
            dspexpr_02_in1 = 0;
            dspexpr_02_in2 = 1;
            param_03_value = 0;
            dial_03_value = 0;
            param_04_value = 200;
            dial_04_value = 0;
            param_05_value = 0;
            dial_05_value = 0;
            data_01_sizeout = 0;
            data_01_size = 100;
            data_01_sizems = 0;
            data_01_normalize = 0.995;
            data_01_channels = 1;
            phasor_01_freq = 0;
            param_06_value = 1;
            dial_06_value = 0;
            param_07_value = 0;
            dial_07_value = 0;
            phasor_02_freq = 0;
            param_08_value = 1;
            dial_08_value = 0;
            param_09_value = 0;
            dial_09_value = 0;
            phasor_03_freq = 0;
            param_10_value = 0;
            dial_10_value = 0;
            param_11_value = 0;
            dial_11_value = 0;
            param_12_value = 0;
            dial_12_value = 0;
            param_13_value = 0;
            dial_13_value = 0;
            param_14_value = 1;
            dial_14_value = 0;
            latch_tilde_01_x = 0;
            latch_tilde_01_control = 0;
            codebox_tilde_02_in1 = 0;
            codebox_tilde_02_in2 = 0;
            recordtilde_01_record = 0;
            recordtilde_01_begin = 0;
            recordtilde_01_end = -1;
            recordtilde_01_loop = 1;
            param_15_value = 0;
            recordtilde_02_record = 0;
            recordtilde_02_begin = 0;
            recordtilde_02_end = -1;
            recordtilde_02_loop = 0;
            dial_15_value = 0;
            param_16_value = 1;
            dial_16_value = 0;
            toggle_01_value_number = 0;
            toggle_01_value_number_setter(toggle_01_value_number);
            param_17_value = 0;
            param_18_value = 0;
            toggle_02_value_number = 0;
            toggle_02_value_number_setter(toggle_02_value_number);
            toggle_03_value_number = 0;
            toggle_03_value_number_setter(toggle_03_value_number);
            param_19_value = 0;
            expr_02_in1 = 0;
            expr_02_out1 = 0;
            select_01_test1 = 1;
            dial_17_value = 0;
            param_20_value = 3;
            dspexpr_03_in1 = 0;
            dspexpr_03_in2 = 0.71;
            dspexpr_04_in1 = 0;
            dspexpr_04_in2 = 0;
            dspexpr_05_in1 = 0;
            dspexpr_05_in2 = 0;
            dspexpr_06_in1 = 0;
            dspexpr_06_in2 = 1;
            dial_18_value = 0;
            param_21_value = 0;
            dcblock_tilde_01_x = 0;
            dcblock_tilde_01_gain = 0.9997;
            dspexpr_07_in1 = 0;
            dspexpr_07_in2 = 0;
            codebox_tilde_03_in1 = 0;
            codebox_tilde_03_in2 = 0;
            data_02_sizeout = 0;
            data_02_size = 0;
            data_02_sizems = 0;
            data_02_normalize = 0.995;
            data_02_channels = 1;
            data_03_sizeout = 0;
            data_03_size = 0;
            data_03_sizems = 20000;
            data_03_normalize = 0.995;
            data_03_channels = 1;
            ctlin_01_input = 0;
            ctlin_01_controller = 2;
            ctlin_01_channel = -1;
            expr_01_in1 = 0;
            expr_01_in2 = 0.007874015748;
            expr_01_out1 = 0;
            _currentTime = 0;
            audioProcessSampleCount = 0;
            //sampleOffsetIntoNextAudioBuffer = 0;
            zeroBuffer = nullptr;
            dummyBuffer = nullptr;
            signals[0] = nullptr;
            signals[1] = nullptr;
            signals[2] = nullptr;
            signals[3] = nullptr;
            signals[4] = nullptr;
            signals[5] = nullptr;
            didAllocateSignals = 0;
            vs = 0;
            maxvs = 0;
            sr = 44100;
            invsr = 0.00002267573696;
            edge_02_setupDone = false;
            limi_01_last = 0;
            limi_01_lookaheadIndex = 0;
            limi_01_recover = 0;
            limi_01_lookaheadInv = 0;
            limi_01_dc1_xm1 = 0;
            limi_01_dc1_ym1 = 0;
            limi_01_dc2_xm1 = 0;
            limi_01_dc2_ym1 = 0;
            limi_01_setupDone = false;
            //codebox_01_intelligent_offset = 0;
            codebox_01_newphas = 0;
            codebox_01_oldphas = 0;
            //len_in_samps = 0;
            codebox_01_mphasor_currentPhase = 0;
            codebox_01_mphasor_conv = 0;
            codebox_02_pos = 0;
            codebox_02_len = 0;
            codebox_02_f_r = 0;
            codebox_02_ptc = 0;
            codebox_02_vol = 0;
            codebox_02_pan = 0;
            codebox_tilde_01_old_sub_phase = 0;
            codebox_tilde_01_newphas = 0;
            codebox_tilde_01_oldphas = 0;
            codebox_tilde_01_rdmdel = 0;
            codebox_tilde_01_n_subd_div = 1;
            codebox_tilde_01_n_subd_nextdiv = 1;
            codebox_tilde_01_n_subd_posstep = 0;
            codebox_tilde_01_n_subd_nextposstep = 0;
            codebox_tilde_01_n_subd_outstep = 0;
            codebox_tilde_01_n_subd_nextoutstep = 0;
            codebox_tilde_01_n_subd_prev = 0;
            codebox_tilde_01_n_subd_prev2 = 0;
            codebox_tilde_01_n_subd_playing = true;
            codebox_tilde_01_n_subd_lockprob = true;
            codebox_tilde_01_mphasor_currentPhase = 0;
            codebox_tilde_01_mphasor_conv = 0;
            codebox_tilde_01_del__maxdelay = 0;
            codebox_tilde_01_del_sizemode = 0;
            codebox_tilde_01_del_wrap = 0;
            codebox_tilde_01_del_reader = 0;
            codebox_tilde_01_del_writer = 0;
            codebox_tilde_01_setupDone = false;
            param_01_lastValue = 0;
            dial_01_lastValue = 0;
            param_02_lastValue = 0;
            dial_02_lastValue = 0;
            param_03_lastValue = 0;
            dial_03_lastValue = 0;
            param_04_lastValue = 0;
            dial_04_lastValue = 0;
            param_05_lastValue = 0;
            dial_05_lastValue = 0;
            data_01_sizemode = 1;
            data_01_setupDone = false;
            phasor_01_sigbuf = nullptr;
            phasor_01_lastLockedPhase = 0;
            phasor_01_conv = 0;
            phasor_01_setupDone = false;
            param_06_lastValue = 0;
            dial_06_lastValue = 0;
            param_07_lastValue = 0;
            dial_07_lastValue = 0;
            phasor_02_sigbuf = nullptr;
            phasor_02_lastLockedPhase = 0;
            phasor_02_conv = 0;
            phasor_02_setupDone = false;
            param_08_lastValue = 0;
            dial_08_lastValue = 0;
            param_09_lastValue = 0;
            dial_09_lastValue = 0;
            phasor_03_sigbuf = nullptr;
            phasor_03_lastLockedPhase = 0;
            phasor_03_conv = 0;
            phasor_03_setupDone = false;
            param_10_lastValue = 0;
            dial_10_lastValue = 0;
            param_11_lastValue = 0;
            dial_11_lastValue = 0;
            param_12_lastValue = 0;
            dial_12_lastValue = 0;
            param_13_lastValue = 0;
            dial_13_lastValue = 0;
            param_14_lastValue = 0;
            dial_14_lastValue = 0;
            latch_tilde_01_value = 0;
            latch_tilde_01_setupDone = false;
            codebox_tilde_02_len = 0;
            codebox_tilde_02_stoppin_time = 0;
            codebox_tilde_02_gonow = 0;
            codebox_tilde_02_record = 0;
            codebox_tilde_02_timer_carry = 0;
            codebox_tilde_02_timer_count = 0;
            codebox_tilde_02_redge1_active = false;
            codebox_tilde_02_redge0_active = false;
            codebox_tilde_02_trig_active = false;
            recordtilde_01_wIndex = 0;
            recordtilde_01_lastRecord = 0;
            param_15_lastValue = 0;
            recordtilde_02_wIndex = 0;
            recordtilde_02_lastRecord = 0;
            dial_15_lastValue = 0;
            param_16_lastValue = 0;
            dial_16_lastValue = 0;
            toggle_01_lastValue = 0;
            param_17_lastValue = 0;
            param_18_lastValue = 0;
            toggle_02_lastValue = 0;
            toggle_03_lastValue = 0;
            param_19_lastValue = 0;
            dial_17_lastValue = 0;
            param_20_lastValue = 0;
            dial_18_lastValue = 0;
            param_21_lastValue = 0;
            dcblock_tilde_01_xm1 = 0;
            dcblock_tilde_01_ym1 = 0;
            dcblock_tilde_01_setupDone = false;
            feedbacktilde_01_feedbackbuffer = nullptr;
            codebox_tilde_03_frz = 0;
            codebox_tilde_03_len = 0;
            codebox_tilde_03_offs = 0;
            codebox_tilde_03_counterstate = { 0, 0, 0 };
            codebox_tilde_03_c = 0;
            codebox_tilde_03_reached = 0;
            codebox_tilde_03_offs_rev_carry = 0;
            codebox_tilde_03_offs_rev_count = 0;
            codebox_tilde_03_done_active = false;
            codebox_tilde_03_startrec_active = false;
            data_02_sizemode = 3;
            data_02_setupDone = false;
            data_03_sizemode = 2;
            data_03_setupDone = false;
            ctlin_01_status = 0;
            ctlin_01_byte1 = -1;
            ctlin_01_inchan = 0;
            globaltransport_tempo = nullptr;
            globaltransport_tempoNeedsReset = false;
            globaltransport_lastTempo = 120;
            globaltransport_state = nullptr;
            globaltransport_stateNeedsReset = false;
            globaltransport_lastState = 0;
            globaltransport_beatTimeChanges = { 0, 0 };
            globaltransport_timeSignatureChanges = { 4, 4, 0 };
            globaltransport_notify = false;
            globaltransport_setupDone = false;
            stackprotect_count = 0;
            _voiceIndex = 0;
            _noteNumber = 0;
            isMuted = 1;
        }

        // member variables

        number limi_01_bypass;
        number limi_01_dcblock;
        number limi_01_lookahead;
        number limi_01_preamp;
        number limi_01_postamp;
        number limi_01_threshold;
        number limi_01_release;
        number dspexpr_01_in1;
        number dspexpr_01_in2;
        list codebox_01_out1;
        //number codebox_01_len;
        //number codebox_01_den;
        //number codebox_01_cpo;
        //number codebox_01_drf;
        //number codebox_01_rvo;
        //number codebox_01_pwi;
        //number codebox_01_frp;
        //number codebox_01_psh;
        //number codebox_01_rpt;
        //number codebox_01_env;
        //number codebox_01_frz;
        //number codebox_01_rle;
        Index p_01_target;
        list codebox_02_in1;
        //number codebox_02_in2;
        list codebox_02_out1;
        list codebox_02_out2;
        //number codebox_02_out3;
        number codebox_tilde_01_in1;
        number codebox_tilde_01_in2;
        number codebox_tilde_01_in3;
        number codebox_tilde_01_in4;
        number codebox_tilde_01_in5;
        number codebox_tilde_01_in6;
        number codebox_tilde_01_in7;
        number codebox_tilde_01_in8;
        number codebox_tilde_01_in9;
        number codebox_tilde_01_in10;
        number codebox_tilde_01_in11;
        number param_01_value;
        number dial_01_value;
        number param_02_value;
        number dial_02_value;
        number dspexpr_02_in1;
        number dspexpr_02_in2;
        number param_03_value;
        number dial_03_value;
        number param_04_value;
        number dial_04_value;
        number param_05_value;
        number dial_05_value;
        number data_01_sizeout;
        number data_01_size;
        number data_01_sizems;
        number data_01_normalize;
        number data_01_channels;
        number phasor_01_freq;
        number param_06_value;
        number dial_06_value;
        number param_07_value;
        number dial_07_value;
        number phasor_02_freq;
        number param_08_value;
        number dial_08_value;
        number param_09_value;
        number dial_09_value;
        number phasor_03_freq;
        number param_10_value;
        number dial_10_value;
        number param_11_value;
        number dial_11_value;
        number param_12_value;
        number dial_12_value;
        number param_13_value;
        number dial_13_value;
        number param_14_value;
        number dial_14_value;
        number latch_tilde_01_x;
        number latch_tilde_01_control;
        number codebox_tilde_02_in1;
        number codebox_tilde_02_in2;
        number recordtilde_01_record;
        number recordtilde_01_begin;
        number recordtilde_01_end;
        number recordtilde_01_loop;
        number param_15_value;
        number recordtilde_02_record;
        number recordtilde_02_begin;
        number recordtilde_02_end;
        number recordtilde_02_loop;
        number dial_15_value;
        number param_16_value;
        number dial_16_value;
        number toggle_01_value_number;
        number param_17_value;
        number param_18_value;
        number toggle_02_value_number;
        number toggle_03_value_number;
        number param_19_value;
        number expr_02_in1;
        number expr_02_out1;
        number select_01_test1;
        list bufferop_01_channels;
        number dial_17_value;
        number param_20_value;
        number dspexpr_03_in1;
        number dspexpr_03_in2;
        number dspexpr_04_in1;
        number dspexpr_04_in2;
        number dspexpr_05_in1;
        number dspexpr_05_in2;
        number dspexpr_06_in1;
        number dspexpr_06_in2;
        number dial_18_value;
        number param_21_value;
        number dcblock_tilde_01_x;
        number dcblock_tilde_01_gain;
        number dspexpr_07_in1;
        number dspexpr_07_in2;
        number codebox_tilde_03_in1;
        number codebox_tilde_03_in2;
        number data_02_sizeout;
        number data_02_size;
        number data_02_sizems;
        number data_02_normalize;
        number data_02_channels;
        number data_03_sizeout;
        number data_03_size;
        number data_03_sizems;
        number data_03_normalize;
        number data_03_channels;
        number ctlin_01_input;
        number ctlin_01_controller;
        number ctlin_01_channel;
        number expr_01_in1;
        number expr_01_in2;
        number expr_01_out1;
        MillisecondTime _currentTime;
        SampleIndex audioProcessSampleCount;
        //SampleIndex sampleOffsetIntoNextAudioBuffer;
        signal zeroBuffer;
        signal dummyBuffer;
        SampleValue* signals[6];
        bool didAllocateSignals;
        Index vs;
        Index maxvs;
        number sr;
        number invsr;
        number edge_02_currentState;
        bool edge_02_setupDone;
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
        //number codebox_01_intelligent_offset;
        number codebox_01_newphas;
        number codebox_01_oldphas;
        //number len_in_samps;
        //number cpo_in_samps;
        //number drf_in_samps;
        number codebox_01_mphasor_currentPhase;
        number codebox_01_mphasor_conv;
        //UInt codebox_01_rdm_state[4] = { };
        //number codebox_02_voiceState[24][2] = { };
        bool voice_state[24] = { };
        number codebox_02_pos;
        number codebox_02_len;
        number codebox_02_f_r;
        number codebox_02_ptc;
        number codebox_02_vol;
        number codebox_02_pan;
        number codebox_tilde_01_old_sub_phase;
        number codebox_tilde_01_newphas;
        number codebox_tilde_01_oldphas;
        number codebox_tilde_01_rdmdel;
        number codebox_tilde_01_n_subd_div;
        number codebox_tilde_01_n_subd_nextdiv;
        number codebox_tilde_01_n_subd_posstep;
        number codebox_tilde_01_n_subd_nextposstep;
        number codebox_tilde_01_n_subd_outstep;
        number codebox_tilde_01_n_subd_nextoutstep;
        number codebox_tilde_01_n_subd_lastoutstep;
        number codebox_tilde_01_n_subd_prev;
        number codebox_tilde_01_n_subd_prev2;
        list codebox_tilde_01_n_subd_p2;
        list codebox_tilde_01_n_subd_synco;
        list codebox_tilde_01_n_subd_p2prob;
        list codebox_tilde_01_n_subd_pattern;
        list codebox_tilde_01_n_subd_prob;
        number codebox_tilde_01_n_subd_p2length;
        bool codebox_tilde_01_n_subd_playing;
        number codebox_tilde_01_n_subd_num;
        number codebox_tilde_01_n_subd_patternphase;
        number codebox_tilde_01_n_subd_patternstep;
        number codebox_tilde_01_n_subd_probstep;
        bool codebox_tilde_01_n_subd_lockprob;
        number codebox_tilde_01_mphasor_currentPhase;
        number codebox_tilde_01_mphasor_conv;
        Float64BufferRef codebox_tilde_01_del_buffer;
        Index codebox_tilde_01_del__maxdelay;
        Int codebox_tilde_01_del_sizemode;
        Index codebox_tilde_01_del_wrap;
        Int codebox_tilde_01_del_reader;
        Int codebox_tilde_01_del_writer;
        UInt codebox_tilde_01_rdm_state[4] = { };
        bool codebox_tilde_01_setupDone;
        number param_01_lastValue;
        number dial_01_lastValue;
        number param_02_lastValue;
        number dial_02_lastValue;
        number param_03_lastValue;
        number dial_03_lastValue;
        number param_04_lastValue;
        number dial_04_lastValue;
        number param_05_lastValue;
        number dial_05_lastValue;
        Float32BufferRef data_01_buffer;
        Int data_01_sizemode;
        bool data_01_setupDone;
        signal phasor_01_sigbuf;
        number phasor_01_lastLockedPhase;
        number phasor_01_conv;
        bool phasor_01_setupDone;
        number param_06_lastValue;
        number dial_06_lastValue;
        number param_07_lastValue;
        number dial_07_lastValue;
        signal phasor_02_sigbuf;
        number phasor_02_lastLockedPhase;
        number phasor_02_conv;
        bool phasor_02_setupDone;
        number param_08_lastValue;
        number dial_08_lastValue;
        number param_09_lastValue;
        number dial_09_lastValue;
        signal phasor_03_sigbuf;
        number phasor_03_lastLockedPhase;
        number phasor_03_conv;
        bool phasor_03_setupDone;
        number param_10_lastValue;
        number dial_10_lastValue;
        number param_11_lastValue;
        number dial_11_lastValue;
        number param_12_lastValue;
        number dial_12_lastValue;
        number param_13_lastValue;
        number dial_13_lastValue;
        number param_14_lastValue;
        number dial_14_lastValue;
        number latch_tilde_01_value;
        bool latch_tilde_01_setupDone;
        number codebox_tilde_02_len;
        number codebox_tilde_02_stoppin_time;
        number codebox_tilde_02_gonow;
        number codebox_tilde_02_record;
        int codebox_tilde_02_timer_carry;
        number codebox_tilde_02_timer_count;
        bool codebox_tilde_02_redge1_active;
        bool codebox_tilde_02_redge0_active;
        bool codebox_tilde_02_trig_active;
        Float32BufferRef recordtilde_01_buffer;
        SampleIndex recordtilde_01_wIndex;
        number recordtilde_01_lastRecord;
        number param_15_lastValue;
        Float32BufferRef recordtilde_02_buffer;
        SampleIndex recordtilde_02_wIndex;
        number recordtilde_02_lastRecord;
        number dial_15_lastValue;
        number param_16_lastValue;
        number dial_16_lastValue;
        number toggle_01_lastValue;
        number param_17_lastValue;
        number param_18_lastValue;
        number toggle_02_lastValue;
        number toggle_03_lastValue;
        number param_19_lastValue;
        Float32BufferRef bufferop_01_buffer;
        number dial_17_lastValue;
        number param_20_lastValue;
        number dial_18_lastValue;
        number param_21_lastValue;
        number dcblock_tilde_01_xm1;
        number dcblock_tilde_01_ym1;
        bool dcblock_tilde_01_setupDone;
        signal feedbacktilde_01_feedbackbuffer;
        number codebox_tilde_03_frz;
        number codebox_tilde_03_len;
        number codebox_tilde_03_offs;
        list codebox_tilde_03_counterstate;
        number codebox_tilde_03_c;
        number codebox_tilde_03_reached;
        int codebox_tilde_03_offs_rev_carry;
        number codebox_tilde_03_offs_rev_count;
        bool codebox_tilde_03_done_active;
        bool codebox_tilde_03_startrec_active;
        Float32BufferRef data_02_buffer;
        Int data_02_sizemode;
        bool data_02_setupDone;
        Float32BufferRef data_03_buffer;
        Int data_03_sizemode;
        bool data_03_setupDone;
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
        DataRef codebox_tilde_01_del_bufferobj;
        DataRef inter_databuf_01;
        Index _voiceIndex;
        Int _noteNumber;
        Index isMuted;
        indexlist paramInitIndices;
        indexlist paramInitOrder;
        RTGrainVoice* rtgrainvoice[24];

    };

    PatcherInterface* creaternbomatic()
    {
        return new rnbomatic();
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

