/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#include "DynamicWaveshaper.h"


void DynamicWaveshaper::setThreshold(float threshold) noexcept {
	mThreshold = Decibels::decibelsToGain(threshold);
}
void DynamicWaveshaper::setAttack(float attack) noexcept {
	mAttackCoeff = exp(-1000 / (attack * mSampleRate));
	mOneMinusAttackCoeff = 1 - mAttackCoeff;
}
void DynamicWaveshaper::setRelease(float release) noexcept {
	mReleaseCoeff = exp(-1000 / (release * mSampleRate));
}
