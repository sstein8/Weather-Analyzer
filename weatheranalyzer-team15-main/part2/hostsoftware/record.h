#ifndef record_h_
#define record_h_

#define RECORDLEN 36
#define NUMRECORDS (24 * 3)
#define RECORD_FILESIZE (NUMRECORDS * RECORDLEN)

int _records_so_far;

int update_record(char *record, unsigned char tmp, unsigned char prs,
                  unsigned char hmd, unsigned char rained,
                  char timeString[12]) {

  /*
   * Makes sure that we do not write over the length of the file
   */
  if (_records_so_far >= NUMRECORDS) {
    return -1;
  }

  /*
   *  Write out data in a CSV format
   *  and counts the number of records archived
   */

  //TODO: your code here
  
  //Writes all of the recorded vales in CSV format and then prints
  sprintf(record + (_records_so_far * RECORDLEN * sizeof(char)),
          "  %03u, %03u, %03u, %03u, %.12s,\n", tmp, prs, hmd, rained,
          timeString);

  
  //Increments the amount of records created
  _records_so_far++;
  return 0;
}

/*
 * construct_record is responsible for opening the file and mmaping the file
 * the file descriptor is loaded into user provided pointer
 * the address of the mapped region is loaded into another user provided
 * pointer return value is 0 on success
 */
int construct_record(char *record_fname, int *fd, char **buffer) {
  _records_so_far = 0;
  /*
   * Open a file for reading and writing with permission granted to user
   * Use error checking!
   */
  
  //TODO: your code here
  *fd = open(record_fname, O_CREAT | O_RDWR, (mode_t)0600);
  if (*fd < 0){
    perror("Issue Opening");
    return -1;
  }

  /*
   * Seek to the end of the file and write a \0 to extend the file
   * Use error checking!
   */
  
  //TODO: your code here
  if (-1 == lseek(*fd, RECORD_FILESIZE, SEEK_SET)){
    perror("Issue with lseek");
    return -1;
  }

  int result = write(*fd, "\0", 1);
  if (result != 1){
    perror("Issue extending file");
    return -1;
  }

  /*
   * Map the proper region of the file into the user's address space
   * Use error checking!
   */
  
  //TODO: your code here
  *buffer = (char *)mmap(0, RECORD_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  if (*buffer == MAP_FAILED){
    perror("Issue mmaping record file");
    return -1;
  }

  //_records_so_far++;
  return 0;
}

int deconstruct_record(int record_fd, char *record) {
  /*
   * No need to sync the file, unless there are multiple reading processes
   * so simply un-map and close
   */
  
  //TODO: your code here
  if (msync(record, FILESIZE, MS_SYNC) == -1){
    perror("Could not sync the file to desk");
  }
  munmap(record, FILESIZE);
  close(record_fd);
  return 0;
}

#endif