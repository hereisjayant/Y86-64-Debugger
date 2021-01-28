#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "instruction.h"
#include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

#define MAX_LINE 256

static void addBreakpoint(uint64_t address);
static void deleteBreakpoint(uint64_t address);
static void deleteAllBreakpoints(void);
static int  hasBreakpoint(uint64_t address);

static const char *instrName[256][256] = {
  [I_HALT]   = {"halt"},
  [I_NOP]    = {"nop"},
  [I_RRMVXX] = {
    [C_NC] = "rrmovq",
    [C_LE] = "cmovle",
    [C_L]  = "cmovl",
    [C_E]  = "cmove",
    [C_NE] = "cmovne",
    [C_GE] = "cmovge",
    [C_G]  = "cmovg"},
  [I_IRMOVQ] = {"irmovq"},
  [I_RMMOVQ] = {"rmmovq"},
  [I_MRMOVQ] = {"mrmovq"},
  [I_OPQ]    = {
    [A_ADDQ] = "addq",
    [A_SUBQ] = "subq",
    [A_ANDQ] = "andq",
    [A_XORQ] = "xorq",
    [A_MULQ] = "mulq",
    [A_DIVQ] = "divq",
    [A_MODQ] = "modq"
  },
  [I_JXX]    = {
    [C_NC] = "jmp",
    [C_LE] = "jle",
    [C_L]  = "jl",
    [C_E]  = "je",
    [C_NE] = "jne",
    [C_GE] = "jge",
    [C_G]  = "jg"},
  [I_CALL]   = {"call"},
  [I_RET]    = {"ret"},
  [I_PUSHQ]  = {"pushq"},
  [I_POPQ]   = {"popq"}
};

int main(int argc, char **argv) {
  
  int fd;
  struct stat st;
  
  machine_state_t state;
  y86_instruction_t nextInstruction;
  memset(&state, 0, sizeof(state));

  char line[MAX_LINE + 1], previousLine[MAX_LINE + 1] = "";
  char *command, *parameters;
  int c;

  // Verify that the command line has an appropriate number of
  // arguments
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s InputFilename [startingPC]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it for
  // reading and verify that the open did occur.
  fd = open(argv[1], O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "Failed to stat %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  state.programSize = st.st_size;

  // If there is a 2nd argument present it is an offset so convert it
  // to a numeric value.
  if (3 <= argc) {
    errno = 0;
    state.programCounter = strtoul(argv[2], NULL, 0);
    if (errno != 0) {
      perror("Invalid program counter on command line");
      close(fd);
      return ERROR_RETURN;
    }
    if (state.programCounter > state.programSize) { 
      fprintf(stderr, "Program counter on command line (%" PRIu64 ") "
	      "larger than file size (%" PRIu64 ").\n",
	      state.programCounter, state.programSize);
      close(fd);
      return ERROR_RETURN;     
    }
  }

  // Maps the entire file to memory. This is equivalent to reading the
  // entire file using functions like fread, but the data is only
  // retrieved on demand, i.e., when the specific region of the file
  // is needed.
  state.programMap = mmap(NULL, state.programSize, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE, fd, 0);
  if (state.programMap == MAP_FAILED) {
    fprintf(stderr, "Failed to map %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  // Move to first non-zero byte
  while (!state.programMap[state.programCounter]) state.programCounter++;
  
  printf("# Opened %s, starting PC 0x%" PRIX64 "\n",
	 argv[1], state.programCounter);

  fetchInstruction(&state, &nextInstruction);
  printInstruction(stdout, &nextInstruction);
  
  while(1) {
    
    // Show prompt, but only if input comes from a terminal
    if (isatty(STDIN_FILENO))
      printf("> ");

    // Read one line, if EOF break loop
    if (!fgets(line, sizeof(line), stdin))
      break;

    // If line could not be read entirely
    if (!strchr(line, '\n')) {
      // Read to the end of the line
      while ((c = fgetc(stdin)) != EOF && c != '\n');
      if (c == '\n') {
	      printErrorCommandTooLong(stdout);
	      continue;
      }
      else {
	      // In this case there is an EOF at the end of a line.
	      // Process line as usual.
      }
    }

    // Obtain the command name, separate it from the arguments.
    command = strtok(line, " \t\n\f\r\v");
    // If line is blank, repeat previous command.
    if (!command) {
      strcpy(line, previousLine);
      command = strtok(line, " \t\n\f\r\v");
      // If there's no previous line, do nothing.
      if (!command) continue;
    }

    // Get the arguments to the command, if provided.
    parameters = strtok(NULL, "\n\r"); //this gets the next token 

    sprintf(previousLine, "%s %s\n", command, parameters ? parameters : "");

    /* THIS PART TO BE COMPLETED BY THE STUDENT */
    if((strcasecmp(command, "quit")==0) || (strcasecmp(command, "exit")==0)){
      break;
    }
    else if(strcasecmp(command, "step")==0){
      fetchInstruction(&state, &nextInstruction);
      executeInstruction(&state, &nextInstruction);
      printInstruction(stdout, &nextInstruction);
    }
    else if(strcasecmp(command, "run")==0){
      while(1){
        const char *instruction_name = instrName[nextInstruction.icode][nextInstruction.ifun];
        int valid = fetchInstruction(&state, &nextInstruction);
        executeInstruction(&state, &nextInstruction);
        // printf("%s\n",instruction_name );
        if(valid!=1){
          printErrorInvalidInstruction(stdout, &nextInstruction);
        }
        if(strcasecmp(instruction_name, "halt")==0){
          break;
        } 
        if(hasBreakpoint(state.programCounter)){
          fetchInstruction(&state, &nextInstruction);
          printInstruction(stdout, &nextInstruction);
          break;
        }
      }  
    }
    else if(strcasecmp(command, "next")==0){
      
    }
    else if(strcasecmp(command, "jump")==0){
      uint64_t jump_address = strtoul(parameters, NULL, 0);
      nextInstruction.valP = jump_address;
      state.programCounter = jump_address;
      int valid = fetchInstruction(&state, &nextInstruction);//also checks for errors
      if(valid==1){
      printInstruction(stdout, &nextInstruction);
      }
      else{
        printErrorInvalidInstruction(stdout, &nextInstruction);
      }
    }
    else if(strcasecmp(command, "break")==0){
      uint64_t break_address = strtoul(parameters, NULL, 0)+10;
      if(hasBreakpoint(break_address)==0){
        if(state.programSize>=break_address){
          // printf("BREAK ADDED AT %ld",break_address);
          addBreakpoint(break_address);
        }
      }
    }
    else if(strcasecmp(command, "delete")==0){
      uint64_t break_address = strtoul(parameters, NULL, 0)+10;
      if(hasBreakpoint(break_address)==1){
        deleteBreakpoint(break_address);
        // printf("deleted breakpoint");
      }
    }
    else if(strcasecmp(command, "registers")==0){
      printRegisterValue(stdout, &state, R_RAX);
      printRegisterValue(stdout, &state, R_RCX);
      printRegisterValue(stdout, &state, R_RDX);
      printRegisterValue(stdout, &state, R_RBX);
      printRegisterValue(stdout, &state, R_RSI);
      printRegisterValue(stdout, &state, R_RDI);
      printRegisterValue(stdout, &state, R_RSP);
      printRegisterValue(stdout, &state, R_RBP);
      printRegisterValue(stdout, &state, R_R8);
      printRegisterValue(stdout, &state, R_R9);
      printRegisterValue(stdout, &state, R_R10);
      printRegisterValue(stdout, &state, R_R11);
      printRegisterValue(stdout, &state, R_R12);
      printRegisterValue(stdout, &state, R_R13);
      printRegisterValue(stdout, &state, R_R14);
    }
    else if(strcasecmp(command, "examine")==0){
      uint64_t examine_addr = strtoul(parameters, NULL, 0);
      printMemoryValueQuad(stdout, &state, examine_addr);
    }
    else{ 
    }
  }
  deleteAllBreakpoints();
  munmap(state.programMap, state.programSize);
  close(fd);
  return SUCCESS;
}

#define MAX_NUM_BREAKPOINTS 50

static uint64_t breakpointList[MAX_NUM_BREAKPOINTS];
static unsigned int numBreakpoints = 0;

/* Adds an address to the list of breakpoints. If the address is
 * already in the list, it is not added again. */
static void addBreakpoint(uint64_t address) {

  if (hasBreakpoint(address)) return;
  if (numBreakpoints >= MAX_NUM_BREAKPOINTS) return;
  breakpointList[numBreakpoints++] = address;
}

/* Deletes an address from the list of breakpoints. If the address is
 * not in the list, nothing happens. */
static void deleteBreakpoint(uint64_t address) {

  for (unsigned int i = 0; i < numBreakpoints; i++) {
    if (breakpointList[i] == address)
      // Replace element with last
      breakpointList[i] = breakpointList[--numBreakpoints];
  }
}

/* Deletes and frees all breakpoints. */
static void deleteAllBreakpoints(void) {

  numBreakpoints = 0;
}

/* Returns true (non-zero) if the address corresponds to a breakpoint
 * in the list of breakpoints, or false (zero) otherwise. */
static int hasBreakpoint(uint64_t address) {

  for (unsigned int i = 0; i < numBreakpoints; i++) {
    if (breakpointList[i] == address)
      return 1;
  }
  
  return 0;
}
