
inline bool AtomicCompareAndExchange(AtomicU32 *atom, U32 exchange, U32 comparand) {
  U32 originalValue = (U32)_InterlockedCompareExchange(&atom->value, exchange, comparand);
  bool result = originalValue == comparand;
  return result;
}

inline bool AtomicCompareAndExchange(AtomicU64 *atom, U64 exchange, U64 comparand) {
  U64 orignalValue = (U64)Win32::_InterlockedCompareExchange64((volatile long long *)&atom->value, (long long)exchange, (long long)comparand);
  bool result = orignalValue == comparand;
  return result;
}

#ifndef VENOM_TRACK_LOCKS
inline bool TryLockDefault(AtomicU32 *lock) {
  U32 result = (U32)Win32::_InterlockedCompareExchange((volatile long *)&lock->value, (long)1, (long)0);
  return result == 0;
}

inline void SpinLockDefault(AtomicU32 *lock) {
  while (Win32::_InterlockedCompareExchange(&lock->value, 1, 0) == 1) {

  }
}

#else//VENOM_TRACK_LOCKS
inline void SpinLockTracked(AtomicU32 *lock, size_t line, const char *file, const char *procedure) {
  while (Win32::_InterlockedCompareExchange(&lock->value, 1, 0) == 1) { }
  lock->line = line;
  lock->file = file;
  lock->procedure = procedure;
}

inline bool TryLockTracked(AtomicU32 *lock, size_t line, const char *file, const char *procedure) {
  U32 result = (U32)Win32::_InterlockedCompareExchange((volatile long *)&lock->value, (long)1, (long)0);
  if (result == 0) {
    lock->line = line;
    lock->file = file;
    lock->procedure = procedure;
    return true;
  }
  return false;
}
#endif//VENOM_TRACK_LOCKS


inline void ReleaseLock(AtomicU32 *lock) {
  strict_assert(lock->value == 1);
  strict_assert(((uintptr_t)lock & 0b11) == 0);
#ifdef VENOM_TRACK_LOCKS
  lock->line = 0;
  lock->file = 0;
  lock->procedure = 0;
#endif//VENOM_TRACK_LOCKS
  lock->value = 0;
}