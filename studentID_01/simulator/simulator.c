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

      //copy the insruction lines to Dmemory

      for(k=8;k<8+(4*D_lines);k++){

            DMemory[D_idx++] = D_img_buffer[k];
        }

      //finished storing IMemory and DMemory from buffer

      unsigned opcode;

      int Write_0 = 0 ,Num_overflow = 0 ,Address_overflow = 0 ,Misalign = 0 ,Need_halt = 0;
      //flags to decide the errors

      while(cycle < 500000){



        //get the instruction
        opcode = IMemory[pc];
        opcode = opcode >> 2 << 26 >> 26;
        printf("opcode: %u\n", opcode);



        //print snapshot
        fprintf(Snapshot, "cycle %u\n", cycle);

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
                    //rd_sign should be after the addition

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if ((rs_sign == rt_sign) && (rs_sign != rd_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == addu){

                    reg[rd] = reg[rs] + reg[rt];

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == sub){

                    unsigned rs_sign,rt_sign,rd_sign;

                    rs_sign = reg[rs] >> 31;//get the first bit
                    rt_sign = (-reg[rt]) >> 31;//get the first bit

                    reg[rd] = reg[rs] - reg[rt];

                    rd_sign = reg[rd] >> 31;//get the first bit
                    //need to get rd sign after subtraction

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if ((rs_sign == rt_sign) && (rs_sign != rd_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == and){

                    reg[rd] = reg[rs] & reg[rt];

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == or){

                    reg[rd] = reg[rs] | reg[rt];

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == xor){

                    reg[rd] = reg[rs] ^ reg[rt];

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == nor){

                    reg[rd] = ~(reg[rs] | reg[rt]);

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == nand){

                    reg[rd] = ~(reg[rs] & reg[rt]);

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == slt){
                    //signed rs rt
                    int rs_int , rt_int;

                    rs_int = reg[rs];
                    rt_int = reg[rt];

                    if(rs_int < rt_int){
                        reg[rd] = 1;
                    }
                    else{
                        reg[rd] = 0;
                    }

                    //ERROR Write to register $0
                    if(rd == 0){
                        fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        printf("Write $0 Error\n");
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == sll){

                    reg[rd] = reg[rt] << shamt;

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        if( (shamt == 0) && (rt == 0) ){

                        }
                        else{
                            Write_0 = 1;
                            reg[rd] = 0;
                        }
                        //test case 101062201 occur problems
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == srl){

                    reg[rd] = reg[rt] >> shamt;

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == sra){
                    //signed rt
                    int rt_int;
                    rt_int = reg[rt];
                    rt_int = rt_int >> shamt;
                    reg[rd] = rt_int;

                    //ERROR Write to register $0
                    if(rd == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rd] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(funct == jr){

                    pc = reg[rs] - 4;

                    pc = pc + 4;
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
                    //rt_sign should be after the addition

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }
                    //ERROR Number Overflow
                    if ((rs_sign == imm_sign) && (rs_sign != rt_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //****************************
                    }

                    pc = pc + 4;

                }
                else if(opcode == addiu){

                    reg[rt] = reg[rs] + immediate;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(opcode == lw){

                    unsigned bit0_7,bit8_15,bit16_23,bit24_31;

                    unsigned rs_sign,imm_sign,Pos_sign,Pos;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if( (Pos > 1024) || (Pos + 3 >= 1024) ){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    //ERROR Data Misaligned
                    if(Pos % 4){
                        //fprintf(Err_dump, "In cycle %d: Misalignment Error\n", cycle+1);
                        //printf("Misalignment Error\n");
                        //break;
                        Misalign = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    bit24_31 = DMemory[Pos];
                    bit16_23 = DMemory[Pos+1];
                    bit8_15 = DMemory[Pos+2];
                    bit0_7 = DMemory[Pos+3];

                    bit24_31 = bit24_31 << 24;//4 byte
                    bit16_23 = bit16_23 << 24 >> 8;//4 byte
                    bit8_15 = bit8_15 << 24 >> 16;//4 byte
                    bit0_7 = bit0_7 << 24 >> 24;//4 byte

                    reg[rt] = bit0_7 + bit8_15 + bit16_23 + bit24_31;

                    if(rt == 0){
                        reg[rt] = 0;
                        //We did not set $0 to zero when detected
                        //Now we set it to zero
                    }

                    pc = pc + 4;


                }
                else if(opcode == lh){

                    unsigned bit0_7,bit8_15;
                    short half;

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if( (Pos > 1024) || (Pos + 1 >= 1024) ){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    //ERROR Data Misaligned
                    if(Pos % 2){
                        //fprintf(Err_dump, "In cycle %d: Misalignment Error\n", cycle+1);
                        //printf("Misalignment Error\n");
                        //break;
                        Misalign = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    bit0_7 = DMemory[Pos];
                    bit8_15 = DMemory[Pos+1];

                    bit0_7 = bit0_7 << 24 >> 16;//2 byte
                    bit8_15 = bit8_15 << 24 >> 24;//2 byte

                    half = bit0_7 + bit8_15;
                    reg[rt] = half;

                    if(rt == 0){
                        reg[rt] = 0;
                        //We did not set $0 to zero when detected
                        //Now we set it to zero
                    }

                    pc = pc + 4;


                }
                else if(opcode == lhu){

                    unsigned bit0_7,bit8_15;

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if( (Pos > 1024) || (Pos + 1 >= 1024) ){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    //ERROR Data Misaligned
                    if(Pos % 2){
                        //fprintf(Err_dump, "In cycle %d: Misalignment Error\n", cycle+1);
                        //printf("Misalignment Error\n");
                        //break;
                        Misalign = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    bit0_7 = DMemory[Pos];
                    bit8_15 = DMemory[Pos+1];

                    bit0_7 = bit0_7 << 24 >> 16;//2 byte
                    bit8_15 = bit8_15 << 24 >> 24;//2 byte

                    reg[rt] = bit0_7 + bit8_15;

                    if(rt == 0){
                        reg[rt] = 0;
                        //We did not set $0 to zero when detected
                        //Now we set it to zero
                    }

                    pc = pc + 4;

                }
                else if(opcode == lb){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;


                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if(Pos >= 1024){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    reg[rt] = DMemory[Pos];

                    if(rt == 0){
                        reg[rt] = 0;
                        //We did not set $0 to zero when detected
                        //Now we set it to zero
                    }

                    pc = pc + 4;

                }
                else if(opcode == lbu){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        //************************************
                    }

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if(Pos >= 1024){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    reg[rt] = DMemory[Pos];
                    reg[rt] = reg[rt] << 24 >> 24;

                    if(rt == 0){
                        reg[rt] = 0;
                        //We did not set $0 to zero when detected
                        //Now we set it to zero
                    }

                    pc = pc + 4;
                }
                else if(opcode == sw){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if(  (Pos > 1024) || (Pos + 3 >= 1024) ){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    //ERROR Data Misaligned
                    if(Pos % 4){
                        //fprintf(Err_dump, "In cycle %d: Misalignment Error\n", cycle+1);
                        //printf("Misalignment Error\n");
                        Misalign = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    DMemory[Pos] = reg[rt] >> 24;//get the value in rt stored back to DMemory
                    DMemory[Pos+1] = reg[rt] << 8 >> 24;
                    DMemory[Pos+2] = reg[rt] << 16 >> 24;
                    DMemory[Pos+3] = reg[rt] << 24 >> 24;

                    pc = pc + 4;

                }
                else if(opcode == sh){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if( (Pos > 1024) || (Pos + 1 >= 1024) ){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    //ERROR Data Misaligned
                    if(Pos % 2){
                        //fprintf(Err_dump, "In cycle %d: Misalignment Error\n", cycle+1);
                        //printf("Misalignment Error\n");
                        //break;
                        Misalign = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    DMemory[Pos] = reg[rt] << 16 >> 24;//store back half word 2 bytes
                    DMemory[Pos+1] = reg[rt] << 24 >> 24;

                    pc = pc + 4;

                }
                else if(opcode == sb){

                    unsigned rs_sign,imm_sign,Pos_sign;

                    Pos = reg[rs] + immediate;//$s + C(signed)
                    rs_sign =  reg[rs] >> 31;
                    imm_sign = immediate >> 31;
                    Pos_sign = Pos >> 31;

                    //ERROR Number Overflow
                    if((rs_sign == imm_sign) && (rs_sign != Pos_sign)){
                        //fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
                        //printf("Number Overflow\n");
                        Num_overflow = 1;
                        //************************************
                    }

                    //ERROR Memory Address Overflow
                    if(Pos >= 1024){
                        //fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
                        //printf("Address Overflow\n");
                        //break;
                        Address_overflow = 1;
                        Need_halt = 1;
                        //break;
                        //************************************
                    }

                    if(Need_halt == 1){
                        break;
                    }

                    DMemory[Pos]= reg[rt] << 24 >> 24;//store back 1 byte

                    pc = pc + 4;

                }
                else if(opcode == lui){

                    reg[rt] = un_immediate << 16;//$t = C << 16

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(opcode == andi){

                    reg[rt] = reg[rs] & un_immediate;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }

                    pc = pc + 4;
                }
                else if(opcode == ori){

                    reg[rt] = reg[rs] | un_immediate;

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(opcode == nori){

                    reg[rt] = ~(reg[rs] | un_immediate);

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }

                    pc = pc + 4;

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

                    //ERROR Write to register $0
                    if(rt == 0){
                        //fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
                        //printf("Write $0 Error\n");
                        Write_0 = 1;
                        reg[rt] = 0;
                        //************************************
                    }

                    pc = pc + 4;

                }
                else if(opcode == beq){

                    if(reg[rs] == reg[rt]){
                        immediate = immediate << 2;//4 * C(signed)
                        pc = pc + immediate;//pc + 4 + 4 * C(signed)
                    }

                    pc = pc + 4;

                }
                else if(opcode == bne){

                    if(reg[rs] != reg[rt]){
                        immediate = immediate << 2;//4 * C(signed)
                        pc = pc + 4 + immediate;//pc + 4 + 4 * C(signed)

                    }
                    else{
                        pc = pc + 4;
                    }

                }
                else if(opcode == bgtz){

                    int rs_int = reg[rs];
                    if(0 < rs_int){
                        immediate = immediate << 2;//4 * C(signed)
                        pc = pc + 4 + immediate;//pc + 4 + 4 * C(signed)

                    }
                    else{
                        pc = pc + 4;
                    }

                }

            }

        }

        if(Write_0 == 1){

            fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
            printf("Write $0 Error\n");

            Write_0 = 0;
        }

        if(Num_overflow == 1){

            fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
            printf("Number Overflow\n");

            Num_overflow = 0;
        }

        /*if(Need_halt == 1){
            break;
        }*/
        //segmentation fault will occur don't know the reason

        cycle++;
      }


      if(Write_0 == 1){

            fprintf(Err_dump, "In cycle %d: Write $0 Error\n", cycle+1);
            printf("Write $0 Error\n");

        }

        if(Num_overflow == 1){

            fprintf(Err_dump, "In cycle %d: Number Overflow\n", cycle+1);
            printf("Number Overflow\n");

        }

        if(Address_overflow == 1){

            fprintf(Err_dump, "In cycle %d: Address Overflow\n", cycle+1);
            printf("Address Overflow\n");

        }

        if(Misalign == 1){

            fprintf(Err_dump, "In cycle %d: Misalignment Error\n", cycle+1);
            printf("Misalignment Error\n");

        }


      free (I_img_buffer);
      free (D_img_buffer);
      return 0;

}
