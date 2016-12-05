
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

inline int
FindMatchingString(const char *source, const char **list, size_t listLength) {
  for (size_t i = 0; i < listLength; i++)
    if (strcmp(source, list[i]) == 0) return (int)i;
  return -1;
}


static void
FuzzyFind(const char *input, const char** searchList, size_t searchListCount, bool *output) {
  if (input[0] == 0) {
    memset(output, 1, searchListCount);
    return;
  }

  size_t inputLength = strlen(input);

  for (size_t i = 0; i < searchListCount; i++) {
    int finalScore = -1;
    const char* rootSearchPos = searchList[i];

    while (*rootSearchPos != 0) {
      int score = -1;
      int lastMatchIndex = -1;
      int currentInputIndex = 0;
      const char* searchChar = rootSearchPos;
      while (*searchChar != 0) {
        if (input[currentInputIndex] == 0) break;
        if (*searchChar == input[currentInputIndex] ||
          ((input[currentInputIndex] >= 'a' && input[currentInputIndex] <= 'z') &&
          (input[currentInputIndex] - ('a' - 'A')) == (*searchChar)))
        {
          score += (currentInputIndex - lastMatchIndex);
          lastMatchIndex = currentInputIndex;
          currentInputIndex++;
        }

        searchChar++;
      }

      if (currentInputIndex != (int)inputLength) score = -1;
      if (score != -1 && (score < finalScore || finalScore == -1))
        finalScore = score;
      rootSearchPos++;
    }

    if (finalScore != -1) {
      output[i] = 1;
    } else {
      output[i] = 0;
    }
  }
}


