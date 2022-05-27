/*
 * 보드게임
 *
 * 2022.05.13
 *
 * [포트설정]
 * 1. 버튼 : D2
 * 2. 버저 : D8
 * 3. 회전 네오픽셀 : D3
 * 4. 플레이어 네오픽셀 : D4, D5, D6, D7
 *
 * [게임 프로세스]
 * 1. 게임 준비
 * 2. 플레이어 선정
 * 3. 기본 점수 세팅 (50점)
 * 4. 게임 시작
 * 5. 점수를 파란색 칸과 나머지 색상으로 표시 (100, 87, 34 - 3칸 파란, 1칸 (4))
 * 6. 점수가 0 이하면, 초록색으로 전체 표시 - 게임은 계속
 * 7. 점수가 100 이상이면, 빨간색으로 전체 표시 - 게임에서 제외됨
 * 8. 1명이 남을 때 까지 (나머지는 모두 아웃됨) 게임 진행
 * 9. 게임 종료
 * 10. 다시 하려면 버튼 누름
 * 
 */

#include "FastLED.h"  // 네오픽셀 제어하는 라이브러리
#include "pitches.h"  // 부저를 사용하기 위해서 , 음표

// 핀 설정
#define pBtn 2      // 아케이드 버튼
#define pLED4Roll 3 // 회전판 LED
#define pLED4P1 4   // 플레이어 1 LED
#define pLED4P2 5   // 플레이어 2 LED
#define pLED4P3 6   // 플레이어 3 LED
#define pLED4P4 7   // 플레이어 4 LED
#define pBuzzer 8   // 부저

#define nLEDs4Players 12 // 플레이어용 LED 개수
#define nLEDs4Roll 54    // 회전판 LED 개수
#define topLED 11        // 맨 꼭대기 LED 주소
#define nScoreLED 10     // 점수 LED 개수

#define maxPlayers 4 // 최대 플레이어 수 = 4명

#define PUSHED 3   // 스위치 눌림
#define RELEASED 4 // 스위치 놓임

#define IN 1  // 플레이어가 게임에 참여
#define OUT 0 // 플레이어가 게임에 미참여

#define C_BLUE 0x0000ff // 진한 빨강


// 네오픽셀 선언
// 플레이어용 네오픽셀 (4명, 12개씩)
CRGB player[maxPlayers][nLEDs4Players];
// 회전판용 네오픽셀 (54개)
CRGB rolls[nLEDs4Roll];

// 버튼이 눌리는 것을 확실하게 알 수 있는 방법
int buttonState;
int old_btnValue = HIGH;

unsigned long lastDebounceTime = 0; //
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers

// 구슬이 몇바퀴 돌다가 멈추는 것을 표현
// 멈출 LED 위치 랜덤 발생 (0~53)
unsigned long randomPosition = 0;

// 멈출 위치 랜덤 발생 = 바퀴수(랜덤) + randomPosition
unsigned long nRound = 0;

// 멈춘 칸의 위치 (0~17)
unsigned int stopPosition = 0;

// 멈춘 칸에 설정된 값을 어레이(배열)로 설정
//int pointValue[18] = {10, -25, 15, -20, 15, 15, 25, -5, 20, -20, 20, -20, 20, -10, 10, -10, -20, 15};
int pointValue[18] = {32, -51, 34, -55, 36, -37, 38, -53, 32, -33, 35, -32, 51, -39, 38, -37, 53, -30};

// 플레이어의 점수 : 모두 50점 시작
// int pScore[4] = {50, 50, 50, 50};
int pScore[4] = {54, 25, 71, 68};
int qValue = 0;
int rValue = 0;

// 이전 점수 저장하는 변수
int old_Score = 0;
int old_qValue = 0;
int old_rValue = 0;

// 기본 플레이어 (1,3번 - 대각선)
// 플레이어가 플레이중에 있어야 회전판 적용
int pPlaying[4] = {IN, OUT, IN, OUT};

// 10단계 색상 0xRRGGBB (RGB 색상값) - 16진수 (0~F)
uint32_t scoreColor[11] = {0x000000, 0x663333, 0x666633, 0x669933, 0x55eb2f, 0x008888, 0x004d40, 0x01579B, 0x000022, 0x000066, C_BLUE};
// 버튼 대기중에 회전하는 색깔
uint32_t tempColor = 0x202000;
// 버튼 대기에 필요한 변수
int tempRoll = 0;

void setup()
{
  Serial.begin(115200);

  // 네오픽셀 연결하기
  FastLED.addLeds<NEOPIXEL, pLED4P1>(player[0], nLEDs4Players);
  FastLED.addLeds<NEOPIXEL, pLED4P2>(player[1], nLEDs4Players);
  FastLED.addLeds<NEOPIXEL, pLED4P3>(player[2], nLEDs4Players);
  FastLED.addLeds<NEOPIXEL, pLED4P4>(player[3], nLEDs4Players);

  FastLED.addLeds<NEOPIXEL, pLED4Roll>(rolls, nLEDs4Roll);

  pinMode(pBtn, INPUT_PULLUP);
  pinMode(pBuzzer, OUTPUT);
}

void loop()
{ 
  // 0. 사용되는 변수들 초기화 작업
  // 모든 LED 끄기
  FastLED.clear();
  FastLED.show();

  // 게임에 사용된 값 초기화
  // 플레이어의 점수 : 모두 50점 시작
  for (int i = 0; i < maxPlayers; i++)
  {
    pScore[i] = 50;
  }

  qValue = 0;
  rValue = 0;

  // 기본 플레이어 (1,3번 - 대각선)
  // 플레이어가 플레이중에 있어야 회전판 적용
  pPlaying[0] = IN;
  pPlaying[1] = OUT;
  pPlaying[2] = IN;
  pPlaying[3] = OUT;

  delay(1000);

  Serial.println("<<<< GAME START >>>>");
  sound_Open();

  // 0. 게임에 참여할 플레이어 결정하기
  setPlayers();
  delay(1000);

  // 0. 처음 점수 표시하기
  for (int x = 0; x < maxPlayers; x++)
  {
    showScore(x);
  }

  delay(1000);

  // 언제까지 플레이를 할 것인가?
  // 한 사람만 남을 때 까지
  while (pPlaying[0] + pPlaying[1] + pPlaying[2] + pPlaying[3] != 1)
  {
    // 플레이어의 수 만큼 반복 : 버튼을 눌러서 난수발생 -> 점수 추가 -> 표시 -> 다음 턴
    for (int x = 0; x < maxPlayers; x++)
    {
      // 플레잉중인 플레이어일 참여하고 있는 경우만 적용
      if (pPlaying[x] == IN)
      {        
        // 1. 현재 플레이어(x)의 차례가 되어 순서표시 LED가 켜진다. (색상은 0xRRGGBB, 0x330033 = 연보라)
        player[x][topLED] = CRGB(0x330033);
        FastLED.show();
        sound_Next();
        delay(500);

        // 2. 버튼을 누를 때 까지 회전판 LED를 빙글빙글 돌고 있다.
        tempRolling();
        sound_BtnPush();
        // 3. 버튼이 눌려져서 위의 while 문을 빠져 나오면, 랜덤값을 발생시키고, 해당 위치에 멈춘다.
        letsRoll(); // 랜덤값 위치까지 LED 이동하기

        // 4. 멈춘 위치에 해당하는 값을 플레이어의 점수에 더하기
        old_Score = pScore[x]; // 점수 업데이트 되기 전에 oldScore 에 저장해 놓음
        pScore[x] = pScore[x] + pointValue[stopPosition];

        Serial.println((String) "======== Player " + x + "(" + old_Score + ") : get " + pointValue[stopPosition] + " = " + pScore[x]);

        delay(500);

        // 5. 점수를 새로 표시 하기 위해 원래 켜져 있던 등을 한 번에 끄고
        for (int i = nScoreLED - 1; i >= 0; i--)
        {
          player[x][i] = CRGB::Black;
        }
        FastLED.show();

        delay(500);

        // 5-1. 기존에 있던 점수를 한 번에 표시한 뒤
        if (old_Score > 0) // 47점 이라면
        {   
          old_qValue = old_Score / 10;  // 몫은 4 (LED는 4개가 파란색 - 0,1,2,3번 LED가 파란색)
          old_rValue = old_Score % 10;
          for (int i = 0; i < old_qValue; i++)  // 0부터 3까지(old_qvalue(4)보다 작을 때 까지)
          {
            player[x][i] = CRGB(scoreColor[10]); // 제일 파란색으로 켜고
          }
          player[x][old_qValue] = CRGB(scoreColor[old_rValue]); //4번째 LED(player[x][old_qVlaue] 칸)은 나머지(7)에 해당하는 색으로 켠다.
        }
        else
        {
          for (int i = 0; i < nScoreLED; i++)
          {
            player[x][i] = CRGB(0x005500); // 초록색
          }
        }
        FastLED.show();

        delay(1000);

        // 6. 업데이트 된 점수를 반영하여 표시한다.
        // 점수를 10으로 나눈 몫(qValue)과 나머지(rValue)
        qValue = pScore[x] / 10;
        rValue = pScore[x] % 10;

        // 6-1. pScore가 99보다 크면 환경파괴로 아웃 : 빨간불 전체 점등
        if (pScore[x] > 99)
        {
          // LED를 빨간색으로 채운다.
          for (int i = 0; i < nScoreLED; i++)
          {
            player[x][i] = CRGB(0x550000); // 적당한 밝기의 빨간색
            FastLED.show();
            sound_ScoreDown();
            delay(100);
          }

          sound_AllRed();
          // 게임에서 아웃시킨다.
          pPlaying[x] = OUT;
          // 시리얼 모니터에 아웃된 플레이어와 남아있는 플레이어 표시
          Serial.println((String) "Player " + x + " OUT! ");
          Serial.print((String) "Remaining Players are ");
          for (int y = 0; y < maxPlayers; y++)
          {
            if (pPlaying[y] == IN)
              Serial.print((String)y + ", ");
          }
          Serial.println();

          // 게임 플레이어가 1명만 남으면 게임 끝
          if (pPlaying[0] + pPlaying[1] + pPlaying[2] + pPlaying[3] == 1)
            break;
        }
        // 6-2. pScore가 1보다 작으면 환경보호로 : 초록불 전체 점등
        else if (pScore[x] < 1)
        {
          // LED를 초록색으로 채운다.
          for (int i = 0; i < nScoreLED; i++)
          {
            player[x][i] = CRGB(0x005500); // 적당한 밝기의 초록색
            FastLED.show();
            sound_ScoreUp();
            delay(100);
          }
        }
        // 6-3. 보통의 경우, 점수가 올랐을 때 양수에서 양수로
        else if (pScore[x] > old_Score)
        {          
          // 몫 만큼의 LED를 짙은 파란색으로 채우고
          for (int i = old_qValue; i < qValue; i++)
          {
            player[x][i] = CRGB(scoreColor[10]); // 제일 파란색
            FastLED.show();
            sound_ScoreUp();
            delay(200);
          }
          // 그 윗칸을 나머지값을 채운다.
          player[x][qValue] = CRGB(scoreColor[rValue]);
          FastLED.show();
          sound_ScoreUp();
          delay(200);

          // 그 나머지 구간이 초록색일 경우는 다 지운다.
          if(old_Score <= 0)
          {
            for(int i = qValue+1; i<nScoreLED; i++)
            {
              player[x][i] = CRGB(scoreColor[0]); // 검정색(꺼진다)
              FastLED.show();
              sound_ScoreUp();
              delay(200);
            }            
          }
        }
        // 6-4. 보통의 경우, 점수가 내려갔을 때
        else
        {
          // 작아진 부분은 지우고
          for (int i = old_qValue; i > qValue; i--)
          {
            //Serial.println((String)"old_qValue = " + old_qValue + "\t qValue = " + qValue);
            player[x][i] = CRGB(scoreColor[0]); // 검정색
            FastLED.show();
            sound_ScoreDown();
            delay(200);
          }
          // 그 윗칸을 나머지값을 채운다.
          player[x][qValue] = CRGB(scoreColor[rValue]);
          FastLED.show();
          sound_ScoreDown();
        }
        // 1초 대기 후 플레이어 표시 LED 끄고 다음 턴으로 넘어가기
        delay(1000);

        player[x][topLED] = CRGB::Black;
        rolls[randomPosition - 1] = CRGB::Black;
        FastLED.show();
        delay(500);
      }
      // 플레이어마다 게임 끝
      Serial.println((String) "> Player" + x + " Finished, Next Turn < ");

      // 회전판 끄기
      for (int i = 0; i < nLEDs4Roll; i++)
      {
        rolls[i] = CRGB(0x000000);
      }
      FastLED.show();
    }
    // 플레이어 수가 1이 될 때 까지 반복중인 루프
    Serial.println("> Next Round < ");
  }

  Serial.println("> Game Over < ");  
  // 한 사람 남았을 때 게임은 끝이 나고
  // 회전판 전체 켜기
  for (int i = 0; i < nLEDs4Roll; i++)
  {
    rolls[i] = CRGB(0x003333);
  }
  FastLED.show();
  sound_Close();

  // 게임 끝
  // 다시 하려면 버튼 누르기
  while (!isBtnPushed()){}
  sound_BtnPush();
  // 다시 떨어질 때 까지 기다렸다가
  while (isBtnPushed()){}

  Serial.println("----------ReStart-----------");

}

//////////////////////////////////////////////////////////////////////
//
// 필요한 함수
//
/////////////////////////////////////////////////////////////////////

void sound_PlayerIn()
{
  tone(pBuzzer,NOTE_E5,125);
  delay(130);
  tone(pBuzzer,NOTE_G5,125);
  delay(130);
  tone(pBuzzer,NOTE_E6,125);
  delay(130);
  noTone(pBuzzer);
}

void sound_BtnPush()
{
  tone(pBuzzer,NOTE_G4,35);
  delay(35);
  tone(pBuzzer,NOTE_G5,35);
  delay(35);
  tone(pBuzzer,NOTE_G6,35);
  delay(35);
  noTone(pBuzzer);
}

void sound_ScoreUp()
{
  tone(pBuzzer, NOTE_G4, 30);
  delay(30);
  noTone(pBuzzer);
}

void sound_ScoreDown()
{
  tone(pBuzzer, NOTE_A3, 30);
  delay(30);
  noTone(pBuzzer);
}

void sound_Open()
{
  for (int i = 100; i < 1500; i = i * 1.4) 
  {
    tone(pBuzzer, i, 60);
    delay(60);
  }
}

void sound_Phone()
{
  for (int j = 1; j < 4; j++) 
  {
    for (int i = 1; i < 25; i++) 
    {
      tone(pBuzzer, NOTE_C5, 33);
      delay(33);
      tone(pBuzzer, NOTE_E5, 27);
      delay(27);
    }  
  }
}

void sound_Close()
{
  tone(pBuzzer, NOTE_C4, 250);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_G3, 125);    delay(165); noTone(pBuzzer);
  tone(pBuzzer, NOTE_G3, 125);    delay(165); noTone(pBuzzer);
  tone(pBuzzer, NOTE_A3, 250);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_G3, 250);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, 0, 250);          delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_B3, 250);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_C4, 250);    delay(325); noTone(pBuzzer);
}

void sound_AllGreen()
{
  tone(pBuzzer, NOTE_C4, 250);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_E3, 125);    delay(165); noTone(pBuzzer);
  tone(pBuzzer, NOTE_G3, 125);    delay(165); noTone(pBuzzer);
  tone(pBuzzer, NOTE_C5, 250);    delay(325); noTone(pBuzzer);  
}

void sound_AllRed()
{  
  tone(pBuzzer, NOTE_D3, 300);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_A2, 300);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_G2, 300);    delay(325); noTone(pBuzzer);
  tone(pBuzzer, NOTE_A1, 800);    delay(1000); noTone(pBuzzer);   
}

void sound_Next()
{
  tone(pBuzzer, NOTE_E4, 125);    delay(125); noTone(pBuzzer);
  tone(pBuzzer, NOTE_G4, 125);    delay(125); noTone(pBuzzer);
  tone(pBuzzer, NOTE_C5, 125);    delay(125); noTone(pBuzzer);
}

// 게임에 참여할 플레이어 결정하기
void setPlayers()
{
  Serial.println("JOIN IN");

  // 기본으로 참여하는 0번 2번 플레이어는 표시
  player[0][topLED] = CRGB(0x330033);
  player[2][topLED] = CRGB(0x330033);

  // 버튼 눌렀을 때 각각의 플레이어 변환
  for (int i = 0; i < nLEDs4Roll; i++)
  {
    rolls[i] = CRGB(0x330000);
    FastLED.show();

    for (long j = 0; j < 40000; j++)
    {
      // 버튼이 눌리면,
      if (!digitalRead(pBtn))
      {
        delay(5);
        if (!digitalRead(pBtn))
        {	
          sound_PlayerIn();
          // 다시 떨어질 때 까지 기다렸다가
          while (!digitalRead(pBtn)){}

          Serial.println("Click!");

          // 플레이어 추가/제거
          if (pPlaying[1] == OUT && pPlaying[3] == OUT)
          {
            pPlaying[1] = IN;
            player[1][topLED] = CRGB(0x330033);
            pPlaying[3] = OUT;
            player[3][topLED] = CRGB::Black;
          }
          else if (pPlaying[1] == IN && pPlaying[3] == OUT)
          {
            pPlaying[1] = IN;
            player[1][topLED] = CRGB(0x330033);
            pPlaying[3] = IN;
            player[3][topLED] = CRGB(0x330033);
          }
          else if (pPlaying[1] == IN && pPlaying[3] == IN)
          {
            pPlaying[1] = OUT;
            player[1][topLED] = CRGB::Black;
            pPlaying[3] = IN;
            player[3][topLED] = CRGB(0x330033);
          }
          else if (pPlaying[1] == OUT && pPlaying[3] == IN)
          {
            pPlaying[1] = OUT;
            player[1][11] = CRGB::Black;
            pPlaying[3] = OUT;
            player[3][11] = CRGB::Black;
          }
        }
        FastLED.show();
      }
    }
  }

  // 완료되었음을 파란색으로 한 바퀴 표시
  just1Roll(0x000033);

  // 모든 LED 끄기
  FastLED.clear();
  FastLED.show();
}

// 점수 표시하기
void showScore(int x)
{
  // 플레잉중인 플레이어일 참여하고 있는 경우만 적용
  if (pPlaying[x] == IN)
  {
    // 점수를 10으로 나눈 몫(qValue)과 나머지(rValue)
    qValue = pScore[x] / 10;
    rValue = pScore[x] % 10;

    // 몫 만큼의 LED를 짙은 파란색으로 채우고
    for (int i = 0; i < qValue; i++)
    {
      player[x][i] = CRGB(scoreColor[10]); // 제일 파란색
      FastLED.show();
      sound_ScoreUp();
      delay(100);
    }
    // 그 윗칸을 나머지값에 10을 곱한 파란색으로 채운다.
    player[x][qValue] = CRGB(scoreColor[rValue]);
    FastLED.show();
    sound_ScoreUp();
  }
}

// 회전판 돌리기 전에 임시로 돌아가기
void tempRolling()
{
  tempRoll = 0;

  while (digitalRead(pBtn))
  {
    rolls[tempRoll] = CRGB(tempColor);
    FastLED.show();
    rolls[tempRoll] = CRGB::Black;
    delay(5);
    FastLED.show();
    tempRoll++;
    if (tempRoll == nLEDs4Roll)
      tempRoll = 0;
  }
}

// 회전판 한바퀴 돌리기 함수
void just1Roll(uint32_t _color)
{
  for (int i = 0; i < nLEDs4Roll; i++)
  {
    rolls[i] = CRGB(_color);
    FastLED.show();
    rolls[i] = CRGB::Black;
    delay(5);
  }
  FastLED.show();
}

// 회전판 돌리기 함수
void letsRoll()
{
  // 랜덤값 발생
  randomSeed(millis()); // 난수발생의 씨앗. 
  nRound = random(2, 5);
  randomPosition = random(0, nLEDs4Roll - 1);
  stopPosition = int(randomPosition * 1.0 / 3.0);

  for(int i=0; i<nRound; i++)
  {
    just1Roll(CRGB::Green);
  }

  for (int i = 0; i < randomPosition; i++)
  {
    rolls[i] = CRGB::Green;
    FastLED.show();
    rolls[i] = CRGB::Black;
    delay(50);
  }

  for (int i = 0; i < 3; i++)
  {
    rolls[randomPosition] = CRGB::Green;
    FastLED.show();
    delay(300);
    rolls[randomPosition] = CRGB::Black;
    FastLED.show();
    delay(300);
  }
  rolls[randomPosition] = CRGB::Green;
  FastLED.show();
}

// 버튼이 눌렸는지 확인해 주는 함수
bool isBtnPushed()
{
  if (!digitalRead(pBtn))
  {
    delay(5);
    if (!digitalRead(pBtn))
      return true;
  }

  return false;
}
