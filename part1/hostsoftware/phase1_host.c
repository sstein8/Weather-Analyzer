#define _GNU_SOURCE

#include "arduinocom.h"
#include "hist.h"
#include "record.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// function prototypes
void main_loop(int tty_fd, char *record, char **hists);
char *names[3] = {"Temperature", "Pressure", "Humidity"};
char *hist_file_names[3] = {"tmp_hist.bin", "prs_hist.bin", "hmd_hist.bin"};


/*
  * MAIN: receives the Serial port / socat pseudo-serial port 
  * or defaults to /dev/ttyACM0 and then sets up the comms 
  * and starts the comms loop
  */
int main(int argc, char **argv) {

  int res = 0;

  /*
   * /dev/ttyACM0 unless the user specifies a file
   */
  char *serial_file = (argc == 2) ? argv[1] : "/dev/ttyACM0";

  printf("Device is : %s\n", serial_file);
  /*
   * Need the following flags to open:
   * O_RDWR: to read from/write to the devices
   * O_NOCTTY: Do not become the process's controlling terminal
   * O_NDELAY: Open the resource in nonblocking mode
   */
  int serial_fd = open(serial_file, O_RDWR | O_NOCTTY | O_NDELAY);
  if (serial_fd == -1) {
    perror("Error opening serial");
    res = -1;
    goto done;
  }

  /* Configure settings on the serial port */
  if (init_tty(serial_fd) == -1) {
    perror("Issue setting up serial file");
    res = -1;
    goto done;
  }

  // Three histograms, Temp, Pressure, Humidity
  char *hists[3];
  int hist_fds[3];
  for (int i = 0; i < 3; i++) {
    /*
     * construct_hist is responsible for opening the file and mmaping the file
     * reports a file descriptor by filling a user provided pointer
     * reports the mapped region address by filling another user provided
     * pointer return value is 0 on success
     */
    if (construct_hist(hist_file_names[i], &hist_fds[i], &hists[i])) {
      res = -1;
      goto done;
    }
  }
  int record_fd = 0;
  char *record = NULL;
  /*
   * construct_record is responsible for opening the file and mmaping the file
   * the file descriptor is loaded into user provided pointer
   * the address of the mapped region is loaded into another user provided
   * pointer return value is 0 on success
   */
  if (construct_record("record.bin", &record_fd, &record)) {
    res = -1;
    goto done;
  } 
  main_loop(serial_fd, record, hists);

  /*
   * cleanup resources
   */
done:
  if (serial_fd)
    close(serial_fd);
  for (int i = 0; i < 3; i++) {
    if (hist_fds[i])
      deconstruct_hist(hist_fds[i], hists[i]);
  }
  if (record_fd)
    deconstruct_record(record_fd, record);
  return res;
}


/*
 * MAIN_LOOP: runs data collection from the sensorsoftware
 * at one reading per second (each represents an hour) and
 * updates 3 histograms oof data and a list of records
 */

void main_loop(int tty_fd, char *record, char **hists) {

  time_t next_time = 0;
  int num_readings = 0;

  printf("Beginning Sensor Reading\n");
  while (num_readings < 3 * 24) {
    /*
     * Only collect sensor readings once a second
     */
    time_t t;
    time(&t);
    if (t < next_time) {
      continue;
    }
    next_time = t + 1;

    /*
     * Send a 1 byte message requesting the next sensor reading
     */
    char request = REQUEST;
    printf("sending: %c    ", request);
    fflush(stdout);
    send_msg(&request, 1, tty_fd);

    /*
     * Format of reply as follows
     * BYTE # | VALUE
     * 0      | Temp reading (between 0 and 255)
     * 1      | Pressure reading (0-255)
     * 2      | Humidity reading (0-255)
     * 3      | Rain - 0 means no observation, 1 means no rain, 2 means rain
     * 4      | timestamp year index 0 (ISO-8601)
     * 5      | timestamp year index 1 (ISO-8601)
     * 6      | timestamp year index 2 (ISO-8601)
     * 7      | timestamp year index 3 (ISO-8601)
     * 8      | timestamp month index 0 (ISO-8601)
     * 9      | timestamp month index 1 (ISO-8601)
     * 10     | timestamp day index 0 (ISO-8601)
     * 11     | timestamp day index 1 (ISO-8601)
     * 12     | timestamp hour index 0 (ISO-8601)
     * 13     | timestamp hour index 1 (ISO-8601)
     * 14     | timestamp minute index 0 (ISO-8601)
     * 15     | timestamp minute index 1 (ISO-8601)
     *
     * e.g.
     * 202001010600 represents January 1st 2020 at exactly 6 AM
     */

    printf("Reading reply from \" Arduino\"\n");
    /*
     * read and accumulate a 16 byte reply
     * (kept in a null terminated 17 byte buffer)
     */
    char reply[17];
    reply[16] = '\0';
    int consumed = 0;
    while (consumed < 16) {
      int res = read(tty_fd, reply + (consumed * sizeof(char)), 16 - consumed);
      if (res >= 0) {
        consumed += res;
      } else {
        perror("Issue reading from serial");
        return;
      }
    }
    num_readings++;
    printf("reply READ; num_readings: %d\n", num_readings);

    /*
     * Insert new sensor reading into hists and record data structures
     */
    int time = (reply[12] - '0') * 10 + (reply[13] - '0');
    for (int i = 0; i < 3; i++) {
      // hist only needs the hour between 0 and 23
     update_hist(hists[i], reply[i], time);

      /*
       * Every 10 readings, print the histograms to stdout
       */
      if (num_readings % 10 == 0) {
        printf("Readings for %s up to timestamp %s\n", names[i], reply + 4);
        print_hist(hists[i]);
      }
    }
    update_record(record, reply[0], reply[1], reply[2], reply[3], reply + 4);
  }
  printf("DONE\n");
}
