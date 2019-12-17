# D-lay
A Distorted Delay based off of the traditional analog BBD (Bucket Brigade Delay)

## Delay Algorithm (My modifications to the code from the following [JUCE tutorial][1]):
DelayLine:
-made a mWriteBlock for nonlinear processing before storing in 'mDelayBuffer'
-made custom class with contructor that conforms to JUCE's "spec" initialization
-getFromDelayBuffer: 
	1. add accumulated delayed signal (wet) to buffer (dry) with gain of 'mWet'
	2. made separate wet and feedback controls
-fillDelayBuffer:
	1. fix mDelayBuffer's wraparound case (else block) so that the second part of 'bufferData' writes to the beginning of 'mDelayBuffer' starting at 'bufferData + bufferRemaining' rather than at 'bufferData'
-general refactoring:
	1. relocated definitions for IO channels and buffer lengths from processBlock to prepareToPlay (now only called once)

### TODO:
- [ ] envelope triggered waveshaper (modifies mWriteBlock after anti-alias filter)
- [ ] UI

###### VST3 files in D-lay\Builds\VisualStudio2019\x64\Release or D-lay\Builds\VisualStudio2019\Win32\Release32

[1]: https://www.youtube.com/watch?v=IRFUYGkMV8w
