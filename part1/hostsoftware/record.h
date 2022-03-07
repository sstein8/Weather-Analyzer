#ifndef record_h_  
#define record_h_
#include <stdio.h>

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
   
  // get the position of the end of the file (total current entries)
  int end_of_file = _records_so_far * RECORDLEN * sizeof(char);
  //get the index of where the new records need to go
  char* index = end_of_file + record; 



  //print the values to the end of the record where index points to
  sprintf(index, " %u, %u, %u, %u, %s\n", tmp, prs, hmd, rained, timeString);
  //update the amund of records that have been recorded
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
  *fd = open(record_fname, O_CREAT| O_RDWR | O_TRUNC, (mode_t)0600);

  if(*fd < 0){
    return-1;
  }

  /*
   * Seek to the end of the file and write a \0 to extend the file
   * Use error checking!
   */
  
  //TODO: your code here
  //check if lsees returns invalid outcome
  if(lseek(*fd, RECORD_FILESIZE, SEEK_SET) ==-1)
  {
    return -1;
  }
  //check if write function returns invalid outcome
  if(write(*fd, "\0", 1)==-1)
  {
    return -1;
  }

  /*
   * Map the proper region of the file into the user's address space
   * Use error checking!
   */
  
  //TODO: your code here

  //mmap into virtual memory
  *buffer =(char*) mmap(0, RECORD_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  //check if mmapping was successful
  if (*buffer == MAP_FAILED)
  {
        // close(fd);
      perror("Error mmapping the file");
      return -1;
  }

  //reset this portion of memory to have blank characters
  memset(*buffer, ' ', RECORD_FILESIZE); 


  return 0;
}

int deconstruct_record(int record_fd, char *record) {
  /*
   * No need to sync the file, unless there are multiple reading processes
   * so simply un-map and close
   */
  
  //TODO: your code here
  //unmapping, check if there are errors
  if (munmap(record,  RECORD_FILESIZE) == -1)
    {
        close(record_fd);
        perror("Error un-mmapping the file");
        return -1;
    }

  // close the file
  close(record_fd);


  return 0;
}

#endif
