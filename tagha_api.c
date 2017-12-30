
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"


/* Tagha File Structure (Dec 23, 2017)
 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack segment size (aligned by 8 bytes)
 * 4 bytes: data segment size
 * 1 byte: safemode and debugmode flags
 * ------------------------------ end of header ------------------------------
 * .natives table
 * 4 bytes: amount of natives
 * n bytes: native table
 * 		4 bytes: string size + '\0' of native string
 * 		n bytes: native string.
 * 
 * .functions table
 * 4 bytes: amount of functions
 * n bytes: functions table
 * 		4 bytes: string size + '\0' of func string
 * 		n bytes: function string
 * 		4 bytes: offset
 * 
 * .globalvars table
 * 4 bytes: amount of global vars
 * n bytes: global vars table
 * 		4 bytes: string size + '\0' of global var string
 * 		n bytes: global var string
 * 		4 bytes: offset
 * 
 * n bytes: .data section initial values.
 * n bytes: .text section
 */

static uint64_t get_file_size(FILE *pFile);
static uint32_t scripthdr_read_natives_table(struct Tagha **ppSys, FILE **ppFile);
static uint32_t scripthdr_read_func_table(struct Tagha **ppSys, FILE **ppFile);
static uint32_t scripthdr_read_global_table(struct Tagha **ppSys, FILE **ppFile);


struct Tagha *Tagha_New(void)
{
	struct Tagha *pNewVM = calloc(1, sizeof(struct Tagha));
	Tagha_Init(pNewVM);
	return pNewVM;
}

void Tagha_Init(struct Tagha *pSys)
{
	if( !pSys )
		return;
	
	*pSys = (struct Tagha){0};
	
	if( !pSys->m_pmapNatives ) {
		pSys->m_pmapNatives = calloc(1, sizeof(struct Hashmap));
		if( !pSys->m_pmapNatives )
			printf("[%sTagha Init Error%s]: **** %sUnable to initialize Native Map%s ****\n", KRED, RESET, KGRN, RESET);
		else Map_Init(pSys->m_pmapNatives);
	}
	if( !pSys->m_pArgv ) {
		pSys->m_iArgc = 0;
		pSys->m_pArgv = calloc(pSys->m_iArgc+1, sizeof(union CValue));
		if( !pSys->m_pArgv )
			printf("[%sTagha Init Error%s]: **** %sUnable to initialize Command-line Args String Vector%s ****\n", KRED, RESET, KGRN, RESET);
		pSys->m_pArgv[pSys->m_iArgc].Str = NULL;
	}
}

static bool is_c_file(const char *filename)
{
	if( !filename )
		return false;
	
	const char *p = filename;
	// iterate to end of string and then check backwards.
	while( *p )
		p++;
	return( (*(p-1)=='c' or *(p-1)=='C') and (*(p-2)=='.') );
}
static bool is_tbc_file(const char *filename)
{
	if( !filename )
		return false;
	
	const char *p = filename;
	// iterate to end of string and then check backwards.
	while( *p )
		p++;
	return( (*(p-3)=='t' and *(p-2)=='b' and *(p-1)=='c') or (*(p-3)=='T' and *(p-2)=='B' and *(p-1)=='C') and *(p-4)=='.' );
}


void Tagha_LoadScriptByName(struct Tagha *pSys, char *restrict strFilename)
{
	if( !pSys )
		return;
	
	// allocate our script.
	Tagha_BuildFromFile(pSys, strFilename);
	
	// set up our standard I/O streams
	// and global self-referencing script ptr
	// Downside is that the script-side host var MUST be a pointer.
	if( pSys->m_pmapGlobals ) {
		FILE **ppFile=Tagha_GetGlobalByName(pSys, "stdin");
		if( ppFile )
			*ppFile = stdin;
		
		ppFile = Tagha_GetGlobalByName(pSys, "stderr");
		if( ppFile )
			*ppFile = stderr;
		
		ppFile = Tagha_GetGlobalByName(pSys, "stdout");
		if( ppFile )
			*ppFile = stdout;
		
		struct Tagha **ppSelf=Tagha_GetGlobalByName(pSys, "self");
		if( ppSelf )
			*ppSelf = pSys;
	}
}

void Tagha_LoadScriptFromMemory(struct Tagha *restrict pSys, void *restrict pMemory, const uint64_t memsize)
{
	if( !pSys )
		return;
	
	// allocate our script.
	Tagha_BuildFromPtr(pSys, pMemory, memsize);
	
	// set up our standard I/O streams
	// and global self-referencing script ptr
	// Downside is that the script-side host var MUST be a pointer.
	if( pSys->m_pmapGlobals ) {
		FILE **ppFile=Tagha_GetGlobalByName(pSys, "stdin");
		if( ppFile )
			*ppFile = stdin;
		
		ppFile = Tagha_GetGlobalByName(pSys, "stderr");
		if( ppFile )
			*ppFile = stderr;
		
		ppFile = Tagha_GetGlobalByName(pSys, "stdout");
		if( ppFile )
			*ppFile = stdout;
		
		struct Tagha **ppSelf=Tagha_GetGlobalByName(pSys, "self");
		if( ppSelf )
			*ppSelf = pSys;
	}
}


bool Tagha_RegisterNatives(struct Tagha *pSys, struct NativeInfo arrNatives[])
{
	if( !pSys or !pSys->m_pmapNatives or !arrNatives )
		return false;
	
	for( struct NativeInfo *n=arrNatives ; n->pFunc and n->strName ; n++ )
		Map_Insert(pSys->m_pmapNatives, n->strName, (uintptr_t)n->pFunc);
	return true;
}


int32_t Tagha_RunScript(struct Tagha *pSys)
{
	if( !pSys or !pSys->m_pMemory )
		return -1;
	
	// make sure 'main' exists.
	if( !pSys->m_pmapFuncs ) {
		Tagha_PrintErr(pSys, __func__, "Cannot call main with a NULL function table!");
		return -1;
	}
	// make sure we have the memory for running.
	else if( ((pSys->m_Regs[rsp].UCharPtr-pSys->m_pMemory)-32) >= pSys->m_uiMemsize ) {
		Tagha_PrintErr(pSys, __func__, "stack overflow!");
		return -1;
	}
	// get instruction offset to main.
	uint32_t *offset = (uint32_t *)(uintptr_t)Map_Get(pSys->m_pmapFuncs, "main");
	if( !offset ) {
		Tagha_PrintErr(pSys, __func__, "function \'main\' doesn't exist!");
		return -1;
	}
	
	pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + *offset;
	
	// push argv and argc to registers.
	// use 'uintptr_t' so we can force 4-byte pointers as 8-byte.
	pSys->m_Regs[res].UInt64 = (uintptr_t)pSys->m_pArgv;
	pSys->m_Regs[rds].Int64 = pSys->m_iArgc;
	
	(--pSys->m_Regs[rsp].SelfPtr)->Int64 = -1L;	// push bullshit ret address.
	(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = pSys->m_Regs[rbp].UInt64; // push rbp
	
	
	if( pSys->m_bDebugMode )
		printf("Tagha_RunScript :: pushed argc: %" PRIi32 " and argv %p\n", pSys->m_iArgc, pSys->m_pArgv);
	
	return Tagha_Exec(pSys);
}

int32_t Tagha_CallFunc(struct Tagha *restrict pSys, const char *restrict strFunc)
{
	if( !pSys or !strFunc )
		return -1;
	
	if( !pSys->m_pmapFuncs ) {
		Tagha_PrintErr(pSys, __func__, "Cannot call functions using a NULL function table!");
		return -1;
	}
	else if( ((pSys->m_Regs[rsp].UCharPtr-pSys->m_pMemory)-16) >= pSys->m_uiMemsize ) {
		Tagha_PrintErr(pSys, __func__, "stack overflow!");
		return -1;
	}
	
	uint32_t *offset = (uint32_t *)(uintptr_t)Map_Get(pSys->m_pmapFuncs, strFunc);
	if( !offset ) {
		Tagha_PrintErr(pSys, __func__, "function \'%s\' doesn't exist!", strFunc);
		return -1;
	}
	
	// save return address.
	(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = (uintptr_t)pSys->m_Regs[rip].UCharPtr+1;
	
	// jump to the function entry address.
	pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + *offset;
	
	// push bp and copy sp to bp.
	(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = (uintptr_t)pSys->m_Regs[rbp].UCharPtr;
	
	return Tagha_Exec(pSys);
}


// need this to determine the text segment size.
static uint64_t get_file_size(FILE *pFile)
{
	uint64_t size = 0L;
	if( !pFile )
		return size;
	
	if( !fseek(pFile, 0, SEEK_END) ) {
		size = (uint64_t)ftell(pFile);
		rewind(pFile);
	}
	return size;
}

static uint32_t scripthdr_read_natives_table(struct Tagha **ppSys, FILE **ppFile)
{
	if( !*ppSys or !*ppFile )
		return 0;
	
	struct Tagha *pSys = *ppSys;
	uint32_t	bytecount = 0;
	int32_t		ignores = 0;
	
	// see if the script is using any natives.
	pSys->m_pstrNativeCalls = NULL;
	ignores = fread(&pSys->m_uiNatives, sizeof(uint32_t), 1, *ppFile);
	printf("[Tagha Load Script] :: Amount of Natives: \'%" PRIu32 "\'\n", pSys->m_uiNatives);
	bytecount += sizeof(uint32_t);
	if( !pSys->m_uiNatives )
		return bytecount;
	
	// script has natives? Copy their names so we can use them on VM natives hashmap later.
	pSys->m_pstrNativeCalls = calloc(pSys->m_uiNatives, sizeof(char *));
	if( !pSys->m_pstrNativeCalls ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native Table%s ****\n", KRED, RESET, KGRN, RESET);
		Tagha_Free(pSys), *ppSys = NULL;
		fclose(*ppFile), *ppFile=NULL;
		return 0;
	}
	
	for( uint32_t i=0 ; i<pSys->m_uiNatives ; i++ ) {
		uint32_t str_size;
		ignores = fread(&str_size, sizeof(uint32_t), 1, *ppFile);
		bytecount += sizeof(uint32_t);
		
		// allocate memory to hold the native's name.
		pSys->m_pstrNativeCalls[i] = calloc(str_size, sizeof(char));
		if( !pSys->m_pstrNativeCalls[i] ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native String%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys), *ppSys = NULL;
			fclose(*ppFile), *ppFile=NULL;
			return 0;
		}
		
		// read in the native's name.
		ignores = fread(pSys->m_pstrNativeCalls[i], sizeof(char), str_size, *ppFile);
		bytecount += str_size;
		printf("[Tagha Load Script] :: Copied Native Name: \'%s\' @ %p\n", pSys->m_pstrNativeCalls[i], pSys->m_pstrNativeCalls+i);
	}
	pSys = NULL;
	return bytecount;
}

static uint32_t scripthdr_read_func_table(struct Tagha **ppSys, FILE **ppFile)
{
	if( !*ppSys or !*ppFile )
		return 0;
	
	struct Tagha *pSys = *ppSys;
	uint32_t	bytecount = 0;
	int32_t		ignore_warns = 0;
	
	// see if the script has its own functions.
	// This table is so host or other script can call these functions by name or address.
	ignore_warns = fread(&pSys->m_uiFuncs, sizeof(uint32_t), 1, *ppFile);
	bytecount += sizeof(uint32_t);
	if( !pSys->m_uiFuncs )
		return bytecount;
	
	// allocate our function hashmap so we can call functions by name.
	pSys->m_pmapFuncs = calloc(1, sizeof(struct Hashmap));
	if( !pSys->m_pmapFuncs ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Hashmap%s ****\n", KRED, RESET, KGRN, RESET);
		Tagha_Free(pSys), *ppSys = NULL;
		fclose(*ppFile), *ppFile=NULL;
		return 0;
	}
	Map_Init(pSys->m_pmapFuncs);
	
	// copy the function data from the header.
	for( uint32_t i=0 ; i<pSys->m_uiFuncs ; i++ ) {
		uint32_t str_size;
		ignore_warns = fread(&str_size, sizeof(uint32_t), 1, *ppFile);
		bytecount += sizeof(uint32_t);
		
		// allocate the hashmap function key.
		char *strFunc = calloc(str_size, sizeof(char));
		if( !strFunc ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Table String%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys), *ppSys = NULL;
			fclose(*ppFile), *ppFile=NULL;
			return 0;
		} /* if */
		ignore_warns = fread(strFunc, sizeof(char), str_size, *ppFile);
		bytecount += str_size;
		
		// copy func's header data to our table
		// then store the table to our function hashmap with the key
		// we allocated earlier.
		uint32_t *offset = calloc(1, sizeof(uint32_t));
		if( !offset ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Table Offset%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys), *ppSys = NULL;
			fclose(*ppFile), *ppFile=NULL;
			return 0;
		} /* if */
		
		ignore_warns = fread(offset, sizeof(uint32_t), 1, *ppFile);
		bytecount += sizeof(uint32_t);
		printf("[Tagha Load Script] :: Copied Function name \'%s\' | offset: %" PRIu32 "\n", strFunc, *offset);
		Map_Insert(pSys->m_pmapFuncs, strFunc, (uintptr_t)offset);
		strFunc = NULL;
	} /* for */
	pSys = NULL;
	return bytecount;
}

static uint32_t scripthdr_read_global_table(struct Tagha **ppSys, FILE **ppFile)
{
	if( !*ppSys or !*ppFile )
		return 0;
	
	struct Tagha *pSys = *ppSys;
	uint32_t	bytecount = 0;
	int32_t		ignore_warns = 0;
	
	// check if the script has global variables.
	ignore_warns = fread(&pSys->m_uiGlobals, sizeof(uint32_t), 1, *ppFile);
	printf("[Tagha Load Script] :: Amount of Global Vars: %" PRIu32 "\n", pSys->m_uiGlobals);
	bytecount += sizeof(uint32_t);
	pSys->m_pmapGlobals = NULL;
	uint32_t globalbytes = 0;
	if( !pSys->m_uiGlobals )
		return bytecount;
	
	// script has globals, allocate our global var hashmap.
	pSys->m_pmapGlobals = calloc(1, sizeof(struct Hashmap));
	if( !pSys->m_pmapGlobals ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var Hashmap%s ****\n", KRED, RESET, KGRN, RESET);
		Tagha_Free(pSys), *ppSys = NULL;
		fclose(*ppFile), *ppFile=NULL;
		return 0;
	}
	Map_Init(pSys->m_pmapGlobals);
	
	for( uint32_t i=0 ; i<pSys->m_uiGlobals ; i++ ) {
		uint32_t str_size;
		ignore_warns = fread(&str_size, sizeof(uint32_t), 1, *ppFile);
		bytecount += sizeof(uint32_t);
		// allocate string to use as a key for our global var.
		char *strGlobal = calloc(str_size, sizeof(char));
		if( !strGlobal ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var String%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys), *ppSys = NULL;
			fclose(*ppFile), *ppFile=NULL;
			return 0;
		}
		ignore_warns = fread(strGlobal, sizeof(char), str_size, *ppFile);
		bytecount += str_size;
		
		// read the global var's data from the header and add it to our hashmap.
		// same procedure as our function hashmap.
		uint32_t *offset = calloc(1, sizeof(uint32_t));
		if( !offset ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var Offset%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys), *ppSys = NULL;
			fclose(*ppFile), *ppFile=NULL;
			return 0;
		} /* if */
		
		ignore_warns = fread(offset, sizeof(uint32_t), 1, *ppFile);
		bytecount += sizeof(uint32_t);
		
		printf("[Tagha Load Script] :: copied global var's name: \'%s\' | offset: %" PRIu32 "\n", strGlobal, *offset);
		
		// insert the global var's table to our hashmap.
		Map_Insert(pSys->m_pmapGlobals, strGlobal, (uintptr_t)offset);
		strGlobal = NULL;
	} /* for( uint32_t i=0 ; i<pSys->m_uiGlobals ; i++ ) */
	pSys = NULL;
	return bytecount;
}

void Tagha_BuildFromFile(struct Tagha *pSys, const char *restrict strFilename)
{
	if( !pSys or !strFilename )
		return;
	
	// open up our script in read-only binary mode.
	FILE *pFile = fopen(strFilename, "rb");
	if( !pFile ) {
		printf("[%sTagha Load Script Error%s]: **** %sFile not found: \'%s\'%s ****\n", KRED, RESET, KGRN, strFilename, RESET);
		return;
	}
	
	// get_file_size stays here, it'll rewind the file 'cursor' back to beginning.
	uint64_t filesize = get_file_size(pFile);
	
	uint32_t bytecount = 0;	// bytecount is for separating the header data from the actual instruction stream.
	uint16_t verify;
	int32_t ignore_warns;
	ignore_warns = fread(&verify, sizeof(uint16_t), 1, pFile);
	bytecount += sizeof(uint16_t);
	
	if( verify != 0xC0DE ) {
		printf("[%sTagha Load Script Error%s]: **** %sUnknown or Invalid script file format%s ****\n", KRED, RESET, KGRN, RESET);
		goto error;
	}
	puts("[Tagha Load Script] :: Verified Script!\n");
	
	// get stack memory size.
	uint32_t stacksize, datasize;
	ignore_warns = fread(&stacksize, sizeof(uint32_t), 1, pFile);
	bytecount += sizeof(uint32_t);
	stacksize = stacksize<0x1000 ? 0x1000 : stacksize;
	
	// align by 8 bytes
	stacksize = stacksize + 7 & -8;
	printf("[Tagha Load Script] :: Stack Size: %" PRIu32 "\n", stacksize);
	
	// get static data (global vars, string literals) memory size.
	ignore_warns = fread(&datasize, sizeof(uint32_t), 1, pFile);
	bytecount += sizeof(uint32_t);
	printf("[Tagha Load Script] :: Data Size: %" PRIu32 "\n", datasize);
	
	// check if the script is either in safemode or debug mode.
	char modeflags;
	ignore_warns = fread(&modeflags, sizeof(char), 1, pFile);
	bytecount += sizeof(char);
	pSys->m_bSafeMode = (modeflags & 1) >= 1;
	printf("[Tagha Load Script] :: Safe Mode: %" PRIu32 "\n", pSys->m_bSafeMode);
	
	pSys->m_bDebugMode = (modeflags & 2) >= 1;
	printf("[Tagha Load Script] :: Debug Mode: %" PRIu32 "\n", pSys->m_bDebugMode);
	
	bytecount += scripthdr_read_natives_table(&pSys, &pFile);
	bytecount += scripthdr_read_func_table(&pSys, &pFile);
	if( !pFile )
		return;
	
	bytecount += scripthdr_read_global_table(&pSys, &pFile);
	if( !pFile )
		return;
	
	pSys->m_uiMaxInstrs = 0xffff;	// helps to stop infinite/runaway loops
	
	
	// header data is finished, subtract the filesize with the bytecount
	// and data segment size to get the size of our instruction stream.
	// If the instruction stream is invalid, we can't load the script.
	printf("[Tagha Load Script] :: Header Byte Count: %" PRIu32 "\n", bytecount);
	pSys->m_uiInstrSize = filesize - bytecount - datasize;
	
	pSys->m_uiMemsize = stacksize + datasize + pSys->m_uiInstrSize;
	pSys->m_pMemory = calloc(pSys->m_uiMemsize, sizeof(uint8_t));
	
	// scripts NEED memory, if memory is invalid then we can't use the script.
	if( !pSys->m_pMemory ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for script%s ****\n", KRED, RESET, KGRN, RESET);
		Tagha_Free(pSys), pSys = NULL;
		goto error;
	}
	
	/*	+------------------+
		|    stack   |     |      highest address
		|            v     |
		|                  |
		|                  |
		|                  |
		|                  |
		|                  |
		|                  |
		+------------------+      stack segment
		| data segment     |
		+------------------+
		| text segment     |      lowest address
		+------------------+
	*/
	
	// set up the stack ptrs to the top of the stack.
	pSys->m_Regs[rsp].UCharPtr = pSys->m_Regs[rbp].UCharPtr = pSys->m_pMemory + (pSys->m_uiMemsize-1);
	
	// set up our stack segment by taking the top of the stack and subtracting it with the size.
	pSys->m_pStackSegment = pSys->m_Regs[rsp].UCharPtr - (stacksize-1);
	
	// set up the data segment by taking the stack segment - 1
	pSys->m_pDataSegment = pSys->m_pStackSegment - 1;
	
	// set up the text segment similar to how the stack ptrs are set up
	pSys->m_pTextSegment = pSys->m_pMemory + (pSys->m_uiInstrSize-1);
	
	// copy the initialized values to the data segment if there's any global values.
	ignore_warns = fread(pSys->m_pTextSegment+1, sizeof(uint8_t), pSys->m_pDataSegment-pSys->m_pTextSegment, pFile);
	
	// instruction stream is valid, copy every last instruction.
	ignore_warns = fread(pSys->m_pMemory, sizeof(uint8_t), pSys->m_uiInstrSize, pFile);
	
	puts("Tagha_BuildFromFile :: Finished Reading Header!\n");
error:;
	fclose(pFile), pFile=NULL;
}

void Tagha_BuildFromPtr(struct Tagha *restrict pSys, void *restrict pProgram, const uint64_t Programsize)
{
	if( !pSys or !pProgram )
		return;
	
	union CValue Reader = (union CValue){.UCharPtr = pProgram};
	uint32_t bytecount = 0;	// bytecount is for separating the header data from the actual instruction stream.
	
	if( *Reader.UShortPtr != 0xC0DE ) {
		printf("[%sTagha Load Script Error%s]: **** %sUnknown or Invalid Script file format%s ****\n", KRED, RESET, KGRN, RESET);
		return;
	}
	bytecount += sizeof(uint16_t);
	Reader.UShortPtr++;
	
	puts("[Tagha Load Script Ptr] :: Verified Script!\n");
	
	// get stack memory size.
	uint32_t stacksize, datasize;
	stacksize = *Reader.UInt32Ptr++;
	bytecount += sizeof(uint32_t);
	
	stacksize = stacksize<0x1000 ? 0x1000 : stacksize;
	stacksize = stacksize + 7 & -8;	// align size by 8 bytes
	printf("[Tagha Load Script Ptr] :: Stack Size: %" PRIu32 "\n", stacksize);
	
	// get static data (global vars, string literals) memory size.
	datasize = *Reader.UInt32Ptr++;
	bytecount += sizeof(uint32_t);
	printf("[Tagha Load Script Ptr] :: Data Size: %" PRIu32 "\n", datasize);
	
	
	char modeflags = *Reader.CharPtr++;
	bytecount += sizeof(char);
	pSys->m_bSafeMode = (modeflags & 1) >= 1;
	printf("[Tagha Load Script Ptr] :: script Safe Mode: %" PRIu32 "\n", pSys->m_bSafeMode);
	
	pSys->m_bDebugMode = (modeflags & 2) >= 1;
	printf("[Tagha Load Script Ptr] :: script Debug Mode: %" PRIu32 "\n", pSys->m_bDebugMode);
	
	// get natives table.
	pSys->m_uiNatives = *Reader.UInt32Ptr++;
	bytecount += sizeof(uint32_t);
	if( pSys->m_uiNatives ) {
		// script has natives? Copy their names so we can use them on VM natives hashmap later.
		pSys->m_pstrNativeCalls = calloc(pSys->m_uiNatives, sizeof(char *));
		if( !pSys->m_pstrNativeCalls ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native Table%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys);
			return;
		} /* if */
		
		for( uint32_t i=0 ; i<pSys->m_uiNatives ; i++ ) {
			uint32_t str_size = *Reader.UInt32Ptr++;
			bytecount += sizeof(uint32_t);
			
			// allocate memory to hold the native's name.
			pSys->m_pstrNativeCalls[i] = calloc(str_size, sizeof(char));
			if( !pSys->m_pstrNativeCalls[i] ) {
				printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native String%s ****\n", KRED, RESET, KGRN, RESET);
				Tagha_Free(pSys);
				return;
			}
			
			// read in the native's name.
			memcpy(pSys->m_pstrNativeCalls[i], Reader.Ptr, str_size);
			bytecount += str_size;
			Reader.UCharPtr += str_size;
			printf("[Tagha Load Script Ptr] :: copied native name \'%s\'\n", pSys->m_pstrNativeCalls[i]);
		} /* for( ... ) */
	} /* if( pSys->m_uiNatives ) */
	
	// see if the script has its own functions.
	// This table is so host or other script can call these functions by name or address.
	pSys->m_uiFuncs = *Reader.UInt32Ptr++;
	bytecount += sizeof(uint32_t);
	
	// if we have no functions, that means main is missing...
	if( pSys->m_uiFuncs ) {
		// allocate our function hashmap so we can call functions by name.
		pSys->m_pmapFuncs = calloc(1, sizeof(struct Hashmap));
		if( !pSys->m_pmapFuncs ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Hashmap%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys);
			return;
		}
		Map_Init(pSys->m_pmapFuncs);
		
		// copy the function data from the header.
		for( uint32_t i=0 ; i<pSys->m_uiFuncs ; i++ ) {
			uint32_t str_size = *Reader.UInt32Ptr++;
			bytecount += sizeof(uint32_t);
			
			// allocate the hashmap function key.
			char *strFunc = calloc(str_size, sizeof(char));
			if( !strFunc ) {
				printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Table String%s ****\n", KRED, RESET, KGRN, RESET);
				Tagha_Free(pSys);
				return;
			}
			memcpy(strFunc, Reader.Ptr, str_size);
			Reader.UCharPtr += str_size;
			bytecount += str_size;
			
			// copy func's header data to our table
			// then store the table to our function hashmap with the key
			// we allocated earlier.
			uint32_t *offset = calloc(1, sizeof(uint32_t));
			if( !offset ) {
				printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Offset%s ****\n", KRED, RESET, KGRN, RESET);
				Tagha_Free(pSys);
				return;
			} /* if */
			*offset = *Reader.UInt32Ptr++;
			bytecount += sizeof(uint32_t);
			
			printf("[Tagha Load Script Ptr] :: Copied Function name \'%s\' | offset: %" PRIu32 "\n", strFunc, *offset);
			Map_Insert(pSys->m_pmapFuncs, strFunc, (uintptr_t)offset);
			strFunc = NULL;
		} /* for( uint32_t i=0 ; i<pSys->m_uiFuncs ; i++ ) */
	}
	else {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native String%s ****\n", KRED, RESET, KGRN, RESET);
		Tagha_Free(pSys);
		return;
	}
	
	// check if the script has global variables.
	pSys->m_uiGlobals = *Reader.UInt32Ptr++;
	printf("[Tagha Load Script Ptr] :: Amount of Global Vars: %" PRIu32 "\n", pSys->m_uiGlobals);
	bytecount += sizeof(uint32_t);
	
	uint32_t globalbytes = 0;
	if( pSys->m_uiGlobals ) {
		// script has globals, allocate our global var hashmap.
		pSys->m_pmapGlobals = calloc(1, sizeof(struct Hashmap));
		if( !pSys->m_pmapGlobals ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var Hashmap%s ****\n", KRED, RESET, KGRN, RESET);
			Tagha_Free(pSys);
			return;
		}
		Map_Init(pSys->m_pmapGlobals);
		
		for( uint32_t i=0 ; i<pSys->m_uiGlobals ; i++ ) {
			uint32_t str_size = *Reader.UInt32Ptr++;
			bytecount += sizeof(uint32_t);
			// allocate string to use as a key for our global var.
			char *strGlobal = calloc(str_size, sizeof(char));
			if( !strGlobal ) {
				printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var String%s ****\n", KRED, RESET, KGRN, RESET);
				Tagha_Free(pSys);
				return;
			}
			memcpy(strGlobal, Reader.Ptr, str_size);
			Reader.UCharPtr += str_size;
			bytecount += str_size;
			
			// read the global var's data from the header and add it to our hashmap.
			// same procedure as our function hashmap.
			uint32_t *offset = calloc(1, sizeof(uint32_t));
			if( !offset ) {
				printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var Offset%s ****\n", KRED, RESET, KGRN, RESET);
				Tagha_Free(pSys);
				return;
			} /* if */
			*offset = *Reader.UInt32Ptr++;
			bytecount += sizeof(uint32_t);
			
			printf("[Tagha Load Script Ptr] :: copied global var's name: \'%s\' | offset: %" PRIu32 "\n", strGlobal, *offset);
			
			// insert the global var's table to our hashmap.
			Map_Insert(pSys->m_pmapGlobals, strGlobal, (uintptr_t)offset);
			strGlobal = NULL;
		} /* for( uint32_t i=0 ; i<pSys->m_uiGlobals ; i++ ) */
	}
	
	pSys->m_uiMaxInstrs = 0xffff;	// helps to stop infinite/runaway loops
	
	// header data is finished, subtract the filesize with the bytecount
	// and data segment size to get the size of our instruction stream.
	// If the instruction stream is invalid, we can't load the script.
	printf("[Tagha Load Script Ptr] :: Header Byte Count: %" PRIu32 "\n", bytecount);
	pSys->m_uiInstrSize = Programsize - bytecount - datasize;
	
	pSys->m_uiMemsize = stacksize + datasize + pSys->m_uiInstrSize;
	pSys->m_pMemory = calloc(pSys->m_uiMemsize, sizeof(uint8_t));
	
	// scripts NEED memory, if memory is invalid then we can't use the script.
	if( !pSys->m_pMemory ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for script%s ****\n", KRED, RESET, KGRN, RESET);
		Tagha_Free(pSys);
		return;
	}
	
	/*	+------------------+
		|    stack   |     |      highest address
		|            v     |
		|                  |
		|                  |
		|                  |
		|                  |
		|                  |
		|                  |
		+------------------+      stack segment
		| data segment     |
		+------------------+
		| text segment     |      lowest address
		+------------------+
	*/
	// set up the stack ptrs to the top of the stack.
	pSys->m_Regs[rsp].UCharPtr = pSys->m_Regs[rbp].UCharPtr = pSys->m_pMemory + (pSys->m_uiMemsize-1);
	
	// set up our stack segment by taking the top of the stack and subtracting it with the size.
	pSys->m_pStackSegment = pSys->m_Regs[rsp].UCharPtr - (stacksize-1);
	
	// set up the data segment by taking the stack segment - 1
	pSys->m_pDataSegment = pSys->m_pStackSegment - 1;
	
	// set up the text segment similar to how the stack ptrs are set up
	pSys->m_pTextSegment = pSys->m_pMemory + (pSys->m_uiInstrSize-1);
	
	// copy the initialized values to the data segment if there's any global values.
	memcpy(pSys->m_pTextSegment+1, Reader.Ptr, pSys->m_pDataSegment-pSys->m_pTextSegment);
	Reader.UCharPtr += pSys->m_pDataSegment-pSys->m_pTextSegment;
	
	// instruction stream is valid, copy every last instruction.
	memcpy(pSys->m_pMemory, Reader.Ptr, pSys->m_uiInstrSize);
	
	puts("\n");
}

void Tagha_Free(struct Tagha *pSys)
{
	if( !pSys )
		return;
	
	// kill memory
	FREE_MEM(pSys->m_pMemory);
	
	// free our native table
	uint32_t i, Size;
	if( pSys->m_pstrNativeCalls ) {
		for( i=0 ; i<pSys->m_uiNatives ; i++ ) {
			FREE_MEM(pSys->m_pstrNativeCalls[i]);
		}
		memset(pSys->m_pstrNativeCalls, 0, pSys->m_uiNatives);
		FREE_MEM(pSys->m_pstrNativeCalls);
	}
	// free our function hashmap and all the tables in it.
	if( pSys->m_pmapFuncs ) {
		struct KeyNode
			*kv = NULL,
			*next = NULL
		;
		Size = pSys->m_pmapFuncs->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = pSys->m_pmapFuncs->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				if( kv->pData )
					free((void *)(uintptr_t)kv->pData), kv->pData = 0;
				if( kv->strKey )
					free((char *)kv->strKey), kv->strKey = NULL;
			}
		}
		Map_Free(pSys->m_pmapFuncs);
		FREE_MEM(pSys->m_pmapFuncs);
	}
	// free our global var hashmap and all the tables in it.
	if( pSys->m_pmapGlobals ) {
		struct KeyNode
			*kv = NULL,
			*next = NULL
		;
		Size = pSys->m_pmapGlobals->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = pSys->m_pmapGlobals->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				if( kv->pData )
					free((void *)(uintptr_t)kv->pData), kv->pData = 0;
				if( kv->strKey )
					free((char *)kv->strKey), kv->strKey = NULL;
			}
		}
		Map_Free(pSys->m_pmapGlobals);
		FREE_MEM(pSys->m_pmapGlobals);
	}
	
	// since the system's native hashmap has nothing allocated,
	// we just free the hashmap's internal data and then the hashmap itself.
	if( pSys->m_pmapNatives ) {
		Map_Free(pSys->m_pmapNatives);
		FREE_MEM(pSys->m_pmapNatives);
	}
	
	// free our script argument vector.
	if( pSys->m_pArgv ) {
		for( uint32_t i=0 ; i<pSys->m_iArgc ; i++ )
			FREE_MEM(pSys->m_pArgv[i].Str);
		FREE_MEM(pSys->m_pArgv);
	}
	
	// set our stack pointer pointers to NULL
	pSys->m_Regs[rip].UCharPtr = pSys->m_Regs[rsp].UCharPtr = pSys->m_Regs[rbp].UCharPtr = NULL;
}


void Tagha_Reset(struct Tagha *pSys)
{
	if( !pSys )
		return;
	
	// resets the script without crashing Tagha and the host.
	memset(pSys->m_pTextSegment+1, 0, pSys->m_uiMemsize-pSys->m_uiInstrSize);
	pSys->m_Regs[rsp].UCharPtr = pSys->m_Regs[rbp].UCharPtr = pSys->m_pMemory + (pSys->m_uiMemsize-1);
	
	memset(pSys->m_Regs, 0, sizeof(union CValue) * rsp);
	// TODO: reset global variable data to original values?
}


void *Tagha_GetGlobalByName(struct Tagha *restrict pSys, const char *restrict strGlobalName)
{
	if( !pSys or !pSys->m_pmapGlobals )
		return NULL;
	
	// get the global's .data segment offset then return the pointer to that offset.
	uint32_t *offset = (uint32_t *)(uintptr_t)Map_Get(pSys->m_pmapGlobals, strGlobalName);
	return offset ? (pSys->m_pTextSegment+1)+ *offset : NULL;
}

void Tagha_PushValues(struct Tagha *restrict pSys, const uint32_t uiArgs, union CValue values[])
{
	if( !pSys or !pSys->m_pMemory )
		return;
	
	// remember that arguments must be passed right to left.
	// we have enough args to fit in registers.
	if( uiArgs <= 10 ) {
		memcpy(pSys->m_Regs+rds, values, sizeof(union CValue)*uiArgs);
	}
	// we have too many args, use both regs and stack.
	else if( uiArgs>10 ) {
		// first push args into reg.
		memcpy(pSys->m_Regs+rds, values, sizeof(union CValue)*10);
		
		// next, push the remaining values from last to first.
		if( pSys->m_bSafeMode and (pSys->m_Regs[rsp].SelfPtr-(uiArgs-10)) < (union CValue *)pSys->m_pStackSegment ) {
			Tagha_PrintErr(pSys, __func__, "stack overflow!");
			return;
		}
		memcpy(pSys->m_Regs[rsp].SelfPtr, values+10, sizeof(union CValue)*(uiArgs-10));
		pSys->m_Regs[rsp].SelfPtr -= (uiArgs-10);
	}
}

union CValue Tagha_PopValue(struct Tagha *pSys)
{
	union CValue val={ .UInt64=0L };
	if( !pSys or !pSys->m_pMemory ) {
		printf("[%sTagha Pop%s]: **** %pSys is NULL%s ****\n", KRED, RESET, KGRN, RESET);
		return val;
	}
	return pSys->m_Regs[ras];
}

void Tagha_SetCmdArgs(struct Tagha *restrict pSys, char *argv[])
{
	if( !pSys or !pSys->m_pMemory or !argv )
		return;
	
	// clear old arguments, if any.
	for( uint32_t i=0 ; i<pSys->m_iArgc ; i++ )
		FREE_MEM(pSys->m_pArgv[i].Str);
	
	// get the size of argument vector
	uint32_t newargc = 0;
	while( argv[++newargc] != NULL );
	
	// resize our system's argument vector.
	if( pSys->m_iArgc != newargc ) {
		pSys->m_iArgc = newargc;
		pSys->m_pArgv = realloc(pSys->m_pArgv, newargc);
	}
	
	/* For Implementing 'int argc' and 'char *argv[]'
	 * C Standards dictates the following...
	 * - The value of argc shall be nonnegative.
	 * - The parameters argc and argv and the strings pointed to by the argv array shall be modifiable by the program
	 * - argv[argc] shall be a null pointer.
	 * - If the value of argc is greater than zero, the array members argv[0] through argv[argc-1] inclusive shall contain pointers to strings, which are given implementation-defined values by the host environment prior to program startup.
	 * - If the value of argc is greater than zero, the string pointed to by argv[0] represents the program name; argv[0][0] shall be the null character if the program name is not available from the host environment. If the value of argc is greater than one, the strings pointed to by argv[1] through argv[argc-1] represent the program parameters.
	*/
	
	// Copy down our argument vector's strings.
	for( uint32_t i=0 ; i<pSys->m_iArgc ; i++ ) {
		size_t strsize = strlen(argv[i])+1;
		pSys->m_pArgv[i].Str = calloc(strsize, sizeof(char));
		
		if( pSys->m_pArgv[i].Str ) {
			strncpy(pSys->m_pArgv[i].Str, argv[i], strsize);
			pSys->m_pArgv[i].Str[strsize-1] = 0;
		}
	}
	pSys->m_pArgv[pSys->m_iArgc].Str = NULL;
}


uint32_t Tagha_GetMemSize(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_uiMemsize;
}
uint32_t Tagha_GetInstrSize(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_uiInstrSize;
}
uint32_t Tagha_GetMaxInstrs(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_uiMaxInstrs;
}
uint32_t Tagha_GetNativeCount(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_uiNatives;
}
uint32_t Tagha_GetFuncCount(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_uiFuncs;
}
uint32_t Tagha_GetGlobalsCount(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_uiGlobals;
}
bool Tagha_IsSafemodeActive(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_bSafeMode;
}
bool Tagha_IsDebugActive(const struct Tagha *pSys)
{
	return !pSys ? 0 : pSys->m_bDebugMode;
}



void Tagha_PrintStack(const struct Tagha *pSys)
{
	if( !pSys or !pSys->m_pMemory )
		return;
	
	printf("DEBUG PRINT: .stack Segment\n");
	
	uint32_t size = pSys->m_uiMemsize;
	union CValue *p = (union CValue *)(pSys->m_pMemory + (size-1));
	
	while( (uint8_t *)p >= pSys->m_pStackSegment ) {
		if( pSys->m_Regs[rsp].SelfPtr == p )
			printf("Stack[%.10" PRIu32 "] == %" PRIu64 " - T.O.S.\n", (uint8_t *)p-pSys->m_pMemory, p->UInt64);
		else printf("Stack[%.10" PRIu32 "] == %" PRIu64 "\n", (uint8_t *)p-pSys->m_pMemory, p->UInt64);
		--p;
	} /* while( p>=pSys->m_pStackSegment ) */
	printf("\n");
}

void Tagha_PrintData(const struct Tagha *pSys)
{
	if( !pSys or !pSys->m_pMemory )
		return;
	
	printf("DEBUG PRINT: .data Segment\n");
	for( uint8_t *p = pSys->m_pDataSegment ; p > pSys->m_pTextSegment ; --p )
		printf("Data[%.10" PRIu32 "] == %" PRIu32 "\n", p-pSys->m_pMemory, *p);
	
	printf("\n");
}

void Tagha_PrintInstrs(const struct Tagha *pSys)
{
	if( !pSys or !pSys->m_pMemory )
		return;
	
	printf("DEBUG PRINT: .text Segment\n");
	for( uint8_t *p = pSys->m_pMemory ; p <= pSys->m_pTextSegment ; p++ )
		printf("Text[%.10" PRIu32 "] == %" PRIu32 "\n", p-pSys->m_pMemory, *p);
	
	printf("\n");
}

void Tagha_PrintPtrs(const struct Tagha *pSys)
{
	if( !pSys )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Ptr: %p\
			\nStack Ptr: %p\
			\nStack Frame Ptr: %p\n", pSys->m_Regs[rip].UCharPtr, pSys->m_Regs[rsp].UCharPtr, pSys->m_Regs[rbp].UCharPtr);
	printf("\n");
}

void Tagha_PrintErr(struct Tagha *restrict pSys, const char *restrict funcname, const char *restrict err, ...)
{
	if( !pSys or !err )
		return;
	
	va_list args;
	va_start(args, err);
	printf("[%sTagha Error%s]: **** %s reported: \'", KRED, KNRM, funcname);
	vprintf(err, args);
	va_end(args);
	printf("\' ****\nCurrent Instr Addr: %s%p | offset: %" PRIuPTR "%s\nCurrent Stack Addr: %s%p | offset: %" PRIuPTR "%s\n",
		KGRN, pSys->m_Regs[rip].UCharPtr, pSys->m_Regs[rip].UCharPtr-pSys->m_pMemory, RESET, KGRN, pSys->m_Regs[rsp].UCharPtr, pSys->m_Regs[rsp].UCharPtr-pSys->m_pMemory, RESET);
}

void Tagha_PrintRegData(const struct Tagha *pSys)
{
	puts("\n\tPRINTING REGISTER DATA ==========================\n");
	for( uint8_t i=0 ; i<regsize ; i++ )
		printf("register[%s] == %" PRIu64 "\n", RegIDToStr(i), pSys->m_Regs[i].UInt64);
	puts("\tEND OF PRINTING REGISTER DATA ===============\n");
}








