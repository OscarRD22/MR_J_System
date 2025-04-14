/*
* Operating Systems - Compression Project
* Year 2024-25
*/
#ifndef _SO_COMPRESSION_H_
#define _SO_COMPRESSION_H_

// -------- LIBRARIES --------

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


/*
 If the process is too fast for you to debug, increment this value.
 This will add a delay to the process.
 The delay unit is in ms.
*/
#define ADDITIONAL_DELAY 0

// -------- ERROR CODES --------
/*
No need to handle them separately but it is necessary to know if something failed or not.

In case of an error, SO_compressImage will return a negative value corresponding to one of the following error codes.
*/
#define NO_ERROR 0
#define ERROR_DECODING_IMAGE -1             // The image could not be read
#define ERROR_SCALING_FACTOR -2             // The scaling factor is too large for the image
#define ERROR_MEM_ALLOC -3                  // Memory allocation error
#define ERROR_CREATING_TMP_FILE -4          // Error creating the temporary file
#define ERROR_UNSUPPORTED_FORMAT -5         // Unsupported image format
#define ERROR_CREATING_FINAL_FILE -6        // Error creating the final file
#define ERROR_NOT_WAV -7                    // The audio file is not a WAV file. Only WAV files are supported

// -------- FUNCTIONS --------

/**
 * Compresses the image by a scaling factor.
 * @param input_image Path to the image.
 * @param scale_factor Scaling factor. This can be >= 1 and must be less than the size of the image.
                       If the image is 100x100, the maximum scaling factor is 100. The result would be a 1x1 image.
                       If the image is 60x200, the maximum scaling factor is 60. The result would be a 1x3 image.
 * @return Will return an error code if any problem occurred. Otherwise, it will return 0.
 */
int SO_compressImage(char *input_image, int scale_factor);

/**
 * Compresses the audio by skipping intervals of time. Every X milliseconds, that part of the audio will be "skipped".
 * IMPORTANT: Only WAV audio files are supported.
 * @param input_file Path to the audio file.
 * @param interval_ms Interval in milliseconds. Every X milliseconds, that part of the audio will be "skipped".
                            
 * @return Will return an error code if any problem occurred. Otherwise, it will return 0.
 */
int SO_compressAudio(char *input_file, int interval_ms);

/** 
 * Deletes a file.
 * @param file_name Name of the file to delete.
 */
void SO_deleteFile(char *file_name);

#endif
