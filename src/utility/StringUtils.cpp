
//NOTE(Torin) Does not return the index of the last char
//Will return the offset from the front of the string which
//might be zero if there is no char that is found
size_t LastOffsetOfChar(char c, const char *s) {
  size_t length = strlen(s);
  for (int i = length - 1; i >= 0; i--) {
    if (s[i] == c) return i;
  }
  return 0;
}

char CharToLowercaseIfAlpha(char c) {
  if (c >= 'A' & c <= 'Z') {
    c = c + ('a' - 'A');
  }
  return c;
}

U64 StringToU64(const char *str, size_t length) {
  uint64_t result = 0;
  size_t scalar = 1;
  for (int64_t i = length - 1; i >= 0; i--) {
    assert(str[i] >= '0' && str[i] <= '9');
    uint64_t value = str[i] - '0';
    result += value * scalar;
    scalar *= 10;
  }
  return result;
}

inline char *CStringDuplicate(const char *str) {
  size_t length = strlen(str);
  char *result = (char *)MemoryAllocate(length + 1);
  memcpy(result, str, length + 1);
  return result;
}

inline bool CStringContainsCSubstringCaseInsensitive(const char *substring, const char *text) {
  //TODO(Torin) Optimize!
  size_t textLength = strlen(text);
  size_t substringLength = strlen(substring);

  for (size_t i = 0; i < textLength; i++) {
    char a = CharToLowercaseIfAlpha(text[i]);
    char b = CharToLowercaseIfAlpha(substring[0]);
    if (a == b) {
      bool isValid = true;
      for (size_t j = 0; j < substringLength; j++) {
        a = CharToLowercaseIfAlpha(text[i + j]);
        b = CharToLowercaseIfAlpha(substring[j]);
        if (a != b) {
          isValid = false;
          break;
        }
      }
      if (isValid) return true;
    }
  }
  return false;
}


inline bool cstrings_are_equal(const char *a, const char *b) {
  while (*a != 0) {
    if (*a != *b) return false;
    a++;
    b++;
  }
  return *a == *b;
}

inline int FindMatchingString(const char *source, const char **list, size_t listLength) {
  for (size_t i = 0; i < listLength; i++)
    if (strcmp(source, list[i]) == 0) return (int)i;
  return -1;
}

int CalculateFuzzyScore(const char *input, const char *target) {
  int finalScore = -1;
  const char* searchPos = target;
  size_t inputLength = strlen(input);

  while (*searchPos != 0) {
    int score = -1;
    int lastMatchIndex = -1;
    int currentInputIndex = 0;
    const char* searchChar = searchPos;
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
    searchPos++;
  }
  return finalScore;
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