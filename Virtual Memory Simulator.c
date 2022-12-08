#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void loop();
void init();
void execute(char * args[]);
void readAddress(int virtual_addr);
void writeAddress(int virtual_addr, int num);
void showMain(int ppn); 
void showPTable(); 
void fifoPageFaultHandler(int faulted_page,int num);
void fifoReplace(int out, int replacing);
void lruPageFaultHandler(int faulted_page, int num);
void lruKickBackToDisk(int og_disk_page, int replacing_disk_page, int main_page);

struct Memory{
  int address, data;
};

struct PageTable{
  int v_page_num, valid_bit, dirty_bit, page_num, time_stamp;
};

struct Memory main_memory[32];
struct Memory virtual_memory[128];
struct PageTable p_table[16];

int fifo = 0, lru = 0, mainCount = 0, globalClock = 0, numberWrites = 0;

int main(int argc, char ** argv){
  
  if ((argv[1] == NULL) || (strcmp(argv[1], "FIFO") == 0)){
   
    fifo = 1;
  }
  else if (strcmp(argv[1], "LRU") == 0){
  
    lru = 1;
  }

  init();
  loop();
  return 0;
}

void loop(){
  char input[100];
  char* token;
  char* args[20];
  int i = 0;
  do{
        printf("> ");
        fgets(input, 80, stdin);
        if (strcmp(input, "quit") == 0 || strcmp(input, "quit\n") == 0){
        break;
        }

        token = strtok(input, " ");
        while (token != NULL) {
        args[i] = token;
        i++;
        token = strtok(NULL, " ");
        }
        args[i] = NULL;
    
        execute(args);

  }while(1);
}

void init(){

  int i;
  for (i = 0; i < sizeof(main_memory) / sizeof(main_memory[0]); i++){
    main_memory[i].data = -1;
    main_memory[i].address = i;
  }
  for (i = 0; i < sizeof(virtual_memory) / sizeof(virtual_memory[0]); i++){
    virtual_memory[i].data = -1;
    virtual_memory[i].address = i;
  }
  for (i = 0; i < sizeof(p_table) / sizeof(p_table[0]); i++){
    p_table[i].v_page_num = p_table[i].page_num = i;
    p_table[i].valid_bit = p_table[i].dirty_bit = p_table[i].time_stamp = 0;
  }
}

void execute(char * args[]){

  char* value1, value2;
  if (!(strcmp(args[0], "read"))){
    
    int x = atoi(args[1]);

    readAddress(x);
    globalClock++;
  } else if (!(strcmp(args[0], "write"))){

    int x = atoi(args[1]);
    int y = atoi(args[2]); 

    writeAddress(x,y);
    globalClock++;
  } else if (!(strcmp(args[0], "showmain"))){

    int x = atoi(args[1]);
    showMain(x);
  } else if (!(strcmp(args[0], "showptable"))){

    showPTable();
  }
}

void readAddress(int virtual_addr){ 
  
  int page, offset;
  page = virtual_addr / 8;
  offset = virtual_addr % 8;

  if (lru == 1){
    p_table[page].time_stamp = globalClock;
  }

  if (p_table[page].valid_bit == 0){
    printf("A Page Fault Has Occurred\n");
    

    if (fifo == 1){

      fifoPageFaultHandler(page, -1);
      printf("%d\n", virtual_memory[virtual_addr].data);

    }else{

      lruPageFaultHandler(page,-1);
      printf("%d\n", virtual_memory[virtual_addr].data);
    }
  }else{

    page = p_table[page].page_num;
    printf("%d\n", main_memory[(page + offset)].data);
  }
}

void writeAddress(int virtual_addr, int num){ // want to keep variable to track least recently used 
  
  int page, offset;
  page = virtual_addr / 8;
  offset = virtual_addr % 8;

  if (lru == 1){
    p_table[page].time_stamp = globalClock;
  }

  if (p_table[page].valid_bit == 0){
    printf("A Page Fault Has Occurred\n");

    if (numberWrites >= 4){
      if (fifo == 1){
        fifoPageFaultHandler(page, num);
        p_table[page].valid_bit = 1;
        main_memory[(24 + offset)].data = num;
        p_table[page].dirty_bit = 1;
        p_table[page].valid_bit = 1;

      }else{
        lruPageFaultHandler(page,num);
        int mainPage;
        mainPage = p_table[page].page_num;
        main_memory[(mainPage + offset)].data = num;
        p_table[page].dirty_bit = 1;
        p_table[page].valid_bit = 1;

      }

    }else{
      if (fifo == 1){
        p_table[page].time_stamp = globalClock;
      }

      int startPage;
      startPage = globalClock * 8;
      main_memory[(startPage + offset)].data = num;
      p_table[page].dirty_bit = 1;
      p_table[page].valid_bit = 1;

    }
  }else{

    int mainPage;
    mainPage = p_table[page].page_num * 8;
    main_memory[(mainPage + offset)].data = num;
    p_table[page].dirty_bit = 1;
    numberWrites++;

  }
}

void showMain(int ppn){
  int i, j;
  i = 8 * ppn;
  j = i + 8;

  for (i; i < j; i++){
    printf("%d:%d \n", main_memory[i].address, main_memory[i].data);
  }
}

void showPTable(){
  int i;
  for (i = 0; i < sizeof(p_table); i++){
    printf("%d:%d:%d:%d \n", p_table[i].v_page_num, p_table[i].valid_bit, p_table[i].dirty_bit, p_table[i].page_num);
  }
}

void fifoPageFaultHandler(int faulted_page, int num){

  int kickedOut;
  int i;
  if(numberWrites >=4){
  for ( i = 0; i < 16; i++) {

    if (p_table[i].valid_bit == 1){

      if (p_table[i].page_num == 0){
        kickedOut = p_table[i].v_page_num;
        p_table[i].page_num = p_table[i].v_page_num;
      } else if ((p_table[i].page_num == 1) || (p_table[i].page_num == 2) || (p_table[i].page_num == 3)) {
        p_table[i].page_num = (p_table[i].page_num - 1);
      }
    }
  }

  kickedOut = kickedOut * 8;

  fifoReplace(kickedOut, faulted_page);
  }else{
      if(num != -1){
      int start = numberWrites * 8;
       
       
    for(i = start; i < start + 8; i++){

        main_memory[start + i].data = num;
        

    }
    p_table[faulted_page].page_num = numberWrites;
    numberWrites++;
    }   
  }
}

void fifoReplace(int out_addr, int replacing_addr){

  int i, j, in_addr;
 
  j = 8;
  in_addr = replacing_addr * 8;

  for (i = 0; i < 24; i++){
    
    if (i < 8){
      virtual_memory[out_addr + i].data = main_memory[i].data;
    }

    main_memory[i].data = main_memory[j + i].data;
  }

  
  for (i = 0; i < 32; i++){
    main_memory[24 + i].data = virtual_memory[replacing_addr + i].data;
  }

  p_table[in_addr].page_num = 3;
}

void lruPageFaultHandler(int faulted_page, int num){

  int i, num0, num1, num2, num3, kickedOut1, kickedOut2;
  if(numberWrites >=4){
    for (i = 0; i < 16; i++){
      if (p_table[i].valid_bit == 1){

        if ((p_table[i].page_num == 1)){
          num0 = i;
        }else if (p_table[i].page_num == 2){
          num1 = i;
        }else if (p_table[i].page_num == 2){
          num2 = i;
        }else if (p_table[i].page_num == 3){
         num3 = i;
        }
      }
    }

  if (p_table[num0].time_stamp < p_table[num1].time_stamp){
    kickedOut1 = num0
  }else{
    kickedOut1 = num1;
  }

  if (p_table[num2].time_stamp < p_table[num3].time_stamp){
    kickedOut1 = num2;
  }else{
    kickedOut1 = num3;
  }

  if (p_table[kickedOut1].time_stamp > p_table[kickedOut2].time_stamp){
    kickedOut1 = kickedOut2;
  }

  kickedOut2 = p_table[kickedOut2].v_page_num;

  lruKickBackToDisk(kickedOut1, faulted_page, kickedOut2);
  }else{
     
    if(num != -1){
     
    int start = numberWrites * 8, in_addr = faulted_page * 8;
         
    for(i = start; i < start + 8; i++){

      main_memory[start + i] = virtual_memory[faulted_page];
    }
    
    p_table[faulted_page].page_num = numberWrites;
    }   
  }
}

void lruKickBackToDisk(int og_disk_page, int replacing_disk_page, int main_page){

  int i, og_Address, replacingAddress;
  og_Address = (og_disk_page * 8);
  replacingAddress = (replacing_disk_page * 8);
  
  for (i = 0; i < 8; i++){

    virtual_memory[og_Address + i] = main_memory[main_page + i];
    main_memory[main_page + i] = virtual_memory[replacingAddress + i];
  }

  p_table[og_disk_page].valid_bit = 0;
  p_table[og_disk_page].dirty_bit = 0;
  p_table[replacing_disk_page].valid_bit = 1;
}