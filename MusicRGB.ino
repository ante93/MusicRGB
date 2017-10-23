#define LOG_OUT 1 // use the log output function
#define FFT_N 16 // set to 256 point fft
#define AVRG_SIZE 10 //size of rolling average array

#include <FFT.h>
//function for calculating rolling minimum element
void calcMin(int input, int* minel) {
  if (fft_log_out[0] < *minel) {
    *minel = fft_log_out[0];
  }
}
//function for handling LED outputs
void ledControl(int input, float average, int margin, int led) {
  if ((input - average) <= margin) { //if input is significally lower than average, peak detected
    digitalWrite(led, HIGH);
  }
  else
    digitalWrite(led, LOW);
}

//function for calculating rolling average of each frequency bin
float rollingAverage(float *store, int siz, float entry)
{
  float ledVal1 = 0;
  int l;
  float total = 0;
  float result;

  for (l = 0; l < siz - 1; l++)
  {
    store[l] = store[l + 1];
    total += store[l];
  }
  store[siz - 1] = entry;
  total += entry;
  result = total / (float)siz;

  return result;
}
// variables for calculatng rolling average for each frequency bin
float average_array0[AVRG_SIZE] = {0};
float average0 = 0;

float average_array4[AVRG_SIZE] = {0};
float average4 = 0;

float average_array7[AVRG_SIZE] = {0};
float average7 = 0;

//led outputs
int led1 = A2;
int led2 = A3;
int led3 = A4;

//variables for storing rolling minimum values of frequency bins
int minElement0 = 1000;
int minElement4 = 1000;
int minElement7 = 1000;

void setup() {
  //define output LEDs
  pinMode( A2, OUTPUT);
  pinMode( A3, OUTPUT);
  pinMode( A4, OUTPUT);
  pinMode( A1, INPUT);
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
  analogReference(INTERNAL); //using 1V analog reference
}

void loop() {
  while (1) {
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 32 ; i += 2) { // save 256 samples
      while (!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i + 1] = 0; // set odd bins to 0
    }
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();

    //calculate minimum value of each bin, used to determine margin in peak detection
    calcMin(fft_log_out[0], &minElement0);
    calcMin(fft_log_out[4], &minElement4);
    calcMin(fft_log_out[7], &minElement7);

    //calculate average value based on previous AVRG_SIZE elements
    average0 = rollingAverage(average_array0, AVRG_SIZE, fft_log_out[0]);
    average4 = rollingAverage(average_array4, AVRG_SIZE, fft_log_out[4]);
    average7 = rollingAverage(average_array7, AVRG_SIZE, fft_log_out[7]);

    //with determined frequency value of each bin and margin activate output LEDs
    ledControl(fft_log_out[0], average0, (minElement0 - average0)/3, led3);
    ledControl(fft_log_out[4], average4, (average4 - minElement4)/5, led2);
    ledControl(fft_log_out[7], average7, (average7 - minElement7)/4, led1);

  }
}
