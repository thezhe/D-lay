# D-lay
A gradually deteriorating Delay based off of the traditional analog BBD (Bucket Brigade Delay)

## Delay Algorithm (My modifications to the code from the following [JUCE tutorial](www.youtube.com/watch?v=IRFUYGkMV8w):
-getFromDelayBuffer: 
	1. add accumulated delayed signal (wet) to buffer (dry) with gain of 'mFeedback'
-fillDelayBuffer:
	1. fix 'mDelayBuffer''s wraparound case (else block) so that the second part of 'bufferData' writes to the beginning of 'mDelayBuffer' starting at 'bufferData + bufferRemaining' rather than at 'bufferData'
	2. copy with gain of 1
-processBlock: 
	1. removed 'feedbackDelay' method
	2. switched 'getFromDelayBuffer' and 'fillDelayBuffer''s order
-general refactoring:
	1. relocated definitions for IO channels and buffer lengths from processBlock to prepareToPlay (now only called once)


### TODO:
- [ ] add saturation and slew rate degredation
- [ ] UI

###### VST3 files in D-lay\Builds\VisualStudio2019\x64\Release or D-lay\Builds\VisualStudio2019\Win32\Release32


