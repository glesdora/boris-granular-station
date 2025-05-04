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

        struct voiceState {
            bool isActive;
            bool hasGrainInQueue;
            SampleIndex endOfGrain;
        };

        class RTGrainVoice {

        public:

            RTGrainVoice(rnbomatic* grainsMaster, int voiceIndex) : _grainsMaster(grainsMaster), _voiceIndex(voiceIndex) {}
            ~RTGrainVoice()
            {
                for (int i = 0; i < 2; i++) {
                    free(realtime_grain_out[i]);
                }
                free(zeroBuffer);
                free(dummyBuffer);
            }

            rnbomatic* getPatcher() const {
                return _grainsMaster;
            }

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

            Index voice() {
                return this->_voiceIndex;
            }

            void process(
                const SampleValue* const* inputs,
                Index numInputs,
                SampleValue* const* outputs,
                Index numOutputs,
                Index n
            ) {
                this->vs = n;
                SampleValue* out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
                SampleValue* out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);
                const SampleValue* revoffs = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
                const SampleValue* recpointer = (numInputs >= 2 && inputs[1] ? inputs[1] : this->zeroBuffer);

                auto trgindx = this->triggerindex;
                if (trgindx >= 0) {
                    if (trgindx >= this->vs) {
                        this->triggerindex -= this->vs;
                        return;
                    }

                    this->endOfGrain = triggerindex + grainsize_value / pishift_value;      // ti + gsamps
                    this->setIsMuted(0);
					this->hasGrainInQueue = false;                                          // grain is still in queue, but will start on this block
                    this->triggerindex = -1;
                }

                if (this->getIsMuted())
                    return;

                if (trgindx >= 0) {
                    reverse_offset = revoffs[trgindx];
                }

                this->processBrain(
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

                this->signalaccum(this->realtime_grain_out[0], out1, n);
                this->signalaccum(this->realtime_grain_out[1], out2, n);
            }

            void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
                if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
                    Index i;

                    for (i = 0; i < 2; i++) {
                        this->realtime_grain_out[i] = resizeSignal(this->realtime_grain_out[i], this->maxvs, maxBlockSize);
                    }

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
            }

            Index getIsMuted() {
                return this->isMuted;
            }

            void setIsMuted(Index v) {
                this->isMuted = v;
            }

            void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
                if (index == 0) {
                    this->rtaudiobuf = new Float32Buffer(this->getPatcher()->borisinrnbo_v01_rtbuf);
                }

                if (index == 1) {
                    this->intrpenvbuf = new Float32Buffer(this->getPatcher()->interpolated_envelope);
                }
            }

            void initialize() {
                for (int i = 0; i < 2; i++) {
                    realtime_grain_out[i] = nullptr;
                }
                this->rtaudiobuf = new Float32Buffer(this->getPatcher()->borisinrnbo_v01_rtbuf);
                this->intrpenvbuf = new Float32Buffer(this->getPatcher()->interpolated_envelope);
            }

            void allocateDataRefs() {                                                                               //mmmmmm
                this->rtaudiobuf = this->rtaudiobuf->allocateIfNeeded();
                this->intrpenvbuf = this->intrpenvbuf->allocateIfNeeded();
            }

            void initiateVoice(SampleIndex trigatindex, const list& v) {
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

        protected:
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
            ) {
                auto& rtbuf = this->rtaudiobuf;
                auto rtbuf_dim = rtbuf->getSize();

                auto& envbuf = this->intrpenvbuf;
                auto envbuf_dim = envbuf->getSize();

                number __trigger_index = in1;

                bool playsound = true;

                if (__trigger_index >= 0) {                     //if the grain will start in this block
                    this->envelope_counter_count = -1;
                    playsound = false;
                }

                Index i;

                for (i = 0; i < n; i++) {
                    bool triggernow = (__trigger_index == i);

                    number __pan = (in7 > 1 ? 1 : in7 < 0 ? 0 : in7);
                    number __vol = (in6 > 1 ? 1 : (in6 < 0 ? 0 : in6));
                    number __trgt_samps = std::max(in3, 0.);
                    number __pitch = (in5 > 4 ? 4 : (in5 < 0.25 ? 0.25 : in5));
                    number g_samps = __trgt_samps / __pitch;

                    bool env_counter_hit = 0;
                    SampleIndex env_counter_count = 0;

                    //envelope counter
                    {
                        bool carry_flag = 0;

                        if (triggernow) {
                            this->envelope_counter_count = 0;
                            playsound = true;
                        }
                        else if (!playsound) {
                            out1[(Index)i] = 0;
                            out2[(Index)i] = 0;
                            continue;
                        }
                        else {
                            this->envelope_counter_count += this->env_inc_history;

                            if ((this->env_inc_history > 0 && this->envelope_counter_count >= g_samps)) {
                                this->envelope_counter_count = 0;
                                carry_flag = 1;
                            }
                        }

                        env_counter_hit = carry_flag;
                        env_counter_count = this->envelope_counter_count;
                    }

                    bool env_counter_incr = ((env_counter_hit) ? 0 : 1);
					this->env_inc_history = (triggernow + env_counter_hit) ? env_counter_incr : this->env_inc_history;  // if just triggered, or counter hit the max, update the value
                    number pos_in_grain = (g_samps == 0. ? 0. : env_counter_count / g_samps);           // why is this necessary?

                    if (env_counter_hit) {
                        this->endOfGrain = 0;
                        this->setIsMuted(true);
                    }

					//envelope value
                    number envelope_value = 0;
                    {
                        auto& __buffer = intrpenvbuf;

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
					this->rel_start_pos_history = (triggernow) ? rel_pos_in_buffer : this->rel_start_pos_history;  //position in buffer (0-1) at the momoment of trigger
                    number rel_end_of_grain = this->rel_start_pos_history + relative_gsize;

                    number playstart = (isforw ? this->rel_start_pos_history : rel_end_of_grain);
                    number playend = (isforw ? rel_end_of_grain : this->rel_start_pos_history);

                    SampleIndex play_counter_count = 0;
                    bool play_counter_hit = 0;

                    //audio buf counter
                    {
                        bool carry_flag = 0;

                        if (triggernow) {
                            this->audio_counter_count = 0;
                        }
                        else {
                            this->audio_counter_count += play_inc_history;

                            if ((play_inc_history > 0 && this->audio_counter_count >= g_samps)) {
                                this->audio_counter_count = 0;
                                carry_flag = 1;
                            }
                        }

                        play_counter_hit = carry_flag;
                        play_counter_count = this->audio_counter_count;
                    }

                    bool play_counter_incr = ((bool)(play_counter_hit) ? 0 : 1);
					this->play_inc_history = (triggernow + play_counter_hit) ? play_counter_incr : this->play_inc_history;
                    number progress_in_grain = (g_samps == 0. ? 0. : play_counter_count / g_samps);
					this->start_play_pos_history = (triggernow) ? playstart : this->start_play_pos_history;
                    number grain_relsize_wsign = playend - playstart;
                    number grain_relprogress = progress_in_grain * grain_relsize_wsign;
                    number playpos = grain_relprogress + this->start_play_pos_history;
                    number sub_67_62 = playpos - relative_reverse_offset;
                    auto offsettedpos = this->wrap(playpos - relative_reverse_offset, 0, 1);

					//audio samp value
                    number sample_rtbuf = 0;
                    {
                        auto& __buffer = rtaudiobuf;

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

                    out2[(Index)i] = outright;
                    out1[(Index)i] = outleft;
                }

                if (this->endOfGrain > 0)
                    this->endOfGrain -= this->vs;
				this->getPatcher()->updateVoiceState(this->voice(), { !getIsMuted(), this->hasGrainInQueue, this->endOfGrain});
            }

            void signalaccum(
                const SampleValue* in1,
                SampleValue* out,
                Index n
            ) {
                Index i;

                for (i = 0; i < n; i++) {
                    out[(Index)i] = in1[(Index)i] + out[(Index)i];
                }
            }

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

            rnbomatic* _grainsMaster;

        };

        rnbomatic()
        {
        }

        ~rnbomatic()
        {
            for (int i = 0; i < 6; i++) {
                free(this->signals[i]);
            }

            free(this->phasor_01_sigbuf);
            free(this->phasor_02_sigbuf);
            free(this->phasor_03_sigbuf);
            free(this->feedbackbuffer);
            free(this->globaltransport_tempo);
            free(this->globaltransport_state);
            free(this->zeroBuffer);
            free(this->dummyBuffer);

            for (int i = 0; i < 24; i++) {
                delete rtgrainvoice[i];
            }
        }

        rnbomatic* getTopLevelPatcher() {
            return this;
        }

        void cancelClockEvents() {}

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

        number mstosamps(MillisecondTime ms) {
            return ms * this->sr * 0.001;
        }

        number maximum(number x, number y) {
            return (x < y ? y : x);
        }

        inline number safediv(number num, number denom) {
            return (denom == 0.0 ? 0.0 : num / denom);
        }

        MillisecondTime currenttime() {
            return this->_currentTime;
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

        Index vectorsize() {
            return this->vs;
        }

        number tempo() {
            return this->getTopLevelPatcher()->globaltransport_getTempo(this->currenttime());
        }

        number beatstohz(number beattime) {
            return this->safediv(this->tempo(), beattime * 60);
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
			auto status = data[0] & 240;
			auto channel = (data[0] & 15) + 1;

            if (status == 0xB0 && (channel == this->ctlin_01_channel || this->ctlin_01_channel == -1) && (data[1] == this->ctlin_01_controller || this->ctlin_01_controller == -1)) {
                this->setParameterValue(16, this->fromnormalized(16, data[2] * 0.007874015748), time);
                this->ctlin_01_status = 0;
            }
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

			auto mut = paramvalues[16];
            auto len = paramvalues[3];
			auto den = paramvalues[0];
			auto cha = normalizedparamvalues[1];
			auto rdl = normalizedparamvalues[2];
			auto syc = paramvalues[18];
			auto tmp = paramvalues[19];
			auto rtm = paramvalues[20];

            auto frz = paramvalues[17];
            auto scroll = (frz == 0);

            auto gai = paramvalues[13];
			auto fdb = paramvalues[14];

            auto wet = paramvalues[15];

            this->generate_triggers(
                mut,
                len,
				den,
				cha,
				rdl,
				syc,
				tmp,
				rtm,
                this->signals[0],
                this->signals[1],
                this->signals[2],
                n
            );

            this->delayRecStop(
                len,
                scroll,
                this->signals[3],
                n
            );

            //mixdown chans + gain
            for (Index i = 0; i < n; i++) {
                this->signals[2][i] = ((in1[i] + in2[i]) * 0.71) * gai;
            }

            this->revoffsethandler(
                frz,
                len,
                this->signals[1],
                n
            );

            //feedback
            for (Index i = 0; i < n; i++) {
                this->signals[0][i] = this->feedbackbuffer[i] * fdb + this->signals[2][i];
            }

            this->recordtilde_01_perform(
                this->signals[3],
                this->recordtilde_01_begin,
                this->recordtilde_01_end,
                this->signals[0],
                this->signals[4],
                n
            );

            auto rp = this->recpointer_at_scroll;
            for (Index i = 0; i < n; i++) {
                if (scroll)
                    rp = signals[4][i];

                this->signals[2][i] = rp;
            }
            this->recpointer_at_scroll = rp;

            {
                ConstSampleArray<2> ins = { signals[1], signals[2] };
                SampleArray<2> outs = { signals[4], signals[5] };

                for (number chan = 0; chan < 2; chan++)
                    zeroSignal(outs[(Index)chan], n);

                for (Index i = 0; i < 24; i++)
                    this->rtgrainvoice[i]->process(ins, 2, outs, 2, n);
            }

            this->dcblock_tilde_01_perform(this->signals[4], this->dcblock_tilde_01_gain, this->signals[2], n);

            for (Index i = 0; i < n; i++) {
                this->feedbackbuffer[i] = this->signals[2][i];
            }

            this->limi_01_perform(this->signals[4], this->signals[5], this->signals[2], this->signals[1], n);

            //volume
			for (Index i = 0; i < n; i++) {
                out2[i] = this->signals[1][i] * wet;
				out1[i] = this->signals[2][i] * wet;
			}

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
                this->feedbackbuffer = resizeSignal(this->feedbackbuffer, this->maxvs, maxBlockSize);
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

            this->phasor_01_dspsetup(forceDSPSetup);
            this->phasor_02_dspsetup(forceDSPSetup);
            this->phasor_03_dspsetup(forceDSPSetup);
            this->codebox_tilde_01_dspsetup(forceDSPSetup);
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
            return 3;
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
            }

            if (index == 2) {
                this->recordtilde_02_buffer = new Float32Buffer(this->inter_databuf_01);
            }

            for (Index i = 0; i < 24; i++) {
                this->rtgrainvoice[i]->processDataViewUpdate(index, time);
            }
        }

        void initialize() {
            this->borisinrnbo_v01_rtbuf = initDataRef("borisinrnbo_v01_rtbuf", true, nullptr, "data");
            this->interpolated_envelope = initDataRef("interpolated_envelope", false, nullptr, "buffer~");
            this->inter_databuf_01 = initDataRef("inter_databuf_01", false, nullptr, "data");
            this->assign_defaults();
            this->setState();
            this->borisinrnbo_v01_rtbuf->setIndex(0);
            this->recordtilde_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->bufferop_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->interpolated_envelope->setIndex(1);
            this->inter_databuf_01->setIndex(2);
            this->recordtilde_02_buffer = new Float32Buffer(this->inter_databuf_01);
            this->allocateDataRefs();
            this->startup();
        }

        Index getPatcherSerial() const {
            return 0;
        }

        void getState(PatcherStateInterface&) {}

        void setState() {
            for (int i = 0; i < 24; i++) {
                this->rtgrainvoice[i] = new RTGrainVoice(this, i + 1);
                this->rtgrainvoice[i]->initialize();
            }
        }

        void getPreset(PatcherStateInterface& preset) {
            preset["__presetid"] = "rnbo";

            for (Index i = 0; i < getNumParameters(); i++) {
                this->param_getPresetValue((getSubState(preset, getParameterId(i))), i);
            }
        }

        void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
            this->updateTime(time);

            for (Index i = 0; i < getNumParameters(); i++) {
                this->param_setPresetValue(getSubState(preset, getParameterId(i)), i, time);
            }
        }

        void processTempoEvent(MillisecondTime time, Tempo tempo) {
            this->updateTime(time);

            if (this->globaltransport_setTempo(this->_currentTime, tempo, false)) {

                this->timevalue_01_sendValue();
				this->timevalue_02_sendValue();
				this->timevalue_03_sendValue();
            }
        }

        void processTransportEvent(MillisecondTime time, TransportState state) {
            this->updateTime(time);
        }

        void processBeatTimeEvent(MillisecondTime time, BeatTime beattime) {
            this->updateTime(time);
        }

        void onSampleRateChanged(double samplerate) {
        }

        void processTimeSignatureEvent(MillisecondTime time, int numerator, int denominator) {
            this->updateTime(time);
        }

        void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
            this->updateTime(time);

            ParameterInfo pinfo;
			this->getParameterInfo(index, &pinfo);

            v = (v < pinfo.min ? pinfo.min : (v > pinfo.max ? pinfo.max : v));

			paramvalues[index] = v;
			normalizedparamvalues[index] = this->tonormalized(index, v);

            this->sendParameter(index, false);

            if (v != paramlastvalues[index]) {
                this->getEngine()->presetTouched();
                this->paramlastvalues[index] = v;
            }

            if (index == 17 && v == 0)
                this->empty_audio_buffer();
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
			return paramvalues[index];
        }

        ParameterIndex getNumSignalInParameters() const {
            return 0;
        }

        ParameterIndex getNumSignalOutParameters() const {
            return 0;
        }

        ParameterIndex getNumParameters() const {
            return 21;
        }

        ConstCharPointer getParameterName(ParameterIndex index) const {
			return getParameterId(index);
        }

        ConstCharPointer getParameterId(ParameterIndex index) const {
            ParameterInfo pinfo;
			this->getParameterInfo(index, &pinfo);

			return pinfo.displayName;
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
                    info->displayName = "den";
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
                    info->exponent = 1;
                    info->steps = 101; 
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "cha";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "rdl";
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
                    info->displayName = "len";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "rle";
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
                    info->exponent = 1;
                    info->steps = 0;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "psh";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "rpt";
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
                    info->displayName = "env";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "frp";
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
                    info->displayName = "cpo";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "drf";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "pwi";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "rvo";
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
                    info->displayName = "gai";
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
                    info->exponent = 1;
                    info->steps = 101;
                    info->debug = false;
                    info->saveable = true;
                    info->transmittable = true;
                    info->initialized = true;
                    info->visible = true;
                    info->displayName = "fdb";
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
                    info->displayName = "wet";
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
                    info->displayName = "mut";
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
                    info->displayName = "frz";
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
                    info->displayName = "syc";
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
                    info->displayName = "tmp";
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
                    info->displayName = "rtm";
                    info->unit = "";
                    info->ioType = IOTypeUndefined;
                    info->signalIndex = INVALID_INDEX;
                    break;
                }
                default:
                {
                    break;
                }
                }
            }
        }

        ParameterValue applyJuceSkew(ParameterValue normalizedValue, ParameterValue exp) const
        {
            return pow(2.718281828459045, log(normalizedValue) / exp);
        }

        ParameterValue applyJuceDeskew(ParameterValue normalizedValue, ParameterValue exp) const
        {
            return std::pow(normalizedValue, exp);
        }

        ParameterValue applyPitchDenormalization(ParameterValue normalizedValue, ParameterValue min, ParameterValue max) const
        {
            auto semitoneRatio = std::pow(2.0, 1.0 / 12.0);
            auto n = std::round(std::log(normalizedValue) / std::log(semitoneRatio));

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

        ParameterIndex getParameterOffset(BaseInterface* subpatcher) const { return 0; }

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
			ParameterInfo pinfo;
			this->getParameterInfo(index, &pinfo);

            if (index == 5)
				value = this->applyPitchNormalization(value, pinfo.min, pinfo.max);
            else if (index < this->getNumParameters()) {
                value = (value - pinfo.min) / (pinfo.max - pinfo.min);

                if (pinfo.steps) {
                    value = this->applyStepsToNormalizedParameterValue(value, pinfo.steps);
                }
                if (pinfo.exponent != 1) {
                    value = this->applyJuceDeskew(value, pinfo.exponent);
                }
			}

			return value;
        }

        ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
            ParameterInfo pinfo;
            this->getParameterInfo(index, &pinfo);

            value = (value < 0 ? 0 : (value > 1 ? 1 : value));

            if (index == 5)
                value = this->applyPitchDenormalization(value, pinfo.min, pinfo.max);
            else if (index < this->getNumParameters()) {
                if (pinfo.steps)
                    value = this->applyStepsToNormalizedParameterValue(value, pinfo.steps);
                if (pinfo.exponent != 1) {
                    value = this->applyJuceSkew(value, pinfo.exponent);
                }

                value = value * (pinfo.max - pinfo.min) + pinfo.min;

            }

            return value;
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

        void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {}

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

        void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {}

        void processListMessage(
            MessageTag tag,
            MessageTag objectId,
            MillisecondTime time,
            const list& payload
        ) {}

        void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {}

        MessageTagInfo resolveTag(MessageTag tag) const {
            switch (tag) {
            case TAG("valout"):
            {
                return "valout";
            }
            case TAG("valin"):
            {
                return "valin";
            }
            }

            return "";
        }

        MessageIndex getNumMessages() const { return 0; }

        const MessageInfo& getMessageInfo(MessageIndex index) const { return NullMessageInfo; }

    protected:

        void updateVoiceState(int voice, voiceState voicestate) {
			auto voiceindex = voice - 1;

			voiceStates[voiceindex] = voicestate;
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

			this->recordtilde_01_buffer->requestSize(this->mstosamps(20000), 1);
			this->recordtilde_01_buffer->setSampleRate(this->sr);
            this->recordtilde_01_buffer = this->recordtilde_01_buffer->allocateIfNeeded();
            this->bufferop_01_buffer = this->bufferop_01_buffer->allocateIfNeeded();
			this->recordtilde_02_buffer->requestSize(this->mstosamps(20000), 1);
			this->recordtilde_02_buffer->setSampleRate(this->sr);
			this->recordtilde_02_buffer = this->recordtilde_02_buffer->allocateIfNeeded();

            if (this->borisinrnbo_v01_rtbuf->hasRequestedSize()) {
                if (this->borisinrnbo_v01_rtbuf->wantsFill())
                    this->zeroDataRef(this->borisinrnbo_v01_rtbuf);

                this->getEngine()->sendDataRefUpdated(0);
            }


            if (this->interpolated_envelope->hasRequestedSize()) {
                if (this->interpolated_envelope->wantsFill())
                    this->zeroDataRef(this->interpolated_envelope);

                this->getEngine()->sendDataRefUpdated(1);
            }

            this->recordtilde_02_buffer = this->recordtilde_02_buffer->allocateIfNeeded();

            if (this->inter_databuf_01->hasRequestedSize()) {
                if (this->inter_databuf_01->wantsFill())
                    this->zeroDataRef(this->inter_databuf_01);

                this->getEngine()->sendDataRefUpdated(2);
            }
        }

        void sendOutlet(OutletIndex index, ParameterValue value) {
            this->getEngine()->sendOutlet(this, index, value);
        }

        void startup() {
            this->updateTime(this->getEngine()->getCurrentTime());
            for (Index i = 0; i < 24; i++) {
                voiceStates[i].isActive = false;
				voiceStates[i].hasGrainInQueue = false;
                voiceStates[i].endOfGrain = 0;
            }

            this->timevalue_01_sendValue();
            this->timevalue_02_sendValue();
            this->timevalue_03_sendValue();

            this->scheduleParamInit(0, 3);
            this->scheduleParamInit(1, 2);
            this->scheduleParamInit(2, 10);
            this->scheduleParamInit(3, 5);
            this->scheduleParamInit(4, 10);
            this->scheduleParamInit(5, 9);
            this->scheduleParamInit(6, 10);
            this->scheduleParamInit(7, 11);
            this->scheduleParamInit(8, 12);
            this->scheduleParamInit(9, 7);
            this->scheduleParamInit(10, 8);
            this->scheduleParamInit(11, 14);
            this->scheduleParamInit(12, 13);
            this->scheduleParamInit(13, 0);
            this->scheduleParamInit(14, 17);
            this->scheduleParamInit(15, 19);
            this->scheduleParamInit(16, 1);
            this->scheduleParamInit(17, 16);
            this->scheduleParamInit(18, 0);
            this->scheduleParamInit(19, 0);
            this->scheduleParamInit(20, 0);

            this->processParamInitEvents();
        }

        void empty_audio_buffer() {
            auto& buffer = this->bufferop_01_buffer;
            buffer->setZero();
            buffer->setTouched(true);
        }

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

			auto grainprops = list{
				this->setGrainPosition(cpo_in_samps, drf_in_samps, len_in_samps, psh, rpt, intelligent_offset),
				len_in_samps,
				this->setGrainDirection(frp),
				this->setGrainPshift(psh, rpt),
				this->setGrainVol(rvo),
				this->setGrainPan(pwi)
			};

            int target = this->findtargetvoice(trigatindex);

            if (target >= 0) {
                this->rtgrainvoice[target - 1]->initiateVoice(trigatindex, grainprops);
            }
        }

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

        void generate_triggers(
            bool mut,
            number len,
            number den,
            number cha,
            number rdl,
            bool sync,
            int notetempo,
            int noterythm,
            const Sample* sync_n_phasor,
            const Sample* sync_nd_phasor,
            const Sample* sync_nt_phasor,
            Index n
        ) {
            auto syncphase_old = this->gentrggs_syncphase_old;
            auto freerunphase_old = this->gentrggs_freerunphase_old;

            number maxdelay = 0;
            number frq = 0;

            for (Index i = 0; i < n; i++) {
                number sync_n_phase = sync_n_phasor[i];
                number sync_nd_phase = sync_nd_phasor[i];
                number sync_nt_phase = sync_nt_phasor[i];

                if (sync == 0) {
                    frq = (-0.60651 + 41.4268 * rnbo_exp(-0.001 * len)) * den * (1 - mut);
                    auto freerunphase = this->codebox_tilde_01_mphasor_next(frq, -1);

                    if (frq < ((2 * this->samplerate() == 0. ? 0. : (number)1 / (2 * this->samplerate())))) {
                        maxdelay = rdl * 2 * this->samplerate();
                    }
                    else {
                        maxdelay = ((frq == 0. ? 0. : rdl / frq)) * this->samplerate();
                    }

                    if (freerunphase < freerunphase_old) {
                        number r = rand01();
                        if (r <= cha) {
                            r = rand01();
							auto delayedtrig = i + r * maxdelay;        //maxdelay is in samples
                            this->setGrainProperties(delayedtrig);
                        }
                    }
                    freerunphase_old = freerunphase;
                }
                else {
                    number div;
                    div = 1 << (6 - (int)notetempo);
                    frq = this->beatstohz((div == 0. ? 0. : (number)4 * (1 - mut) / div));                  // -mod: if mute frq is 0

                    if (frq < ((2 * this->samplerate() == 0. ? 0. : (number)1 / (2 * this->samplerate())))) {
                        maxdelay = rdl * 2 * this->samplerate();
                    }
                    else {
                        maxdelay = ((frq == 0. ? 0. : rdl / frq)) * this->samplerate();
                    }

                    number new_sub_phase = 0;

                    switch ((int)noterythm) {
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
                        if (new_sub_phase < syncphase_old) {
                            number r = rand01();
                            if (r <= cha) {
                                r = rand01();
                                auto delayedtrig = i + r * maxdelay;
                                this->setGrainProperties(delayedtrig);
                            }
                        }
                    }

                    syncphase_old = new_sub_phase;

                }
            }

            this->gentrggs_freerunphase_old = freerunphase_old;
            this->gentrggs_syncphase_old = syncphase_old;
        }

        void delayRecStop(number glength, bool scroll, SampleValue* out1, Index n) {
            auto delaysamps = this->delrecstop_delaysamps;
            auto record = this->delrecstop_record;
                    
            if (scroll != this->delrecstop_scrollhistory) {
                if (scroll) {
                    this->delrecstop_inc = 0;
                    record = true;
                }
                else {
                    this->delrecstop_count = 0;
                    delrecstop_inc = 1;
                    delaysamps = static_cast<SampleIndex>(this->mstosamps(glength)) + 1;
                }
            }

            for (Index i = 0; i < n; i++) {
                this->delrecstop_count += this->delrecstop_inc;

                if (this->delrecstop_inc && this->delrecstop_count >= delaysamps) {
                    this->delrecstop_inc = 0;
                    record = false;
                }

                out1[i] = record;
            }

			this->delrecstop_scrollhistory = scroll;
            this->delrecstop_record = record;
            this->delrecstop_delaysamps = delaysamps;
        }

        void revoffsethandler(
            bool frz,
            number len,
            SampleValue* out1,
            Index n
        ) {
            auto offset = this->revoffhndlr_offset;
            auto hit = this->revoffhndlr_hit;
            auto c = this->revoffhndlr_c;
			auto glensamps = trunc(this->mstosamps(len)) + 1;               // +1 to be sure that the offset is at least as long as the grain length

            if (frz < this->revoffhndlr_frzhistory) {
                c = 0;
                hit = 0;
            }

            if (!frz) {
                offset = glensamps;
            }

            for (Index i = 0; i < n; i++) {
                if (frz) {
                    if (!hit) {
                        if (c == offset) {
                            DBG("HIT");
                            hit = 1;
                        }

                        if (!hit) {
                            c++;
                        }
                    }
                }

                out1[i] = offset - c;
            }

            this->revoffhndlr_c = c;
            this->revoffhndlr_hit = hit;
            this->revoffhndlr_offset = offset;
            this->revoffhndlr_frzhistory = frz;
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
                                sync[(Index)i] = (__recordtilde_01_wIndex - loopBegin) / (loopEnd - loopBegin);
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
        //LIMITER
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
        //~LIMITER

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

        number setGrainSize(number len, number rle)
        {
            number newlen = (1 - rand01() * rle) * len;
			newlen = this->clip(newlen, 1, len);        // sets 1 ms as the minimum length
            return newlen;
        }

        number clip(number v, number inf, number sup)
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

			//maybe better: r_marg = std::max(1, leninsamps * (psh * (1 + rpt) - 1)); worst case, r_marg = leninsamps * (4 * (1 + 1) - 1) = 7 * leninsamps
			//If length = 2 s, 14 s is a lot, but in theory inside the buffer. Need to test it. I'm talking about grains played at 8x speed. Can I really access the left half of the buffer?
			//With the current limit at 3 * leninsamps, with psh = 4, rpt = 1 and len = 2 s, I should hear half of the grains glitching.

            number pos = rnbo_ceil(this->clip(
                posinsamps + rand01() * drfinsamps,
                std::max(r_marg, leninsamps - intelligent_offset),
                this->samplerate() * 10
            ));

			//scale between 0 and 0.5, to cover the accessible half of the buffer
            pos /= (this->samplerate() * 10. * 2.);

            return pos;
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
            number vol = 1. - r * rvo;
            return vol;
        }

        number setGrainPan(number pwi)
        {
            number pan = 0.5 * (1 + rand01() * pwi);
            return pan;
        }

        /*  · If a voice is muted, but in timer state, it is ignored.
            · If a voice is unmuted, and eog is smaller then newtrig, it's good.
            · If a voice is muted and not in timer state, it's good.
            */

        int findtargetvoice(SampleIndex ti)     //VOICE MANAGER
        {
            for (int i = 0; i < 24; i++) {
                auto candidate_isActive = voiceStates[i].isActive;
				auto candidate_hasGrainInQueue = voiceStates[i].hasGrainInQueue;
				auto candidate_endOfGrain = voiceStates[i].endOfGrain;

                if(candidate_hasGrainInQueue)
                    continue;
                
				// the main class here only updates the hasGrainInQueue flag, while the active state and the eog are handled by the voice
                if (!candidate_isActive || (candidate_isActive && candidate_endOfGrain < ti)) {
                    int target = i + 1;
					voiceStates[i].hasGrainInQueue = true; 
                    return target;
                }
            }

            return -1;
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

        void codebox_tilde_01_dspsetup(bool force) {
            if ((bool)(this->codebox_tilde_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->codebox_tilde_01_setupDone = true;
            this->codebox_tilde_01_mphasor_dspsetup();
        }

		void param_getPresetValue(PatcherStateInterface& preset, Index paramIndex) {
			preset["value"] = this->getParameterValue(paramIndex);
		}

		void param_setPresetValue(PatcherStateInterface& preset, Index paramIndex, MillisecondTime time) {
			if ((bool)(stateIsEmpty(preset)))
				return;

			this->setParameterValue(paramIndex, preset["value"], time);
		}

        void phasor_01_dspsetup(bool force) {
            if ((bool)(this->phasor_01_setupDone) && (bool)(!(bool)(force)))
                return;

            this->phasor_01_conv = (number)1 / this->samplerate();
            this->phasor_01_setupDone = true;
        }

        void phasor_02_dspsetup(bool force) {
            if ((bool)(this->phasor_02_setupDone) && (bool)(!(bool)(force)))
                return;

            this->phasor_02_conv = (number)1 / this->samplerate();
            this->phasor_02_setupDone = true;
        }

        void phasor_03_dspsetup(bool force) {
            if ((bool)(this->phasor_03_setupDone) && (bool)(!(bool)(force)))
                return;

            this->phasor_03_conv = (number)1 / this->samplerate();
            this->phasor_03_setupDone = true;
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

        void timevalue_01_sendValue() {
			this->phasor_01_freq = this->tickstohz(1920);
        }

        void timevalue_02_sendValue() {
            this->phasor_02_freq = this->tickstohz(2880);
        }

        void timevalue_03_sendValue() {
            this->phasor_03_freq = this->tickstohz(1280);
        }

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

            for (int i = 0; i < 21; i++) {
                ParameterInfo pinfo;
                this->getParameterInfo(i, &pinfo);

				paramvalues[i] = pinfo.initialValue;
				normalizedparamvalues[i] = tonormalized(i, paramvalues[i]);
                paramlastvalues[i] = 0;
            }

            recordtilde_01_record = 0;
            recordtilde_01_begin = 0;
            recordtilde_01_end = -1;
            recordtilde_01_loop = 1;
            
            recordtilde_02_record = 0;
            recordtilde_02_begin = 0;
            recordtilde_02_end = -1;
            recordtilde_02_loop = 0;

            dcblock_tilde_01_x = 0;
            dcblock_tilde_01_gain = 0.9997;
            ctlin_01_input = 0;
            ctlin_01_controller = 2;
            ctlin_01_channel = -1;

            _currentTime = 0;
            audioProcessSampleCount = 0;
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
            limi_01_last = 0;
            limi_01_lookaheadIndex = 0;
            limi_01_recover = 0;
            limi_01_lookaheadInv = 0;
            limi_01_dc1_xm1 = 0;
            limi_01_dc1_ym1 = 0;
            limi_01_dc2_xm1 = 0;
            limi_01_dc2_ym1 = 0;
            limi_01_setupDone = false;
            codebox_01_mphasor_currentPhase = 0;
            codebox_01_mphasor_conv = 0;
            gentrggs_syncphase_old = 0;
            gentrggs_freerunphase_old = 0;
            codebox_tilde_01_mphasor_currentPhase = 0;
            codebox_tilde_01_mphasor_conv = 0;
            codebox_tilde_01_setupDone = false;

            phasor_01_freq = 0;
            phasor_02_freq = 0;
            phasor_03_freq = 0;
            
            phasor_01_sigbuf = nullptr;
            phasor_01_lastLockedPhase = 0;
            phasor_01_conv = 0;
            phasor_01_setupDone = false;
            
            phasor_02_sigbuf = nullptr;
            phasor_02_lastLockedPhase = 0;
            phasor_02_conv = 0;
            phasor_02_setupDone = false;
            
            phasor_03_sigbuf = nullptr;
            phasor_03_lastLockedPhase = 0;
            phasor_03_conv = 0;
            phasor_03_setupDone = false;
            
            recpointer_at_scroll = 0;
            delrecstop_delaysamps = 0;
            delrecstop_record = 0;
            delrecstop_scrollhistory = 0;
            recordtilde_01_wIndex = 0;
            recordtilde_01_lastRecord = 0;

            recordtilde_02_wIndex = 0;
            recordtilde_02_lastRecord = 0;
        
            dcblock_tilde_01_xm1 = 0;
            dcblock_tilde_01_ym1 = 0;
            dcblock_tilde_01_setupDone = false;
            feedbackbuffer = nullptr;
            revoffhndlr_offset = 0;
            revoffhndlr_frzhistory = 0;
            revoffhndlr_c = 0;
            revoffhndlr_hit = 0;
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
        }

        // member variables

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