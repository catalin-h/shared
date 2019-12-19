#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
	Problem: get top 50 high value pixels from an image with N pixels, where N = row x columns.

	Chosen solution: use a min heap to hold the pixel positions of the highest X values.

	Runtime: O(N * log (X))
	Memory space: O(2 * X), the `2` comes from the fact that 
*/

#define MAX_HEAP_CAPACITY 50
#define HIGH_PIXELS_NUM MAX_HEAP_CAPACITY 
#define IMAGE_SIZE_X 256
#define IMAGE_SIZE_Y 256

#if HIGH_PIXELS_NUM <= 0
#error "invalid pixel count"
#endif

/* Image object type */
typedef struct {
	uint16_t* pixels; /* pixel array with size = size_x * size_y*/
	uint16_t size_x; /* image rows count */
	uint16_t size_y; /* image columns */
} image_t;

/* Integer heap type */
typedef struct {
	uint32_t capacity; /* heap max capacity */
	uint32_t size;	   /* current heap size */
	uint32_t *index;   /* holds pixel index */
	uint16_t *priority;    /* holds priority or pixel value */
	/*	use separate arrays for index and priority in order to
		improve locality on cache and allocate less memory
		that for e.g. putting them in a structure  
	*/
} heap_t;

/* 	Initialize image with random 16-bit data 
	returns negative value on fail, 0 otherwise
*/
int32_t init_image(image_t* image, uint16_t x, uint16_t y)
{
	if (!image)
		return -1;

	uint32_t size = x * y;

	image->pixels = (uint16_t*)malloc(size);
	image->size_x = x;
	image->size_y = y;

	srand(0);

	for (int i = 0; i < size; i++) {
		image->pixels[i] = rand() & 0xff;
		//printf(" %d ", image->pixels[i]);
	}

	return 0;
}

/*
	Frees image object.
*/
void free_image(image_t* image)
{
	if (!image || !image->pixels)
		return;

	free(image->pixels);
}

/*
	Initialize heap object with provided capacity.

	return 0 on success, != 0 otherwise
*/
int32_t init_heap(heap_t* heap, uint32_t capacity)
{
	if (capacity > MAX_HEAP_CAPACITY || !heap)
		return -1;

	heap->index = (uint32_t*)malloc(capacity * sizeof(*heap->index));

	if (!heap->index)
		return -1;

	heap->priority = (uint16_t*)malloc(capacity * sizeof(*heap->priority));

	if (!heap->priority) {
		free(heap->index);
		return -1;
	}

	heap->capacity = capacity;
	heap->size = 0;

	return 0;
}

/*
	Deallocates heap resources
*/
void free_heap(heap_t* heap)
{
	if (!heap)
		return;

	if (heap->index)
		free(heap->index);

	if (heap->priority)
		free(heap->priority);
}

/*
	Push new item in heap min.

	return 0 on success, != 0 otherwise
*/
int32_t heap_min_push(heap_t* heap, uint32_t new_index, uint16_t priority)
{
	if (!heap || heap->size >= heap->capacity)
		return -1;

	uint32_t parent  = heap->size;
	uint32_t child;

	while (parent > 0) { /* stop if we reached the root */
		child = parent;
		parent = (parent - 1) >> 1; /* get parent index: (child - 1) / 2 */

		if (heap->priority[parent] >= priority) {
			heap->priority[child] = heap->priority[parent];
			heap->index[child] = heap->index[parent];
		} else /* reach insertion point */
			break;
	}

	heap->priority[parent] = priority;
	heap->index[parent] = new_index;

	return 0;
}

/*
	Return the min child index (within the heap) for a parent
	or negative if doesn't exits.
*/
int32_t heap_min_child_for_parent(heap_t* heap, uint32_t parent)
{
	uint32_t left = (parent << 2) + 1;

	if (!heap || !heap->size || left >= heap->size)
		return -1;

	uint32_t right = (parent + 1) << 2;

	if (right >= heap->size)
		return left;

	return heap->priority[left] < heap->priority[right] ? left : right;
}

/*
	Pop min element from heap

	return negative value on failure, >= 0 otherwise representing the index
*/
int32_t heap_min_pop(heap_t* heap)
{
	if (!heap || !heap->size)
		return -1;

	heap->size -= 1;

	/* reach last element */
	if (!heap->size)
		return heap->priority[heap->size];

	/* tumble down root element */

	int32_t top_index = heap->index[0];
	uint32_t parent = 0;
	uint16_t priority = heap->priority[heap->size];
	uint32_t index = heap->index[heap->size];

	/* temp set last element as heap root */
	heap->index[0] = index;
	heap->priority[0] = priority;

	for (uint32_t child = heap_min_child_for_parent(heap, parent);
		 child > 0; /* stop if we reached the leaf level */
		 child = heap_min_child_for_parent(heap, parent)) {

		/* upgrade child */
		if (priority <= heap->priority[child]) {
			heap->priority[parent] = heap->priority[child];
			heap->index[parent] = heap->index[child];
		} else /* reach insertion point */
			break;
	}

	heap->priority[parent] = priority;
	heap->index[parent] = index;

	return top_index;
}

int main()
{
	heap_t high_pixels;
	image_t image;

	if (init_heap(&high_pixels, HIGH_PIXELS_NUM))
		exit(1);

	if (init_image(&image, IMAGE_SIZE_X, IMAGE_SIZE_Y)) {
		free_heap(&high_pixels);
		exit(1);
	}

	uint32_t size = image.size_x * image.size_y;

	for (int i = 0; i < size; ++i) {
		if (high_pixels.size &&
			high_pixels.size == HIGH_PIXELS_NUM &&
			image.pixels[i] > high_pixels.priority[0])
			heap_min_pop(&high_pixels);

		if (high_pixels.size < HIGH_PIXELS_NUM)
			heap_min_push(&high_pixels, i, image.pixels[i]);
	}

	for (int i = 0; i < HIGH_PIXELS_NUM; i++)
		printf("[%d, %d, %d] ",
				high_pixels.index[i] / image.size_y,
				high_pixels.index[i] % image.size_y,
				high_pixels.priority[i]);

	free_image(&image);
	free_heap(&high_pixels);

	return 0;
}