#include <Arduino.h>
#include "SPIFFS.h"

#define GRAPHIC_CONTROL        0xF9
#define APPLICATION_EXTENSION  0xFF
#define COMMENT_EXTENSION      0xFE
#define PLAINTEXT_EXTENSION    0x01

typedef struct {
  char application_id[8];
  char version[3];
} application_extension_t;

typedef struct {
  uint16_t width;
  uint16_t height;
  uint8_t fields;
  uint8_t background_color_index;
  uint8_t pixel_aspect_ratio;
} screen_descriptor_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb;

typedef struct {
  uint16_t image_left_position;
  uint16_t image_top_position;
  uint16_t image_width;
  uint16_t image_height;
  uint8_t fields;
} image_descriptor_t;

typedef struct {
  uint8_t byte;
  int prev;
  int len;
} dictionary_entry_t;

typedef struct {
  uint8_t fields;
  uint16_t delay_time;
  uint8_t transparent_color_index;
} graphic_control_extension_t;

typedef struct {
  uint16_t left;
  uint16_t top;
  uint16_t width;
  uint16_t height;
  uint8_t cell_width;
  uint8_t cell_height;
  uint8_t foreground_color;
  uint8_t background_color;
} plaintext_extension_t;

typedef struct {
  uint8_t extension_code;
  uint8_t block_size;
  union {
    graphic_control_extension_t gce;
    application_extension_t app;
    plaintext_extension_t pte; //currently disabled to save memory, bc its not used anyways
  };
  uint8_t* data;
} extension_t;

//structs for saving
typedef struct {
  image_descriptor_t image_descriptor;
  int local_color_table_size;
  rgb* local_color_table;
  int data_length;
  uint8_t* decoded_data;
} normal_block_t;

typedef struct block_list{
  bool isExtension;
  union {
    normal_block_t block;
    extension_t extensionHeader;
  };
  struct block_list* next;
} block_list_t;

typedef struct {
  screen_descriptor_t screen_descriptor;
  int global_color_table_size; // number of entries in global_color_table
  rgb* global_color_table;
  block_list_t* blocks;
  block_list_t* lastBlock;
} gif_image_t;

bool process_gif_stream( File gif_file, gif_image_t* image);
void heapcheck(String tag);