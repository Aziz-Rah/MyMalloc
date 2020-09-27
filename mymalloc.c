#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

#define malloc ( x ) mymalloc(x,__FILE__,__LINE__)
#define free ( x ) myfree(x,__FILE__,__LINE__)

int MEMSIZE = 5000; //625 max metadata can fit
char memory[MEMSIZE]; // main memory emulated
int isInit = 0;

/*
* [isAlloc][isAlloc][isAlloc][isAlloc][size][size][size][size] = 8 bytes of Metadata
*/
typedef struct Metadata{ 
	int isAlloc;
	int size; 
} Metadata;

/*
* Set first metadata
*/
void initMem(){
	isInit = 1;
	
	Metadata* firstMd = (Metadata*) &memory;
	firstMd->size = MEMSIZE - sizeof(Metadata);
	firstMd->isAlloc = 0; 
}

/*
* Find ptr to first fit of desired space, else return NULL
*/
char* firstFit(int size){
	Metadata* pMetadata = (Metadata*)&memory;	// start with first block
	int sizeOfMemory = sizeof(memory); 
	while (pMetadata < (sizeOfMemory-sizeof(Metadata))){ // traverse memory until find free spot or no spot
		if (isAlloc(pMetadata) == 0 && getSize(pMetadata) >= size){ // found free space && fits
			return (char*)(&pMetadata+sizeof(Metadata)); // return ptr past metadata
		}
		pMetadata = pMetadata+getSize(Metadata); // move ptr to next block 
	}
	return NULL; // no avl space => return null pointer
}

/*
* Metadata helper methods (inputs ptr of malloc return, not metadata ptr)
*/
char* getPtrMetadata(char* ptr){
	if (ptr == NULL){
		return NULL; 
	}
	return &ptr - sizeof(Metadata);
}
int isAlloc(char* ptr){ // boolean
	if (ptr == NULL){
		return -1; 
	}
	ptr = &ptr - sizeof(Metadata);
	return (Metadata*)ptr->isAlloc;
} 
int getSize(char* ptr){
	if (ptr == NULL){
		return -1; 
	}	
	ptr = &ptr - sizeof(Metadata);
	return (Metadata*)ptr->size;
}

/*
* Input pointer to data and size, set metadata size
*/
void setSize(char* ptr, int size){
	if (size < 0){
		printf("Critical Error: size = %d", size);
		return;
	}
	Metadata* pMetadata = (Metadata*)(getPtrMetadata(ptr));
	pMetadata->size = size; // note: size set to be even
}

/*
* Input pointer to data and boolAlloc, set metadata boolAlloc
*/
void setAlloc(char* ptr, int boolAlloc){
	if (boolAlloc != 1 || boolAlloc != 0){
		printf("Critical Error: boolAlloc = %d", boolAlloc);		
		return;
	}
	Metadata* pMetadata = (Metadata*)(getPtrMetadata(ptr));
	pMetadata->isAlloc = boolAlloc;
}

/*
* Detectable free errors
*/
int checkValidPtr(char* ptrToCheck){
	//Traverse memory O(n) to find matching ptr location: see if alrdy freed or invalid location
	char* ptr = memory+4;
	int sizeOfMemory = sizeof(memory);
	while (ptr < sizeOfMemory && ptr < ptrToCheck){ //traverse mem until past size or past ptrToCheck location
		if (ptrToCheck == ptr){ //matches ptr
			if ((Metadata*)ptr->isAlloc == 1){ //is allocated
				return 1;
			}
			printf("Error: Already free pointer");
			return -1; 
		}
	}
	printf("Error: Pointer never allocated before");
	return -1;
}

/*
*  Traverse O(n) through memory and merge 2 free consecutive blocks
*/
void coalesceFree(){
	Metadata* nextMetadata = (Metadata*)&memory;
	Metadata* prevMetadata = NULL;
	while (nextMetadata < memory[MEMSIZE]){
		if (isAlloc(prevMetadata) == 0 && isAlloc(nextMetadata) == 0)){ //both pointers are free
			//merge both blocks
			int mergeSize = getSize(prevMetadata) + getSize(nextMetadata) + sizeof(Metadata);
			prevMetadata->size = mergeSize; // merge
			printf("coalesce size = %d", mergeSize);
		}
		prevMetadata = nextMetadata;
		nextMetadata = nextMetadata + getSize(nextMetadata+sizeof(Metadata)); //to next metadata
	}
}

/*
* Input size to clear, clear size
*/
void clearBlock(char* ptr, int size){
	int i;
	for (i = 0; i < size; i++){
		ptr[i] = '\0';
	}
}

/*
* Input size to clear, clear size
*/
void printErr(char *errMsg, char* fileName, int lineNum){
	printf("ERROR in %s LINE %d: %s\n", fileName, lineNum, msg);
}





/*
****************************************************** mymalloc
****************************************************** free
*/






void* mymalloc(int size, char *file, int line){
	// setup
	if (isInit == 0){ // initialize memory
		initMem();  //init to create first metadata
	}
	if (size <= 0){ // error check
		printErr("Cannot malloc non-positive number of bytes", file, line);
	}
	if (size % 2 == 1){ // if size is odd, make even (+1)
		size++;
	}

    char* pStart = firstFit(size); // find free block that fits
	if (pStart == NULL){
		printErr("No space left in memory", fileName, lineNum);
		return NULL;
	}
	
	// Save metadata info for later
	int prevSize = getSize(pStart); // current size of block   
	int nextSize = prevSize - size; //size of next block
	int newSize = size; // new size when block is malloc
	
	// set info on malloc block
	setAlloc(pStart, 1);
	setSize(pStart, newSize);
	
	// set info on remaining free block
	if (nextSize > 0){
		char* pNext = pStart + newSize + sizeof(Metadata); 
		setAlloc(pNext, 0);
		setSize(pNext, nextSize);
	}
	
	//conclusion
	printf("malloc %d bytes at address: %p", newSize, pStart); // debug, slows down program
    return (void*) pStart;
}

/*
*  Check if valid ptr of malloc start
*  Free malloc and clear bytes to '\0' 
*  Coalesce blocks 
*/
void myfree(void *vPtr, char *file, int line){
	char* ptr = (char*)vPtr; //convert from void ptr
	
	//Error case: null ptr
	if (ptr == NULL){
		printErr("Cannot free NULL pointer", file, line);
		badFrees++;
		return; 
	}
	
	//Error case: ptr already free OR cannot find pointer
	if (checkValidPtr(ptr) == 0){
		printErr("Cannot free invalid pointer", file, line);
		badFrees++;
		return;		
	}
	
	//save size info
	int freedSize = getSize(ptr);
	
    //turn block into free
	setAlloc(ptr, 0);
	
	//set freed blocks to '/0'
	clearBlock(ptr, freedSize);
	
	// go through linked list and merge free blocks (check if next is free and/or if prev is free then merge)
	coalesceFree(); 
	
	//conclusion
	printf("free: " + size + " bytes");
	countFrees++;
	
}