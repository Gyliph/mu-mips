#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {

	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */
/***************************************************************/
void rdump() {
	int i;
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */
/***************************************************************/
void handle_command() {
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll();
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value;
			NEXT_STATE.HI = hi_reg_value;
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program();
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;

	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}

	/*load program*/
	load_program();

	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */
/************************************************************/
void handle_instruction()
{
	uint32_t instr, opc;
	uint32_t rs, rt, rd, sa, immediate, result;
	uint32_t sign7,sign15, sign31, target, vAddr;
	uint64_t result64;

	if(HI_CTR > 0x0){HI_CTR--;}
	if(LO_CTR > 0x0){LO_CTR--;}

	NEXT_STATE.PC+=4;
	instr = mem_read_32(CURRENT_STATE.PC);
	rs = (instr&0x03E00000) >> 21;
	rt = (instr&0x001F0000) >> 16;
	rd = (instr&0x0000F800) >> 11;
	sa = (instr&0x000001C0) >> 6;
	immediate = instr&0x0000FFFF;
	target = instr&0x03FFFFFF;
	sign7 = 0x00000080; // 0000 0000 0000 0000 0000 0000 0000 1000 0000
	sign15 = 0x00008000; // 0000 0000 0000 0000 0000 1000 0000 0000 0000
	sign31 = 0x80000000;// 1000 0000 0000 0000 0000 0000 0000 0000 0000
	opc = instr&0xFC000000;
	if(opc == 0x00000000){//SPECIAL INSTRUCTION
		opc = instr&0x0000003F;
		switch(opc){
			case 0x00000020: //ADD
				result = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
				if(CURRENT_STATE.REGS[rs] > 0 && CURRENT_STATE.REGS[rt] > UINT_MAX - CURRENT_STATE.REGS[rs]){
					/*handle overflow*/
				}else if((CURRENT_STATE.REGS[rs] < 0 && CURRENT_STATE.REGS[rt] < UINT_MIN - (CURRENT_STATE.REGS[rs]){
					/*handle overflow*/
				}
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000021: //ADDU
				result = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000022: //SUB
				result = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
				if(CURRENT_STATE.REGS[rt] > CURRENT_STATE.REGS[rs]){
					/*handle overflow*/
				}
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000023: //SUBU
				result = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000018: //MULT
				if(HI_CTR == 0x0 && LO_CTR == 0x0){
					result64 = CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt];
					NEXT_STATE.LO = result64&0x00000000FFFFFFFF;
					NEXT_STATE.HI = (result64&0xFFFFFFFF00000000)>>32;
				}
				break;
			case 0x00000019: //MULTU
				if(HI_CTR == 0x0 && LO_CTR == 0x0){
					result64 = (uint32_t)(CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt]);
					NEXT_STATE.LO = result64&0x00000000FFFFFFFF;
					NEXT_STATE.HI = (result64&0xFFFFFFFF00000000)>>32;
				}
				break;
			case 0x0000001A: //DIV
				if(HI_CTR == 0x0 && LO_CTR == 0x0){
					NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
					NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
				}
				break;
			case 0x0000001B: //DIVU
				if(HI_CTR == 0x0 && LO_CTR == 0x0){
					NEXT_STATE.LO = (uint32_t)(CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt]);
					NEXT_STATE.HI = (uint32_t)(CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt]);
				}
				break;
			case 0x00000024: //AND
				result = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000025: //OR
				result = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000026: //XOR
				result = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000027: //NOR
				result = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x0000002A: //SLT
				if(CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]){
					result = 0x1;
				}else{
					result = 0x0;
				}
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000002: //SRL
				result = CURRENT_STATE.REGS[rt]>>sa;
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000003: //SRA
				result = ((int32_t)CURRENT_STATE.REGS[rt])>>sa;
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x00000010: //MFHI
				result = CURRENT_STATE.HI;
				NEXT_STATE.REGS[rd] = result;
				HI_CTR = 0x2;
				break;
			case 0x00000012: //MFLO
				result = CURRENT_STATE.LO;
				NEXT_STATE.REGS[rd] = result;
				LO_CTR = 0x2;
				break;
			case 0x00000011: //MTHI
				if(HI_CTR == 0x0){
					result = CURRENT_STATE.REGS[rs];
					NEXT_STATE.HI = result;
				}
				break;
			case 0x00000013: //MTLO
				if(LO_CTR == 0x0){
					result = CURRENT_STATE.REGS[rs];
					NEXT_STATE.LO = result;
				}
				break;
			case 0x00000008: //JR
				NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
				break;
			case 0x00000009: //JALR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.PC+8;
				NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
				break;
			case 0x00000000: //SLL
				result = CURRENT_STATE.REGS[rt]<<sa;
				NEXT_STATE.REGS[rd] = result;
				break;
			case 0x0000000C: //SYSCALL
				NEXT_STATE.REGS[2] = 0xA;
				RUN_FLAG = FALSE;
				break;
			default:
				printf("SPECIAL ERROR\n");
				break;
		}
	}else if(opc == 0x04000000){//REGIMM INSTRUCTION
		opc = instr&0x001F0000;
		switch(opc){
			case 0x00010000: //BGEZ
				immediate = immediate << 2;
				if((immediate>>2)&sign15){immediate += 0xFFFC0000;}
				if((CURRENT_STATE.REGS[rs]&sign31) == 0x0){
					NEXT_STATE.PC = CURRENT_STATE.PC + immediate;
				}
				break;
			case 0x00000000: //BLTZ
				immediate = immediate << 2;
				if((immediate>>2)&sign15){immediate += 0xFFFC0000;}
				if(CURRENT_STATE.REGS[rs]&sign31){
					NEXT_STATE.PC = CURRENT_STATE.PC + immediate;
				}
				break;
			default:
				printf("REGIMM ERROR\n");
				break;
		}
	}else{//NORMAL INSTRUCTION
		switch(opc){
			case 0x20000000: //ADDI
				if(immediate&sign15){immediate += 0xFFFF0000;}
				result = CURRENT_STATE.REGS[rs] + immediate;
				if(result >= immediate){NEXT_STATE.REGS[rt] = result;}
				break;
			case 0x24000000: //ADDIU
				if(immediate&sign15){immediate += 0xFFFF0000;}
				result = CURRENT_STATE.REGS[rs] + immediate;
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x30000000: //ANDI
				result = CURRENT_STATE.REGS[rs] & immediate;
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x34000000: //ORI
				result = CURRENT_STATE.REGS[rs] | immediate;
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x38000000: //XORI
				result = CURRENT_STATE.REGS[rs] ^ immediate;
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x28000000: //SLTI
				if(immediate&sign15){immediate += 0xFFFF0000;}
				if(CURRENT_STATE.REGS[rs] < immediate){
					result = 0x00000001;
				}else{result = 0x00000000;}
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x8C000000: //LW
				if(immediate&sign15){immediate += 0xFFFF0000;}//sign extend offset
				vAddr = CURRENT_STATE.REGS[rs] + immediate;
				if((vAddr) & 0x00000003){break;}
				result = mem_read_32(vAddr);
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x80000000: //LB
				if(immediate&sign15){immediate += 0xFFFF0000;}//sign extend offset
				vAddr = CURRENT_STATE.REGS[rs] + immediate;
				if((vAddr) & 0x00000001){break;}
				result = mem_read_32(vAddr) & 0x000000FF;//***Double check this***
				if(result&sign7){result += 0xFFFFFF00;}//sign extend result
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0x84000000: //LH
				if(immediate&sign15){immediate += 0xFFFF0000;}//sign extend offset
				vAddr = CURRENT_STATE.REGS[rs] + immediate;
				if((vAddr) & 0x00000001){break;}
				result = mem_read_32(vAddr) & 0x0000FFFF;//***Double check this***
				if(result&sign15){result += 0xFFFF0000;}//sign extend result
				NEXT_STATE.REGS[rt] = result;
				break;
			case 0xAC000000: //SW
				if(immediate&sign15){immediate += 0xFFFF0000;}//sign extend offset
				vAddr = CURRENT_STATE.REGS[rs] + immediate;
				if((vAddr) & 0x00000003){break;}
				mem_write_32(vAddr, CURRENT_STATE.REGS[rt]);
				break;
			case 0xA0000000: //SB
				if(immediate&sign15){immediate += 0xFFFF0000;}//sign extend offset
				vAddr = CURRENT_STATE.REGS[rs] + immediate;
				if((vAddr) & 0x00000001){break;}//address error exception
				mem_write_32(vAddr, (CURRENT_STATE.REGS[rt] & 0x000000FF));
				break;
			case 0xA4000000: //SH
				if(immediate&sign15){immediate += 0xFFFF0000;}//sign extend offset
				vAddr = CURRENT_STATE.REGS[rs] + immediate;
				if((vAddr) & 0x00000001){break;}//address error exception
				mem_write_32(vAddr, (CURRENT_STATE.REGS[rt] & 0x0000FFFF));
				break;
			case 0x10000000: //BEQ
				immediate = immediate << 2;
				if((immediate>>2)&sign15){immediate += 0xFFFC0000;}
				if(CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]){
					NEXT_STATE.PC = CURRENT_STATE.PC + immediate;
				}
				break;
			case 0x14000000: //BNE
				immediate = immediate << 2;
				if((immediate>>2)&sign15){immediate += 0xFFFC0000;}
				if(CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]){
					NEXT_STATE.PC = CURRENT_STATE.PC + immediate;
				}
				break;
			case 0x18000000: //BLEZ
				immediate = immediate << 2;
				if((immediate>>2)&sign15){immediate += 0xFFFC0000;}
				if(CURRENT_STATE.REGS[rs]&sign31 || CURRENT_STATE.REGS[rs]==0x0){
 			 		NEXT_STATE.PC = CURRENT_STATE.PC + immediate;
 	 		 	}
				break;
			case 0x1C000000: //BGTZ
				immediate = immediate << 2;
				if((immediate>>2)&sign15){immediate += 0xFFFC0000;}
				if(CURRENT_STATE.REGS[rs]&sign31 && CURRENT_STATE.REGS[rs]==0x0){
					NEXT_STATE.PC = CURRENT_STATE.PC + immediate;
				}
				break;
			case 0x08000000: //J
				NEXT_STATE.PC = (CURRENT_STATE.PC&0xF0000000) + (target<<2);
				break;
			case 0x0C000000: //JAL
				NEXT_STATE.REGS[31] = CURRENT_STATE.PC+8;
				NEXT_STATE.PC = (CURRENT_STATE.PC&0xF0000000) + (target<<2);
				break;
			case 0x3C000000: //LUI
				NEXT_STATE.REGS[rt] = immediate<<16;
				break;
			default:
				printf("NORMAL ERROR\n");
				break;
		}
	}
}


/************************************************************/
/* Initialize Memory                                                                                                    */
/************************************************************/
void initialize() {
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */
/************************************************************/
void print_program(){
	uint32_t instr, opc;
	uint32_t rs, rt, rd, sa, immediate, target;

	instr = mem_read_32(CURRENT_STATE.PC);
	rs = (instr&0x03E00000) >> 21;
	rt = (instr&0x001F0000) >> 16;
	rd = (instr&0x0000F800) >> 11;
	sa = (instr&0x000001C0) >> 6;
	immediate = instr&0x0000FFFF;
	target = instr&0x03FFFFFF;
	opc = instr&0xFC000000;
	if(opc == 0x00000000){//SPECIAL INSTRUCTION
		opc = instr&0x0000003F;
		switch(opc){
			case 0x00000020: //ADD
				printf("ADD %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000021: //ADDU
				printf("ADDU %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000022: //SUB
				printf("SUB %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000023: //SUBU
				printf("SUBU %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000018: //MULT
				printf("MULT %s, %s\n", REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000019: //MULTU
				printf("MULTU %s, %s\n", REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x0000001A: //DIV
				printf("DIV %s, %s\n", REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x0000001B: //DIVU
				printf("DIVU %s, %s\n", REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000024: //AND
				printf("AND %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000025: //OR
				printf("OR %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000026: //XOR
				printf("XOR %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000027: //NOR
				printf("NOR %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x0000002A: //SLT
				printf("SLT %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000002: //SRL
				printf("SLT %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs], REG_LOOKUP[rt]);
				break;
			case 0x00000003: //SRA
				printf("SRA %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rt], REG_LOOKUP[sa]);
				break;
			case 0x00000010: //MFHI
				printf("MFHI %s\n", REG_LOOKUP[rd]);
				break;
			case 0x00000012: //MFLO
				printf("MFLO %s\n", REG_LOOKUP[rd]);
				break;
			case 0x00000011: //MTHI
				printf("MTHI %s\n", REG_LOOKUP[rs]);
				break;
			case 0x00000013: //MTLO
				printf("MTLO %s\n", REG_LOOKUP[rs]);
				break;
			case 0x00000008: //JR
				printf("JR %s\n", REG_LOOKUP[rs]);
				break;
			case 0x00000009: //JALR
				printf("JALR %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rs]);
				break;
			case 0x00000000: //SLL
				printf("SLL %s, %s, %s\n", REG_LOOKUP[rd], REG_LOOKUP[rt], REG_LOOKUP[sa]);
				break;
			case 0x0000000C: //SYSCALL
				printf("SYSCALL\n");
				break;
			default:
				printf("SPECIAL ERROR\n");
				break;
		}
	}else if(opc == 0x04000000){//REGIMM INSTRUCTION
		opc = instr&0x001F0000;
		switch(opc){
			case 0x00010000: //BGEZ
				printf("BGEZ %s, 0x%08x\n", REG_LOOKUP[rs], immediate);
				break;
			case 0x00000000: //BLTZ
				printf("BLTZ %s, 0x%08x\n", REG_LOOKUP[rs], immediate);
				break;
			default:
				printf("REGIMM ERROR\n");
				break;
		}
	}else{//NORMAL INSTRUCTION
		switch(opc){
			case 0x20000000: //ADDI
				printf("ADDI %s, %s, 0x%08x\n", REG_LOOKUP[rt], REG_LOOKUP[rs], immediate);
				break;
			case 0x24000000: //ADDIU
				printf("ADDIU %s, %s, 0x%08x\n", REG_LOOKUP[rt], REG_LOOKUP[rs], immediate);
				break;
			case 0x30000000: //ANDI
				printf("ANDI %s, %s, 0x%08x\n", REG_LOOKUP[rt], REG_LOOKUP[rs], immediate);
				break;
			case 0x34000000: //ORI
				printf("ORI %s, %s, 0x%08x\n", REG_LOOKUP[rt], REG_LOOKUP[rs], immediate);
				break;
			case 0x38000000: //XORI
				printf("XORI %s, %s, 0x%08x\n", REG_LOOKUP[rt], REG_LOOKUP[rs], immediate);
				break;
			case 0x28000000: //SLTI
				printf("SLTI %s, %s, 0x%08x\n", REG_LOOKUP[rt], REG_LOOKUP[rs], immediate);
				break;
			case 0x8C000000: //LW
				printf("LW %s, 0x%08x(%s)", REG_LOOKUP[rt], immediate, REG_LOOKUP[rs]);
				break;
			case 0x80000000: //LB
				printf("LB %s, 0x%08x(%s)", REG_LOOKUP[rt], immediate, REG_LOOKUP[rs]);
				break;
			case 0x84000000: //LH
				printf("LH %s, 0x%08x(%s)", REG_LOOKUP[rt], immediate, REG_LOOKUP[rs]);
				break;
			case 0xAC000000: //SW -- SW rt, offset(base)
				printf("SW %s, 0x%08x(%s)", REG_LOOKUP[rt], immediate, REG_LOOKUP[rs]);
				break;
			case 0xA0000000: //SB
				printf("SB %s, 0x%08x(%s)", REG_LOOKUP[rt], immediate, REG_LOOKUP[rs]);
				break;
			case 0xA4000000: //SH
				printf("SH %s, 0x%08x(%s)", REG_LOOKUP[rt], immediate, REG_LOOKUP[rs]);
				break;
			case 0x10000000: //BEQ
				printf("BEQ %s, %s, 0x%08x\n", REG_LOOKUP[rs], REG_LOOKUP[rt], immediate);
				break;
			case 0x14000000: //BNE
				printf("BNE %s, %s, 0x%08x\n", REG_LOOKUP[rs], REG_LOOKUP[rt], immediate);
				break;
			case 0x18000000: //BLEZ
				printf("BLEZ %s, 0x%08x\n", REG_LOOKUP[rs], immediate);
				break;
			case 0x1C000000: //BGTZ
				printf("BGTZ %s, 0x%08x\n", REG_LOOKUP[rs], immediate);
				break;
			case 0x08000000: //J
				printf("J 0x%08x\n", target);
				break;
			case 0x0C000000: //JAL
				printf("JAL 0x%08x\n", target);
				break;
			case 0x3C000000: //LUI
				printf("LUI %s, 0x%08x\n", REG_LOOKUP[rt], immediate);
				break;
			default:
				printf("NORMAL ERROR\n");
				break;
		}
	}
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {
	HI_CTR = 0x0;
	LO_CTR = 0x0;

	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");

	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
