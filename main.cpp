/* Author: Johannes Schmid, 2006, johnny@grob.org */
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL.h>
#include <SDL_thread.h>

//#include "SDLfluid.h"
#include "fluid.h"
#include "viewer.h"
#include "genfunc.h"

SDL_Surface* screen = NULL;
SDL_Thread* simthread = NULL;

Fluid* fluid = NULL;
Viewer* viewer = NULL;

char infostring[256];
int frames = 0, simframes = 0;
float fps = 0.0f, sfps = 0.0f;

float t = 0;
float* gfparams;

enum Mode { SIMULATE, PLAY };
Mode mode;

bool paused = true, quitting = false, redraw = false, update = false, wasupdate = false;;

int EventLoop(FILE* fp);

void error(const char* str)
{
#ifdef WIN32
	MessageBox(NULL, str, "Error", MB_OK | MB_ICONSTOP);
#else
	printf("Error: %s/n", str);
#endif
}

bool init(void)
{
	char str[256];

	/* SDL */
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
        sprintf(str, "Unable to init SDL: %s\n", SDL_GetError());
		error(str);
        return false;
    }
    atexit(SDL_Quit);

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    screen = SDL_SetVideoMode(500, 500, 32, SDL_OPENGL | SDL_RESIZABLE);
	if ( screen == NULL ) {
        sprintf(str, "Unable to set video: %s\n", SDL_GetError());
		error(str);
        return false;
	}

	/* Fluid */

	fluid = new Fluid();
	fluid->diffusion = 0.00001f;
	fluid->viscosity = 0.000000f;
	fluid->buoyancy  = 4.0f;
	fluid->vc_eps    = 5.0f;

	/* Viewer */

	viewer = new Viewer();
	viewer->viewport(screen->w, screen->h);

	gfparams = randfloats(256);

	return true;
}

int simulate(void* bla)
{
	assert(fluid != NULL);

	float dt = 0.1;
	float f;

	while (!quitting)
	{
		if (!paused && !update) {
			/*const int xpos = 27;
			const int ypos = 20;
			for (int i=0; i<4; i++)
			{
				for (int j=0; j<4; j++)
				{
					fluid->d[_I(xpos,i+ypos,j+14)] = 1.0f;
					fluid->u[_I(xpos,i+ypos,j+14)] = -2.0f;
				}
			}*/

			/*const int ypos = 60;
			for (int i=0; i<8; i++)
			{
				float d = (rand()%1000)/1000.0f;
				for (int j=0; j<8; j++)
				{
					f = genfunc(i,j,8,8,t,gfparams);
					fluid->d[_I(i+28,ypos,j+28)] = d;
					fluid->v[_I(i+28,ypos,j+28)] = -(1.0f + 2.0f*f);
				}
			}*/

			const int xpos = 60;
			const int ypos = 50;
			for (int i=0; i<8; i++)
			{
				float d = (rand()%1000)/1000.0f;
				for (int j=0; j<8; j++)
				{
					f = genfunc(i,j,8,8,t,gfparams);
					fluid->d[_I(xpos,i+ypos,j+28)] = d;
					//fluid->u[_I(xpos,i+ypos,j+28)] = -(1.0f + 2.0f*f);
					fluid->u[_I(xpos,i+ypos,j+28)] = -3.0f;
				}
			}

			fluid->step(dt);
			update = true;
			t += dt;
			simframes++;
		}
	}

	return 0;
}

SDL_UserEvent nextframe = { SDL_USEREVENT, 0, NULL, NULL};
Uint32 timer_proc(Uint32 interval, void* bla)
{
	/*if (!paused)
		SDL_PushEvent((SDL_Event*) &nextframe);*/
	wasupdate = update;
	update = true;
	return interval;
}

Uint32 showfps(Uint32 interval, void* bla)
{
	static int lastf = 0, lastsf = 0;

	sprintf(infostring, "FPS: %d, sFPS: %d, simframes: %d",
		frames - lastf,
		simframes - lastsf,
		simframes);

	lastf = frames;
	lastsf = simframes;

	return interval;
}

int EventLoop(FILE* fp)
{
    SDL_Event event;
	unsigned int ts;

	paused = false;

    while (1) {
		//ts = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYUP:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							quitting = true;
							return 0;
						case SDLK_c:
							viewer->_draw_cube = !viewer->_draw_cube;
							break;
						case SDLK_i:
							if (viewer->_dispstring != NULL)
								viewer->_dispstring = NULL;
							else
								viewer->_dispstring = infostring;
							break;
						case SDLK_s:
							viewer->_draw_slice_outline = !viewer->_draw_slice_outline;
							break;
						case SDLK_SPACE:
							paused = !paused;
							break;
					}
					break;

				case SDL_QUIT:
					quitting = true;
					return 0;

				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 32, SDL_OPENGL | SDL_RESIZABLE);
					viewer->viewport(screen->w, screen->h);
					redraw = true;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button)
					{
						case SDL_BUTTON_WHEELUP:
							viewer->anchor(0,0);
							viewer->dolly(-screen->h/10);
							redraw = true;
							break;
						case SDL_BUTTON_WHEELDOWN:
							viewer->anchor(0,0);
							viewer->dolly(screen->h/10);
							redraw = true;
							break;
						default:
							viewer->anchor(event.button.x, event.button.y);
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					switch (event.motion.state) {
						case SDL_BUTTON(SDL_BUTTON_LEFT):
							viewer->rotate(event.motion.x, event.motion.y);
							redraw = true;
							break;
						case SDL_BUTTON(SDL_BUTTON_RIGHT):
							viewer->rotate_light(event.motion.x, event.motion.y);
							redraw = true;
							break;
					}
					break;
				/*case SDL_USEREVENT:
					viewer->load_frame();
					redraw = true;
					break;*/
			}
		}

		if (update)
		{
			if (mode == SIMULATE) {
				viewer->frame_from_sim(fluid);
				if (fp)
					fluid->store(fp);
			} else
				viewer->load_frame();
			redraw = true;
			update = wasupdate;
			wasupdate = false;
		}

		if (redraw)
		{
			viewer->draw();
			SDL_GL_SwapBuffers();
			redraw = false;
			frames++;
		}
    }

	return 1;
}

int main(int argc, char* argv[])
{
	char str[256];
	FILE *fp = NULL;
	int h[3];
	SDL_TimerID playtimer, fpstimer;


	if (!init())
		return 1;

	if (argc>1) {
		if (!strcmp(argv[1],"-l") && (argc==3)) {
			mode = PLAY;
			if (!viewer->open(argv[2]))	{
				error("Unable to load data file\n");
				exit(1);
			}
		} else if (!strcmp(argv[1], "-w") && (argc==3)) {
			mode = SIMULATE;
			fp = fopen(argv[2], "wb");
			fwrite(h, sizeof(int), 3, fp);
		} else {
			printf("Invalid command line argument. Usage: fluid [-l|-w <filename>]\n");
			exit(1);
		}
	} else {
		mode = SIMULATE;
	}

	if (mode == SIMULATE)
		simthread = SDL_CreateThread(simulate, NULL);

	if (mode == PLAY)
		playtimer = SDL_AddTimer(1000/16, timer_proc, NULL);

	fpstimer = SDL_AddTimer(1000, showfps, NULL);
	EventLoop(fp);
	SDL_RemoveTimer(fpstimer);
	if (mode==PLAY)
		SDL_RemoveTimer(playtimer);

	if (mode == SIMULATE) {
		quitting = true;
		SDL_WaitThread(simthread, NULL);
	}

	if (fp && (mode == SIMULATE))	{
		int pos;
		h[0] = h[1] = N+2;
		h[2] = simframes;
		pos = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fwrite(h, sizeof(int), 3, fp);
		fclose(fp);
		printf("%d frames written to file %s, %d kiB\n", simframes, argv[2], pos>>10);
	}
}
