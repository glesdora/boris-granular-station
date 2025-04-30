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
            free(this->feedbacktilde_01_feedbackbuffer);
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

            this->generate_triggers(
                this->mute_for_gentriggs,
                this->len_for_gentrggs,
                this->den_for_gentrggs,
                this->cha_for_gentrggs,
                this->rdl_for_gentrggs,
                this->syc_for_gentrggs,
                this->tmp_for_gentrggs,
                this->rtm_for_gentrggs,
                this->signals[0],
                this->signals[1],
                this->signals[2],
                n
            );

            this->delayRecStop(
                this->glength_for_delayrecstop,
                this->scrollvalue_for_delayrecstop,
                this->signals[3],
                n
            );

            //mixdown chans + gain
            for (Index i = 0; i < n; i++) {
                this->signals[2][i] = ((in1[i] + in2[i]) * 0.71) * this->gai_for_process;
            }

            this->revoffsethandler(
                this->frz_for_revoffhndlr,
                this->glength_for_revoffhndlr,
                this->signals[1],
                n
            );

            this->feedbackreader_01_perform(this->signals[0], n);                                       //fills signal0 with audio buf values
			this->dspexpr_07_perform(this->signals[0], this->dspexpr_07_in2, this->signals[4], n);      //multiplies signal0 with feedback ratio, and reports output to signal4
			this->dspexpr_05_perform(this->signals[2], this->signals[4], this->signals[0], n);          //sums signal2 and signal4, and reports output to signal0

            this->recordtilde_01_perform(
                this->signals[3],
                this->recordtilde_01_begin,
                this->recordtilde_01_end,
                this->signals[0],
                this->signals[4],
                n
            );

            this->latch_tilde_01_perform(this->signals[4], this->latch_tilde_01_control, this->signals[2], n);

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
                this->data_03_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
                this->data_03_bufferUpdated();
            }

            if (index == 1) {
                this->data_01_buffer = new Float32Buffer(this->interpolated_envelope);
                this->data_01_bufferUpdated();
            }


            if (index == 2) {
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
            this->inter_databuf_01 = initDataRef("inter_databuf_01", false, nullptr, "data");
            this->assign_defaults();
            this->setState();
            this->borisinrnbo_v01_rtbuf->setIndex(0);
            this->recordtilde_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->bufferop_01_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->data_03_buffer = new Float32Buffer(this->borisinrnbo_v01_rtbuf);
            this->interpolated_envelope->setIndex(1);
            this->data_01_buffer = new Float32Buffer(this->interpolated_envelope);
            this->inter_databuf_01->setIndex(2);
            this->recordtilde_02_buffer = new Float32Buffer(this->inter_databuf_01);
            this->data_02_buffer = new Float32Buffer(this->inter_databuf_01);
            this->initializeObjects();
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
                //this->rtgrainvoice[i]->setEngineAndPatcher(this->getEngine(), this);
                this->rtgrainvoice[i]->initialize();
                //this->rtgrainvoice[i]->setParameterOffset(this->getParameterOffset(this->rtgrainvoice[0]));
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

            //for (Index i = 0; i < 24; i++)
            //    this->rtgrainvoice[i]->getPreset(getSubStateAt(getSubState(preset, "__sps"), "rtgrains", i));
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
                //for (Index i = 0; i < 24; i++) {
                //    this->rtgrainvoice[i]->processTempoEvent(time, tempo);
                //}

                this->timevalue_01_onTempoChanged(tempo);
                this->timevalue_02_onTempoChanged(tempo);
                this->timevalue_03_onTempoChanged(tempo);
            }
        }

        void processTransportEvent(MillisecondTime time, TransportState state) {
            this->updateTime(time);

            //if (this->globaltransport_setState(this->_currentTime, state, false)) {
            //    for (Index i = 0; i < 24; i++) {
            //        this->rtgrainvoice[i]->processTransportEvent(time, state);
            //    }
            //}
        }

        void processBeatTimeEvent(MillisecondTime time, BeatTime beattime) {
            this->updateTime(time);

            //if (this->globaltransport_setBeatTime(this->_currentTime, beattime, false)) {
            //    for (Index i = 0; i < 24; i++) {
            //        this->rtgrainvoice[i]->processBeatTimeEvent(time, beattime);
            //    }
            //}
        }

        void onSampleRateChanged(double samplerate) {
            this->timevalue_01_onSampleRateChanged(samplerate);
            this->timevalue_02_onSampleRateChanged(samplerate);
            this->timevalue_03_onSampleRateChanged(samplerate);
        }

        void processTimeSignatureEvent(MillisecondTime time, int numerator, int denominator) {
            this->updateTime(time);

            if (this->globaltransport_setTimeSignature(this->_currentTime, numerator, denominator, false)) {
                this->timevalue_01_onTimeSignatureChanged(numerator, denominator);
                //for (Index i = 0; i < 24; i++) {
                //    this->rtgrainvoice[i]->processTimeSignatureEvent(time, numerator, denominator);
                //}

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

                //if (index < this->rtgrainvoice[0]->getNumParameters())
                    //this->rtgrainvoice[0]->setPolyParameterValue((PatcherInterface**)this->rtgrainvoice, index, v, time);

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

                //if (index < this->rtgrainvoice[0]->getNumParameters())
                //    return this->rtgrainvoice[0]->getPolyParameterValue((PatcherInterface**)this->rtgrainvoice, index);

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
            //return 21 + this->rtgrainvoice[0]->getNumParameters();
            return 21;
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

                //if (index < this->rtgrainvoice[0]->getNumParameters()) {
                //    {
                //        return this->rtgrainvoice[0]->getParameterName(index);
                //    }
                //}

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

                //if (index < this->rtgrainvoice[0]->getNumParameters()) {
                //    {
                //        return this->rtgrainvoice[0]->getParameterId(index);
                //    }
                //}

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
                    info->exponent = 1;
                    info->steps = 101; 
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
                    info->exponent = 1;
                    info->steps = 101;
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
                    info->exponent = 1;
                    info->steps = 101;
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
                    info->exponent = 1;
                    info->steps = 101;
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
                    info->exponent = 1;
                    info->steps = 101;
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
                    info->exponent = 1;
                    info->steps = 101;
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
                    info->exponent = 1;
                    info->steps = 101;
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
                    info->exponent = 1;
                    info->steps = 101;
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

                    //if (index < this->rtgrainvoice[0]->getNumParameters()) {
                    //    for (Index i = 0; i < 24; i++) {
                    //        this->rtgrainvoice[i]->getParameterInfo(index, info);
                    //    }
                    //}

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
            //if (subpatcher == this->rtgrainvoice[0])
            //    return 21;

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

                //if (index < this->rtgrainvoice[0]->getNumParameters()) {
                //    {
                //        return this->rtgrainvoice[0]->convertToNormalizedParameterValue(index, value);
                //    }
                //}

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

                //if (index < this->rtgrainvoice[0]->getNumParameters()) {
                //    {
                //        return this->rtgrainvoice[0]->convertFromNormalizedParameterValue(index, value);
                //    }
                //}

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

                //if (index < this->rtgrainvoice[0]->getNumParameters()) {
                //    {
                //        return this->rtgrainvoice[0]->constrainParameterValue(index, value);
                //    }
                //}

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

        void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
            this->updateTime(time);

            //for (Index i = 0; i < 24; i++) {
            //    this->rtgrainvoice[i]->processNumMessage(tag, objectId, time, payload);
            //}
        }

        void processListMessage(
            MessageTag tag,
            MessageTag objectId,
            MillisecondTime time,
            const list& payload
        ) {
            RNBO_UNUSED(objectId);
            this->updateTime(time);

            //for (Index i = 0; i < 24; i++) {
            //    this->rtgrainvoice[i]->processListMessage(tag, objectId, time, payload);
            //}
        }

        void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
            RNBO_UNUSED(objectId);
            this->updateTime(time);

            //for (Index i = 0; i < 24; i++) {
            //    this->rtgrainvoice[i]->processBangMessage(tag, objectId, time);
            //}
        }

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

            //auto subpatchResult_0 = this->rtgrainvoice[0]->resolveTag(tag);

            //if (subpatchResult_0)
            //    return subpatchResult_0;

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

        void updateVoiceState(int voice, voiceState voicestate) {
			auto voiceindex = voice - 1;

			voiceStates[voiceindex] = voicestate;
        }

        void param_01_value_set(number v) {
            v = this->param_01_value_constrain(v);
            this->param_01_value = v;
            this->sendParameter(0, false);

            if (this->param_01_value != this->param_01_lastValue) {
                this->getEngine()->presetTouched();
                this->param_01_lastValue = this->param_01_value;
            }

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

            this->glength_for_revoffhndlr = v;
            this->glength_for_delayrecstop = v;
            this->codebox_tilde_01_in2_set(v);
        }

        void param_05_value_set(number v) {
            v = this->param_05_value_constrain(v);
            this->param_05_value = v;
            this->sendParameter(4, false);

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
        }

        void param_07_value_set(number v) {
            v = this->param_07_value_constrain(v);
            this->param_07_value = v;
            this->sendParameter(6, false);

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
        }

        void param_09_value_set(number v) {
            v = this->param_09_value_constrain(v);
            this->param_09_value = v;
            this->sendParameter(8, false);

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
        }

        void param_11_value_set(number v) {
            v = this->param_11_value_constrain(v);
            this->param_11_value = v;
            this->sendParameter(10, false);

            if (this->param_11_value != this->param_11_lastValue) {
                this->getEngine()->presetTouched();
                this->param_11_lastValue = this->param_11_value;
            }
        }

        void param_12_value_set(number v) {
            v = this->param_12_value_constrain(v);
            this->param_12_value = v;
            this->sendParameter(11, false);

            if (this->param_12_value != this->param_12_lastValue) {
                this->getEngine()->presetTouched();
                this->param_12_lastValue = this->param_12_value;
            }
        }

        void param_13_value_set(number v) {
            v = this->param_13_value_constrain(v);
            this->param_13_value = v;
            this->sendParameter(12, false);

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

            this->gai_for_process = v;
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

            this->frz_for_revoffhndlr = static_cast<bool>(v);

            bool scroll = (v == 0);
            if (scroll)
                this->empty_audio_buffer();
            this->stop_rec_head(scroll);
            this->scrollvalue_for_delayrecstop = scroll;
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

            this->recordtilde_02_buffer = this->recordtilde_02_buffer->allocateIfNeeded();
            this->data_02_buffer = this->data_02_buffer->allocateIfNeeded();

            if (this->inter_databuf_01->hasRequestedSize()) {
                if (this->inter_databuf_01->wantsFill())
                    this->zeroDataRef(this->inter_databuf_01);

                this->getEngine()->sendDataRefUpdated(3);
            }
        }

        void initializeObjects() {
            this->codebox_tilde_01_rdm_init();
            this->data_01_init();
            this->data_02_init();
            this->data_03_init();
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

        void codebox_tilde_01_in3_set(number v) {
            this->den_for_gentrggs = v;
        }

        static number param_02_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        void codebox_tilde_01_in4_set(number v) {
            this->cha_for_gentrggs = v;
        }

        void param_02_normalized_set(number v) {
            this->codebox_tilde_01_in4_set(v);
        }

        static number param_03_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        void codebox_tilde_01_in5_set(number v) {
            this->rdl_for_gentrggs = v;
        }

        void param_03_normalized_set(number v) {
            this->codebox_tilde_01_in5_set(v);
        }

        static number param_04_value_constrain(number v) {
            v = (v > 2000 ? 2000 : (v < 20 ? 20 : v));
            return v;
        }

        //void codebox_tilde_03_in2_set(number v) {
        //    this->codebox_tilde_03_in2 = v;
        //}

        void codebox_tilde_01_in2_set(number v) {
            this->len_for_gentrggs = v;
        }

        static number param_05_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_06_value_constrain(number v) {
            v = (v > 4 ? 4 : (v < 0.25 ? 0.25 : v));
            return v;
        }

        static number param_07_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_08_value_constrain(number v) {
            v = (v > 3 ? 3 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_09_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_10_value_constrain(number v) {
            v = (v > 1 ? 1 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_11_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_12_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_13_value_constrain(number v) {
            v = (v > 100 ? 100 : (v < 0 ? 0 : v));
            return v;
        }

        static number param_14_value_constrain(number v) {
            v = (v > 1.5 ? 1.5 : (v < 0 ? 0 : v));
            return v;
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
            this->mute_for_gentriggs = v;
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

        //void codebox_tilde_03_in1_set(number v) {
        //    this->frz_for_revoffhndlr = v;
        //}

        //void bufferop_01_trigger_bang() {
        //    auto& buffer = this->bufferop_01_buffer;

        //    buffer->setZero();
        //    buffer->setTouched(true);
        //}

        //void select_01_match1_bang() {
        //    this->bufferop_01_trigger_bang();
        //}

        //void select_01_nomatch_number_set(number) {}

        //void select_01_input_number_set(number v) {
        //    if (v == this->select_01_test1)
        //        this->select_01_match1_bang();
        //    else
        //        this->select_01_nomatch_number_set(v);
        //}

        void empty_audio_buffer() {
            auto& buffer = this->bufferop_01_buffer;
            buffer->setZero();
            buffer->setTouched(true);
        }

        void latch_tilde_01_control_set(number v) {
            this->latch_tilde_01_control = v;
        }

        void stop_rec_head(number v) {
            this->latch_tilde_01_control_set(v);
        }

        //void codebox_tilde_02_in2_set(number v) {
        //    this->codebox_tilde_02_in2 = v;
        //}

        //void handle_freeze_delay(number v) {
        //    this->codebox_tilde_02_in2 = v;
        //}

        //void trigger_02_input_number_set(number v) {
        //    this->trigger_02_out3_set(trunc(v));
        //    this->trigger_02_out2_set(trunc(v));
        //    this->trigger_02_out1_set(trunc(v));
        //}

        //void expr_02_out1_set(number v) {                                           
        //    this->expr_02_out1 = v;
        //    this->trigger_02_input_number_set(this->expr_02_out1);
        //}

        //void expr_02_in1_set(number in1) {                                          //negation
        //    this->expr_02_in1 = in1;
        //    this->expr_02_out1_set(this->expr_02_in1 == 0);//#map:!_obj-60:1        
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
            this->syc_for_gentrggs = v;
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
            this->tmp_for_gentrggs = v;
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
            this->rtm_for_gentrggs = v;
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
                            number r = this->codebox_tilde_01_rdm_next() / (number)2 + 0.5;
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

        void delayRecStop(number glength, bool scroll, SampleValue* out1, Index n) {     //not sample accurate, shouldn't be a problem
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

            if (frz < this->revoffhndlr_frzhistory) {                       //at scrl pressed               
                DBG("SCROLL PRESSED, OFFSET: " << offset);
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

        /*  · If a voice is muted, but in timer state, it is ignored.
            · If a voice is unmuted, and eog is smaller then newtrig, it's good.
            · If a voice is muted and not in timer state, it's good.
            */

        int findtargetvoice(SampleIndex ti)     //VOICE MANAGER
        {
            for (int i = 0; i < 24; i++) {
                //bool candidate_state = voice_state[i];

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
            this->codebox_tilde_01_mphasor_dspsetup();
        }

        void param_01_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_01_value;
        }

        void param_01_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_01_value_set(preset["value"]);
        }

        void param_02_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_02_value;
        }

        void param_02_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_02_value_set(preset["value"]);
        }

        void param_03_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_03_value;
        }

        void param_03_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_03_value_set(preset["value"]);
        }

        void param_04_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_04_value;
        }

        void param_04_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_04_value_set(preset["value"]);
        }

        void param_05_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_05_value;
        }

        void param_05_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_05_value_set(preset["value"]);
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

        void param_07_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_07_value;
        }

        void param_07_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_07_value_set(preset["value"]);
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

        void param_09_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_09_value;
        }

        void param_09_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_09_value_set(preset["value"]);
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

        void param_11_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_11_value;
        }

        void param_11_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_11_value_set(preset["value"]);
        }

        void param_12_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_12_value;
        }

        void param_12_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_12_value_set(preset["value"]);
        }

        void param_13_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_13_value;
        }

        void param_13_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_13_value_set(preset["value"]);
        }

        void param_14_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_14_value;
        }

        void param_14_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_14_value_set(preset["value"]);
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

        void param_16_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_16_value;
        }

        void param_16_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_16_value_set(preset["value"]);
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

        void param_19_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_19_value;
        }

        void param_19_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_19_value_set(preset["value"]);
        }

        void param_20_getPresetValue(PatcherStateInterface& preset) {
            preset["value"] = this->param_20_value;
        }

        void param_20_setPresetValue(PatcherStateInterface& preset) {
            if ((bool)(stateIsEmpty(preset)))
                return;

            this->param_20_value_set(preset["value"]);
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
            mute_for_gentriggs = 0;
            len_for_gentrggs = 0;
            den_for_gentrggs = 0;
            cha_for_gentrggs = 0;
            rdl_for_gentrggs = 0;
            syc_for_gentrggs = 0;
            tmp_for_gentrggs = 0;
            rtm_for_gentrggs = 0;
            codebox_tilde_01_in9 = 0;
            codebox_tilde_01_in10 = 0;
            codebox_tilde_01_in11 = 0;
            param_01_value = 0.52;
            param_02_value = 100;
            dspexpr_02_in1 = 0;
            dspexpr_02_in2 = 1;
            param_03_value = 0;
            param_04_value = 200;
            param_05_value = 0;
            data_01_sizeout = 0;
            data_01_size = 100;
            data_01_sizems = 0;
            data_01_normalize = 0.995;
            data_01_channels = 1;
            phasor_01_freq = 0;
            param_06_value = 1;
            param_07_value = 0;
            phasor_02_freq = 0;
            param_08_value = 1;
            param_09_value = 0;
            phasor_03_freq = 0;
            param_10_value = 0;
            param_11_value = 0;
            param_12_value = 0;
            param_13_value = 0;
            param_14_value = 1;
            latch_tilde_01_x = 0;
            latch_tilde_01_control = 0;
            glength_for_delayrecstop = 0;
            scrollvalue_for_delayrecstop = 0;
            recordtilde_01_record = 0;
            recordtilde_01_begin = 0;
            recordtilde_01_end = -1;
            recordtilde_01_loop = 1;
            param_15_value = 0;
            recordtilde_02_record = 0;
            recordtilde_02_begin = 0;
            recordtilde_02_end = -1;
            recordtilde_02_loop = 0;
            param_16_value = 1;
            param_17_value = 0;
            param_18_value = 0;
            param_19_value = 0;
            expr_02_in1 = 0;
            expr_02_out1 = 0;
            select_01_test1 = 1;
            param_20_value = 3;
            dspexpr_03_in1 = 0;
            dspexpr_04_in1 = 0;
            dspexpr_04_in2 = 0;
            dspexpr_05_in1 = 0;
            dspexpr_05_in2 = 0;
            dspexpr_06_in1 = 0;
            gai_for_process = 1;
            param_21_value = 0;
            dcblock_tilde_01_x = 0;
            dcblock_tilde_01_gain = 0.9997;
            dspexpr_07_in1 = 0;
            dspexpr_07_in2 = 0;
            frz_for_revoffhndlr = 0;
            glength_for_revoffhndlr = 0;
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
            codebox_01_newphas = 0;
            codebox_01_oldphas = 0;
            codebox_01_mphasor_currentPhase = 0;
            codebox_01_mphasor_conv = 0;
            codebox_02_pos = 0;
            codebox_02_len = 0;
            codebox_02_f_r = 0;
            codebox_02_ptc = 0;
            codebox_02_vol = 0;
            codebox_02_pan = 0;
            gentrggs_syncphase_old = 0;
            gentrggs_freerunphase_old = 0;
            codebox_tilde_01_mphasor_currentPhase = 0;
            codebox_tilde_01_mphasor_conv = 0;
            codebox_tilde_01_setupDone = false;
            param_01_lastValue = 0;
            param_02_lastValue = 0;
            param_03_lastValue = 0;
            param_04_lastValue = 0;
            param_05_lastValue = 0;
            data_01_sizemode = 1;
            data_01_setupDone = false;
            phasor_01_sigbuf = nullptr;
            phasor_01_lastLockedPhase = 0;
            phasor_01_conv = 0;
            phasor_01_setupDone = false;
            param_06_lastValue = 0;
            param_07_lastValue = 0;
            phasor_02_sigbuf = nullptr;
            phasor_02_lastLockedPhase = 0;
            phasor_02_conv = 0;
            phasor_02_setupDone = false;
            param_08_lastValue = 0;
            param_09_lastValue = 0;
            phasor_03_sigbuf = nullptr;
            phasor_03_lastLockedPhase = 0;
            phasor_03_conv = 0;
            phasor_03_setupDone = false;
            param_10_lastValue = 0;
            param_11_lastValue = 0;
            param_12_lastValue = 0;
            param_13_lastValue = 0;
            param_14_lastValue = 0;
            latch_tilde_01_value = 0;
            latch_tilde_01_setupDone = false;
            delrecstop_delaysamps = 0;
            delrecstop_record = 0;
            delrecstop_scrollhistory = 1;
            recordtilde_01_wIndex = 0;
            recordtilde_01_lastRecord = 0;
            param_15_lastValue = 0;
            recordtilde_02_wIndex = 0;
            recordtilde_02_lastRecord = 0;
            param_16_lastValue = 0;
            param_17_lastValue = 0;
            param_18_lastValue = 0;
            param_19_lastValue = 0;
            dial_17_lastValue = 0;
            param_20_lastValue = 0;
            dial_18_lastValue = 0;
            param_21_lastValue = 0;
            dcblock_tilde_01_xm1 = 0;
            dcblock_tilde_01_ym1 = 0;
            dcblock_tilde_01_setupDone = false;
            feedbacktilde_01_feedbackbuffer = nullptr;
            revoffhndlr_offset = 0;
            revoffhndlr_frzhistory = 0;
            revoffhndlr_c = 0;
            revoffhndlr_hit = 0;
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
            //isMuted = 1;
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
        list codebox_02_in1;
        list codebox_02_out1;
        list codebox_02_out2;
        bool mute_for_gentriggs;
        number len_for_gentrggs;
        number den_for_gentrggs;
        number cha_for_gentrggs;
        number rdl_for_gentrggs;
        bool syc_for_gentrggs;
        int tmp_for_gentrggs;
        int rtm_for_gentrggs;
        number codebox_tilde_01_in9;
        number codebox_tilde_01_in10;
        number codebox_tilde_01_in11;
        number param_01_value;
        number param_02_value;
        number dspexpr_02_in1;
        number dspexpr_02_in2;
        number param_03_value;
        number param_04_value;
        number param_05_value;
        number data_01_sizeout;
        number data_01_size;
        number data_01_sizems;
        number data_01_normalize;
        number data_01_channels;
        number phasor_01_freq;
        number param_06_value;
        number param_07_value;
        number phasor_02_freq;
        number param_08_value;
        number param_09_value;
        number phasor_03_freq;
        number param_10_value;
        number param_11_value;
        number param_12_value;
        number param_13_value;
        number param_14_value;
        number latch_tilde_01_x;
        number latch_tilde_01_control;
        SampleIndex glength_for_delayrecstop;
        bool scrollvalue_for_delayrecstop;
        number recordtilde_01_record;
        number recordtilde_01_begin;
        number recordtilde_01_end;
        number recordtilde_01_loop;
        number param_15_value;
        number recordtilde_02_record;
        number recordtilde_02_begin;
        number recordtilde_02_end;
        number recordtilde_02_loop;
        number param_16_value;
        number param_17_value;
        number param_18_value;
        number param_19_value;
        number expr_02_in1;
        number expr_02_out1;
        number select_01_test1;
        list bufferop_01_channels;
        number param_20_value;
        number dspexpr_03_in1;
        number dspexpr_04_in1;
        number dspexpr_04_in2;
        number dspexpr_05_in1;
        number dspexpr_05_in2;
        number dspexpr_06_in1;
        number gai_for_process;
        number param_21_value;
        number dcblock_tilde_01_x;
        number dcblock_tilde_01_gain;
        number dspexpr_07_in1;
        number dspexpr_07_in2;
        bool frz_for_revoffhndlr;
        number glength_for_revoffhndlr;
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
        number codebox_01_newphas;
        number codebox_01_oldphas;
        number codebox_01_mphasor_currentPhase;
        number codebox_01_mphasor_conv;
        //bool voice_state[24] = { };

		voiceState voiceStates[24] = { };

        number codebox_02_pos;
        number codebox_02_len;
        number codebox_02_f_r;
        number codebox_02_ptc;
        number codebox_02_vol;
        number codebox_02_pan;
        number gentrggs_syncphase_old;
        number gentrggs_freerunphase_old;
        number codebox_tilde_01_mphasor_currentPhase;
        number codebox_tilde_01_mphasor_conv;
        UInt codebox_tilde_01_rdm_state[4] = { };
        bool codebox_tilde_01_setupDone;
        number param_01_lastValue;
        number param_02_lastValue;
        number param_03_lastValue;
        number param_04_lastValue;
        number param_05_lastValue;
        Float32BufferRef data_01_buffer;
        Int data_01_sizemode;
        bool data_01_setupDone;
        signal phasor_01_sigbuf;
        number phasor_01_lastLockedPhase;
        number phasor_01_conv;
        bool phasor_01_setupDone;
        number param_06_lastValue;
        number param_07_lastValue;
        signal phasor_02_sigbuf;
        number phasor_02_lastLockedPhase;
        number phasor_02_conv;
        bool phasor_02_setupDone;
        number param_08_lastValue;
        number param_09_lastValue;
        signal phasor_03_sigbuf;
        number phasor_03_lastLockedPhase;
        number phasor_03_conv;
        bool phasor_03_setupDone;
        number param_10_lastValue;
        number param_11_lastValue;
        number param_12_lastValue;
        number param_13_lastValue;
        number param_14_lastValue;
        number latch_tilde_01_value;
        bool latch_tilde_01_setupDone;
        SampleIndex delrecstop_delaysamps;
        bool delrecstop_record;
        bool delrecstop_scrollhistory;
        SampleIndex delrecstop_count;
        bool delrecstop_inc;
        Float32BufferRef recordtilde_01_buffer;
        SampleIndex recordtilde_01_wIndex;
        number recordtilde_01_lastRecord;
        number param_15_lastValue;
        Float32BufferRef recordtilde_02_buffer;
        SampleIndex recordtilde_02_wIndex;
        number recordtilde_02_lastRecord;
        number param_16_lastValue;
        number param_17_lastValue;
        number param_18_lastValue;
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
        SampleIndex revoffhndlr_offset;
        bool revoffhndlr_frzhistory;
        SampleIndex revoffhndlr_c;
        bool revoffhndlr_hit;
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
        DataRef inter_databuf_01;
        Index _voiceIndex;
        Int _noteNumber;
        //Index isMuted;
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

