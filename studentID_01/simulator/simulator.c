#include <stdio.h>
#include <stdlib.h>

//define R insructions
#define add 32
#define addu 33
#define sub 34
#define and 36
#define or 37
#define xor 38
#define nor 39
#define nand 40
#define slt 42
#define sll 0
#define srl 2
#define sra 3
#define jr 8

//define I insructions
#define addi 8
#define addiu 9
#define lw 35
#define lh 33
#define lhu 37
#define lb 32
#define lbu 36
#define sw 43
#define sh 41
#define sb 40
#define lui 15
#define andi 12
#define ori 13
#define nori 14
#define slti 10
#define beq 4
#define bne 5
#define bgtz 7

//define J insructions
#define j 2
#define jal 3

//define S insructions
#define halt 63

//to I_img D_img


FILE *I_img, *D_img, *Err_dump, *Snapshot;
unsigned I_img_size, D_img_size;
char *I_img_buffer, *D_img_buffer;
unsigned I_img_size_result, D_img_size_result;
unsigned pc, reg[32];
char IMemory[1024], DMemory[1024];

unsigned cycle = 0;
unsigned Pos;

void printSnap();


int main () {

      I_img = fopen("iimage.bin","rb");
      D_img = fopen("dimage.bin","rb");
      Err_dump = fopen("error_dump.rpt","wb");
      Snapshot = fopen("snapshot.rpt","wb");


      if (I_img == NULL) {fputs ("File error",stderr); exit (1);}
      if (D_img == NULL) {fputs ("File error",stderr); exit (1);}

      // obtain file size:
      fseek (I_img , 0 , SEEK_END);
      fseek (D_img , 0 , SEEK_END);
      I_img_size = ftell(I_img);
      D_img_size = ftell(D_img);

      rewind (I_img);
      rewind (D_img);

      // allocate memory to contain the whole file:
      I_img_buffer = (char*) malloc (sizeof(char)*I_img_size);
      D_img_buffer = (char*) malloc (sizeof(char)*D_img_size);

      if (I_img_buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
      if (D_img_buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

      // copy the file into the buffer:
      I_img_size_result = fread (I_img_buffer, 1, I_img_size, I_img);
      D_img_size_result = fread (D_img_buffer, 1, D_img_size, D_img);
      if (I_img_size_result != I_img_size) {fputs ("Reading error",stderr); exit (3);}
      if (D_img_size_result != D_img_size) {fputs ("Reading error",stderr); exit (3);}

      /* the whole file is now loaded in the memory buffer. */

      // terminate
      fclose (I_img);
      fclose (D_img);

      //******************************************
      //free (I_img_buffer);
      //free (D_img_buffer);
      //finished with opening files

      //get the 1st line PC of Iimage

      unsigned int i, I_temp=0, I_idx, I_lines=0;

      for(i=0;i<4;i++){

            I_temp = (I_temp<<8) + (unsigned char)I_img_buffer[i];
        }

        pc = I_temp;

      //get the lines of Iimage

      for(i=4;i<8;i++){

            I_lines = (I_lines<<8) + (unsigned char)I_img_buffer[i];
        }

      // I_lines = I_temp;
      //copy the insruction lines to Imemory
      I_idx = pc ;

      for(i=8;i<8+(4*I_lines);i++){

            IMemory[I_idx++] = I_img_buffer[i];
        }


      //get the 1st line $sp of Dimage

      unsigned int k, D_temp=0, D_idx=0 , D_lines=0;

      for(k=0;k<4;k++){

            D_temp = (D_temp<<8) + (unsigned char)D_img_buffer[k];
        }
      //************************************
        reg[29] = D_temp;

      //get the lines of Dimage

      for(k=4;k<8;k++){

            D_lines = (D_lines<<8) + (unsigned char)D_img_buffer[k];
        }

      // D_lines = D_temp;
      //copy the insruction lines to Dmemory

      for(k=8;k<8+(4*D_lines);k++){

            DMemory[D_idx++] = D_img_buffer[k];
        }


      //finished IMemory and DMemory


      unsigned opcode;
      //get the first instruction


      while(500000){


        opcode = IMemory[pc];
        opcode = opcode >> 2 << 26 >> 26;
        printf("opcode: %u\n", opcode);



        //print snapshot
        fprintf(Snapshot, "cycle %u\n", cycle++);

            unsigned i;

            for (i = 0; i < 32; ++i) {
                fprintf(Snapshot, "$%02u: 0x", i);
                fprintf(Snapshot, "%08X\n", reg[i]);
                }

            fprintf(Snapshot, "PC: 0x");
            fprintf(Snapshot, "%08X\n\n\n", pc);
        //*****************************************



        //J insructions and S insructions
        if(opcode == halt){
            break;
        }
        else if(opcode == j){

                unsigned address,a,b,c,d;

                a = IMemory[pc];
                b = IMemory[pc+1];
                c = IMemory[pc+2];
                d = IMemory[pc+3];

                a = a << 30 >> 6;
                b = b << 24 >> 8;
                c = c << 24 >> 16;
                d = d << 24 >> 24;

                address = a + b + c + d;
                pc = ((pc+4) >> 28 << 28) | address << 2;//pc = (pc+4)[31:28] | 4*C(unsigned)

        }
        else if(opcode == jal){

                unsigned address,a,b,c,d;

                reg[31] = pc + 4;//$31 = pc + 4

                a = IMemory[pc];
                b = IMemory[pc+1];
                c = IMemory[pc+2];
                d = IMemory[pc+3];

                a = a << 30 >> 6;
                b = b << 24 >> 8;
                c = c << 24 >> 16;
                d = d << 24 >> 24;

                address = a + b + c + d;
                pc = ((pc+4) >> 28 << 28) | address << 2;//pc = (pc+4)[31:28] | 4*C(unsigned)


        }
        else{
            //R insructions
            if(opcode == 0){
                unsigned funct,shamt,rs,rt,rd;

                funct = IMemory[pc+3];
                funct = funct << 26 >> 26;

                // now we find the value of rs ,rt ,rd
                unsigned tmp1,tmp2;

                tmp1 = IMemory[pc];
                tmp2 = IMemory[pc+1];

                tmp1 = tmp1 << 30 >> 27;//get the top 2 bits and leave 3 zeros
                tmp2 = tmp2 << 24 >> 29;//get rid of the lower 5 bits

                rs = tmp1 + tmp2;

                rt = IMemory[pc+1];
                rt = rt << 27 >> 27;//get the lower 5 bits

                rd = IMemory[pc+2];
                rd = rd << 24 >> 27;//get the top 5 bits

                printf("rs rt rd: %u %u %u\n", rs, rt, rd);
                //***********************************

                //finished the values of rs rt rd


                //get the value of shamt ignore what instruction set we are doing
                unsigned tmp3,tmp4;

                tmp3 = IMemory[pc+2];
                tmp4 = IMemory[pc+3];

                tmp3 = tmp3 << 29 >> 27;//get the bot 3 bits and leave 2 zeros
                tmp4 = tmp4 >> 6 << 30 >> 30;//get the top 2 bit and set to the right

                shamt = tmp3 + tmp4;

                printf("shamt: %u\n", shamt);
                //**********************************

                //finished the value of shamt

                printf("funct: %u\n\n", funct);
                //**********************************

                if(funct == add){

                    unsigned rs_sign,rt_sign,rd_sign;

                    rs_sign = reg[rs] >> 31;//get the first bit
                    rt_sign = reg[rt] >> 31;//get the first bit

                    reg[rd] = reg[rs] + reg[rt];
                    rd_sign = reg[rd] >> 31;//get the first bit

                    if ((rs_sign == rt_sign) && (rs_sign != rd_sign)){
                        fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle);
                        printf("Number Overflow\n");
                        //************************************
                    }


                }
                else if(funct == addu){

                    reg[rd] = reg[rs] + reg[rt];

                }
                else if(funct == sub){

                    unsigned rs_sign,rt_sign,rd_sign;

                    rs_sign = reg[rs] >> 31;//get the first bit
                    rt_sign = (-reg[rt]) >> 31;//get the first bit

                    reg[rd] = reg[rs] - reg[rt];
                    rd_sign = reg[rd] >> 31;//get the first bit

                    if ((rs_sign == rt_sign) && (rs_sign != rd_sign)){
                        fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle);
                        printf("Number Overflow\n");
                        //************************************
                    }

                }
                else if(funct == and){

                    reg[rd] = reg[rs] & reg[rt];

                }
                else if(funct == or){

                    reg[rd] = reg[rs] | reg[rt];

                }
                else if(funct == xor){

                    reg[rd] = reg[rs] ^ reg[rt];

                }
                else if(funct == nor){

                    reg[rd] = ~(reg[rs] | reg[rt]);

                }
                else if(funct == nand){

                    reg[rd] = ~(reg[rs] & reg[rt]);

                }
                else if(funct == slt){
                    //signed rs rt
                    int rs_int , rt_int;

                    reg[rd] = (rs_int < rt_int);

                }
                else if(funct == sll){

                    reg[rd] = reg[rt] << shamt;

                }
                else if(funct == srl){

                    reg[rd] = reg[rt] >> shamt;

                }
                else if(funct == sra){
                    //signed rt
                    int rt_int;
                    rt_int = reg[rt];
                    rt_int = rt_int >> shamt;
                    reg[rd] = rt_int;

                }
                else if(funct == jr){

                    //Pos = reg[rs] - pc - 4;
                    pc = reg[rs];
                }

                //***************************
                //end of R instructions

            }

            //I insructions
            else{

                unsigned rs,rt,immediate,un_immediate;
                // now we find the value of rs ,rt There is no rd in I instruction
                unsigned tmp1,tmp2;

                tmp1 = IMemory[pc];
                tmp2 = IMemory[pc+1];

                tmp1 = tmp1 << 30 >> 27;//get the top 2 bits and leave 3 zeros
                tmp2 = tmp2 << 24 >> 29;//get rid of the lower 5 bits

                rs = tmp1 + tmp2;

                rt = IMemory[pc+1];
                rt = rt << 27 >> 27;//get the lower 5 bits

                printf("rs rt: %u %u\n", rs, rt);
                //***********************************

                //finished the values of rs rt

                //get the value of immediate(unsigned)

                unsigned tmp3,tmp4;

                tmp3 = IMemory[pc+2];
                tmp4 = IMemory[pc+3];
                tmp3 = tmp3 << 24 >> 16;//get the 8 bits and leave 8 zeros
                tmp4 = tmp4 << 24 >> 24;//get the 8 bits

                un_immediate = tmp3 + tmp4;

                printf("unsigned immediate: %u\n", un_immediate);
                //***********************************

                //finished the value of immediate(unsigned)

                //get the value of immediate(signed) no matter what instruction is selected

                unsigned tmp5,tmp6;
                int imm_int;

                tmp5 = IMemory[pc+2];
                tmp6 = IMemory[pc+3];

                tmp5 = tmp5 << 24 >> 16;
                tmp6 = tmp6 << 24 >> 24;

                imm_int = tmp5 + tmp6;
                imm_int = imm_int << 16 >> 16;
                immediate = imm_int;

                printf("immediate: %u\n\n", immediate);
                //***********************************

                //finished the value of immediate(signed)


                if(opcode == addi){

                    unsigned rs_sign,rt_sign,imm_sign;

                    rs_sign = reg[rs] >> 31;//get the first bit
                    imm_sign = immediate >> 31;//get the first bit
                    reg[rt] = reg[rs] + immediate;
                    rt_sign = reg[rt] >> 31;//get the first bit

                    if ((rs_sign == imm_sign) && (rs_sign != rt_sign)){
                        fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle);
                        printf("Number Overflow\n");
                        //****************************
                    }

                }
                else if(opcode == addiu){

                    reg[rt] = reg[rs] + un_immediate;

                }
                else if(opcode == lw){

                    unsigned bit0_7,bit8_15,bit16_23,bit24_31;

                    unsigned rs_sign,imm_sign,Pos_sign,Pos;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    bit24_31 = DMemory[Pos];
                    bit16_23 = DMemory[Pos+1];
                    bit8_15 = DMemory[Pos+2];
                    bit0_7 = DMemory[Pos+3];

                    bit24_31 = bit24_31 << 24;//4 byte
                    bit16_23 = bit16_23 << 24 >> 8;//4 byte
                    bit8_15 = bit8_15 << 24 >> 16;//4 byte
                    bit0_7 = bit0_7 << 24 >> 24;//4 byte

                    reg[rt] = bit0_7 + bit8_15 + bit16_23 + bit24_31;


                }
                else if(opcode == lh){

                    unsigned bit0_7,bit8_15;
                    short half;

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    bit0_7 = DMemory[Pos];
                    bit8_15 = DMemory[Pos+1];

                    bit8_15 = bit8_15 << 24 >> 16;//2 byte
                    bit0_7 = bit0_7 << 24 >> 24;//2 byte

                    reg[rt] = half;


                }
                else if(opcode == lhu){

                    unsigned bit0_7,bit8_15;

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    bit0_7 = DMemory[Pos];
                    bit8_15 = DMemory[Pos+1];

                    bit8_15 = bit8_15 << 24 >> 16;//2 byte
                    bit0_7 = bit0_7 << 24 >> 24;//2 byte

                    reg[rt] = bit0_7 + bit8_15;

                }
                else if(opcode == lb){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    reg[rt] = DMemory[Pos];

                }
                else if(opcode == lbu){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    reg[rt] = DMemory[Pos];
                    reg[rt] = reg[rt] << 24 >> 24;
                }
                else if(opcode == sw){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    DMemory[Pos] = reg[rt] >> 24;//get the value in rt stored back to DMemory
                    DMemory[Pos+1] = reg[rt] << 8 >> 24;
                    DMemory[Pos+2] = reg[rt] << 16 >> 24;
                    DMemory[Pos+3] = reg[rt] << 24 >> 24;

                }
                else if(opcode == sh){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    DMemory[Pos] = reg[rt] << 16 >> 24;//store back half word 2 bytes
                    DMemory[Pos+1] = reg[rt] << 24 >> 24;

                }
                else if(opcode == sb){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;
                    //********************
                    //need error detect
                    //********************
                    DMemory[Pos]= reg[rt] << 24 >> 24;//store back 1 byte


                }
                else if(opcode == lui){

                    reg[rt] = un_immediate << 16;//$t = C << 16

                }
                else if(opcode == andi){

                    reg[rt] = reg[rs] & un_immediate;
                }
                else if(opcode == ori){

                    reg[rt] = reg[rs] | un_immediate;

                }
                else if(opcode == nori){

                    reg[rt] = ~(reg[rs] | un_immediate);

                }
                else if(opcode == slti){

                    int imm,rs_int;

                    imm = immediate;
                    rs_int = reg[rs];

                    if(rs_int < imm){
                        reg[rt] = 1;
                    }
                    else{
                        reg[rt] = 0;
                    }


                }
                else if(opcode == beq){

                    if(reg[rs] == reg[rt]){
                        immediate = immediate << 2;//4 * C(signed)
                        pc = pc + immediate;//pc + 4 + 4 * C(signed)
                        //we will + 4 at the end of while loop
                    }

                }
                else if(opcode == bne){

                    if(reg[rs] != reg[rt]){
                        immediate = immediate << 2;//4 * C(signed)
                        pc = pc + immediate;//pc + 4 + 4 * C(signed)
                        //we will + 4 at the end of while loop
                    }

                }
                else if(opcode == bgtz){

                    int rs_int = reg[rs];
                    if(0 < rs_int){
                        immediate = immediate << 2;//4 * C(signed)
                        pc = pc + immediate;//pc + 4 + 4 * C(signed)
                        //we will + 4 at the end of while loop
                    }

                }

            }

        }

        pc = pc + 4;
        cycle++;
		//opcode = IMemory[Pos];
		//opcode = opcode >> 2 << 26 >> 26;
		//printf("opcode: %u\n", opcode);

      }


      return 0;
}
