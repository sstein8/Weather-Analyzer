
#include "src/WeatherSensor/WeatherSensor.h"

/// for fake Arduino
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <chrono>
#include <iostream>
#include <cstring>

// defines
#define ERROR(x) \
    do { \
        perror(x); \
        ret = -1; \
        goto done; \
    } while (0)

// Function Prototypes
int init_tty(int fd);
int main_loop(int fd);
int send_msg(const char *msg, int len, int fd);
void flipLED();
void handleRequest(int fd);
void handleBlinkState();

WeatherSensor ws(17703959l); //2021-01-01-00:00

/*
 * - BLink is sent with an argument, and blinks that many times quickly
 * - Request sends a 16 byte response to the user
 * - Pause stops any LED blinking
 * - Resume unpauses system
 */
enum message {
    BLINK   = '1',
    REQUEST = '2',
    PAUSE   = '3',
    RESUME  = '4',
};

bool isPaused = false;
bool hasSentBlink = false;
bool hasDeadline = false;
int fastBlinksLeft = 0;
unsigned long blinkDeadline;
//int ledState = LOW;
int ledState = 0; /// fake Arduino 
auto start = std::chrono::system_clock::now();

// main
int
main(int argc, char **argv) {
    int fd;
    std::string device = "/dev/ttyACM0";
    int ret;

   
    /*
     * Read the device path from input,
     * or default to /dev/ttyACM0
     */
    if (argc == 2) {
        device = argv[1];
    }

    printf("Device is : %s\n", device.c_str());

    /*
     * Need the following flags to open:
     * O_RDWR: to read from/write to the devices
     * O_NOCTTY: Do not become the process's controlling terminal
     * O_NDELAY: Open the resource in nonblocking mode
     */
    fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error opening serial");
        return -1;
    }

    /* Configure settings on the serial port */
    if (init_tty(fd) == -1) {
        ERROR("init");
    }
    
    ret = main_loop(fd);

done:
    close(fd);
    return ret;
}

int
main_loop(int fd) {
    start = std::chrono::system_clock::now();

    while(1){
      handleBlinkState();    
      int count;
      char buf[2];
      buf[1] = '\0';                  

      count = read(fd, &buf, 2);
      // if there is nothing to read, restart loop
      if (count == 0) { 
        continue;
      }

      printf("\nGOT A Message: %s\n", buf);
      fflush(stdout);

      switch (buf[0]) { 
          case BLINK:
            printf("Switch is BLINK\n");
            hasSentBlink = true;
            break;
          case REQUEST:
            printf("Switch is REQUEST\n");
            handleRequest(fd);
            break;
          case PAUSE:
            printf("Switch is PAUSE\n");
            isPaused = true;
            break;
          case RESUME:
            printf("Switch is RESUME\n");
            isPaused = false;
            break;
      }

    }

}


void flipLED() {
  ledState = !ledState;
  //digitalWrite(LED_BUILTIN, ledState);/// fake Arduino
}

// write a 16 byte message back over serial
void handleRequest(int fd) {
  weatherData_t datum;
  // for debugging
  // weatherData_t datum = { .temperature = '\1', 
  //                         .pressure = '\2', 
  //                         .humidity = '\3', 
  //                         .rained = '\1', 
  //                         .dateTime = "202103261200" };



  // for debugging
  //printf("Getting Datum \n");
  ws.readNextHour(&datum);


  printf("temp: %u \n",datum.temperature);
  printf("pres: %u \n",datum.pressure);
  printf("humi: %u \n",datum.humidity);
  printf("rain: %u \n",datum.rained);
  printf("time: %s \n",datum.dateTime);

  // note not null terminated
  send_msg((char *)&datum,16,fd);
  if (!hasDeadline) {
    flipLED();
    auto end = std::chrono::system_clock::now();
    blinkDeadline = std::chrono::
      duration_cast<std::chrono::milliseconds>(end - start).count() + 500;
    hasDeadline = true;
  }
}

void handleBlinkState(){
  if (isPaused) {
    //ledState = LOW; /// fake Arduino
    ledState = 0;
    //digitalWrite(LED_BUILTIN, ledState); /// fake Arduino
    hasDeadline = false;
    return;
  }
  
  unsigned long duration = 0;
  auto end = std::chrono::system_clock::now();
  duration = std::chrono::
    duration_cast<std::chrono::milliseconds>(end - start).count() + 500;

  if (hasDeadline && blinkDeadline <= duration) {
    flipLED();
    if (fastBlinksLeft) {
      fastBlinksLeft -= 1;
      auto end = std::chrono::system_clock::now();
      blinkDeadline = std::chrono::
      duration_cast<std::chrono::milliseconds>(end - start).count() + 200;
    } else {
      hasDeadline = false;
    }
  }
}


// blocking write
int send_msg(const char *msg, int len, int fd) {
  // for debugging
  //printf("Sending msg \n");
  int quantity = 0;
  while (len - quantity) {
    int res = write(fd, msg + quantity, len - quantity);
    if (res >= 0) {
      quantity += res;
    } else {
      return res;
    }
  }
  // for debugging
  //printf(" msg  SENT\n");

  // removed for socat fake serial connection to work
  //tcflush(fd, TCIOFLUSH);
  return quantity;
}

int init_tty(int fd) {
  struct termios tty;
  /*
   * Configure the serial port.
   * First, get a reference to options for the tty
   * Then, set the baud rate to 9600 (same as on Arduino)
   */
  memset(&tty, 0, sizeof(tty));
  if (tcgetattr(fd, &tty) == -1) {
    perror("tcgetattr");
    return -1;
  }

  if (cfsetospeed(&tty, (speed_t)B9600) == -1) {
    perror("ctsetospeed");
    return -1;
  }
  if (cfsetispeed(&tty, (speed_t)B9600) == -1) {
    perror("ctsetispeed");
    return -1;
  }

  // 8 bits, no parity, no stop bits
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;

  // No flow control
  tty.c_cflag &= ~CRTSCTS;

  // Set local mode and enable the receiver
  tty.c_cflag |= (CLOCAL | CREAD);

  // Disable software flow control
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Make raw
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_oflag &= ~OPOST;

  // Infinite timeout and return from read() with >1 byte available
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  // Update options for the port
  if (tcsetattr(fd, TCSANOW, &tty) == -1) {
    perror("tcsetattr");
    return -1;
  }
  sleep(4); // setup takes time
  return 0;
}