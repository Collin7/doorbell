void buttonPressedHomeSoundEffect() {
  for (int note : successFailTone) {
    tone(SPEAKER_PIN, note);
    delay(50);
  }
  noTone(SPEAKER_PIN);
}

void singleBeepSoundEffect() {
  tone(SPEAKER_PIN, normalKeypressTone);
  delay(100);
  noTone(SPEAKER_PIN);
}

void buttonPressedAwaySoundEffect() {
  for (int i = 11; i > 0; i--) {
    tone(SPEAKER_PIN, successFailTone[i]);
    delay(50);
  }
  noTone(SPEAKER_PIN);
}