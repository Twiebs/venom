
struct AtomicU32 {
  volatile U32 value;
};

struct AtomicU64 { 
  volatile U64 value; 
};

struct SpinLock {
  volatile U32 value;
#if VENOM_TRACK_LOCKS
  size_t line;
  const char *file;
  const char *procedure;
  U32 threadID;
#endif//VENOM_TRACK_LOCKS
};

//Returns true if the extange value was stored in the atomic variable
inline bool AtomicCompareAndExtange(AtomicU32 *atom, U32 extange, U32 comparand);
inline bool AtomicCompareAndExtange(AtomicU64 *atom, U64 extange, U64 comparand);

//Returns the value of the atomic variable after the increment
inline U32 AtomicIncrement(AtomicU32 *atom);
inline U64 AtomicIncrement(AtomicU64 *atom);


#ifndef VENOM_TRACK_LOCKS
#define TryLock(lock) TryLockDefault(lock)
#define AquireLock(lock) AquireLockDefault(lock)
inline bool TryLockDefault(SpinLock *lock);     //Returns true if the lock was sucuessfully aquired.  Does not wait
inline void AquireLockDefault(SpinLock *lock);    //Spins until the lock can be aquired
#else//VENOM_TRACK_LOCKS
#define TryLock(lock) TryLockTracked(lock, __LINE__, __FILE__, __FUNCTION__)
#define AquireLock(lock) AquireLockTracked(lock, __LINE__, __FILE__, __FUNCTION__)
inline bool TryLockTracked(SpinLock *lock, size_t line, const char *file, const char *procedure);
inline void AquireLockTracked(SpinLock *lock, size_t line, const char *file, const char *procedure);
#endif//VENOM_TRACK_LOCKS
inline void ReleaseLock(SpinLock *lock); //Releases an aquired lock