/*This source code copyrighted by Lazy Foo' Productions (2004-2013)
and may not be redistributed without written permission.*/

//Using SDL, SDL_image, standard IO, math, and strings
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <algorithm>
#include <time.h>
#include "../simplex_noise/simplexnoise.h"


//Screen dimension constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int SQUARE_SIZE = 4;
const int X_SQUARES = SCREEN_WIDTH / SQUARE_SIZE;
const int Y_SQUARES = SCREEN_HEIGHT / SQUARE_SIZE;

struct Colour {
	Uint8 r;
	Uint8 g;
	Uint8 b;
};

Colour map[X_SQUARES][Y_SQUARES];
int threshold = 0x1E;
//0x71 + 6 squares
//0x1E + 7 squares

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//Loads individual image as texture
SDL_Texture* loadTexture( std::string path );

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

bool mutate_map() {

	//Second pass, fill in
	float temp_map[X_SQUARES][Y_SQUARES];
	int numbSquares = 7;

	for(int x = 1; x < X_SQUARES-1; x++) {
		for(int y = 1; y < Y_SQUARES-1; y++) {
			int val = 0;
			for(int xo = -1; xo <= 1; xo++) {
				for(int yo = -1; yo <= 1; yo++) {
					if(map[x+xo][y+yo].r > threshold)
						val++;
				}
			}
			if (val > numbSquares) {
				temp_map[x][y] = 0xFF;
			} else {
				temp_map[x][y] = 0x00;
			}
		}
	}

	for(int x = 0; x < X_SQUARES; x++) {
		for(int y = 0; y < Y_SQUARES; y++) {
			map[x][y].r = temp_map[x][y];
			map[x][y].g = temp_map[x][y];
			map[x][y].b = temp_map[x][y];
		}
	}

	return true;
}

int color_point(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
	int count = 1;
	map[x][y].r = r;
	map[x][y].g = g;
	map[x][y].b = b;
	if(x-1 >= 0 && map[x-1][y].r == 0x00 && map[x-1][y].g == 0x00 && map[x-1][y].b == 0x00)
		count += color_point(x-1, y, r, g, b);

	if(x+1 < X_SQUARES && map[x+1][y].r == 0x00 && map[x+1][y].g == 0x00 && map[x+1][y].b == 0x00)
		count += color_point(x+1, y, r, g, b);

	if(y-1 >= 0 && map[x][y-1].r == 0x00 && map[x][y-1].g == 0x00 && map[x][y-1].b == 0x00)
		count += color_point(x, y-1, r, g, b);

	if(y+1 < Y_SQUARES && map[x][y+1].r == 0x00 && map[x][y+1].g == 0x00 && map[x][y+1].b == 0x00)
		count += color_point(x, y+1, r, g, b);

	return count;
}

int colour_map() {
	int max_count = 0;
	Uint8 max_colour = 0xFF;
	for(int c = 1; c < 31; c++) {
		for(int i = 0; i < 1000; i++) {
			int x = rand() % X_SQUARES;
			int y = rand() % Y_SQUARES;
			if(map[x][y].r == 0x00 && map[x][y].g == 0x00 && map[x][y].b == 0x00) {
				int temp_count = color_point(x,y,0x08*c,0x08*c,0x08*c);
				if(temp_count > max_count) {
					max_count = temp_count;
					max_colour = 0x08*c;
				}
				break;
			}
		}
	}

	for(int x = 0; x < X_SQUARES; x++) {
		for(int y = 0; y < Y_SQUARES; y++) {
			if(map[x][y].r == max_colour) {
				map[x][y].r = 0xFE;
				map[x][y].g = 0xFE;
				map[x][y].b = 0xFE;
			} else {
				map[x][y].r = 0x00;
				map[x][y].g = 0x00;
				map[x][y].b = 0x00;
			}
		}
	}

	return max_count;
}

bool init_map(int count) {
	srand(time(NULL));
	int seed = rand() % 100;
	for(int x = 0; x < X_SQUARES; x++) {
		for(int y = 0; y < Y_SQUARES; y++) {
			Uint8 c = scaled_octave_noise_3d(1,0.5,1,0x00,0xFF,x,y,seed);
			map[x][y].r = c;
			map[x][y].g = c;
			map[x][y].b = c;
		}
	}

	mutate_map();
	mutate_map();
	int max_count = colour_map();
	float f_count = (float)(max_count);
	float total = (float)(X_SQUARES * Y_SQUARES);
	float coverage = f_count / total;
	printf("Threshold %d \n", threshold);
	printf("Coverage: %f / %f = %f \n", f_count, total, coverage);

	if(coverage < 0.25 && count < 10)
		init_map(count + 1);

	return true;
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
		{
			printf( "Warning: Linear texture filtering not enabled!" );
		}

		//Create window
		gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			//Create renderer for window
			gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
			if( gRenderer == NULL )
			{
				printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if( !( IMG_Init( imgFlags ) & imgFlags ) )
				{
					printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
					success = false;
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Nothing to load
	return success;
}

void close()
{
	//Destroy window	
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

SDL_Texture* loadTexture( std::string path )
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	return newTexture;
}

int main( int argc, char* args[] )
{
	//Start up SDL and create window
	if( !init() || !init_map(0))
	{
		printf( "Failed to initialize!\n" );
	}
	else
	{
		//Load media
		if( !loadMedia() )
		{
			printf( "Failed to load media!\n" );
		}
		else
		{	
			//Main loop flag
			bool quit = false;

			//Event handler
			SDL_Event e;

			//While application is running
			while( !quit )
			{
				//Handle events on queue
				while( SDL_PollEvent( &e ) != 0 )
				{
					//User requests quit
					if( e.type == SDL_QUIT )
					{
						quit = true;
					}
					if( e.type == SDL_KEYDOWN )
					{
						switch( e.key.keysym.sym ) {
							case SDLK_EQUALS:
								threshold ++;
								break;
							case SDLK_MINUS:
								threshold --;
								break;
							case SDLK_F5:
								init_map(0);
								break;
							case SDLK_ESCAPE:
								quit = true;
								break;
							default:
								break;colour_map();
						}
					}
				}


				//Clear screen
				SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
				SDL_RenderClear( gRenderer );
				for(int x = 0; x < X_SQUARES; x++) {
					for(int y = 0; y < Y_SQUARES; y++) {
						SDL_Rect fillRect = { SQUARE_SIZE*x, SQUARE_SIZE*y, SQUARE_SIZE, SQUARE_SIZE };
						SDL_SetRenderDrawColor( gRenderer, map[x][y].r, map[x][y].g, map[x][y].b, 0x00 );
						SDL_RenderFillRect( gRenderer, &fillRect );
					}
				}

				//Update screen
				SDL_RenderPresent( gRenderer );
			}
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}