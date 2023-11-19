void showWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("WELCOME!");
  lcd.setCursor(0, 1);
  lcd.print("PLEASE RING BELL");
}

void showAwayMessage() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("SORRY!");
  lcd.setCursor(0, 1);
  lcd.print("WE NOT AT HOME!!");
}

void showResidentsNotifiedMessage(){
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("RESIDENTS HAVE");
  lcd.setCursor(1, 1);
  lcd.print("BEEN NOTIFIED");
}

void showResidentsNotifiedNotHomeMessage(){
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("OWNERS NOTIFIED");
  lcd.setCursor(2, 1);
  lcd.print("BUT NOT HOME");
}

void showRestartingMessage(){
   lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RESTARTING ...");
}