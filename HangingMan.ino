 #include <SPI.h>
#include <EEPROM.h>
#include <Arduboy.h>
#include "Dictionary.h"
#include "Images.h"

#define _BV0(bit) ((bit < 32) ? ((bit < 16)? (1U << bit):(1UL << bit)):( 1ULL << bit) )

#define BUTTONCD   200
#define BLINKCD    250
#define DISPLAYCD 1000

Arduboy display;

unsigned long currTime;
unsigned long prevTime = 0;
unsigned long lastPress = 0;

int Score;
int LetterOffsets[8];
uint8_t Streak, Strikes;
unsigned long UsedLetters = 0;
char SelectedWord[9];
byte SelectedWordMask, WinningMask;

void setup() {
  Serial.begin(9600);
  display.begin( );
  Title( );
}
void loop( ) {
  SelectLetter( );
  if ( WinningMask == SelectedWordMask ) {
    int x = 128;
    while ( x > -126 ) {
      display.clear( );
      DisplayAll( );
      display.drawBitmap( x, 24, WooMask, 126, 16, BLACK );
      display.drawBitmap( x, 24, Woo, 126, 16, WHITE );
      display.display( );
      x -= 3;
    }
    Strikes = 0;
    PickWord( );
  }
  if ( Strikes == 6 ) {
    display.clear( );
    SelectedWordMask = 0xFF;
    DisplayAll( );
    DisplayWaitForTap( );
    int y = 63;
    while ( y>=0 ) {
      display.clear( );
      DisplayAll( );
      for ( int a = 64; a >= y; a-- ) display.drawFastHLine( 0, a, 128, BLACK );
      display.drawBitmap( 35, 8+y, GameOver, 58, 48, WHITE );
      display.display( );
      y-=3;
    }
    DisplayWaitForTap( );
    if (Score) enterHighScore(1);
    while ( !displayHighScores(1) ) delay(250);

    Title( );
  }
}

static const uint8_t TitleManCoords[] = { 40, 40, 41, 41, 42, 42, 42, 41, 41, 40 };
static const uint8_t patternCoords[] = {
	0,0,
	1,0,
	2,0,
	3,0,
	3,1,
	3,2,
	3,3,
	2,3,
	1,3,
	0,3,
	0,2,
	0,1,
	1,1,
	2,1,
	2,2,
	1,2
};
void Title( ) {
  unsigned long Counter = 0;
  for ( int a = 16; a > 0; a-- ) {
    display.clear( );
    display.drawBitmap(0, 0, HangingManTitleBG, 128, 64, WHITE);
    display.drawBitmap(40, 25, HangingManTitleMan, 18, 40, WHITE);
    for (uint8_t pCount = 0; pCount < a; pCount++) {
      uint8_t patternX = patternCoords[pCount*2];
      uint8_t patternY = patternCoords[(pCount*2)+1];
      for (uint8_t x = patternX; x < 128; x+=4) {
        for (uint8_t y = patternY; y < 64; y+=4) {
          display.drawPixel(x,y,BLACK);
        }
      }
    }
    display.display( );
  }  
  ActivateButtonCD( );
  while ( true ) {
    if ( ButtonOffCD( ) && display.buttonsState( ) ) break;
    Counter++;
    uint8_t x;
    display.clear( );
    display.drawBitmap(0,0,HangingManTitleBG,128,64,WHITE);
    display.drawBitmap(TitleManCoords[ (Counter/30)%10 ], 25, HangingManTitleMan, 18, 40, WHITE);
    display.display( );
  }
  Score = 0;
  Strikes = 0;
  Streak = 0;
  PickWord( );
  
  int y = 64;
  while ( y>=0 ) {
    display.clear( );
    DisplayAll( );
    for ( int a = 0; a < y; a++ ) display.drawFastHLine( 0, a, 128, BLACK );
    display.display( );
    y-=3;
  }
  ActivateButtonCD( );
}
void DisplayWaitForTap( ) {
  display.display( );
  ActivateButtonCD( );
  while ( true ) {
    if ( ButtonOffCD( ) && display.buttonsState( ) ) {
      ActivateButtonCD( );
      return;
    }
  }
}
void ActivateButtonCD( ) {
  lastPress = millis();
}
bool ButtonOffCD( ) {
  if ( millis() > lastPress + BUTTONCD ) return true;
  return false;
}

bool IsLetterUsed ( char Letter ) {
  if ( _BV0( Letter-65 ) & UsedLetters ) return true;
  else return false;
}
uint8_t UseLetter ( char Letter ) {
  UsedLetters |= _BV0( Letter-65 );
  uint8_t MatchCount = 0;
  for ( uint8_t a = 0; a < strlen( SelectedWord ); a++ ) {
    if ( SelectedWord[a] == Letter ) {
      MatchCount++;
      SelectedWordMask |= _BV0(a);
    }
  }
  if ( MatchCount ) {
    Streak++;
    Score += MatchCount * Streak;
  }
  return MatchCount;
}
char FirstSelectableLetter( ) {
  char Test = 65;                         // ASCII "A"
  while ( IsLetterUsed( Test ) ) Test++;  // If Test letter is used, try the next
  return Test;
}
char NextSelectableLetter ( char CurrentLetter ) {
  while ( true ) {
    CurrentLetter++;
    if ( CurrentLetter > 'Z' ) CurrentLetter = 'A';
    if ( !IsLetterUsed( CurrentLetter ) ) return CurrentLetter;
  }
}
char PrevSelectableLetter ( char CurrentLetter ) {
  while ( true ) {
    CurrentLetter--;
    if ( CurrentLetter < 'A' ) CurrentLetter = 'Z';
    if ( !IsLetterUsed( CurrentLetter ) ) return CurrentLetter;
  }
}
char LetterRowShift ( char CurrentLetter ) {
  char TempLetter = CurrentLetter + 13;
  if ( TempLetter > 'Z' ) TempLetter -= 26;
  if ( !IsLetterUsed( TempLetter ) ) return TempLetter;
  else return CurrentLetter;
}

void PickWord ( ) {
  UsedLetters = 0;
  SelectedWordMask = 0;
  WinningMask = 0;
  randomSeed( millis() );
  strcpy_P( SelectedWord, (char*)pgm_read_word(&(Dictionary[ random( DictionarySize ) ])) );
  for ( uint8_t a = 0; a < strlen( SelectedWord ); a++ ) WinningMask |= _BV0(a);
}
void DisplayWord ( ) {
  uint8_t x = ( 96 - ( ( ( strlen( SelectedWord ) * 2 ) - 1 ) * 6 ) ) / 2;
  uint8_t y = 20;
  display.setCursor( x, y );
  for ( uint8_t a = 0; a < strlen( SelectedWord ); a++ ) {
    if ( _BV0(a) & SelectedWordMask ) display.print( (char) SelectedWord[a] );
    else display.print(F(" "));
    display.drawFastHLine ( x + (a*2*6), y+8, 5, WHITE );
    display.print(F(" "));
  }
}

uint8_t CountDigits ( int Number ) {
  uint8_t Count = 1;
  int Divided = Number;
  while (true) {
    Divided /= 10;
    if ( !Divided ) return Count;
    Count++;
  }
}
void DisplayAlphabet ( uint8_t x, uint8_t y ) {
  display.setCursor( x, y );
  for ( char a = 'A'; a < 'N'; a++ ) {
    if ( !IsLetterUsed( a ) ) display.print ( a );
    else display.print(F("-"));
  }
  display.setCursor( x, y+10 );
  for ( char a = 'N'; a < '['; a++ ) {
    if ( !IsLetterUsed( a ) ) display.print ( a );
    else display.print(F("-"));
  }
}
void DisplayCursor ( uint8_t x, uint8_t y, char SelectedLetter ) {
  if ( SelectedLetter > 'M' ) display.drawFastHLine ( x + ( ( SelectedLetter - 13 - 65 ) * 6 ), y+18, 5, WHITE );
  else display.drawFastHLine ( x + ( ( SelectedLetter - 65 ) * 6 ), y+8, 5, WHITE );
}
void DisplayScore ( ) {
  uint8_t x = ( 96 - ( ( CountDigits(Score) + 7 ) * 6 ) ) / 2;
  uint8_t y = 0;
  display.setCursor( x, y ); display.print(F("Score: ")); display.print( Score );
  //display.setCursor( x, y+9 ); display.print(F("Streak: ")); display.print( Streak ); display.print(F("x"));
}
void DisplayGallows ( ) {
  uint8_t x = 96;
  uint8_t y = 8;
  switch (Strikes) {
    case 6: display.drawBitmap( x+21, y+29, LeftLeg, 3, 16, WHITE );
            display.drawBitmap( x+15, y+8, DeadHead, 11, 16, WHITE );
    case 5: display.drawBitmap( x+17, y+29, RightLeg, 3, 16, WHITE );
    case 4: display.drawBitmap( x+21, y+21, LeftArm, 3, 8, WHITE );
    case 3: display.drawBitmap( x+17, y+21, RightArm, 3, 8, WHITE );
    case 2: display.drawBitmap( x+20, y+20, Torso, 1, 16, WHITE );
    case 1: display.drawBitmap( x+15, y+8, Head, 11, 16, WHITE );
    case 0: display.drawBitmap( x, y, Gallows, 32, 56, WHITE );
  }
}
void DisplayAll( ) {
  DisplayGallows( );
  DisplayWord( );
  DisplayScore( );
  DisplayAlphabet( (96-(13*6))/2, HEIGHT-19 );
}

void SelectLetter ( ) {
  uint8_t x = (96-(13*6))/2;
  uint8_t y = HEIGHT - 19;  
  unsigned long BlinkTime = 0;
  bool Underscore = false;
  char SelectedLetter = FirstSelectableLetter( );
  while ( true ) {
    byte input = display.buttonsState( );
    if ( input && ButtonOffCD( ) ) {
      ActivateButtonCD( );
      if ( ( input & UP_BUTTON ) ||
           ( input & DOWN_BUTTON ) )  SelectedLetter = LetterRowShift( SelectedLetter );
      if   ( input & RIGHT_BUTTON )   SelectedLetter = NextSelectableLetter( SelectedLetter );
      if   ( input & LEFT_BUTTON )    SelectedLetter = PrevSelectableLetter( SelectedLetter );
      if ( ( input & A_BUTTON ) || ( input & B_BUTTON ) ) break;
    }
    if ( millis() > BlinkTime + BLINKCD ) {
      BlinkTime = millis();
      Underscore = !Underscore;
    }
    display.clear( );
    DisplayAll( );
    if ( Underscore ) DisplayCursor ( x, y, SelectedLetter );
    display.display( );
  }
  uint8_t MatchCount = UseLetter( SelectedLetter );
  if ( MatchCount ) {
    if ( WinningMask == SelectedWordMask ) return;
    if ( MatchesAnimation( MatchCount, 24 ) ) return; 
    delay(150);
    if ( MatchesAnimation( MatchCount, -21 ) ) return;
  }
  else {
    DeadHeadAnimation( );
    Strikes++;
    Streak = 0;
  }
}

void DecrementLetterOffsets ( uint8_t index ) {
  if ( index >= 8 ) return;
  LetterOffsets[index] -= 4;
  if ( LetterOffsets[index] < 0 ) LetterOffsets[index] = 0;
  if ( LetterOffsets[index] < 31 ) DecrementLetterOffsets( index+1 );
}
void DisplayMatchesBitmap ( int y, uint8_t MatchCount ) {
  uint8_t x;
  if ( MatchCount > 1 ) { 
    x = 8;
    display.drawBitmap( x+95, y+7+LetterOffsets[6], MatchesEMask, 13, 16, BLACK );
    display.drawBitmap( x+95, y+7+LetterOffsets[6], MatchesE,     13, 16, WHITE );
    display.drawBitmap( x+108, y+4+LetterOffsets[7], MatchesSMask, 12, 24, BLACK );
    display.drawBitmap( x+108, y+4+LetterOffsets[7], MatchesS,     12, 24, WHITE );
  }
  else x = 19;
  display.drawBitmap( x+30, y+LetterOffsets[1],   MatchesMMask, 22, 24, BLACK );
  display.drawBitmap( x+30, y+LetterOffsets[1],   MatchesM,     22, 24, WHITE );
  display.drawBitmap( x+51, y+5+LetterOffsets[2], MatchesAMask, 12, 16, BLACK );
  display.drawBitmap( x+51, y+5+LetterOffsets[2], MatchesA,     12, 16, WHITE );
  display.drawBitmap( x+63, y+1+LetterOffsets[3], MatchesTMask, 9, 24, BLACK );
  display.drawBitmap( x+63, y+1+LetterOffsets[3], MatchesT,     9, 24, WHITE );
  display.drawBitmap( x+72, y+3+LetterOffsets[4], MatchesCMask, 12, 16, BLACK );
  display.drawBitmap( x+72, y+3+LetterOffsets[4], MatchesC,     12, 16, WHITE );
  display.drawBitmap( x+82, y+2+LetterOffsets[5], MatchesHMask, 15, 24, BLACK );
  display.drawBitmap( x+82, y+2+LetterOffsets[5], MatchesH,     15, 24, WHITE );
  const unsigned char * Number;
  const unsigned char * NumberMask;
  switch ( MatchCount ) {
    case 1: Number = Matches1; NumberMask = Matches1Mask; break;
    case 2: Number = Matches2; NumberMask = Matches2Mask; break;
    case 3: Number = Matches3; NumberMask = Matches3Mask; break;
  }
  display.drawBitmap( x, y+LetterOffsets[0], NumberMask, 16, 24, BLACK );
  display.drawBitmap( x, y+LetterOffsets[0], Number, 16, 24, WHITE );
}
bool MatchesAnimation ( uint8_t MatchCount, int y ) {
  for ( uint8_t a = 0; a < 8; a++ ) LetterOffsets[a] = 45;
  ActivateButtonCD( );
  while ( LetterOffsets[7] > 0 ) {
    if ( display.buttonsState( ) && ButtonOffCD( ) ) {
      ActivateButtonCD( );
      return true;
    }
    display.clear( );
    DisplayAll( );
    DecrementLetterOffsets(0);
    DisplayMatchesBitmap( y, MatchCount );
    display.display( );
  }
  return false;
}
void DrawDeadHead( int x, int y ) {
  const unsigned char * Image = (unsigned char*)pgm_read_word(&(DeadFrames[ ((LetterOffsets[0]+y)/8)%8 ]));
  display.drawBitmap( x + ( WIDTH - (LetterOffsets[0])), y, DeadFrameMask, 16, 16, BLACK );
  display.drawBitmap( x + ( WIDTH - (LetterOffsets[0])), y, Image, 16, 16, WHITE );
}
void DisplayDeadHeadBitmap( ) {
  DrawDeadHead( 36-LetterOffsets[6], LetterOffsets[1] );
  DrawDeadHead( 48-LetterOffsets[5], LetterOffsets[2]  );
  DrawDeadHead( 72-LetterOffsets[4], LetterOffsets[3] );
  DrawDeadHead( 83-LetterOffsets[3], LetterOffsets[4]  );
  DrawDeadHead( 96-LetterOffsets[2], LetterOffsets[5] );
  DrawDeadHead( 104-LetterOffsets[1], LetterOffsets[6] );
}
bool DeadHeadAnimation ( ) {
  LetterOffsets[0] = 210;
  for ( uint8_t a = 1; a < 8; a++ ) LetterOffsets[a] = random(48);
  ActivateButtonCD( );
  while ( LetterOffsets[0] ) {
    if ( display.buttonsState( ) && ButtonOffCD( ) ) {
      ActivateButtonCD( );
      return true;
    }
    display.clear( );
    DisplayAll( );
    LetterOffsets[0]-=3;
    DisplayDeadHeadBitmap( );
    display.display( );
  }
  return false;
}

//Function by nootropic design to add high scores
void enterHighScore(byte file) {
  // Each block of EEPROM has 10 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file * 10 * 5;
  byte hi, lo;
  char initials[3];
  char text[16];
  char tmpInitials[3];
  unsigned int tmpScore = 0;

  // High score processing
  for (byte i = 0; i < 10; i++) {
    hi = EEPROM.read(address + (5*i));
    lo = EEPROM.read(address + (5*i) + 1);
    if ((hi == 0xFF) && (lo == 0xFF))
    {
      // The values are uninitialized, so treat this entry
      // as a score of 0.
      tmpScore = 0;
    } else
    {
      tmpScore = (hi << 8) | lo;
    }
    if (Score > tmpScore) {
      char index = 0;
      initials[0] = ' ';
      initials[1] = ' ';
      initials[2] = ' ';
      while (true) {  //    enterInitials();
        display.clear();
        display.setCursor(16,0); display.print(F("HIGH SCORE"));
        sprintf(text, "%u", Score);
        display.setCursor(88, 0); display.print(text);
        display.setCursor(56, 20); display.print(initials[0]);
        display.setCursor(64, 20); display.print(initials[1]);
        display.setCursor(72, 20); display.print(initials[2]);
        for (byte i = 0; i < 3; i++) display.drawLine(56 + (i*8), 27, 56 + (i*8) + 6, 27, 1);
        display.drawLine(56, 28, 88, 28, 0);
        display.drawLine(56 + (index*8), 28, 56 + (index*8) + 6, 28, 1);
        display.display(); 
        delay(150);
        if ( display.pressed(LEFT_BUTTON) || display.pressed(B_BUTTON)) {
          index--;
          if (index < 0) index = 0;
          else display.tunes.tone(1046, 250);
        }
        if (display.pressed(RIGHT_BUTTON)) {
          index++;
          if (index > 2) index = 2;
          else display.tunes.tone(1046, 250);
        }
        if (display.pressed(DOWN_BUTTON)) {
          initials[index]++;
          display.tunes.tone(523, 250);
          // A-Z 0-9 :-? !-/ ' '
          if (initials[index] == '0') initials[index] = ' ';
          if (initials[index] == '!') initials[index] = 'A';
          if (initials[index] == '[') initials[index] = '0';
          if (initials[index] == '@') initials[index] = '!';
        }
        if (display.pressed(UP_BUTTON)) {
          initials[index]--;
          display.tunes.tone(523, 250);
          if (initials[index] == ' ') initials[index] = '?';
          if (initials[index] == '/') initials[index] = 'Z';
          if (initials[index] == 31) initials[index] = '/';
          if (initials[index] == '@') initials[index] = ' ';
        }
        if (display.pressed(A_BUTTON)) {
          if (index < 2) {
            index++;
            display.tunes.tone(1046, 250);
          } else {
            display.tunes.tone(1046, 250);
            break;
          }
        }
      }
      
      for(byte j=i;j<10;j++) {
        hi = EEPROM.read(address + (5*j));
        lo = EEPROM.read(address + (5*j) + 1);
        if ((hi == 0xFF) && (lo == 0xFF)) tmpScore = 0;
        else tmpScore = (hi << 8) | lo;
        tmpInitials[0] = (char)EEPROM.read(address + (5*j) + 2);
        tmpInitials[1] = (char)EEPROM.read(address + (5*j) + 3);
        tmpInitials[2] = (char)EEPROM.read(address + (5*j) + 4);

        // write score and initials to current slot
        EEPROM.write(address + (5*j), ((Score >> 8) & 0xFF));
        EEPROM.write(address + (5*j) + 1, (Score & 0xFF));
        EEPROM.write(address + (5*j) + 2, initials[0]);
        EEPROM.write(address + (5*j) + 3, initials[1]);
        EEPROM.write(address + (5*j) + 4, initials[2]);

        // tmpScore and tmpInitials now hold what we want to
        //write in the next slot.
        Score = tmpScore;
        initials[0] = tmpInitials[0];
        initials[1] = tmpInitials[1];
        initials[2] = tmpInitials[2];
      }
      return;
    }
  }
}
boolean displayHighScores(byte file) {
  char text[16];
  char initials[3];
  unsigned int tScore;
  byte y = 10;
  byte x = 24;
  // Each block of EEPROM has 10 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file*10*5;
  byte hi, lo;
  display.clear();
  display.setCursor(32, 0);
  display.print(F("HIGH SCORES"));
//display.display();

  for(int i = 0; i < 10; i++)
  {
    sprintf(text, "%2d", i+1);
    display.setCursor(x,y+(i*8));
    display.print(text);
//  display.display();
    hi = EEPROM.read(address + (5*i));
    lo = EEPROM.read(address + (5*i) + 1);

    if ((hi == 0xFF) && (lo == 0xFF)) tScore = 0;
    else tScore = (hi << 8) | lo;

    initials[0] = (char)EEPROM.read(address + (5*i) + 2);
    initials[1] = (char)EEPROM.read(address + (5*i) + 3);
    initials[2] = (char)EEPROM.read(address + (5*i) + 4);

    if (tScore > 0)
    {
      sprintf(text, "%c%c%c %u", initials[0], initials[1], initials[2], tScore);
      display.setCursor(x + 24, y + (i*8));
      display.print(text);
//    display.display();
    }
  }
  display.display();
  ActivateButtonCD( );
  if ( ButtonOffCD && display.buttonsState() ) return true;
  return false;
}



