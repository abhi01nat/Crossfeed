# Crossfeed
Externalisation of headphone audio, implemented as VST3 using the JUCE framework. The algorithm estimates the Inter-aural Time Difference for sounds placed externally to introduce a crossfeed between the left and right channels of stereo audio meant for headphone listening. The crossfeed is also filtered to more closely approximate the effect of the acoustic shadow of the head, and further processing is applied to ensure that (1) there is no phase cancellation or comb filtering, and (2) the audio stays spectrally uncolored. 
