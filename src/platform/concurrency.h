
struct AtomicU32 { volatile U32 value; };
struct AtomicU64 { volatile U64 value; };

//Returns true if the extange value was stored in the atomic variable
inline bool AtomicCompareAndExtange(AtomicU32 *atom, U32 extange, U32 comparand);
inline bool AtomicCompareAndExtange(AtomicU64 *atom, U64 extange, U64 comparand);

//Returns the value of the atomic variable after the increment
inline U32 AtomicIncrement(AtomicU32 *atom);
inline U64 AtomicIncrement(AtomicU64 *atom);

//Basic locking procedures that operate on atomic data types
inline bool TryLock(AtomicU32 *lock);     //Returns true if the lock was sucuessfully aquired.  Does not wait
inline void SpinLock(AtomicU32 *lock);    //Spins until the lock can be aquired
inline void ReleaseLock(AtomicU32 *lock); //Releases an aquired lock