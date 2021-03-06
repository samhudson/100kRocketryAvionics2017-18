/*******************************************************
*Title: trace.cpp
*Author: Glenn Upthagrove
*Date: 01/30/18
*Description: A 3D Trace for the flight path of the 
*rocket for the high altitude rocketry challenge. 
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h> //for strcmp
#include <vector>
#include "../conversion/telemetry.h"

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#include "glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"

#include <convert.hpp>
#include <tplane.hpp>


// multiprocessing includes 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

//multithreading include 
#include <pthread.h>


//	This is a 3D trace for the flightpath of the rocket of the Oreon State University 
//	chapter of AIAA high altitude rocketry challenge. Thus uses OpenGL through C++.  
//
//	The objective is to draw a 3D flight path of the rocket using the data retrieved 
//	from the rocket. 
//
//	The left mouse button does rotation
//	The middle mouse button does scaling
//	The user interface allows:
//		1. The axes to be turned on and off
//		2. The color of the axes to be changed
//		3. Debugging to be turned on and off
//		4. Depth cueing to be turned on and off
//		5. The projection to be changed
//		6. The transformations to be reset
//		7. The program to quit
//
//	Author:			Glenn Upthagrove 

//NOTE: Much of this code comes from Dr. Mike Bailey of Oregon State University, but it is 
//being used with his express permission. 

// texture pieces 

int     ReadInt( FILE * );
short   ReadShort( FILE * );


struct bmfh
{
        short bfType;
        int bfSize;
        short bfReserved1;
        short bfReserved2;
        int bfOffBits;
} FileHeader;

struct bmih
{
        int biSize;
        int biWidth;
        int biHeight;
        short biPlanes;
        short biBitCount;
        int biCompression;
        int biSizeImage;
        int biXPelsPerMeter;
        int biYPelsPerMeter;
        int biClrUsed;
        int biClrImportant;
} InfoHeader;

//rocket data
struct rocket_data{
	float x;
	float y;
	float z;
};


//sphere stuff
bool	Distort;		// global -- true means to distort the texture

struct point {
	float x, y, z;		// coordinates
	float nx, ny, nz;	// surface normal
	float s, t;		// texture coords
};

int		NumLngs, NumLats;
struct point *	Pts;

const int birgb = { 0 };


// title of these windows:

const char *WINDOWTITLE = { "3D Trace" };
const char *GLUITITLE   = { "UI Window" };


// what the glui package defines as true and false:

const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };


// the escape key:

#define ESCAPE		0x1b

//define sky radius

#define RADIUS		200.0

//ground params

#define PLANESIZE	400.0
#define PLANERES	512


//define animation params

#define MS_IN_THE_ANIMATION_CYCLE 500000
float Time = 0.0;


// initial window size:

const int INIT_WINDOW_SIZE = { 600 };


// size of the box:

const float BOXSIZE = { 2.f };



// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };


// minimum allowable scale factor:

const float MINSCALE = { 0.05f };


// active mouse buttons (or them together):

const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };


// which projection:

enum Projections
{
	ORTHO,
	PERSP
};


// which button:

enum ButtonVals
{
	RESET,
	QUIT
};


// window background color (rgba):

const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };


// line width for the axes:

const GLfloat AXES_WIDTH   = { 3. };


// the color numbers:
// this order must match the radio button order

enum Colors
{
	RED,
	YELLOW,
	GREEN,
	CYAN,
	BLUE,
	MAGENTA,
	WHITE,
	BLACK
};

char * ColorNames[ ] =
{
	"Red",
	"Yellow",
	"Green",
	"Cyan",
	"Blue",
	"Magenta",
	"White",
	"Black"
};


// the color definitions:
// this order must match the menu order

const GLfloat Colors[ ][3] = 
{
	{ 1., 0., 0. },		// red
	{ 1., 1., 0. },		// yellow
	{ 0., 1., 0. },		// green
	{ 0., 1., 1. },		// cyan
	{ 0., 0., 1. },		// blue
	{ 1., 0., 1. },		// magenta
	{ 1., 1., 1. },		// white
	{ 0., 0., 0. },		// black
};


// fog parameters:

const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE     = { GL_LINEAR };
const GLfloat FOGDENSITY  = { 0.30f };
const GLfloat FOGSTART    = { 1.5 };
const GLfloat FOGEND      = { 4. };


// non-constant global variables:

int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		DepthCueOn;				// != 0 means to use intensity depth cueing
int		DepthBufferOn;			// != 0 means to use the z-buffer
int		DepthFightingOn;		// != 0 means to use the z-buffer
GLuint	BoxList;				// object display list
GLuint  SphereList;				// object display list
GLuint  GroundList;				// object display list
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		WhichColor;				// index into Colors[ ]
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees
unsigned char *skytex1;				// sky texture
unsigned char *maptex1;				// map texture
GLuint skytex;					// current sky texture
GLuint maptex;					// current map texture
int width, height;				// texture details
char apogeebuff[512];
bool center;
float xdist = 0.0;
float zdist = 0.0;
char* defmap = "./resources/textures/defmap.bmp";
char* ndefmap = "./resources/textures/map.bmp";
char* usedmap;
bool nrt;					//nrt flag
bool nrt2;					//nrt flag
vector<struct telem_data>data;
bool inited;
char message[256];
char message2[256];
char* messagecpy;
char buff[10];
bool over = false;
int bytes;
char* end = NULL;
vector<struct rocket_data> r_data;
char* token;
struct rocket_data temprd;
int track;

// function prototypes:

bool	detect_internet_connection();
void	Animate( );
void	Display( );
void	DoAxesMenu( int );
void	DoColorMenu( int );
void	DoDepthBufferMenu( int );
void	DoDepthFightingMenu( int );
void	DoDepthMenu( int );
void	DoDebugMenu( int );
void	DoMainMenu( int );
void	DoProjectMenu( int );
void	DoRasterString( float, float, float, char * );
void	DoStrokeString( float, float, float, float, char * );
float	ElapsedSeconds( );
void	InitTextures();
void	InitGraphics( );
void	InitLists( );
void	InitMenus( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Resize( int, int );
void	Visibility( int );

void	Axes( float );
void	HsvRgb( float[3], float [3] );

struct point * PtsPointer( int lat, int lng );
void DrawPoint( struct point *p );
void MjbSphere( float radius, int slices, int stacks );
//thread function
void* nrt_listen(void*);
void update_data();


// teture functions 

unsigned char *
BmpToTexture( char *filename, int *width, int *height ) // width and height in pixels 
{

        int s, t, e;            // counters
        int numextra;           // # extra bytes each line in the file is padded with
        FILE *fp;
        unsigned char *texture;
        int nums, numt;
        unsigned char *tp;


        fp = fopen( filename, "rb" );
        if( fp == NULL )
        {
                fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
                return NULL;
        }

        FileHeader.bfType = ReadShort( fp );


        // if bfType is not 0x4d42, the file is not a bmp:

        if( FileHeader.bfType != 0x4d42 )
        {
                fprintf( stderr, "Wrong type of file: 0x%0x\n", FileHeader.bfType );
                fclose( fp );
                return NULL;
        }


        FileHeader.bfSize = ReadInt( fp );
        FileHeader.bfReserved1 = ReadShort( fp );
        FileHeader.bfReserved2 = ReadShort( fp );
        FileHeader.bfOffBits = ReadInt( fp );


        InfoHeader.biSize = ReadInt( fp );
        InfoHeader.biWidth = ReadInt( fp );
        InfoHeader.biHeight = ReadInt( fp );

        nums = InfoHeader.biWidth;
        numt = InfoHeader.biHeight;

        InfoHeader.biPlanes = ReadShort( fp );
        InfoHeader.biBitCount = ReadShort( fp );
        InfoHeader.biCompression = ReadInt( fp );
        InfoHeader.biSizeImage = ReadInt( fp );
        InfoHeader.biXPelsPerMeter = ReadInt( fp );
        InfoHeader.biYPelsPerMeter = ReadInt( fp );
        InfoHeader.biClrUsed = ReadInt( fp );
        InfoHeader.biClrImportant = ReadInt( fp );


        // fprintf( stderr, "Image size found: %d x %d\n", ImageWidth, ImageHeight );


        texture = new unsigned char[ 3 * nums * numt ];
        if( texture == NULL )
        {
                fprintf( stderr, "Cannot allocate the texture array!\b" );
                return NULL;
        }


        // extra padding bytes:

        numextra =  4*(( (3*InfoHeader.biWidth)+3)/4) - 3*InfoHeader.biWidth;


        // we do not support compression:

        if( InfoHeader.biCompression != birgb )
        {
                fprintf( stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression );
                fclose( fp );
                return NULL;
        }



        rewind( fp );
        fseek( fp, 14+40, SEEK_SET );

        if( InfoHeader.biBitCount == 24 )
        {
                for( t = 0, tp = texture; t < numt; t++ )
                {
                        for( s = 0; s < nums; s++, tp += 3 )
                        {
                                *(tp+2) = fgetc( fp );          // b
                                *(tp+1) = fgetc( fp );          // g
                                *(tp+0) = fgetc( fp );          // r
                        }

                        for( e = 0; e < numextra; e++ )
                        {
                                fgetc( fp );
                        }
                }
        }

        fclose( fp );

        *width = nums;
        *height = numt;
	cout << "loaded texture from " << filename << endl;
	cout << "width: " << nums << endl;
	cout << "height: " << numt << endl;
        return texture;

}

int
ReadInt( FILE *fp )
{
        unsigned char b3, b2, b1, b0;
        b0 = fgetc( fp );
        b1 = fgetc( fp );
        b2 = fgetc( fp );
        b3 = fgetc( fp );
        return ( b3 << 24 )  |  ( b2 << 16 )  |  ( b1 << 8 )  |  b0;
}


short
ReadShort( FILE *fp )
{
        unsigned char b1, b0;
        b0 = fgetc( fp );
        b1 = fgetc( fp );
        return ( b1 << 8 )  |  b0;
}




// main program:

int
main( int argc, char *argv[ ] )
{
	messagecpy = new char[256];
	XSCALE = YSCALE = ZSCALE = 1;
	inited = 0;
	slat = 45.0;
	slon = 45.0;
	nrt = false;
	nrt2 = false;
//	pl_used = 0;
	char buff1[64];
	char buff2[64];
	int ret;
	pid_t childid;
	int child_exit = -5;
//	pthread_t pipethread;
	track = 59;

	memset(buff1, '\0', 64);
	memset(buff2, '\0', 64);

	//detect internet
	bool connected = detect_internet_connection();

	cdebug = false; //initialize conversion debug 
	center = false; //initialize center bool
	for(int i=0; i<argc; i++){
                if(strcmp(argv[i], "-debug") == 0){ //debug on?
                        cdebug = true;
                }
                else if(strcmp(argv[i], "-center") == 0){ //debug on?
                        center = true;
                }
                else if(strcmp(argv[i], "-x") == 0){ //scale
                        XSCALE = atoi(argv[i+1]);
                }
                else if(strcmp(argv[i], "-y") == 0){ //scale
                        YSCALE = atoi(argv[i+1]);
                }
                else if(strcmp(argv[i], "-z") == 0){ //scale
                        ZSCALE = atoi(argv[i+1]);
                }
                else if(strcmp(argv[i], "-nrt") == 0){ //near real time mode?
                        nrt = true;
			cout << "in nrt mode" << endl;
			fflush(stdout);
                }

	}
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)
	cout << "glut init" << endl;
	glutInit( &argc, argv );


	// setup all the graphics stuff:

	cout << "initing graphics" << endl;
	InitGraphics( );

	//get slat and slon
	if(nrt){//hang until data is recieved
		//read from pipe
        	memset(message, '\0', 256);
        	while(strstr(message, "&&") == NULL){
                        memset(buff, '\0', 10);
                       	bytes = read(3, buff, 9);
                       	strcat(message, buff);
                       	if(bytes == -1){ //error cases
                     	          printf("READ ERROR IN LOGGER, RETURN OF -1\n");
                     	          fflush(stdout);
                     	          exit(4);
                       	}
                    	if(bytes == 0){
                             	  printf("NO READ IN LOGGER, RETURN OF 0\n");
                                  fflush(stdout);
                             	  exit(5);
                        }

                }
		memset(messagecpy, '\0', 256);
                //fix the string
                end = strstr(message, "&&"); //points to first &
                *end = '\0'; //null terminate

		strcpy(messagecpy, message);
		token = strtok(messagecpy, ":");
		token = strtok(NULL, ":");
		token = strtok(NULL, "\"");
		temprd.z = atof(token);
		token = strtok(NULL, ":");
		token = strtok(NULL, "\"");
		temprd.x = atof(token);
		token = strtok(NULL, ":");
		token = strtok(NULL, "\"");
		temprd.y = atof(token);
		slat = temprd.z;
		slon = temprd.x;
		temprd.x = (-(slon - temprd.x) * (cos(slat*(M_PI/180.0))*69.172));
		temprd.y = temprd.y / 5280.0;
		temprd.z = ((slat - temprd.z)*69.0);
		r_data.push_back(temprd);

                fflush(stdout);
	}

//	slat = 45.0;
//	slon = 45.0;
	cout << "starting init lists" << endl;
	InitLists( );
	cout << "leaving init lists" << endl;

	//start recieving data in nrt
	//HERE
	//spawn thread to read data from a pipe


	//get map
        if(connected){
                //cout << "Internet detected" << endl;
                //get non-default map
                childid = fork();
                switch(childid){
                        case -1: //error
                                printf("ERROR SPAWNING CHILD\n");
                                fflush(stdout);
                                exit(2);

                        case 0: //child
				cout << "slat: " << slat << endl;
				cout << "slon: " << slon << endl;
				sprintf(buff1, "%f", slat);
				sprintf(buff2, "%f", slon);
				chdir("./map");
                                ret = execlp("python", "python", "./getmap.py", buff1, buff2, NULL);
                                if(ret == -1){
                                        printf("ERROR GENERATING MAP\n");
                                        fflush(stdout);
                                        exit(1);
                                }
                        default: //parent
                                waitpid(childid, &child_exit, 0); //wait on getmap
				
                }
                usedmap = ndefmap;
        }
        else{
                //cout << "Internet not detected" << endl;
                usedmap = defmap;
        }

	// initialize textures
	InitTextures();

	if(!nrt){
		sprintf(apogeebuff, "%s%s ft.", "Apogee: ", apaltbuff);
	}

	// init all the global variables used by Display( ):
	// this will also post a redisplay
	cout << "reseting" <<endl;
	Reset( );


	// setup all the user interface stuff:

	cout << "calling init menus" <<endl;
	InitMenus( );
	cout << "leaving init menus" <<endl;


	// draw the scene once and wait for some interaction:
	// (this will never return)

	cout << "calling glut set window" <<endl;
	glutSetWindow( MainWindow );
	cout << "leaving glut set window" <<endl;
	cout << "enterting main loop" <<endl;
	glutMainLoop( );


	// this is here to make the compiler happy:

	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutMainLoop( ) do it

void
Animate( )
{
	// put animation stuff in here -- change some global variables
	// for Display( ) to find:

	// force a call to Display( ) next time it is convenient:

	int ms = glutGet( GLUT_ELAPSED_TIME );	// milliseconds
	ms  %=  MS_IN_THE_ANIMATION_CYCLE;
	Time = (float)ms  /  (float)MS_IN_THE_ANIMATION_CYCLE;        // [ 0., 1. )	

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// draw the complete scene:

void
Display( )
{
	//get new data or no new data signal
	//cout << "in display" <<endl;
	if(nrt && (!over)){//hang until data is recieved
		track++;
		cout << "in nrt and not over" << endl;
		//read from pipe
		cout << "memsetting message" << endl;
        	memset(message, '\0', 256);
		if(message2[0] != '\0'){
			strcat(message, message2);
		}
        	memset(message2, '\0', 256);
		cout << "done" << endl;
		cout << "memsetting buff" << endl;
                memset(buff, '\0', 10);
		cout << "done" << endl;
        	while(strstr(message, "&&") == NULL){
			cout << "memsetting buff" << endl;
                        memset(buff, '\0', 10);
			cout << "done" << endl;
			cout << "reading from pipe" << endl;
                       	bytes = read(3, buff, 9);
			cout << "read from pipe: " << buff << endl;
                       	strcat(message, buff);
			cout << "message is now: " << message << endl;
                       	if(bytes == -1){ //error cases
                     	          printf("READ ERROR IN LOGGER, RETURN OF -1\n");
                     	          fflush(stdout);
                     	          exit(4);
                       	}
                    	if(bytes == 0){
                             	  printf("NO READ IN LOGGER, RETURN OF 0\n");
                                  fflush(stdout);
                             	  exit(5);
                        }
			if(strcmp(message, "**&&")==0){
				over = true;
			}
                }
                //fix the string
                end = strstr(message, "&&"); //points to first &
                *end = '\0'; //null terminate
		if(end+2 != '\0'){
			strcat(message2, end+2);
		}
		if(strcmp(message, "**")==0){
			over = true;
			cout << "over signal recieved" << endl;
		}
                //cout << "display recieved: " << message << endl;
		if(track >= 60){
                	cout << "display recieved: " << message << endl;
			cout << " calling update_data()" << endl;
			update_data(); //HERE updates the vector
			cout << "finished call to update_data()" << endl;
			track = 0;
		}
                fflush(stdout);
	}
	if( DebugOn != 0 )
	{
		fprintf( stderr, "Display\n" );
	}


	// set which window we want to do the graphics into:

	glutSetWindow( MainWindow );


	// erase the background:

	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if( DepthBufferOn != 0 )
		glEnable( GL_DEPTH_TEST );
	else
		glDisable( GL_DEPTH_TEST );


	// specify shading to be flat:

	glShadeModel( GL_FLAT );


	// set the viewport to a square centered in the window:

	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );


	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	if( WhichProjection == ORTHO )
		glOrtho( -3., 3.,     -3., 3.,     0.1, 1000. );
	else
		gluPerspective( 90., 1.,	0.1, 1000. );


	// place the objects into the scene:

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );


	// set the eye position, look-at position, and up-vector:

	gluLookAt( 0., 10., 15.,     0., 10., 0.,     0., 1., 0. );


	// translate the scene:
	glTranslatef(xdist, 0., zdist);


	// rotate the scene:

	glRotatef( (GLfloat)Yrot, 0., 1., 0. );
	//if((Xrot > -30) && (Xrot < 150)){
	glRotatef( (GLfloat)Xrot, 1., 0., 0. );
	//}


	// uniformly scale the scene:

	if( Scale < MINSCALE )
		Scale = MINSCALE;
	glScalef( (GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale );


	// set the fog parameters:

	if( DepthCueOn != 0 )
	{
		glFogi( GL_FOG_MODE, FOGMODE );
		glFogfv( GL_FOG_COLOR, FOGCOLOR );
		glFogf( GL_FOG_DENSITY, FOGDENSITY );
		glFogf( GL_FOG_START, FOGSTART );
		glFogf( GL_FOG_END, FOGEND );
		glEnable( GL_FOG );
	}
	else
	{
		glDisable( GL_FOG );
	}


	// possibly draw the axes:

	if( AxesOn != 0 )
	{
		glColor3fv( &Colors[WhichColor][0] );
		glCallList( AxesList );
	}


	// since we are using glScalef( ), be sure normals get unitized:

	glEnable( GL_NORMALIZE );


	// draw the current objects:

//	glColor3f(0.3,0.3,0.5);
//	glColor3f(1.,0.,0.);

	glEnable( GL_TEXTURE_2D ); //enable texturing
	glBindTexture( GL_TEXTURE_2D, skytex ); //bind skytexture

	///draw sky
	glPushMatrix();
//		glScalef(2.0, 2.0, 2.0);
//		glRotatef(90.0, 0., 1., 0.);
//		glRotatef(180.0, 1., 0., 0.);
		glRotatef(360.0*Time, 0., 1., 0.);
		glCallList(SphereList);
	glPopMatrix();

	//glDisable (GL_TEXTURE_2D);
	glBindTexture( GL_TEXTURE_2D, maptex ); //bind maptexture

	//draw ground
	glColor3f(0., 0.5, 0.);
	glPushMatrix();
		glTranslatef(-(PLANESIZE/2.), 0., (PLANESIZE/2.));
		glCallList(BoardList);
	glPopMatrix();

	glDisable (GL_TEXTURE_2D); //disable texturing

	//draw path
	if(nrt){//in nrt mode
		glPushMatrix();
			//glLineWidth(5.);
			//glBegin(GL_LINE_STRIP);

			glColor3f(1.,0.,0.);

			//cout << "drawing" <<endl;
			//for(int i=0; i<r_data.size(); i++){
				//cout << "point: " << r_data[i].x << ", " << r_data[i].y << ", " <<r_data[i].z << endl;
			//	glVertex3f(r_data[i].x, r_data[i].y, r_data[i].z);
			//}
			//cout << "done" <<endl;

			glCallList( PathList );

		glPopMatrix();
	}
	else{// in normal mode 
		glPushMatrix();
			if(center){ // if centered requested
				glTranslatef(-apx*XSCALE, 0., -apz*YSCALE);
			}
			glCallList( PathList );
		glPopMatrix();
	}
	if(!nrt){
		if(center){ // if centered requested
			DoRasterString( 0., apy+1.0, 0., apogeebuff );
		}
		else{
			DoRasterString( apx, apy+1.0, apz, apogeebuff );
		}
	}

	if( DepthFightingOn != 0 )
	{
		glPushMatrix( );
			//glRotatef( 90.,   0., 1., 0. );
			glCallList( PathList );
		glPopMatrix( );
	}


	// draw some gratuitous text that just rotates on top of the scene:

	glDisable( GL_DEPTH_TEST );
	glColor3f( 0., 1., 1. );
//	DoRasterString( 0., 1., 0., "Text That Moves" );
	DoRasterString( 1., 0.1, 0., "1 MI" );


	// draw some gratuitous text that is fixed on the screen:
	//
	// the projection matrix is reset to define a scene whose
	// world coordinate system goes from 0-100 in each axis
	//
	// this is called "percent units", and is just a convenience
	//
	// the modelview matrix is reset to identity as we don't
	// want to transform these coordinates

	glDisable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0., 100.,     0., 100. );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
//	glColor3f( 1., 1., 1. );
//	DoRasterString( 5., 5., 0., "Text That Doesn't" );
//	DoRasterString( 1., 0., 0., "1 MI" );
	

	// swap the double-buffered framebuffers:

	glutSwapBuffers( );


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !

	glFlush( );
}


void
DoAxesMenu( int id )
{
	AxesOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoColorMenu( int id )
{
	WhichColor = id - RED;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoDebugMenu( int id )
{
	DebugOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoDepthBufferMenu( int id )
{
	DepthBufferOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoDepthFightingMenu( int id )
{
	DepthFightingOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoDepthMenu( int id )
{
	DepthCueOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// main menu callback:

void
DoMainMenu( int id )
{
	switch( id )
	{
		case RESET:
			Reset( );
			break;

		case QUIT:
			// gracefully close out the graphics:
			// gracefully close the graphics window:
			// gracefully exit the program:
			glutSetWindow( MainWindow );
			glFinish( );
			glutDestroyWindow( MainWindow );
			exit( 0 );
			break;

		default:
			fprintf( stderr, "Don't know what to do with Main Menu ID %d\n", id );
	}

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoProjectMenu( int id )
{
	WhichProjection = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// use glut to display a string of characters using a raster font:

void
DoRasterString( float x, float y, float z, char *s )
{
	glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );

	char c;			// one character to print
	for( ; ( c = *s ) != '\0'; s++ )
	{
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
	}
}


// use glut to display a string of characters using a stroke font:

void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
	glPushMatrix( );
		glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
		float sf = ht / ( 119.05f + 33.33f );
		glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
		char c;			// one character to print
		for( ; ( c = *s ) != '\0'; s++ )
		{
			glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
		}
	glPopMatrix( );
}


// return the number of seconds since the start of the program:

float
ElapsedSeconds( )
{
	// get # of milliseconds since the start of the program:

	int ms = glutGet( GLUT_ELAPSED_TIME );

	// convert it to seconds:

	return (float)ms / 1000.f;
}


// initialize the glui window:

void
InitMenus( )
{
	glutSetWindow( MainWindow );

	int numColors = sizeof( Colors ) / ( 3*sizeof(int) );
	int colormenu = glutCreateMenu( DoColorMenu );
	for( int i = 0; i < numColors; i++ )
	{
		glutAddMenuEntry( ColorNames[i], i );
	}

	int axesmenu = glutCreateMenu( DoAxesMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int depthcuemenu = glutCreateMenu( DoDepthMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int depthbuffermenu = glutCreateMenu( DoDepthBufferMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int depthfightingmenu = glutCreateMenu( DoDepthFightingMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int debugmenu = glutCreateMenu( DoDebugMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int projmenu = glutCreateMenu( DoProjectMenu );
	glutAddMenuEntry( "Orthographic",  ORTHO );
	glutAddMenuEntry( "Perspective",   PERSP );

	int mainmenu = glutCreateMenu( DoMainMenu );
	glutAddSubMenu(   "Axes",          axesmenu);
	glutAddSubMenu(   "Colors",        colormenu);
	glutAddSubMenu(   "Depth Buffer",  depthbuffermenu);
	glutAddSubMenu(   "Depth Fighting",depthfightingmenu);
	glutAddSubMenu(   "Depth Cue",     depthcuemenu);
	glutAddSubMenu(   "Projection",    projmenu );
	glutAddMenuEntry( "Reset",         RESET );
	glutAddSubMenu(   "Debug",         debugmenu);
	glutAddMenuEntry( "Quit",          QUIT );

// attach the pop-up menu to the right mouse button:

	glutAttachMenu( GLUT_RIGHT_BUTTON );
}



// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions

void
InitGraphics( )
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:

	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

	// open the window and set its title:

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:

	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc( NULL );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutMenuStateFunc( NULL );
	glutTimerFunc( -1, NULL, 0 );
	glutIdleFunc( Animate );

	// init glew (a window must be open to do this):

#ifdef WIN32
	GLenum err = glewInit( );
	if( err != GLEW_OK )
	{
		fprintf( stderr, "glewInit Error\n" );
	}
	else
		fprintf( stderr, "GLEW initialized OK\n" );
	fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )

void
InitLists( )
{
	float dx = BOXSIZE / 2.f;
	float dy = BOXSIZE / 2.f;
	float dz = BOXSIZE / 2.f;
	glutSetWindow( MainWindow );

	// create the objects:

//	glColor3f(0.,0.,0.5);
	SphereList = glGenLists(1);
	glNewList(SphereList, GL_COMPILE);
		//glBindTexture( GL_TEXTURE_2D, skytex ); //bind skytexture
		//glutSolidSphere(RADIUS, 100, 100);
		MjbSphere(RADIUS, 100, 100);
	glEndList();

	GroundList = glGenLists(1);
	glNewList(GroundList, GL_COMPILE);
		glBegin(GL_QUADS);
			glColor3f(0.0,0.3,0.0);
			glVertex3f(-15.0,0.0,15.0);
			glVertex3f(15.0,0.0,15.0);
			glVertex3f(15.0,0.0,-15.0);
			glVertex3f(-15.0,0.0,-15.0);
		glEnd();
	glEndList();

	BoxList = glGenLists( 1 );
	glNewList( BoxList, GL_COMPILE );

		glBegin( GL_QUADS );

			glColor3f( 0., 0., 1. );
			glNormal3f( 0., 0.,  1. );
				glVertex3f( -dx, -dy,  dz );
				glVertex3f(  dx, -dy,  dz );
				glVertex3f(  dx,  dy,  dz );
				glVertex3f( -dx,  dy,  dz );

			glNormal3f( 0., 0., -1. );
				glTexCoord2f( 0., 0. );
				glVertex3f( -dx, -dy, -dz );
				glTexCoord2f( 0., 1. );
				glVertex3f( -dx,  dy, -dz );
				glTexCoord2f( 1., 1. );
				glVertex3f(  dx,  dy, -dz );
				glTexCoord2f( 1., 0. );
				glVertex3f(  dx, -dy, -dz );

			glColor3f( 1., 0., 0. );
			glNormal3f(  1., 0., 0. );
				glVertex3f(  dx, -dy,  dz );
				glVertex3f(  dx, -dy, -dz );
				glVertex3f(  dx,  dy, -dz );
				glVertex3f(  dx,  dy,  dz );

			glNormal3f( -1., 0., 0. );
				glVertex3f( -dx, -dy,  dz );
				glVertex3f( -dx,  dy,  dz );
				glVertex3f( -dx,  dy, -dz );
				glVertex3f( -dx, -dy, -dz );

			glColor3f( 0., 1., 0. );
			glNormal3f( 0.,  1., 0. );
				glVertex3f( -dx,  dy,  dz );
				glVertex3f(  dx,  dy,  dz );
				glVertex3f(  dx,  dy, -dz );
				glVertex3f( -dx,  dy, -dz );

			glNormal3f( 0., -1., 0. );
				glVertex3f( -dx, -dy,  dz );
				glVertex3f( -dx, -dy, -dz );
				glVertex3f(  dx, -dy, -dz );
				glVertex3f(  dx, -dy,  dz );

		glEnd( );

	glEndList( );


	// create the axes:

	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glLineWidth( AXES_WIDTH );
			Axes( 1. );
		glLineWidth( 1. );
	glEndList( );
//	if(nrt){
//		pathlists[pl_used] = glGenLists( 1 );
//		glNewList( pathlists[pl_used], GL_COMPILE );
//			glLineWidth( 1 );
//				glVertex3f(0.,0.1,0.);
//				glVertex3f(0.,0.2,0.);
//			glLineWidth( 1. );
//		glEndList( );
//	}
//	else{
		make_trace_list("./log.txt"); //OPEN TRACE HERE
//	}
	tplane(PLANERES, PLANESIZE);
}


// the keyboard callback:

void
Keyboard( unsigned char c, int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

	switch( c )
	{
		case 'a':
		case 'A':
			if(xdist < 75.0){
				xdist += 1.0;
			}
			break;
		case 's':
		case 'S':
			if(zdist > -75.0){
				zdist -= 1.0;
			}
			break;
		case 'w':
		case 'W':
			if(zdist < 75.0){
				zdist += 1.0;
			}
			break;
		case 'd':
		case 'D':
			if(xdist > -75.0){ 
				xdist -= 1.0;
			}
			break;
		case 'o':
		case 'O':
			WhichProjection = ORTHO;
			break;

		case 'p':
		case 'P':
			WhichProjection = PERSP;
			break;

		case 'q':
		case 'Q':
		case ESCAPE:
			DoMainMenu( QUIT );	// will not return here
			break;				// happy compiler

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}

	// force a call to Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// called when the mouse button transitions down or up:

void
MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:

	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}


	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}
}


// called when the mouse moves while a button is down:

void
MouseMotion( int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 )
	{
		if((Xrot<90) && (Xrot>-25)){
			Xrot += ( ANGFACT*dy );
		}
		else{
			if(Xrot >= 90){
				Xrot -= 1.;
			}
			else{
				Xrot += 1.;
			}
		}
		Yrot += ( ANGFACT*dx );
	}


	if( ( ActiveButton & MIDDLE ) != 0 )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset( )
{
	ActiveButton = 0;
	AxesOn = 1;
	DebugOn = 0;
	DepthBufferOn = 1;
	DepthFightingOn = 0;
	DepthCueOn = 0;
	Scale  = 1.0;
	WhichColor = WHITE;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
}


// called when user resizes the window:

void
Resize( int width, int height )
{
	if( DebugOn != 0 )
		fprintf( stderr, "ReSize: %d, %d\n", width, height );

	// don't really need to do anything since window size is
	// checked each time in Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// handle a change to the window's visibility:

void
Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[ ] = {
		0.f, 1.f, 0.f, 1.f
	      };

static float xy[ ] = {
		-.5f, .5f, .5f, -.5f
	      };

static int xorder[ ] = {
		1, 2, -3, 4
		};

static float yx[ ] = {
		0.f, 0.f, -.5f, .5f
	      };

static float yy[ ] = {
		0.f, .6f, 1.f, 1.f
	      };

static int yorder[ ] = {
		1, 2, 3, -2, 4
		};

static float zx[ ] = {
		1.f, 0.f, 1.f, 0.f, .25f, .75f
	      };

static float zy[ ] = {
		.5f, .5f, -.5f, -.5f, 0.f, 0.f
	      };

static int zorder[ ] = {
		1, 2, 3, 4, -5, 6
		};

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)

void
Axes( float length )
{
	glBegin( GL_LINE_STRIP );
		glVertex3f( length, 0., 0. );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., length, 0. );
	glEnd( );
	glBegin( GL_LINE_STRIP );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., 0., length );
	glEnd( );

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 4; i++ )
		{
			int j = xorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 5; i++ )
		{
			int j = yorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 6; i++ )
		{
			int j = zorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
		}
	glEnd( );

}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );

void
HsvRgb( float hsv[3], float rgb[3] )
{
	// guarantee valid input:

	float h = hsv[0] / 60.f;
	while( h >= 6. )	h -= 6.;
	while( h <  0. ) 	h += 6.;

	float s = hsv[1];
	if( s < 0. )
		s = 0.;
	if( s > 1. )
		s = 1.;

	float v = hsv[2];
	if( v < 0. )
		v = 0.;
	if( v > 1. )
		v = 1.;

	// if sat==0, then is a gray:

	if( s == 0.0 )
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}

	// get an rgb from the hue itself:
	
	float i = floor( h );
	float f = h - i;
	float p = v * ( 1.f - s );
	float q = v * ( 1.f - s*f );
	float t = v * ( 1.f - ( s * (1.f-f) ) );

	float r, g, b;			// red, green, blue
	switch( (int) i )
	{
		case 0:
			r = v;	g = t;	b = p;
			break;
	
		case 1:
			r = q;	g = v;	b = p;
			break;
	
		case 2:
			r = p;	g = v;	b = t;
			break;
	
		case 3:
			r = p;	g = q;	b = v;
			break;
	
		case 4:
			r = t;	g = p;	b = v;
			break;
	
		case 5:
			r = v;	g = p;	b = q;
			break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}
void InitTextures(){
	// skutex is the texture object, skytex1 is the picture	
	//begin first sky texture
	cout << "creating textures..." << endl;

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glGenTextures( 1, &skytex ); // assign binding “handles”
	glBindTexture( GL_TEXTURE_2D, skytex ); // make skytex texture current
	
	// and set its parameters
	skytex1 = BmpToTexture( "./resources/textures/skytex3.bmp", &width, &height );
	//skytex1 = BmpToTexture( "./resources/textures/map.bmp", &width, &height );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ); //repeat if beyond 1 for s or t 
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //blended texels
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE ); //replace surface (no material illumination) 
	glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytex1 );
	//end sky texture

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glGenTextures( 1, &maptex ); // assign binding “handles”
	glBindTexture( GL_TEXTURE_2D, maptex ); // make maptex texture current
	
	// and set its parameters
	maptex1 = BmpToTexture( usedmap, &width, &height );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ); 
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ); //repeat if beyond 1 for s or t
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //blended texels
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //set linear mip map filter
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE ); //replace surface (no material illumination) 
//	glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, maptex1 ); //make texture
	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, maptex1 ); //make mipmaps 
	//end map texture
}


struct point *
PtsPointer( int lat, int lng )
{
	if( lat < 0 )	lat += (NumLats-1);
	if( lng < 0 )	lng += (NumLngs-1);
	if( lat > NumLats-1 )	lat -= (NumLats-1);
	if( lng > NumLngs-1 )	lng -= (NumLngs-1);
	return &Pts[ NumLngs*lat + lng ];
}



void
DrawPoint( struct point *p )
{
	glNormal3f( p->nx, p->ny, p->nz );
	glTexCoord2f( p->s, p->t );
	glVertex3f( p->x, p->y, p->z );
}

void
MjbSphere( float radius, int slices, int stacks )
{
	struct point top, bot;		// top, bottom points
	struct point *p;

	// set the globals:

	NumLngs = slices;
	NumLats = stacks;

	if( NumLngs < 3 )
		NumLngs = 3;

	if( NumLats < 3 )
		NumLats = 3;


	// allocate the point data structure:

	Pts = new struct point[ NumLngs * NumLats ];


	// fill the Pts structure:

	for( int ilat = 0; ilat < NumLats; ilat++ )
	{
		float lat = -M_PI/2.  +  M_PI * (float)ilat / (float)(NumLats-1);
		float xz = cos( lat );
		float y = sin( lat );
		for( int ilng = 0; ilng < NumLngs; ilng++ )
		{
			float lng = -M_PI  +  2. * M_PI * (float)ilng / (float)(NumLngs-1);
			float x =  xz * cos( lng );
			float z = -xz * sin( lng );
			p = PtsPointer( ilat, ilng );
			p->x  = radius * x;
			p->y  = radius * y;
			p->z  = radius * z;
			p->nx = x;
			p->ny = y;
			p->nz = z;
			if( Distort )
			{
				//p->s = ?????
				p->s = ( lng + M_PI    ) / ( 2.*M_PI );
				//p->t = ?????
				p->t = ( lat + M_PI/2. ) / M_PI;
			}
			else
			{
				p->s = ( lng + M_PI    ) / ( 2.*M_PI );
				p->t = ( lat + M_PI/2. ) / M_PI;
			}
		}
	}

	top.x =  0.;		top.y  = radius;	top.z = 0.;
	top.nx = 0.;		top.ny = 1.;		top.nz = 0.;
	top.s  = 0.;		top.t  = 1.;

	bot.x =  0.;		bot.y  = -radius;	bot.z = 0.;
	bot.nx = 0.;		bot.ny = -1.;		bot.nz = 0.;
	bot.s  = 0.;		bot.t  =  0.;


	// connect the north pole to the latitude NumLats-2:

	glBegin( GL_QUADS );
	for( int ilng = 0; ilng < NumLngs-1; ilng++ )
	{
		p = PtsPointer( NumLats-1, ilng );
		DrawPoint( p );

		p = PtsPointer( NumLats-2, ilng );
		DrawPoint( p );

		p = PtsPointer( NumLats-2, ilng+1 );
		DrawPoint( p );

		p = PtsPointer( NumLats-1, ilng+1 );
		DrawPoint( p );
	}
	glEnd( );

	// connect the south pole to the latitude 1:

	glBegin( GL_QUADS );
	for( int ilng = 0; ilng < NumLngs-1; ilng++ )
	{
		p = PtsPointer( 0, ilng );
		DrawPoint( p );

		p = PtsPointer( 0, ilng+1 );
		DrawPoint( p );

		p = PtsPointer( 1, ilng+1 );
		DrawPoint( p );

		p = PtsPointer( 1, ilng );
		DrawPoint( p );
	}
	glEnd( );


	// connect the other 4-sided polygons:

	glBegin( GL_QUADS );
	for( int ilat = 2; ilat < NumLats-1; ilat++ )
	{
		for( int ilng = 0; ilng < NumLngs-1; ilng++ )
		{
			p = PtsPointer( ilat-1, ilng );
			DrawPoint( p );

			p = PtsPointer( ilat-1, ilng+1 );
			DrawPoint( p );

			p = PtsPointer( ilat, ilng+1 );
			DrawPoint( p );

			p = PtsPointer( ilat, ilng );
			DrawPoint( p );
		}
	}
	glEnd( );

	delete [ ] Pts;
	Pts = NULL;
}

/***********************************************
* Title: detect_internet_connection
* Description: Detects if there is an internet 
* connection present.
***********************************************/
bool	detect_internet_connection(){
	int devnul;
	pid_t   childid;
	int child_exit = -5;
	bool con;
	int ret;

	devnul = open("/dev/null", O_RDWR);

	childid = fork();

        switch(childid){

        case -1: //error
                printf("ERROR SPAWNING CHILD\n");
                fflush(stdout);
                exit(2);

        case 0: //child
                //exec into wget
		dup2(devnul, 0); //duplicate /dev/null to stdin
		dup2(devnul, 1); //duplicate /dev/null to stdout
		dup2(devnul, 2); //duplicate /dev/null to stderr
                ret = execlp("wget", "wget", "www.google.com", NULL);
                if(ret == -1){
			printf("ERROR CHECKING CONNECTION\n");
			fflush(stdout);
			exit(1);
                }

        default: //parent
                //wait on wget
                waitpid(childid, &child_exit, 0);
		if(child_exit == 0){
			con = true;
		}
		else{
			con = false;
		}
        }
	if(con == true){ //clean up index.html
		childid = fork();
		        switch(childid){
        
	        case -1: //error
	                printf("ERROR SPAWNING CHILD\n");
	                fflush(stdout);
	                exit(2);
	        
	        case 0: //child
	                //exec into rm
	                ret = execlp("rm", "rm", "./index.html", "-f", NULL);
	                if(ret == -1){
	                        printf("ERROR REMOVING INDEX.HTML\n");
	                        fflush(stdout);
	                        exit(1);
	                }
		        
        	default: //parent
        	        //wait on rm
        	        waitpid(childid, &child_exit, 0);
        	}
	}
	return con;
}

void* nrt_listen(void* input){
//	pthread_mutex_lock(&init_lock);
	char message[256];
	char buff[64];
	bool over = false;
	int bytes;
	char* end = NULL;
	//get lock on inited int
//	cout << "start of thread" << endl;
	while(!over){
//		cout << "start of thread main loop" << endl;
		//read from pipe
		memset(message, '\0', 256);
		while(strstr(buff, "&&") == NULL){
			memset(buff, '\0', 256);
			bytes = read(3, buff, 64-1);
			strcat(message, buff);
                        if(bytes == -1){ //error cases
//                                printf("READ ERROR IN LOGGER, RETURN OF -1\n");
//                                fflush(stdout);
//                                exit(4);
                        }
                        if(bytes == 0){
//                                printf("NO READ IN LOGGER, RETURN OF 0\n");
//                                fflush(stdout);
//                                exit(5);
                        }

		}
		//fix the string
                end = strstr(message, "&&"); //points to first &
                *end = '\0'; //null terminate
//		cout << "trace recieved: " << message << endl;
		fflush(stdout);
/**************************************WORK NEEDED

		//convert data?



		//update list not used
		if(pl_used == 0){
			//update pathlists[1]
		}
		else{
			//update pathlists[0]
		}
		if(!inited){
			//set slat and slon
			inited = true;
			//unlock inited int
			pthread_mutex_unlock(&init_lock);
		}

***********************************/

		//get lock on pl_used int
//		pthread_mutex_lock(&list_lock);
		//switch it
//		if(pl_used == 0){
//			pl_used = 1;
//		}
//		else{
//			pl_used = 0;
//		}
		//unlock pl_used int
//		pthread_mutex_unlock(&list_lock);
//		cout << "thread unlocked and starting over" << endl;
	}
}

void update_data(){
/*
	strcpy(messagecpy, message);
	token = strtok(messagecpy, ":");
	token = strtok(NULL, ":");
	token = strtok(NULL, "\"");
	temprd.z = atof(token);
	token = strtok(NULL, ":");
	token = strtok(NULL, "\"");
	temprd.x = atof(token);
	token = strtok(NULL, ":");
	token = strtok(NULL, "\"");
*/
	cout << "in update_data()" << endl;
	memset(messagecpy, '\0', 256);
	struct telem_data tmp;
	strcpy(messagecpy, message);
	cout << "messagecpy: " << messagecpy << endl;
	cout << "structuring" << endl;
	tmp = structure(&messagecpy);
	cout << "structuing done" << endl;
	//temprd.y = atof(token);

	temprd.y = tmp.alt;
	temprd.x = tmp.lon;
	temprd.z = tmp.lat;
	cout << "X: " << temprd.x << endl;
	cout << "y: " << temprd.y << endl;
	cout << "z: " << temprd.z << endl;

	temprd.x = (-(slon - temprd.x) * (cos(slat*(M_PI/180.0))*69.172));
	temprd.y = temprd.y / 5280.0;
	temprd.z = ((slat - temprd.z)*69.0);
	temprd.x *= static_cast<float>(XSCALE);
	temprd.y *= static_cast<float>(YSCALE);
	temprd.z *= static_cast<float>(ZSCALE);
	r_data.push_back(temprd);
	PathList = glGenLists(1);
	glNewList(PathList, GL_COMPILE);
		glLineWidth(5.);
		glBegin(GL_LINE_STRIP);
		for(int i=0; i<r_data.size(); i++){
			glVertex3f(r_data[i].x, r_data[i].y, r_data[i].z);
		}
		glEnd();
	glEndList();
}
