#pragma once

//NOTE(Torin) Does not return the index of the last char
//Will return the offset from the front of the string which
//might be zero if there is no char that is found
size_t LastOffsetOfChar(char c, const char *s){
  size_t length = strlen(s);
  for(int i = length - 1; i >= 0; i--) {
    if(s[i] == c) return i; 
  }
  return 0;
}
