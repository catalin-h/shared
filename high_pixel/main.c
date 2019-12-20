#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
   Problem: get top 50 high value pixels from an image with N pixels, where
   N = row x columns.

   Assumptions:
        (1) the image is provided as contiguous block of 16-bit values
        (2) the image pixel count is less than 65536 * 65536
        (3) all index are zero based

   Chosen solution: use a min heap to hold the highest X values and their
   positions (offset within the block). The algorithm is implemented in
   build_high_pixels() function.

   Runtime: O(N + X * log (X))
   Memory space: O(2 * X), the `2` comes from the fact that we store both
   pixels values + pixel positions
*/

#define MAX_HEAP_CAPACITY 50
#define HIGH_PIXELS_NUM MAX_HEAP_CAPACITY
#define IMAGE_SIZE_X 64
#define IMAGE_SIZE_Y 64

#if HIGH_PIXELS_NUM <= 0
#error "invalid pixel count"
#endif

#define SWAP(a, b, t) (((t) = (a), (a) = (b), (b) = (t)))

/* Image object type */
typedef struct {
  uint16_t* pixels; /* pixel array with size = size_x * size_y*/
  /* fits most image use cases */
  uint16_t size_x; /* image rows count */
  uint16_t size_y; /* image columns */
} image_t;

/* Integer heap type */
typedef struct {
  uint32_t capacity; /* heap max capacity */
  uint32_t size;     /* current heap size */
  uint32_t* offsets; /* holds pixel index */
  uint16_t* values;  /* holds value or pixel value */

  /* use separate arrays for index and value in order to
     take advantage of data locality (cache usage) and
     allocate less memory than putting them in a structure due to padding:
	 E.g.
     struct {
		uint32_t offsets;
        uint16_t values;
     } items[10];

    uint32_t offsets[10];
	uint16_t values[10];

	sizeof(items) > sizeof(offsets) + sizeof(values)
*/
} heap_t;

/*
	Initialize image with random 16-bit data
    returns negative value on fail, 0 otherwise
*/
int32_t init_image(image_t* image, uint16_t x, uint16_t y) {
  if (!image || !x || !y) return -1;

  uint32_t size = x * y;

  image->pixels = (uint16_t*)malloc(size * sizeof(*image->pixels));

  if (!image->pixels) return -1;

  image->size_x = x;
  image->size_y = y;

  srand(time(0));

  for (int i = 0; i < size; ++i) image->pixels[i] = rand() & 0xff;

  return 0;
}

/* check image validity */
static inline int8_t image_check_valid(const image_t* image) {
  return image && image->pixels;
}

/*
        Frees image object.
*/
void free_image(image_t* image) {
  if (!image_check_valid(image)) return;

  free(image->pixels);
}

/* check heap validity */
static inline int8_t heap_check_valid(const heap_t* heap) {
  return heap && heap->offsets && heap->values;
}

/*
		Initialize heap object with provided capacity.

        return 0 on success, != 0 otherwise
*/
int32_t init_heap(heap_t* heap, uint32_t capacity) {
  if (capacity > MAX_HEAP_CAPACITY || !heap) return -1;

  heap->offsets = (uint32_t*)malloc(capacity * sizeof(*heap->offsets));

  if (!heap->offsets) return -1;

  heap->values = (uint16_t*)malloc(capacity * sizeof(*heap->values));

  if (!heap->values) {
    free(heap->offsets);
    return -1;
  }

  heap->capacity = capacity;
  heap->size = 0;

  return 0;
}

/*
        Deallocates heap resources
*/
void free_heap(heap_t* heap) {
  if (!heap_check_valid(heap)) return;

  free(heap->offsets);
  free(heap->values);
}

/* check if heap if full, i.e. reach its capacity */
static inline int8_t heap_full(const heap_t* heap) {
  return !heap_check_valid(heap) || heap->size >= heap->capacity;
}

/* check if heap is empty */
static inline int8_t heap_empty(const heap_t* heap) {
  return !heap_check_valid(heap) || !heap->size;
}

/* check if heap is empty */
static inline uint16_t heap_peek(const heap_t* heap) {
  return !heap_empty(heap) ? heap->values[0] : ~0;
}

/*
        Return the min child index (within the heap) for a parent
        or negative if doesn't exits.
*/
int32_t heap_min_child_for_parent(const heap_t* heap, uint32_t parent) {
  if (!heap_check_valid(heap)) return -1;

  uint32_t left = (parent << 1) + 1;

  if (left >= heap->size) return -1;

  uint32_t right = (parent + 1) << 1;

  if (right >= heap->size) return left;

  return heap->values[left] < heap->values[right] ? left : right;
}

/*
        Push new item in heap min.

        return 0 on success, != 0 otherwise
*/
int32_t heap_min_push(heap_t* heap, uint32_t offset, uint16_t value) {
  if (heap_full(heap)) return -1;

  uint32_t* offsets = heap->offsets;
  uint16_t* values = heap->values;
  uint32_t parent = heap->size;

  while (parent > 0) { /* stop if we reached the root */
    uint32_t child = parent;
    parent = (parent - 1) >> 1; /* get parent index: (child - 1) / 2 */

    /* reach insertion point */
    if (values[parent] < value) {
      parent = child;
      break;
    }

    /* move parent to child location */
    values[child] = values[parent];
    offsets[child] = offsets[parent];
  }

  values[parent] = value;
  offsets[parent] = offset;
  heap->size += 1;

  return 0;
}

/*
        Pop min element from heap

        return negative value on failure, >= 0 otherwise representing the offset
*/
int32_t heap_min_pop(heap_t* heap) {
  if (heap_empty(heap)) return -1;

  uint32_t* offsets = heap->offsets;
  uint16_t* values = heap->values;
  uint32_t size = heap->size - 1; /* new heap size */
  uint32_t offset;
  uint32_t parent; /* parent pointer */
  uint16_t value;  /* tumble down value */

  heap->size = size;

  /* reach last element */
  if (!size) return offsets[0];

  /* if not the last element then tumble down root */

  /* swap the last and the first element so the latter becomes the root */
  SWAP(values[size], values[0], value);
  SWAP(offsets[size], offsets[0], offset);
  parent = 0;

  for (int32_t child = heap_min_child_for_parent(heap, parent);
       /* stop if we reached the leaf level or reach insertion point */
       child > 0 && value > values[child];
       /* get child index with min value */
       child = heap_min_child_for_parent(heap, parent)) {
    /* upgrade child */
    values[parent] = values[child];
    offsets[parent] = offsets[child];
    parent = child; /* move to child level */
  }

  values[parent] = value;
  offsets[parent] = offset;

  return offsets[size];
}

/* prints heap_t object */
void heap_print(const heap_t* heap, uint16_t columns) {
  for (int32_t i = 0; i < heap->size; ++i)
    printf(" [%d, %d, %hu] ", heap->offsets[i] / columns,
           heap->offsets[i] % columns, heap->values[i]);
  printf("\n");
}

/*
	computes the first X high value pixels; X represents the heap size.
 */
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

static int32_t cmp(const void* a, const void* b) {
  return (*(uint16_t*)a - *(uint16_t*)b);
}

/* tests a particular image size and top X pixel values */
static void run_test(uint16_t x, uint16_t y, uint32_t high_num) {
  uint32_t size = x * y;
  heap_t high_pixels;
  image_t image;

  if (init_heap(&high_pixels, high_num)) exit(1);

  if (init_image(&image, x, y)) {
    free_heap(&high_pixels);
    exit(1);
  }

  /* build the top X pixel heap */
  build_high_pixels(&image, &high_pixels);

  /* test against well known qsort */
  qsort(image.pixels, size, sizeof(uint16_t), cmp);

  uint32_t off = size > high_num ? high_num : size;
  uint16_t* start = image.pixels + (size - off);
  uint16_t* end = image.pixels + size;

  for (; start != end; start++) {
    heap_min_pop(&high_pixels);
    if (*start != high_pixels.values[high_pixels.size]) {
      heap_print(&high_pixels, image.size_y);
      printf("tests failed :(\n");
      exit(1);
    }
  }

  free_image(&image);
  free_heap(&high_pixels);
}

int main() {
  for (uint16_t x = 1; x <= IMAGE_SIZE_X; ++x)
    for (uint16_t y = 1; y <= IMAGE_SIZE_Y; ++y)
      run_test(x, y, HIGH_PIXELS_NUM);

  printf("\nAll test passed :)\n");

  return 0;
}