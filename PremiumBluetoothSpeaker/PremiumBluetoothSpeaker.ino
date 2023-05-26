#include <LiquidCrystal_I2C.h>
#include "BluetoothA2DPSink.h"
#include <Audio.h>
#include "Arduino.h"
#include "SD.h"
#include "FS.h"

//SD Card
#define SD_CS 5
#define SPI_SCK 18
#define SPI_MOSI 23
#define SPI_MISO 19

//Digital I/O used  //Makerfabs Audio V2.0
#define I2S_DOUT 26
#define I2S_BCLK 27
#define I2S_LRC 25

//POTENTIOMETERS
#define VOLUME_KNOB 34
#define EQUAL_HIGH 35
#define EQUAL_MID 32
#define EQUAL_LOW 33

// MEDIA CONTROL
#define PIN_MODE 16
#define PIN_PREVIOUS 0
#define PIN_PLAY 4
#define PIN_NEXT 17

int pin_mode = 1;
int pin_previous = 1;
int pin_play = 1;
int pin_next = 1;

Audio audio;
BluetoothA2DPSink a2dp_sink;

// structure to store song paths
String file_list[20];
int file_num = 0;    // number of files
int file_index = 0;  // index of current music file

// structure to store current song info
struct Music {
  String name;
  int mode;  // 0 = bluetooth, 1 = sd card
  int length;
  int runtime;
  int volume;
  int status;
  int mute_volume;
  bool is_playing;
} music = { "", 0, 0, 0, 0, 0, 0, false };

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

void setup() {
  // SERIAL FOR DEBUG
  Serial.begin(115200);

  // INITIALIZE LCD
  lcd.init();

  // turn on LCD backlight
  lcd.backlight();

  // Control buttons
  pinMode(PIN_MODE, INPUT_PULLUP);
  pinMode(PIN_PREVIOUS, INPUT_PULLUP);
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);

  // SET I2S PINS
  i2s_pin_config_t my_pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // SD(SPI) set up, the rest is tucked in startSD()
  pinMode(SD_CS, OUTPUT);

  // START BLUETOOTH AS DEFAULT
  a2dp_sink.set_pin_config(my_pin_config);
  startBluetooth();

  // open_new_song(file_list[file_index]);
  // Audio(I2S)
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);  // 0...21
}

void loop() {
  // audio.loop();
  // mode_button();
  play_button();
  next_button();
  previous_button();
  volume_knob();
  delay(50);
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void volume_knob() {
  int analogValue = analogRead(34); // GIOP34
  float volume = floatMap(analogValue, 0, 4095, 0, 60);
  a2dp_sink.set_volume((int)volume);
  // Serial.println(volume);
}

void startBluetooth() {
  Serial.println("Start Bluetooth");
  a2dp_sink.start("Preproduction Bluetooth Speaker");
  music.mode = 0;
  music.is_playing = false;
  display("BLUETOOTH", 0);
}

void display(String message, int line) {
  lcd.clear();
  lcd.setCursor(line, 0);
  lcd.print(message);
}

void mode_button() {
  // read the state of the switch/button:
  int currentState = digitalRead(PIN_MODE);

  if (pin_mode == HIGH && currentState == LOW) {  // play button is pressed
    Serial.println("PRESSED MODE");
    if (music.mode == 0) {  // is in Bluetooth
      a2dp_sink.end();      // stop bluetooth
      startSD();
      Serial.println("SD");
    } else {  // is in SD card
      audio.pauseResume();
      startBluetooth();
      music.mode = 0;  // set mode to Bluetooth
      Serial.println("Bluetooth");
    }
  }
  // save the last state
  pin_mode = currentState;
}

int startSD() {
  // START SD CARD AS DEFAULT
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1000000);

  if (!SD.begin(SD_CS, SPI)) {
    Serial.println(F("Card Mount Failed"));
    lcd.setCursor(0, 0);
    lcd.print(F("Card Mount Failed"));
  } else {
    Serial.println(F("Card Mount Successful"));
    //Read SD
    file_num = get_music_list(SD, "/", 0, file_list);
    Serial.print(F("Music file count:"));
    Serial.println(file_num);
    Serial.println("All music:");
    for (int i = 0; i < file_num; i++) {
      Serial.println(file_list[i]);
    }
  }
  // start new song from sd card
  music.mode = 1;  // set mode to SD card
  open_new_song(file_list[file_index]);
  return true;
}

void open_new_song(String filename) {
  Serial.print("Opening song: ");
  Serial.println(filename);

  const int length = filename.length();
  // declaring character array (+1 for null terminator)
  char *char_array = new char[length + 1];
  // copying the contents of the
  // string to char array
  strcpy(char_array, filename.c_str());

  Serial.println("Copied string");
  // start the song
  audio.connecttoFS(SD, char_array);
  Serial.println("Started song");
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print song name
  lcd.print(filename.substring(1, filename.indexOf(".")));
  // debug
}

void previous_button() {
  // read the state of the switch/button:
  int currentState = digitalRead(PIN_PREVIOUS);

  // if(pin_play == HIGH && currentState == LOW) { // play button is pressed
  if (pin_previous == HIGH && currentState == LOW) {  // play button is pressed
    Serial.println("PRESSED NEXT");
    if (music.mode == 0) {  // is in bluetooth
      Serial.println("IS IN BLUETOOTH");
      a2dp_sink.previous();
      Serial.println("PREVIOUS");
    } else {  // is in SD card
      // I don't know what to put here
    }
  }
  // save the last state
  pin_previous = currentState;
}

void next_button() {
  // read the state of the switch/button:
  int currentState = digitalRead(PIN_NEXT);

  if (pin_next == HIGH && currentState == LOW) {  // play button is pressed
    Serial.println(F("PRESSED NEXT"));
    if (music.mode == 0) {  // is in bluetooth
      Serial.println(F("IS IN BLUETOOTH"));
      a2dp_sink.next();
      Serial.println(F("NEXT"));
    } else {  // is in SD card
      // I don't know what to put here
    }
  }
  // save the last state
  pin_next = currentState;
}

void play_button() {  // works perfectly now
  // read the state of the switch/button:
  int currentState = digitalRead(PIN_PLAY);

  if (pin_play == HIGH && currentState == LOW) {  // play button is pressed
    Serial.println(F("PRESSED"));
    if (music.mode == 0) {  // is in bluetooth
      Serial.println(F("IS IN BLUETOOTH"));
      if (music.is_playing) { // is playing
        a2dp_sink.pause();
        music.is_playing = false;
        Serial.println(music.is_playing);
      } else { // is not playing

        a2dp_sink.play();
        music.is_playing = true;
        Serial.println(music.is_playing);
      }
    } else {  // is in SD card
      if (music.is_playing) {
        audio.pauseResume();
        music.is_playing = false;
      } else {
        audio.pauseResume();
        music.is_playing = true;
      }
    }
  }
  pin_play = currentState;
}

int get_music_list(fs::FS &fs, const char *dirname, uint8_t levels, String wavlist[30]) {
  Serial.printf("Listing directory: %s\n", dirname);
  int i = 0;

  File root = fs.open(dirname);
  if (!root) {
    Serial.println(F("Failed to open directory"));
    return i;
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
    return i;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
    } else {
      String temp = file.name();
      if (temp.endsWith(".wav")) {
        wavlist[i] = temp;
        i++;
      } else if (temp.endsWith(".mp3")) {
        wavlist[i] = temp;
        i++;
      }
    }
    file = root.openNextFile();
  }
  return i;
}