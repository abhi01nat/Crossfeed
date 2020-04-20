/*
  ==============================================================================

	DelayLine.h
	Created: 21 Mar 2020 12:59:34am
	Author:  Abhinav Natarajan

  ==============================================================================
*/

#pragma once
//#include <cstddef> - TODO how to make available std::size_t inside the classes for private use
#include <JuceHeader.h>
#include <vector>
#include <cmath>

template <typename Type>
// helper class that implements a single channel delay line
class DelayLine {
public:
	DelayLine () = default;
	~DelayLine () = default;

	void clear () noexcept {
		std::fill (buffer.begin (), buffer.end (), 0);
		writeIndex = 0;
		readIndex = delayInSamples;
	}

	void setSize (size_t newSize) {
		buffer.resize (newSize);
		clear ();
	}

	size_t inline getSize () const noexcept {
		return buffer.size ();
	}

	void inline push (Type val) noexcept {
		buffer[writeIndex] = val;
		if (writeIndex == 0) writeIndex = getSize () - 1;
		else --writeIndex;
	}
	inline Type get () noexcept {
		Type val = buffer[readIndex];
		if (readIndex == 0) readIndex = getSize () - 1;
		else --readIndex;
		return val;
	}

	void inline setDelayInSamples (size_t newDelayInSamples) noexcept {
		jassert (newDelayInSamples < getSize ());
		delayInSamples = newDelayInSamples;
		readIndex = (writeIndex + delayInSamples) % getSize ();
	}

	size_t inline getDelayInSamples () const noexcept {
		return delayInSamples;
	}

private:
	std::vector<Type> buffer;
	size_t delayInSamples { 0 };
	size_t writeIndex { 0 };
	size_t readIndex { 0 };
};

template <typename Type>
class Delay {
public:
	Delay () = default;
	~Delay () = default;

	void reset () noexcept {
		for (auto& d : delayLines) {
			d.clear ();
		}
	}

	void prepare (const juce::dsp::ProcessSpec& spec) {
		delayLines.resize (spec.numChannels);
		setMaxDelayInSamples (maxDelayInSamples);
		sampleRate = static_cast <Type> (spec.sampleRate);
	}

	void inline setDelayInSamples (size_t newDelayInSamples) noexcept {
		jassert (newDelayInSamples <= maxDelayInSamples);
		delayInSamples = newDelayInSamples;
		for (auto& d : delayLines) {
			d.setDelayInSamples (delayInSamples);
		}
	}

	size_t inline getDelayInSamples () const noexcept {
		return delayInSamples;
	}

	/** Always call after calling prepare. */
	void setDelayInSeconds (Type newDelayInSeconds) noexcept {
		setDelayInSamples (size_t (std::floor (newDelayInSeconds * sampleRate)));
	}

	Type getDelayInSeconds () const noexcept {
		return Type (delayInSamples / sampleRate);
	}

	void setMaxDelayInSamples (size_t newMaxDelayInSamples) {
		jassert (newMaxDelayInSamples < std::numeric_limits<size_t>::max ());
		maxDelayInSamples = newMaxDelayInSamples;
		for (auto& d : delayLines) {
			d.setSize (maxDelayInSamples + 1); // automatically clears all delaylines
		}
	}

	size_t getMaxDelayInSamples () {
		return maxDelayInSamples;
	}

	void setMaxDelayInSeconds (Type newMaxDelayInSeconds) noexcept {
		setMaxDelayInSamples (size_t (std::ceil (newMaxDelayInSeconds * sampleRate)));
	}

	Type getMaxDelayInSeconds () const noexcept {
		return Type (maxDelayInSamples / sampleRate);
	}

	template <typename ProcessContext>
	void process (const ProcessContext& context) noexcept {
		if (context.isBypassed)
			processInternal<ProcessContext, true> (context);
		else
			processInternal<ProcessContext, false> (context);
	}

private:
	std::vector<DelayLine<Type>> delayLines;
	size_t delayInSamples { 0 };
	size_t maxDelayInSamples { 150 };
	Type sampleRate { Type (44.1e3) };


	template <typename ProcessContext, bool isBypassed>
	void processInternal (const ProcessContext& context) noexcept {

		static_assert (std::is_same<typename ProcessContext::SampleType, Type>::value,
			"The sample-type of the IIR filter must match the sample-type supplied to this process callback");

		auto&& inputBlock = context.getInputBlock ();
		auto&& outputBlock = context.getOutputBlock ();

		auto numChannels = inputBlock.getNumChannels ();
		jassert (numChannels == delayLines.size ());

		auto numSamples = inputBlock.getNumSamples ();
		jassert (numSamples == outputBlock.getNumSamples ());

		for (size_t chan = 0; chan < numChannels; ++chan) {
			auto src = inputBlock.getChannelPointer (chan);
			auto dst = outputBlock.getChannelPointer (chan);
			auto& d = delayLines[chan];
			for (size_t i = 0; i < numSamples; ++i) {
				d.push (src[i]);
				dst[i] = isBypassed ? d.get (), src[i] : d.get ();
			}
		}
	}
};
