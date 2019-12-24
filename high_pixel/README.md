### Select top high X values

Problem: get top X high value pixels from an image with N pixels, where N = row x columns.

Chosen solution: use a min heap to hold the highest X values and their
positions (offset within the block). The algorithm is implemented in
`build_high_pixels()` function.

### The algorithm
```
void build_high_pixels(const image_t* image, heap_t* high_pixels) {
  if (!image_check_valid(image) || !heap_check_valid(high_pixels)) return;

  uint32_t size = image->size_x * image->size_y;
  int32_t err = 0;

  for (int32_t i = 0; i < size && err >= 0; ++i) {
    /*
        if heap reaches the desired capacity and the minimum heap value
        is smaller than current pixel value then pop the heap in order to
        make room for this pixel.
    */
    if (heap_full(high_pixels) && heap_peek(high_pixels) < image->pixels[i])
      err = heap_min_pop(high_pixels);

    /*
        if heap is not full then insert the pixel offset-value pair;
        the offset will be used later to compute the pixel (x,y)
		coordinates.
    */
    if (err >= 0 && !heap_full(high_pixels))
      err = heap_min_push(high_pixels, i, image->pixels[i]);
  }
}
```

Runtime: `O(N + X * log (X))`, where `N >> X` so we can assume `O(log(X))==O(1)`.
Also, if X is small enough so the entire heap fits into cache we have almost linear O(N) time for building it.
In our case N (> 2^16) >> log(50), so we can the small factor.
If X can't be ignored the actual runtime complexity becomes: O(N * log(X) + X * log(X)).

Memory space: `O(2 * X)`, the `2` comes from the fact that we store both pixels values + pixel positions.

### Implementation
The program is actually a test program that check all possible image size, `x=1..64` and `y=1..64` and `X = 50`: 

```
  for (uint16_t x = 1; x <= IMAGE_SIZE_X; ++x)
    for (uint16_t y = 1; y <= IMAGE_SIZE_X; ++y)
      run_test(x, y, HIGH_PIXELS_NUM);
```

### Build & Run
```
make
./highpixel
```
If all values match at the end you will see:
```
All test passed :)
```
Tested on Linux 64-bit.
