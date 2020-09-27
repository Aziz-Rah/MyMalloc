#ifndef MYMALLOC_H
#define MYMALLOC_H

#define malloc ( x ) mymalloc(x,__FILE__,__LINE__)
#define free ( x ) myfree(x,__FILE__,__LINE__)

int countMallocs;
int badMallocs; 
int countFrees;
int badFrees;

void* mymalloc(int size, char *file, int line);
void myfree(void *vPtr, char *file, int line);


#endif 