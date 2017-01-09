
struct AtomicU32 { 
  volatile U32 value;
#if VENOM_TRACK_LOCKS
  size_t line;
  const char *file;
  const char *procedure;
#endif//VENOM_TRACK_LOCKS
};

struct AtomicU64 { 
  volatile U64 value; 
};

//Returns true if the extange value was stored in the atomic variable
inline bool AtomicCompareAndExtange(AtomicU32 *atom, U32 extange, U32 comparand);
inline bool AtomicCompareAndExtange(AtomicU64 *atom, U64 extange, U64 comparand);

//Returns the value of the atomic variable after the increment
inline U32 AtomicIncrement(AtomicU32 *atom);
inline U64 AtomicIncrement(AtomicU64 *atom);

//Basic locking procedures that operate on atomic data types
#ifndef VENOM_TRACK_LOCKS
#define TryLock(lock) TryLockDefault(lock)
#define SpinLock(lock) SpinLockDefault(lock)
inline bool TryLockDefault(AtomicU32 *lock);     //Returns true if the lock was sucuessfully aquired.  Does not wait
inline void SpinLockDefault(AtomicU32 *lock);    //Spins until the lock can be aquired
#else//VENOM_TRACK_LOCKS
#define TryLock(lock) TryLockTracked(lock, __LINE__, __FILE__, __FUNCTION__)
#define SpinLock(lock) SpinLockTracked(lock, __LINE__, __FILE__, __FUNCTION__)
inline bool TryLockTracked(AtomicU32 *lock, size_t line, const char *file, const char *procedure);
inline void SpinLockTracked(AtomicU32 *lock, size_t line, const char *file, const char *procedure);
#endif//VENOM_TRACK_LOCKS
inline void ReleaseLock(AtomicU32 *lock); //Releases an aquired lock