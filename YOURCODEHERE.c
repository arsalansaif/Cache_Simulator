#include "YOURCODEHERE.h"

/**********************************************************************
    Function    : lg2pow2
    Description : this help funciton for you to calculate the bit number
                  this function is not allowed to modify
    Input       : pow2 - for example, pow2 is 16
    Output      : retval - in this example, retval is 4
***********************************************************************/
unsigned int lg2pow2(uint64_t pow2){
  unsigned int retval=0;
  while(pow2 != 1 && retval < 64){
    pow2 = pow2 >> 1;
    ++retval;
  }
  return retval;
}

void  setSizesOffsetsAndMaskFields(cache* acache, unsigned int size, unsigned int assoc, unsigned int blocksize){
  unsigned int localVAbits=8*sizeof(uint64_t*);
  if (localVAbits!=64){
    fprintf(stderr,"Running non-portable code on unsupported platform, terminating. Please use designated machines.\n");
    exit(-1);
  }
  unsigned int numSet;
  unsigned int sizeIndex;

  acache->numways = assoc;
  acache->blocksize = blocksize;
  acache->numsets = (size / blocksize) / assoc;
  acache->numBitsForBlockOffset = 0;

  // get the log2 of blocksize
  while (blocksize >= 2) {
    blocksize /= 2;
    acache->numBitsForBlockOffset++;
  }

  sizeIndex = 0;
  numSet = acache->numsets;

  // get the log2 of numSet
  while (numSet >= 2) {
    numSet /= 2;
    sizeIndex++;
  }

  acache->VAImask = 0;

  // create index andmask
  for (int curBitI = 0; curBitI < sizeIndex; ++curBitI) {
    acache->VAImask |= ((uint64_t)1 << curBitI);
  }

  acache->numBitsForIndex = acache->numBitsForBlockOffset + sizeIndex;
  unsigned int sizeTag = localVAbits - acache->numBitsForIndex;
  acache->VATmask = 0;

  // create tag andmask
  for (int curBitT = 0; curBitT < sizeTag; ++curBitT) {
    acache->VATmask |= ((uint64_t)1 << curBitT);
  }
}

unsigned long long getindex(cache* acache, unsigned long long address){
  unsigned long long index;
  index = address >> acache->numBitsForBlockOffset;
  index &= acache->VAImask;

  return index;
}

unsigned long long gettag(cache* acache, unsigned long long address){
  unsigned long long tag;
  tag = address >> acache->numBitsForIndex;
  tag &= acache->VATmask;

  return tag;
}

void writeback(cache* acache, unsigned int index, unsigned int waynum){
  unsigned long long wordSize = sizeof(unsigned long long);
  unsigned long long address = 0;
  unsigned long long addressIndex = index << acache->numBitsForBlockOffset;
  unsigned long long addressTag = acache->sets[index].blocks[waynum].tag << acache->numBitsForIndex;
  address |= addressIndex;
  address |= addressTag; // get address value

  for(int curword = 0; curword < (acache->blocksize / wordSize); curword++){
    unsigned long long word = acache->sets[index].blocks[waynum].datawords[curword];
    performaccess(acache->nextcache, address, wordSize, 1, word, curword); // send/write data
    address += wordSize;
  }
}

void fill(cache * acache, unsigned int index, unsigned int waynum, unsigned long long address){
  unsigned long long wordSize = sizeof(unsigned long long);
  unsigned long long buildTag = acache->VATmask << acache->numBitsForIndex;
  unsigned long long buildIndex = acache->VAImask << acache->numBitsForBlockOffset;
  unsigned long long buildTI = buildTag | buildIndex;
  address &= buildTI; // get address value

  for(int curword = 0; curword < (acache->blocksize / wordSize); curword++){
    unsigned long long word = performaccess(acache->nextcache, address, wordSize, 0, 0, curword); // receive/read data
    acache->sets[index].blocks[waynum].datawords[curword] = word; // update datawords content
    unsigned long long tag = gettag(acache, address);
    acache->sets[index].blocks[waynum].tag = tag; // update tag value
    address += wordSize;
  }
}
