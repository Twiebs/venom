#if 0
struct Rectangle {
  F32 minX, minY;
  F32 maxX, maxY; 
};
#endif

inline V2 
Center(const Rectangle& rectangle){
  F32 width = rectangle.maxX - rectangle.minX;
  F32 length = rectangle.maxY - rectangle.minY;
  V2 result = { rectangle.minX + (width * 0.5F),
    rectangle.minY + (length * 0.5F) };
  return result; 
}