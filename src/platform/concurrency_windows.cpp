
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

inline bool TryLock(AtomicU32 *lock) {
  U32 result = (U32)Win32::_InterlockedCompareExchange((volatile long *)&lock->value, (long)1, (long)0);
  return result == 0;
}

inline void SpinLock(AtomicU32 *lock) {
  while (Win32::_InterlockedCompareExchange(&lock->value, 1, 0) == 1) {

  }
}

inline void ReleaseLock(AtomicU32 *lock) {
  strict_assert(lock->value == 1);
  strict_assert(((uintptr_t)lock & 0b11) == 0);
  lock->value = 0;
}