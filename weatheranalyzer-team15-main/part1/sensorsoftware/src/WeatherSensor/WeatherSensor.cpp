/*
  WeatherSensor - Library for bogus weather data
  Created by Joseph Espy, March 26th, 2020
  Modified by Pablo Frank Bolton, March 26th, 2021
  Released into the public domain.
*/

#include "WeatherSensor.h"
#include "limits.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

// Taken from Creative Commons
// https://en.wikipedia.org/wiki/Marsaglia_polar_method
double generateGaussian(double mean, double stdDev) {
  // for debugging
  //printf("generateGaussian(double mean, double stdDev) \n");
  static double spare;
    static bool hasSpare = false;

    if (hasSpare) {
        hasSpare = false;
        return spare * stdDev + mean;
    } else {
        double u, v, s;
        do {
            u = (rand() / ((double)RAND_MAX)) * 2.0 - 1.0;
            v = (rand() / ((double)RAND_MAX)) * 2.0 - 1.0;
            s = u * u + v * v;
        } while (s >= 1.0 || s == 0.0);
        s = sqrt(-2.0 * log(s) / s);
        spare = v * s;
        hasSpare = true;
        return mean + stdDev * u * s;
    }
}

WeatherSensor::WeatherSensor(long seed) {
  // for debugging
  //printf("WeatherSensor::WeatherSensor(long seed) \n");
  srand((unsigned)seed);
  nhours = seed;
  pressureState = 2;
}

int WeatherSensor::readNextHour(weatherData_t *datum) {
  // for debugging
  //printf("WeatherSensor::readNextHour( weatherData_t *datum ) \n");
  nhours += 1;
  updateTime(datum->dateTime);

  // 4 random bits for use in a few places
  int r = (rand() % (16 +1) ) ;
  bool observedRained = r & 1;
  r = r >> 1;

  // tmp is negative cos daily with noise
  float time = (nhours % 24) / 24.0;
  double tmp =
      -100.0 * cos(time * 3.14 * 2.0) + 128.0 + generateGaussian(0.0, 10.0);
  tmp = (tmp > 0 && tmp < 255) ? tmp : 128.0;
  datum->temperature = tmp;

  // humidity is sin(2x) with noise
  double hum =
      -100.0 * sin(time * 3.14 * 4.0) + 128.0 + generateGaussian(0.0, 10.0);
  hum = (hum > 0 && hum < 255) ? hum : 128.0;
  datum->humidity = hum;

  // pressure is a markov model, with 1/8 chance to switch to adjacent state
  // fluctuates in {1,2,3,4}
  if (r == 1 && pressureState < 4)
    pressureState += 1;
  if (r == 2 && pressureState > 1)
    pressureState -= 1;
  datum->pressure = pressureState * 50 + 28 + generateGaussian(0.0, 10.0);

  // 1 is no-rain, 2 is rain, 0 is no data
  unsigned char watermark = generateGaussian(128.0, 20.0);
  if (observedRained) {
    datum->rained =
        1 + (datum->humidity > watermark && datum->temperature > watermark &&
             datum->pressure > watermark);
  } else {
    datum->rained = 0;
  }

  // for debugging
  // printf("ws: temp: %u \n",datum->temperature);
  // printf("ws: pres: %u \n",datum->pressure);
  // printf("ws: humi: %u \n",datum->humidity);
  // printf("ws: rain: %u \n",datum->rained);
  // printf("ws: time: %s \n",datum->dateTime);

  return 0;
}

// neglects to deal with leap year
void WeatherSensor::updateTime(char *dateTime) {
  // for debugging
  //printf("WeatherSensor::updateTime(char *dateTime) \n");
  long hours = nhours % 24;
  long days = nhours / 24;
  long month = 0;
  long year = 0;
  int daysIn[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  year = days / 365;
  days %= 365;

  while (days > daysIn[month]) {
    days -= daysIn[month];
    month += 1;
    month %= 12;
  }

  month += 1;
  days += 1;

  dateTime[0] = (year / 1000) % 10 + '0';
  dateTime[1] = (year / 100) % 10 + '0';
  dateTime[2] = (year / 10) % 10 + '0';
  dateTime[3] = year % 10 + '0';

  dateTime[4] = month / 10 + '0';
  dateTime[5] = month % 10 + '0';

  dateTime[6] = (days / 10) + '0';
  dateTime[7] = (days % 10) + '0';

  dateTime[8] = hours / 10 + '0';
  dateTime[9] = hours % 10 + '0';

  dateTime[10] = '0';
  dateTime[11] = '0';

  dateTime[12] = 0;
}
