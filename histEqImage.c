/*	CSCI366 Assignment 2
	Name: Aaron Colin Foote
	Date: 2nd May 2014
	File: histEqImage.c
*/

// Standard C libraries
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Signal handling library
#include <signal.h>

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

/* Bool not defined in C, defined here to identify false = 0 and true = 1
Could have included the line:
#include <stdbool.h>
But I am not certain what standard of C UOW systems operate on */

typedef enum {false, true} bool;

// Standard procedure, check program usage, send image for transformation, scan image, apply equalization
bool checkUsage(int argc, char *argv[]);
bool imageProcedure(SDL_Surface *image);
void scanImage(SDL_Surface *image, SDL_PixelFormat *fmt);
void applyEqualization(SDL_Surface *image, SDL_PixelFormat *fmt);

// Memory allocations and releases
void allocatePX(SDL_Surface *image);	// Allocate dynamic memory, initialization function
void releasePX();			// De-allocate dynamic memory

// Calculation of equalization of colours
void pixelProbability();
void cumHist();
void equalizeHist();

// Function controlling loop of image and movement / zooming
void imageControl(SDL_Surface *screen, SDL_Surface *image);

struct {
	// Store RGB values of each pixel of image
	Uint8 *red_pixels;
	Uint8 *green_pixels;
	Uint8 *blue_pixels;
	unsigned long int pixelCount;	//store total pixel count of image
	float alpha;	//255 divided by pixel count (for equalization)
	// Tally of each RGB intensity in the image
	int red_Count[256];
	int green_Count[256];
	int blue_Count[256];
	// Percentage / probability of each colour intensity in the image (per respective channel)
	float red_Pb[256];
	float green_Pb[256];
	float blue_Pb[256];
	// Cumulative histograms built for each channel
	int cumHist_Red[256];
	int cumHist_Green[256];
	int cumHist_Blue[256];
	// Lookup tables set for each channel and intensity
	int red_LUT[256];
	int green_LUT[256];
	int blue_LUT[256];
} px;

// Ensure appropriate program usage by user
bool checkUsage(int argc, char *argv[])
{
	// User passed wrong amount of values to program
	if (argc != 2) {
		printf("Usage: %s <bmp file>\n", argv[0]);
		return false;
	}
	else if (!(strcmp(argv[1], "-help"))) {
		// Display help messages followed by usage messages
		printf("\t- In order to use histEqImage, you must specify a bitmap file to be passed to the histEqImage. Specify this file as an argument that is to be passed to the program.\n\t- This image can then be navigated within the opened window, using the arrow keys on the keyboard.\n\t- The image can also be zoomed in or out by a factor of 2, using the + and - keys.\n\t- Exit the program using the 'q' or 'Q' key.\n");
		return false;
	}
	// Program usage appears to be correct so far
	else
		return true;
}

// Function manages the equalization of the image histogram
bool imageProcedure(SDL_Surface *image) {
	
	SDL_PixelFormat *fmt;

	fmt=image->format;
	if (fmt->BytesPerPixel != 3)	// Program only suitable for images with 3 bytes per pixel
		return false;

	allocatePX(image);	// Allocate dynamic memory, initialization function

	SDL_LockSurface(image);

	scanImage(image, fmt);	// Retrieve RGB values of every pixel, store in histogram

	// Calculate equalization histogram
	pixelProbability();
	cumHist();
	equalizeHist();

	applyEqualization(image, fmt);	// Change colours of all pixels to newly-calculated figures

	SDL_UnlockSurface(image);
	releasePX();	// De-allocate dynamic memory

	return true;
}

// Retrieve RGB values of every pixel, store in histogram
void scanImage(SDL_Surface *image, SDL_PixelFormat *fmt)
{
	int x = 0, y = 0;
	Uint8 *pixel = (Uint8 *)image->pixels + y * image->pitch + x * 3;
	Uint8 red = 0, green = 0, blue = 0;
	for (x = 0; x < image->w; x++)
	{
		for (y = 0; y < image->h; y++)
		{
			pixel = ((Uint8*)image->pixels + y * image->pitch + x * 3);	// Proceed to next pixel in image

			// Get red, green and blue pixel components
			red = pixel[0];
			green = pixel[1];
			blue = pixel[2];

			// Increment totals of each colour intensity
			px.red_Count[red]++;
			px.green_Count[green]++;
			px.blue_Count[blue]++;
		}
	}
}

// Store pixel count, declaration of storage for RGB values, set alpha value
void allocatePX(SDL_Surface *image)
{
	px.pixelCount = image->w * image->h;	// Used to identify memory allocation sizes below

	// Allocate enogh space to store RGB values of every pixel of image
	px.red_pixels = (Uint8 *)malloc(sizeof(Uint8)*px.pixelCount);
	px.green_pixels = (Uint8 *)malloc(sizeof(Uint8)*px.pixelCount);
	px.blue_pixels = (Uint8 *)malloc(sizeof(Uint8)*px.pixelCount);

	// Pre-calculate 255/MN
	px.alpha = 255.0/px.pixelCount;
}

// Store probability of each intensity of each colour
void pixelProbability()
{
	int i = 0;
	for (i = 0; i < 256; i++)
	{
		px.red_Pb[i] = (double)px.red_Count[i] / px.pixelCount;
		px.green_Pb[i] = (double)px.green_Count[i] / px.pixelCount;
		px.blue_Pb[i] = (double)px.blue_Count[i] / px.pixelCount;
	}
}

//Store cumulative histograms of each colour
void cumHist()
{
	px.cumHist_Red[0] = px.red_Count[0];
	px.cumHist_Green[0] = px.green_Count[0];
	px.cumHist_Blue[0] = px.blue_Count[0];

	int i = 1;
	for(i = 1; i < 256; i++)
	{
		px.cumHist_Red[i] = px.red_Count[i] + px.cumHist_Red[(i - 1)];
		px.cumHist_Green[i] = px.green_Count[i] + px.cumHist_Green[(i - 1)];
		px.cumHist_Blue[i] = px.blue_Count[i] + px.cumHist_Blue[(i - 1)];
	}
}

//Store values in lookup table, for image transformation at pixel level
void equalizeHist()
{
	int i = 0;
	for (i = 0; i < 256; i++) {
		px.red_LUT[i] = 0;
		px.green_LUT[i] = 0;
		px.blue_LUT[i] = 0;
	}
	for(i = 0; i < 256; i++)
	{
		px.red_LUT[i] = round(px.cumHist_Red[i] * px.alpha);
		px.green_LUT[i] = round(px.cumHist_Green[i] * px.alpha);
		px.blue_LUT[i] = round(px.cumHist_Blue[i] * px.alpha);
	}
}

//Change colours of all pixels to newly-calculated figures
void applyEqualization(SDL_Surface *image, SDL_PixelFormat *fmt)
{
	int x = 0, y = 0;
	Uint8 *pixel = (Uint8 *)image->pixels + y * image->pitch + x * 3;
	Uint8 red = 0, green = 0, blue = 0, newRed = 0, newGreen = 0, newBlue = 0;
	for (x = 0; x < image->w; x++)
	{
		for (y = 0; y < image->h; y++)
		{
			pixel = ((Uint8*)image->pixels + y * image->pitch + x * 3);	// Proceed to next pixel in image

			// Get red, green and blue pixel components
			red = pixel[0];		
			green = pixel[1];
			blue = pixel[2];

			// Configure each colour value of the pixel to the new and corrected value
			newRed=px.red_LUT[red];
			newGreen=px.green_LUT[green];
			newBlue=px.blue_LUT[blue];

			// Set values of pixel colours to new colours
			pixel[0] = newRed;
			pixel[1] = newGreen;
			pixel[2] = newBlue;
		}
	}
}

// De-allocate dynamic memory
void releasePX()
{
	free(px.red_pixels);
	free(px.green_pixels);
	free(px.blue_pixels);
}

// Function manages movement and display of image
void imageControl(SDL_Surface *screen, SDL_Surface *image)
{
	bool done = false;
	SDL_Event event;
	Uint8 *keys;
	SDL_Rect rect;

	rect.x = 0;
	rect.y = 0;
	rect.w = image->w;
	rect.h = image->h;

	// Process the keyboard event
	while (!done) {
		// Poll input queue, run keyboard loop
		while ( SDL_PollEvent(&event) ) {
			if ( event.type == SDL_QUIT ) {
				done = true;
				break;
			}
		}
		keys = SDL_GetKeyState(NULL);
		if (keys[SDLK_q]) {
			done = true;
		}
		// Each key movement is restricted to prevent movement past edges of image (in second part of conditions)
		else if(keys[SDLK_DOWN] && rect.y < (image->h-480) && (image->h > 480)) {
			rect.y++;	
		}
		else if(keys[SDLK_UP] && rect.y > 0) {
			rect.y--;
		}
		else if(keys[SDLK_RIGHT] && rect.x < (image->w - 640) && (image->w > 640)) {
			rect.x++;
		}
		else if(keys[SDLK_LEFT] && rect.x > 0) {
			rect.x--;
		}
		SDL_BlitSurface(image, &rect, screen, NULL);
		SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
		//SDL_Flip(screen);
		SDL_Delay(1);
	}

}

int main(int argc, char *argv[])
{
	SDL_Surface *screen, *image;

	if (!(checkUsage(argc, argv)))	// Returns as 1 for any misuse of command-line arguments
		return 1;

	// Initialize SDL video subsystem
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf("Can't init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	// Set the window's caption and icon
	SDL_WM_SetCaption("Assignment2", "app.ico");

	// Obtain the SDL surface of the video card
	screen = SDL_SetVideoMode(640, 480, 24, SDL_HWSURFACE);
	if (screen == NULL) {
		printf("Can't set video mode: %s\n", SDL_GetError());
		exit(1);
	}
	
	// Load BMP file
	image = SDL_LoadBMP(argv[1]);
	if (image == NULL) {
		printf("Can't load image: %s\n", SDL_GetError());
		exit(1);
	}

	// Failure to equalize the histogram of the image results in overall failure
	if (!imageProcedure(image)) {
		printf("The image enhancement failed\n");
		exit(1);
	}

	imageControl(screen, image);	// Allow the user to move and control the image

	// Free the image if it is no longer needed
	SDL_FreeSurface(image);
	SDL_FreeSurface(screen);
	SDL_Quit();

	return 0;
}
