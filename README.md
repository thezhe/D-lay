# D-lay
A gradually deteriorating Delay based off of the traditional analog BBD (Bucket Brigade Delay)

## Delay Algorithm (My modifications to the code from the following [JUCE tutorial][1]):
-getFromDelayBuffer: 
	1. add accumulated delayed signal (wet) to buffer (dry) with gain of 'mWet'
	2. made separate wet and feedback controls
	3. made a "FX send" for nonlinear processing before storing in 'mDelayBuffer'
-fillDelayBuffer:
	1. fix 'mDelayBuffer''s wraparound case (else block) so that the second part of 'bufferData' writes to the beginning of 'mDelayBuffer' starting at 'bufferData + bufferRemaining' rather than at 'bufferData'
-processBlock: 
	1. removed 'feedbackDelay' method
	2. switched 'getFromDelayBuffer' and 'fillDelayBuffer''s order
-general refactoring:
	1. relocated definitions for IO channels and buffer lengths from processBlock to prepareToPlay (now only called once)

## Saturation (loosely based off of JUCE's dsp modules)
-DynamicWaveshaper:
	1. 'prepare' defines wavetable and spec
	2. 'process' called once per 'processBlock'

### TODO:
- [ ] add saturation 
- [ ] refactor/optimize/UI
###### VST3 files in D-lay\Builds\VisualStudio2019\x64\Release or D-lay\Builds\VisualStudio2019\Win32\Release32

[1]: https://www.youtube.com/watch?v=IRFUYGkMV8w
