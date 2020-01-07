//#include <stdio.h>
//#include <stdlib.h>
//#include <fcntl.h>
#include <Arduino.h>

//#include "pixelDatatypes.h"
#include "gif.h"

#define EXTENSION_INTRODUCER   0x21
#define IMAGE_DESCRIPTOR       0x2C
#define TRAILER                0x3B

inline void heapcheck(String tag) {
  /*Serial.print("Heapcheck ");
  Serial.print(tag);
  Serial.print(": ");
  if (heap_caps_check_integrity_all(true)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    //heap_caps_dump_all();
    Serial.println("\n====================");
    ESP.restart();
  }*/
}

int uncompress( int code_length, const char *input, int input_length, uint8_t *out ) {
  //int maxbits;
  int i, bit;
  int code, prev = -1;
  dictionary_entry_t *dictionary;
  int dictionary_ind;
  unsigned int mask = 0x01;
  int reset_code_length;
  int clear_code; // This varies depending on code_length
  int stop_code;  // one more than clear code
  int match_len;

  clear_code = 1 << ( code_length );
  stop_code = clear_code + 1;
  // To handle clear codes
  reset_code_length = code_length;

  // Create a dictionary large enough to hold "code_length" entries.
  // Once the dictionary overflows, code_length increases
  dictionary = ( dictionary_entry_t * ) malloc( sizeof( dictionary_entry_t ) * ( 1 << ( code_length + 1 ) ) );
  /*Serial.print("uncompressing with dict at ");
  Serial.print((int) dictionary, HEX);
  Serial.print(", clength: ");
  Serial.print(code_length);
  Serial.print(", free: ");
  Serial.print(xPortGetFreeHeapSize());
  Serial.print(", size: ");
  Serial.println((int) (sizeof(dictionary_entry_t) * (1<<(code_length + 1))));*/

  // Initialize the first 2^code_len entries of the dictionary with their
  // indices.  The rest of the entries will be built up dynamically.

  // Technically, it shouldn't be necessary to initialize the
  // dictionary.  The spec says that the encoder "should output a
  // clear code as the first code in the image data stream".  It doesn't
  // say must, though...
  for ( dictionary_ind = 0; dictionary_ind < ( 1 << code_length ); dictionary_ind++ ) {
    dictionary[ dictionary_ind ].byte = dictionary_ind;
    // XXX this only works because prev is a 32-bit int (> 12 bits)
    dictionary[ dictionary_ind ].prev = -1;
    dictionary[ dictionary_ind ].len = 1;
  }

  // 2^code_len + 1 is the special "end" code; don't give it an entry here
  dictionary_ind++;
  dictionary_ind++;
  
  // TODO verify that the very last byte is clear_code + 1
  while ( input_length ) {
    code = 0x0;
    // Always read one more bit than the code length
    for ( i = 0; i < ( code_length + 1 ); i++ ) {
      // This is different than in the file read example; that 
      // was a call to "next_bit"
      bit = ( *input & mask ) ? 1 : 0;
      mask <<= 1;

      if ( mask == 0x100 ) {
        mask = 0x01;
        input++;
        input_length--;
      }

      code = code | ( bit << i );
    }

    if ( code == clear_code ) {
      code_length = reset_code_length;
      dictionary = ( dictionary_entry_t * ) realloc( dictionary,
        sizeof( dictionary_entry_t ) * ( 1 << ( code_length + 1 ) ) );

      for ( dictionary_ind = 0; dictionary_ind < ( 1 << code_length ); dictionary_ind++ ) {
        dictionary[ dictionary_ind ].byte = dictionary_ind;
        // XXX this only works because prev is a 32-bit int (> 12 bits)
        dictionary[ dictionary_ind ].prev = -1;
        dictionary[ dictionary_ind ].len = 1;
      }
      dictionary_ind++;
      dictionary_ind++;
      prev = -1;
      continue;
    } else if ( code == stop_code ) {
      if ( input_length > 1 ) {
        fprintf( stderr, "Malformed GIF (early stop code)\n" );
        exit( 0 );
      }
      break;
    }

    // Update the dictionary with this character plus the _entry_
    // (character or string) that came before it
    if ( ( prev > -1 ) && ( code_length < 12 ) ) {
      if ( code > dictionary_ind ) {
        fprintf( stderr, "code = %.02x, but dictionary_ind = %.02x\n",
          code, dictionary_ind );
        exit( 0 );
      }

      // Special handling for KwKwK
      if ( code == dictionary_ind ) {
        int ptr = prev;

        while ( dictionary[ ptr ].prev != -1 ) {
          ptr = dictionary[ ptr ].prev;
        }
        dictionary[ dictionary_ind ].byte = dictionary[ ptr ].byte;
      } else {
        int ptr = code;
        while ( dictionary[ ptr ].prev != -1 ) {
          ptr = dictionary[ ptr ].prev;
        }
        dictionary[ dictionary_ind ].byte = dictionary[ ptr ].byte;
      }

      dictionary[ dictionary_ind ].prev = prev;

      dictionary[ dictionary_ind ].len = dictionary[ prev ].len + 1;

      dictionary_ind++;

      // GIF89a mandates that this stops at 12 bits
      if ( ( dictionary_ind == ( 1 << ( code_length + 1 ) ) ) && ( code_length < 11 ) ) {
        code_length++;

        dictionary = ( dictionary_entry_t * ) realloc( dictionary,
          sizeof( dictionary_entry_t ) * ( 1 << ( code_length + 1 ) ) );
      }
    }

    prev = code;

    // Now copy the dictionary entry backwards into "out"
    match_len = dictionary[ code ].len;
    while ( code != -1 ) {
      out[ dictionary[ code ].len - 1 ] = dictionary[ code ].byte;
      if ( dictionary[ code ].prev == code ) {
        fprintf( stderr, "Internal error; self-reference." );
        exit( 0 );
      }
      code = dictionary[ code ].prev;
    }

    out += match_len;
  }
  free(dictionary);
}

static int read_sub_blocks( File gif_file, char **data ) {
  int data_length;
  int index;
  char block_size;

  // Everything following are data sub-blocks, until a 0-sized block is
  // encountered.
  data_length = 0;
  *data = NULL;
  index = 0;

  while (1) {
    if ( gif_file.readBytes( &block_size, 1 ) < 1 ) {
      perror( "Invalid GIF file (too short1): " );
      return -1;
    }

    if ( block_size == 0 ) { // end of sub-blocks
      break;
    }

    data_length += block_size;
    *data = (char *) realloc( *data, data_length );

    // TODO this could be split across block size boundaries
    if ( gif_file.readBytes( *data + index, block_size ) < block_size ) {
      perror( "Invalid GIF file (too short2): " );
      return -1;
    }

    index += block_size;
  }

  return data_length;
}

static block_list_t* addBlockListEntry(gif_image_t* image) {
  block_list_t* blockListEntry = (block_list_t*) calloc(1, sizeof(block_list_t));
  blockListEntry->next = 0;
  if (image->lastBlock == 0) {
    image->blocks = blockListEntry;
  } else {
    image->lastBlock->next = blockListEntry;
  }
  image->lastBlock = blockListEntry;
  //heapcheck("BL3");
  return blockListEntry;
}

static int process_image_descriptor( File gif_file, rgb *gct, int gct_size, int resolution_bits, gif_image_t* image ) {
  block_list_t* blockListEntry = addBlockListEntry(image);
  blockListEntry->isExtension = false;
  
  int disposition;
  int compressed_data_length;
  char *compressed_data = NULL;
  char lzw_code_size;

  // TODO there could actually be lots of these
  if ( gif_file.readBytes( (char*) &blockListEntry->block.image_descriptor, 9 ) < 9 ) {
    perror( "Invalid GIF file (too short3)" );
    disposition = 0;
    goto done;
  }
  

  if (blockListEntry->block.image_descriptor.fields && 0x80) {
    //int i;
    // If bit 7 is set, the next block is a global color table; read it
    blockListEntry->block.local_color_table_size = 1 << 
      ( ( ( blockListEntry->block.image_descriptor.fields & 0x07 ) + 1 ) );

    blockListEntry->block.local_color_table = ( rgb * ) malloc( 3 * blockListEntry->block.local_color_table_size );

    // XXX this could conceivably return a short count...
    if ( gif_file.readBytes( (char*) blockListEntry->block.local_color_table, 3 * blockListEntry->block.local_color_table_size ) < 3 * blockListEntry->block.local_color_table_size ) {
      perror( "Unable to read local color table" );
      return false;
    }
  }


  disposition = 1;

  if ( gif_file.readBytes( &lzw_code_size, 1 ) < 1 ) {
    perror( "Invalid GIF file (too short4): " );
    disposition = 0;
    goto done;
  }

  compressed_data_length = read_sub_blocks( gif_file, &compressed_data );

  blockListEntry->block.data_length = blockListEntry->block.image_descriptor.image_width * blockListEntry->block.image_descriptor.image_height;
  blockListEntry->block.decoded_data = (uint8_t *) malloc( blockListEntry->block.data_length );

  uncompress( lzw_code_size, compressed_data, compressed_data_length, blockListEntry->block.decoded_data );

done:
  if ( compressed_data )
    free( compressed_data );

  //if ( uncompressed_data )
  //  free( uncompressed_data );

  return disposition;
}

static int process_extension( File gif_file, gif_image_t* image ) {
  block_list_t* blockListEntry = addBlockListEntry(image);
  blockListEntry->isExtension = true;

  int extension_data_length;

  if ( gif_file.readBytes( (char*) (&blockListEntry->extensionHeader), 2 ) < 2 ) {
    perror( "Invalid GIF file (too short5): " );
    return 0;
  }

  switch ( blockListEntry->extensionHeader.extension_code ) {
    case GRAPHIC_CONTROL:
      //heapcheck("GCE");
      if ( gif_file.readBytes( (char*) &blockListEntry->extensionHeader.gce, 4 ) < 4 ) {
        perror( "Invalid GIF file (too short6): " );
        return 0;
      }

      break;
    case APPLICATION_EXTENSION:
      //heapcheck("APP");
      if ( gif_file.readBytes( (char*) &blockListEntry->extensionHeader.app, 11 ) < 11 ) {
        perror( "Invalid GIF file (too short7): " );
        return 0;
      }
      break;
    case 0xFE:
      //heapcheck("Comment");
      // comment extension; do nothing - all the data is in the
      // sub-blocks that follow.
      break;
    case 0x01:
      //heapcheck("PTE");
      if ( gif_file.readBytes( (char*) &blockListEntry->extensionHeader.pte, 12 ) < 12 ) {
        perror( "Invalid GIF file (too short8): " );
        return 0;
      }
      break;
    default:
      //heapcheck("Default");
      fprintf( stderr, "Unrecognized extension code.\n" );
      exit( 0 );
  }

  // All extensions are followed by data sub-blocks; even if it's
  // just a single data sub-block of length 0
  extension_data_length = read_sub_blocks( gif_file, (char**) &blockListEntry->extensionHeader.data );

  //if ( blockListEntry->extensionHeader.data != NULL ) free( blockListEntry->extensionHeader.data );

  return 1;
}

/**
 * @param gif_file the file descriptor of a file containing a
 *  GIF-encoded file.  This should point to the first byte in
 *  the file when invoked.
 * @param image the gif_image_t object to store the information in
 */
bool process_gif_stream( File gif_file, gif_image_t* image) {
  image->blocks = 0;
  image->lastBlock = 0;

  char header[ 7 ];
  
  int color_resolution_bits;
  
  char block_type = 0x0;

  // A GIF file starts with a Header (section 17)
  
  if ( gif_file.readBytes( header, 6 ) != 6 ) {
    perror( "Invalid GIF file (too short9)" );
    return false;
  }
  header[ 6 ] = 0x0;

  // XXX there's another format, GIF87a, that you may still find
  // floating around.
  if ( strcmp( "GIF89a", (char*) header ) ) {
    fprintf( stderr, 
      "Invalid GIF file (header is '%s', should be 'GIF89a')\n",
      header );
    return false;
  }


  // Followed by a logical screen descriptor
  // Note that this works because GIFs specify little-endian order; on a
  // big-endian machine, the height & width would need to be reversed.

  // Can't use sizeof here since GCC does byte alignment; 
  // sizeof( image.screen_descriptor_t ) = 8!
  if ( gif_file.readBytes( (char*) &image->screen_descriptor, 7 ) < 7 ) {
    perror( "Invalid GIF file (too short10)" );
    return false;
  }

  color_resolution_bits = ( ( image->screen_descriptor.fields & 0x70 ) >> 4 ) + 1;

  if ( image->screen_descriptor.fields & 0x80 ) {
    //int i;
    // If bit 7 is set, the next block is a global color table; read it
    image->global_color_table_size = 1 << 
      ( ( ( image->screen_descriptor.fields & 0x07 ) + 1 ) );

    image->global_color_table = ( rgb * ) malloc( 3 * image->global_color_table_size );

    // XXX this could conceivably return a short count...
    if ( gif_file.readBytes( (char*) image->global_color_table, 3 * image->global_color_table_size ) < 3 * image->global_color_table_size ) {
      perror( "Unable to read global color table" );
      return false;
    }
  }

  //heapcheck("GIF Postheader");
  //int blockcount = 0;
  //image.application_extension = ;

  while ( block_type != TRAILER ) {
    if ( gif_file.readBytes( &block_type, 1 ) < 1 ) {
      perror( "Invalid GIF file (too short11)" );
      return false;
    }

    switch ( block_type ) {
      case IMAGE_DESCRIPTOR:
        //Serial.print("D");
        if ( !process_image_descriptor( gif_file, image->global_color_table, image->global_color_table_size, color_resolution_bits, image ) ) {
          return false;
        }
        break;
      case EXTENSION_INTRODUCER:
        //Serial.print("I");
        if ( !process_extension( gif_file, image ) ) {
          return false;
        }
        break;
      case TRAILER:
        //Serial.print("T");
        return true;
        break;
      default:
        //Serial.print("N");
        fprintf( stderr, "Bailing on unrecognized block type %.02x\n", block_type );
        return false;
    }
    //blockcount++;
    //Serial.print(blockcount);
    //Serial.print(" ");
    //heapcheck("GIF Block");
  }
  return false;
}