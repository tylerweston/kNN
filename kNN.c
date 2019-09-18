/*
*  knn
* ----
* tyler weston, 2019
*
* todo:
*  - Right now this is only k = 1 which works fine for this dataset
*    but finish implementing a max-heap so we can extend to k > 1
*    efficiently.
*/

// to compile:
// gcc -o kNN kNN.c -lm `sdl2-config --cflags --libs`

// includes ---------------------------------------------------
#include <SDL2/SDL.h>   // uses SDL2 for visualization
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// defines ---------------------------------------------------
// we have 60,000 images in MNIST so split into a 50,000 training set
// and a 10,000 digit validation set

#define K_NEAREST 3             //how many neighbors to look at
#define WINDOW_WIDTH 200
#define WINDOW_HEIGHT 100
#define TRAIN_SIZE 50000        // size of training data
#define VALIDATION_SIZE 10000   // size of validation data
#define TOTAL_SIZE  (TRAIN_SIZE + VALIDATION_SIZE)

#define DEBUG false

// macros ----------------------------------------------------
#define ii(x) (784 * x)         // to convert an image index to an index into images
// heap macros
#define left(x) (x << 1)
#define right(x) ((x << 1) + 1)
#define parent(x) (x >> 1)

// structs ---------------------------------------------------

//heap structs
struct
{
    int index;
    float distance;
} typedef heapnode;

struct heap
{
    int heapsize;
    int length;
    heapnode * array;
};

// struct to hold info about main window so
// we can pass it around to functions if we need
struct windowInfo {
    SDL_Renderer *mRenderer;
    SDL_Window *mWindow;
} mainWindow;


typedef unsigned char uint8;
// pointer to training labels
uint8 *labels;
// pointer to image data
uint8 *images;

int image_size = 28 * 28;
int right = 0 , wrong = 0, total = 0;

// function prototypes -------------------------------------
// this can go into a header eventually
int do_quit(struct windowInfo);
uint8* read_labels(void);
uint8* read_images(void);
float distance(int, int);

// max-heap funcs
struct heap* new_heap(int);
int max_heapify(struct heap*, int);
int build_max_heap(struct heap*);
heapnode* get_new_node(int, float);

// main -----------------------------------------------------
int main(void) 
{

    SDL_Event ev;
    SDL_Renderer *renderer;
    SDL_Window *window;
    
    // set random seed based on time
    time_t t;
    srand((unsigned) time(&t));

    int mouseX, mouseY;

    // create and draw our original window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    mainWindow.mRenderer = renderer;
    mainWindow.mWindow = window;

    bool quit = false;
    bool do_training = true;

    labels = read_labels();
    images = read_images();

    int index = 0;

    int label_index = 0;

    int predicted = 0;

    int image_index;

    struct heap* nearest_n = new_heap(K_NEAREST);

    // main game loop here!
    while (!quit) 
    {
        while (SDL_PollEvent(&ev) != 0)   // exhaust events
        {
            // clicked exit
            if (ev.type == SDL_QUIT)
            {
                quit = true;
            }
            if (ev.type == SDL_MOUSEMOTION)
            {
                SDL_GetMouseState( &mouseX, &mouseY);
            }
            // pressed a keyboard button
            else if ( ev.type == SDL_KEYDOWN)
            {
                // select which key was pressed
                switch ( ev.key.keysym.sym )
                { 
                    case SDLK_SPACE:

                        break;
                    case SDLK_UP:
                        //press up to print a random double
                        break;
                    case SDLK_DOWN:
                        // down arrow
                        break;
                    case SDLK_LEFT:
                        // left arrow
                        break;
                    case SDLK_RIGHT:
                        // right arrow
                        break;
                    case SDLK_ESCAPE:
                        do_quit(mainWindow);
                        break;
                    default:
                        break;
                }
            }
        }

        // image_index = rand() % TRAIN_SIZE;          // get a random image from our validation set
        image_index = rand() % VALIDATION_SIZE;
        image_index += TRAIN_SIZE;

        int expected_value = labels[image_index];   // what should this image really be?

        int closest_index = 0;
        float closest_distance = 99999.0; // make this neg so we know we don't have anything yet.
        for (int cur_im = 0; cur_im < TRAIN_SIZE; cur_im++) 
        {
            if (cur_im == image_index)  // don't check against ourself
                continue;

            float this_dis = distance(image_index, cur_im);

            // TODO:
            // if this distance is less than the distance stored at root of heap:
            // replace that node with a new one with this index and distance,
            // then sort the heap as necessary
            if (this_dis < closest_distance) 
            {
                closest_index = cur_im;
                closest_distance = this_dis;
            }
        }

        printf("our image: %d closest image: %d guess: %d\n", 
            image_index, closest_index, labels[closest_index]);
        if (labels[closest_index] == labels[image_index]) 
        {
            printf("got it correct!\n");
            right++;
        } 
        else 
        {
            printf("got it wrong! expected: %d\n", labels[image_index]);
            wrong++;
        }
        total++;

        uint8 brightness;
        // display our image
        int im_index = ii(image_index);
        int i = 0;
        for (int y = 0; y < 28; y++) 
        {
            for (int x = 0; x < 28; x++) 
            {
                brightness = images[im_index + i];
                i++;
                //brightness *= 250;
                SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
                SDL_RenderDrawPoint(renderer, x + 28, y + 28);
            }
        } 

        // display the guess image
        im_index = ii(closest_index);
        i = 0;
        for (int y = 0; y < 28; y++) 
        {
            for (int x = 0; x < 28; x++) 
            {
                brightness = images[im_index + i];
                i++;
                //brightness *= 250;
                SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
                SDL_RenderDrawPoint(renderer, x + 128, y + 28);
            }
        } 

        SDL_RenderPresent(renderer);
        
    }

    // we left the main loop, exit!

    do_quit(mainWindow);
}

// other functions -----------------------------------------------------

int do_quit(struct windowInfo q_window) 
{
    // do an arbitrary quit from anywhere
    // must be passed window to exit
    // this should do any other cleaning up and saving,
    // etc.

    float rp = (right * 100.0) / total;
    float wp =  (wrong * 100.0) / total;
    printf("correct: %d / %d = %.2f %%\n", right, total, rp);
    printf("incorrect: %d / %d = %.2f %%\n", wrong, total, wp);

    SDL_DestroyWindow ( q_window.mWindow );
    SDL_DestroyRenderer ( q_window.mRenderer );
    q_window.mWindow = NULL;
    q_window.mRenderer = NULL;
    SDL_Quit();
    exit(0);
}

float distance(int image1_index, int image2_index) 
{
    // calculate euclidean distance from image1 to image2

    float d = 0.0;
    int image1_offset = ii(image1_index);   
    int image2_offset = ii(image2_index);   
    for (int x1 = 0 ; x1 < 28; x1++) 
    {
        for (int y1 = 0; y1 < 28; y1++) 
        {
            int ind = x1 * 28 + y1;
            float i1 = images[image1_offset + ind];
            float i2 = images[image2_offset + ind];
            if (i1 != i2)
                d += pow(i2 - i1, 2);   // sum over square of distances
        }
    }

    return sqrt(d); // return square root of sum of distances
}

double dot_product(double a[], double b[], int n) 
{
    // return a \cdot b, where a and b are vectors
    // n is length of vectors
    double sum = 0.0;
    for (int i = 0; i < n; i++) 
    {
        sum += a[i] * b[i];
    }
    return sum;
}

// data reading functions ------------------------------------------------

uint8* read_images(void) 
{
    // after this all the labels have been read to
    // uint8 array images
    uint8 *images = NULL;
    images = (uint8 *) calloc(TOTAL_SIZE * image_size, sizeof(uint8));
    FILE *image_file = NULL;
    image_file = fopen("train_images.dat", "r");
    // skip to start of labels
    fseek(image_file, 16L, SEEK_SET);
    // read all data
    fread(images, TOTAL_SIZE * image_size, 1, image_file);
    return images;
}

uint8* read_labels(void) 
{
    // after this all the labels have been read to
    // uint8 array labels
    uint8 *labels = NULL;
    labels = (uint8 *) calloc(TOTAL_SIZE, sizeof(uint8));
    FILE *label_file = NULL;
    label_file = fopen("train_labels.dat", "r");
    // skip to start of labels
    fseek(label_file, 8L, SEEK_SET);
    // read all data
    fread(labels, TOTAL_SIZE, 1, label_file);
    return labels;
}


// max heap functions ---------------------------------------------------
// TO DO STILL: Make this work with heapnodes!

struct heap* new_heap(int maxsize)
{
    // allocate enough space for heap plus enough size for
    // array inside it
    struct heap *s = malloc(sizeof(struct heap) + (sizeof(heapnode) * maxsize));
    s -> heapsize = 0;
    s -> length = maxsize;
    return s;
}

heapnode* get_new_node(int c_index, float dist)
{
    heapnode* r = malloc(sizeof(heapnode));
    r->index = c_index;
    r->distance = dist;
    return r;
}

int max_heapify(struct heap* in_heap, int index)
{
    // assume heap is ordered except for possible node at index
    int heapsize = in_heap->heapsize;
    int largest = index;        // why does this just being int largest; crash???

    int l = left(index);
    int r = right(index);

    // TODO: make sure this sorts by the distances but still
    // keeps track of the nodes properly

    if ((l <= heapsize) && (in_heap->array[l].distance > in_heap->array[index].distance))
        largest = l;
    else
        largest = index;        // why does getting rid of this crash the program???!
    
    if ((r <= heapsize) && (in_heap->array[r].distance > in_heap->array[largest].distance))
        largest = r;

    if (largest != index)
    {
        // swap array[index] <-> array[largest]
        // TODO: this needs to swap heapnodes now!

        // heapnode * tmp = in_heap->array[index];
        // in_heap->array[index] = in_heap->array[largest];
        // in_heap->array[largest] = tmp;

        max_heapify(in_heap, largest);
    }
}

int build_max_heap(struct heap* out_heap)
{
    // converts an unordered array into a max-heap
    out_heap->heapsize = out_heap->length;
    // out_heap->array = array;
    for (int i = (int)(out_heap->length / 2); i >= 0; i--)
    {
        max_heapify(out_heap, i);
    }
}

// training labels: train_labels.dat 
/* Read from the training set label file
*   offset  type            value       description
*   0000    32-bit int      2049        magic number 0x00000801
*   0004    32-bit int      60000       number of images
*   0008    unsigned byte   ???         label
*   0009    unsigned bute   ???         label   
*   ...
*   nnnn    unsigned byte   ???         label
*
* Test data is same but with 10000 values
*
*/


// image file: train_images.dat
/* Read from training set image file
*   offset  type            value       description 
*   0000    32-bit int      2051        magic number
*   0004    32-bit int      60000       number of items
*   0008    32-bit int      28          number of rows
*   0012    32-bit int      28          number of columns
*   0016    unsigned byte   ???         pixel
*   0017    unsigned byte   ???         pixel
*   ....
*   nnnn    unsigned byte   ???         pixel
*
*   pixels are organized row-wise, pixel values 0 - 255
*   0 = background = white
*   255 = foreground = black
*
*
*   Test data is same but with 10000 values
*/

/*
*   IDX File Format
*   magic number
*   size in dimension 0
*   size in dimension 1
*   ...
*   size in dimension n
*   data
*
*   magic number = int
*   first two digits always 0
*   third bit = type of data encoded
*   0x08: unsigned byte
*   0x09: signed byte
*   0x0b: short (2 bytes)
*   0x0C: int (4 bytes)
*   0x0D: double (4 bytes)
*   0x0E: double (8 bytes)
*
*   4-th byte = code # of dimensions of vector/matrix:
*   1 for vectors, 2 for matrices...
*   Size in each dimension in 4-byte int (high endian)
*   data stored like C array (last index change first)
*/
