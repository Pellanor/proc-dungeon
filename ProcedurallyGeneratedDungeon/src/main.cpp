/*This source code copyrighted by Lazy Foo' Productions (2004-2013)
and may not be redistributed without written permission.*/

//Using SDL, SDL OpenGL, GLEW, standard IO, and strings
#include <SDL.h>
#include <gl\glew.h>
#include <SDL_opengl.h>
#include <gl\glu.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <algorithm>
#include <time.h>
#include "../simplex_noise/simplexnoise.h"
#include "../height_map.h"


//Screen dimension constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int SQUARE_SIZE = 4;
const int HEIGHT_MAP_SIZE = 20;
const int X_SQUARES = SCREEN_WIDTH / SQUARE_SIZE;
const int Y_SQUARES = SCREEN_HEIGHT / SQUARE_SIZE;

struct Colour {
	Uint8 r;
	Uint8 g;
	Uint8 b;
};

Colour map[X_SQUARES][Y_SQUARES];

HeightMap heightMap;

int threshold = 0x1E;
//0x71 + 6 squares
//0x1E + 7 squares

//Starts up SDL and creates window
bool init();

//Initializes rendering program and clear color
bool initGL();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//Shader loading utility programs
void printProgramLog( GLuint program );
void printShaderLog( GLuint shader );

//Loads individual image as texture
SDL_Texture* loadTexture( std::string path );

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

//OpenGL context
SDL_GLContext gContext;

//Graphics program
GLuint gProgramID = 0;
GLint gVertexPos2DLocation = -1;
GLuint gVBO = 0;
GLuint gIBO = 0;

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
	srand(time(nullptr));
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
	float f_count = static_cast<float>(max_count);
	float total = static_cast<float>(X_SQUARES * Y_SQUARES);
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
		//Use OpenGL 3.1 core
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

		//Set texture filtering to linear
		if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
		{
			printf( "Warning: Linear texture filtering not enabled!" );
		}

		//Create window
		gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext( gWindow );
			if( gContext == NULL )
			{
				printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				//Initialize GLEW
				glewExperimental = GL_TRUE; 
				GLenum glewError = glewInit();
				if( glewError != GLEW_OK )
				{
					printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
				}

				//Use Vsync
				if( SDL_GL_SetSwapInterval( 1 ) < 0 )
				{
					printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
				}

				//Initialize OpenGL
				if( !initGL() )
				{
					printf( "Unable to initialize OpenGL!\n" );
					success = false;
				}
			}

			////Create renderer for window
			//gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
			//if( gRenderer == NULL )
			//{
			//	printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
			//	success = false;
			//}
			//else
			//{
			//	//Initialize renderer color
			//	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

			//	//Initialize PNG loading
			//	int imgFlags = IMG_INIT_PNG;
			//	if( !( IMG_Init( imgFlags ) & imgFlags ) )
			//	{
			//		printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
			//		success = false;
			//	}
			//}
		}
	}

	return success;
}

bool initGL()
{
	//Success flag
	bool success = true;

	//Generate program
	gProgramID = glCreateProgram();

	//Create vertex shader
	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );

	//Get vertex source
	const GLchar* vertexShaderSource[] =
	{
		"#version 140\nin vec2 LVertexPos2D; void main() { gl_Position = vec4( LVertexPos2D.x, LVertexPos2D.y, 0, 1 ); }"
	};

	//Set vertex source
	glShaderSource( vertexShader, 1, vertexShaderSource, NULL );

	//Compile vertex source
	glCompileShader( vertexShader );

	//Check vertex shader for errors
	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &vShaderCompiled );
	if( vShaderCompiled != GL_TRUE )
	{
		printf( "Unable to compile vertex shader %d!\n", vertexShader );
		printShaderLog( vertexShader );
        success = false;
	}
	else
	{
		//Attach vertex shader to program
		glAttachShader( gProgramID, vertexShader );


		//Create fragment shader
		GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

		//Get fragment source
		const GLchar* fragmentShaderSource[] =
		{
			"#version 140\nout vec4 LFragment; void main() { LFragment = vec4( 1.0, 1.0, 1.0, 1.0 ); }"
		};

		//Set fragment source
		glShaderSource( fragmentShader, 1, fragmentShaderSource, NULL );

		//Compile fragment source
		glCompileShader( fragmentShader );

		//Check fragment shader for errors
		GLint fShaderCompiled = GL_FALSE;
		glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled );
		if( fShaderCompiled != GL_TRUE )
		{
			printf( "Unable to compile fragment shader %d!\n", fragmentShader );
			printShaderLog( fragmentShader );
			success = false;
		}
		else
		{
			//Attach fragment shader to program
			glAttachShader( gProgramID, fragmentShader );


			//Link program
			glLinkProgram( gProgramID );

			//Check for errors
			GLint programSuccess = GL_TRUE;
			glGetProgramiv( gProgramID, GL_LINK_STATUS, &programSuccess );
			if( programSuccess != GL_TRUE )
			{
				printf( "Error linking program %d!\n", gProgramID );
				printProgramLog( gProgramID );
				success = false;
			}
			else
			{
				//Get vertex attribute location
				gVertexPos2DLocation = glGetAttribLocation( gProgramID, "LVertexPos2D" );
				if( gVertexPos2DLocation == -1 )
				{
					printf( "LVertexPos2D is not a valid glsl program variable!\n" );
					success = false;
				}
				else
				{
					//Initialize clear color
					glClearColor( 0.f, 0.f, 0.f, 1.f );

					//VBO data
					GLfloat vertexData[] =
					{
						-0.5f, -0.5f,
						 0.5f, -0.5f,
						 0.5f,  0.5f,
						-0.5f,  0.5f
					};

					//IBO data
					GLuint indexData[] = { 0, 1, 2, 3 };

					//Create VBO
					glGenBuffers( 1, &gVBO );
					glBindBuffer( GL_ARRAY_BUFFER, gVBO );
					glBufferData( GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW );

					//Create IBO
					glGenBuffers( 1, &gIBO );
					glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gIBO );
					glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW );
				}
			}
		}
	}
	
	return success;
}

void printProgramLog( GLuint program )
{
	//Make sure name is shader
	if( glIsProgram( program ) )
	{
		//Program log length
		int infoLogLength = 0;
		int maxLength = infoLogLength;
		
		//Get info string length
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
		
		//Allocate string
		char* infoLog = new char[ maxLength ];
		
		//Get info log
		glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
		if( infoLogLength > 0 )
		{
			//Print Log
			printf( "%s\n", infoLog );
		}
		
		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		printf( "Name %d is not a program\n", program );
	}
}

void printShaderLog( GLuint shader )
{
	//Make sure name is shader
	if( glIsShader( shader ) )
	{
		//Shader log length
		int infoLogLength = 0;
		int maxLength = infoLogLength;
		
		//Get info string length
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
		
		//Allocate string
		char* infoLog = new char[ maxLength ];
		
		//Get info log
		glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
		if( infoLogLength > 0 )
		{
			//Print Log
			printf( "%s\n", infoLog );
		}

		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		printf( "Name %d is not a shader\n", shader );
	}
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
	gWindow = nullptr;
	gRenderer = nullptr;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

SDL_Texture* loadTexture( std::string path )
{
	//The final texture
	SDL_Texture* newTexture = nullptr;

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

void initHeightMap()
{
	heightMap.init(SCREEN_HEIGHT/HEIGHT_MAP_SIZE, SCREEN_WIDTH/HEIGHT_MAP_SIZE);
	heightMap.generate();
}

void drawHeightMap()
{
	//Clear screen
	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
	SDL_RenderClear( gRenderer );
	for(int x = 0; x < heightMap.rows(); x++) {
		for(int y = 0; y < heightMap.cols(); y++) {
			SDL_Rect fillRect = { HEIGHT_MAP_SIZE*x, HEIGHT_MAP_SIZE*y, HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE };
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0x00 );
			SDL_RenderFillRect( gRenderer, &fillRect );
		}
	}
	
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
								break;//colour_map();
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

//				drawHeightMap();

				//Update screen
				SDL_RenderPresent( gRenderer );
			}
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}