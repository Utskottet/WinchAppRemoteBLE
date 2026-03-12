/**
 * buzzer.cpp - Buzzer implementation for XIAO nRF52840
 * Adapted from T3 remote: LEDC replaced with tone()/noTone().
 * All patterns identical to T3.
 */

#include "buzzer.h"
#include "utilities.h"

BuzzerController buzzer;

const uint16_t BuzzerController::stateBaseFrequency[7] = {
  1000,  // State 0
  1200,  // State 1
  1400,  // State 2
  1600,  // State 3
  1800,  // State 4
  2000,  // State 5
  2200   // State 6
};

const uint16_t BUTTON_BEEP_FREQ     = 2000;
const uint16_t BUTTON_BEEP_DURATION = 20;
const uint16_t SWEEP_DURATION       = 150;
const uint16_t TONE_END_DELAY       = 50;

BuzzerController::BuzzerController() :
  buzzerState(BUZZER_IDLE),
  buzzerEndTime(0),
  patternStepTime(0),
  patternStep(0),
  sweepStartFreq(0),
  sweepEndFreq(0),
  sweepCurrentFreq(0),
  sweepStep(0),
  sweepStepTimeMs(0),
  sweepLastStepTime(0)
{}

void BuzzerController::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
}

void BuzzerController::startTone(uint16_t frequency) {
  tone(BUZZER_PIN, frequency);
}

void BuzzerController::stopTone() {
  noTone(BUZZER_PIN);
}

void BuzzerController::playButtonPress() {
  if (buzzerState != BUZZER_IDLE && buzzerState != BUZZER_BEEP) return;
  buzzerState  = BUZZER_BEEP;
  buzzerEndTime = millis() + BUTTON_BEEP_DURATION;
  startTone(BUTTON_BEEP_FREQ);
}

void BuzzerController::playStateChange(uint8_t fromState, uint8_t toState) {
  fromState = (fromState > 6) ? 6 : fromState;
  toState   = (toState   > 6) ? 6 : toState;
  if (fromState == toState) return;

  if (toState == 1 && (fromState == 0 || fromState >= 2)) {
    playJumpToOne(); return;
  }
  if (toState == 0 && fromState == 1) {
    playJumpToZero(); return;
  }
  if (toState > fromState) {
    playUpwardSweep(stateBaseFrequency[fromState], stateBaseFrequency[toState], SWEEP_DURATION);
  } else {
    playDownwardSweep(stateBaseFrequency[fromState], stateBaseFrequency[toState], SWEEP_DURATION);
  }
}

void BuzzerController::playJumpToOne() {
  buzzerState   = BUZZER_PATTERN_TO_1;
  patternStep   = 0;
  patternStepTime = millis();
  startTone(3000);
}

void BuzzerController::playJumpToZero() {
  buzzerState   = BUZZER_PATTERN_TO_0;
  patternStep   = 0;
  patternStepTime = millis();
  startTone(3500);
}

void BuzzerController::update() {
  uint32_t now = millis();

  switch (buzzerState) {
    case BUZZER_IDLE:
      break;

    case BUZZER_BEEP:
      if (now >= buzzerEndTime) { stopTone(); buzzerState = BUZZER_IDLE; }
      break;

    case BUZZER_SWEEP_UP:
      if (now - sweepLastStepTime >= sweepStepTimeMs) {
        sweepLastStepTime = now;
        sweepCurrentFreq += sweepStep;
        if (sweepCurrentFreq >= sweepEndFreq) {
          tone(BUZZER_PIN, sweepEndFreq);
          buzzerState   = BUZZER_ENDING;
          buzzerEndTime = now + TONE_END_DELAY;
        } else {
          tone(BUZZER_PIN, sweepCurrentFreq);
        }
      }
      break;

    case BUZZER_SWEEP_DOWN:
      if (now - sweepLastStepTime >= sweepStepTimeMs) {
        sweepLastStepTime = now;
        sweepCurrentFreq += sweepStep; // sweepStep is negative
        if (sweepCurrentFreq <= sweepEndFreq) {
          tone(BUZZER_PIN, sweepEndFreq);
          buzzerState   = BUZZER_ENDING;
          buzzerEndTime = now + TONE_END_DELAY;
        } else {
          tone(BUZZER_PIN, sweepCurrentFreq);
        }
      }
      break;

    case BUZZER_PATTERN_TO_1:
      switch (patternStep) {
        case 0: if (now >= patternStepTime + 50) { startTone(1200); patternStep++; patternStepTime = now; } break;
        case 1: if (now >= patternStepTime + 50) { startTone(3000); patternStep++; patternStepTime = now; } break;
        case 2: if (now >= patternStepTime + 50) { startTone(1200); patternStep++; patternStepTime = now; } break;
        case 3: if (now >= patternStepTime + 80) { buzzerState = BUZZER_ENDING; buzzerEndTime = now + TONE_END_DELAY; } break;
      }
      break;

    case BUZZER_PATTERN_TO_0:
      if (now >= patternStepTime + 10) {
        patternStepTime = now;
        if (patternStep < 30) {
          float t = patternStep / 30.0f;
          uint16_t freq = 3500 - (uint16_t)(2700 * t * t);
          startTone(freq);
          patternStep++;
        } else {
          buzzerState   = BUZZER_ENDING;
          buzzerEndTime = now + TONE_END_DELAY;
        }
      }
      break;

    case BUZZER_ENDING:
      if (now >= buzzerEndTime) { stopTone(); buzzerState = BUZZER_IDLE; }
      break;
  }
}

void BuzzerController::playUpwardSweep(uint16_t startFreq, uint16_t endFreq, uint16_t durationMs) {
  uint16_t freqDiff = endFreq - startFreq;
  uint8_t  numSteps = 20;
  sweepStartFreq    = startFreq;
  sweepEndFreq      = endFreq;
  sweepCurrentFreq  = startFreq;
  sweepStep         = freqDiff / numSteps;
  if (sweepStep < 1) sweepStep = 1;
  sweepStepTimeMs   = durationMs / numSteps;
  sweepLastStepTime = millis();
  buzzerState       = BUZZER_SWEEP_UP;
  startTone(sweepStartFreq);
}

void BuzzerController::playDownwardSweep(uint16_t startFreq, uint16_t endFreq, uint16_t durationMs) {
  uint16_t freqDiff = startFreq - endFreq;
  uint8_t  numSteps = 20;
  sweepStartFreq    = startFreq;
  sweepEndFreq      = endFreq;
  sweepCurrentFreq  = startFreq;
  sweepStep         = -((int16_t)freqDiff / numSteps);
  if (sweepStep > -1) sweepStep = -1;
  sweepStepTimeMs   = durationMs / numSteps;
  sweepLastStepTime = millis();
  buzzerState       = BUZZER_SWEEP_DOWN;
  startTone(sweepStartFreq);
}
