#include <ncurses.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cround/cround.hpp"

#define PONG_OFFSET_X 1
#define PONG_OFFSET_Y 1
#define PONG_REGULAR 0
#define PONG_DEBUG 1
#define PONG_RATIO (float)(4.0f / 3.0f)
#define PONG_COSYS_XMAX (10.0f * (4.0f / 3.0f))
#define PONG_COSYS_YMAX 10.0f

typedef enum input
{
    INVALID,
    UP_P1,
    UP_P2,
    DOWN_P1,
    DOWN_P2,
    PAUSE,
    RESTART,
    QUIT,
    SELECT
} input;

typedef struct frame
{
    int height, width;
    char** pxlbuf;
} frame;

typedef struct point
{
    float x;
    float y;
} point;

typedef struct vector
{
    point origin;
    point destination;
} vector;

typedef struct gamestate
{
    bool alive;
    int condition;
    int p1pos;
    int p2pos;
    vector ballvec;
    point ballpos;
    input inbuf;
    size_t framecount;
    frame framebuffer;
} gamestate;

typedef enum error
{
    NOERR,
    UKNOWN,
    INVALID_IN,
} error;

gamestate init_game(size_t height, int condition);
frame create_frame(size_t height, size_t width);
error free_frame(frame);
error render(gamestate);
error run(gamestate*);
input get_input();
void cleanup();

point create_point(float x, float y)
{
    point _return;
    _return.x = x;
    _return.y = y;

    return _return;
}

vector create_vector(point dest, point orig)
{
    vector _return;
    _return.destination;
    _return.origin;

    return _return;
}

gamestate init_game(size_t height, int condition)
{

    {
        initscr();
        timeout(0);
        curs_set(0);
        noecho();
        start_color();

        init_pair(1, COLOR_BLUE, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
    }

    size_t width = cround((float)height * PONG_RATIO);

    gamestate _return;

    mvaddch(0, 0, 'x');
    for(int i = 0; i < width * 2 + PONG_OFFSET_X; i++)
        addch('-');
    addch('x');

    if(condition == PONG_DEBUG)
    {
        for(int i = 1; i < width * 2; i++)
        {
            attron(COLOR_PAIR(2));
            mvaddch(cround((float)height / 2.0f) + PONG_OFFSET_Y, 0, ' ');
            for(int i = 0; i < width * 2 + PONG_OFFSET_X; i++)
                addch('-');
            attroff(COLOR_PAIR(2));
        }
    }

    mvaddch(height + PONG_OFFSET_Y, 0, 'x');
    for(int i = 0; i < width * 2 + PONG_OFFSET_X; i++)
        addch('-');
    addch('x');

    for(int i = 1; i < height + PONG_OFFSET_Y; i++)
    {
        mvaddch(i, 0, '|');

        if(condition == PONG_DEBUG)
        {
            attron(COLOR_PAIR(1));
            mvaddch(i, cround((PONG_COSYS_XMAX / 2.0f) / PONG_COSYS_XMAX * width * 2 + PONG_OFFSET_X), '|');
            attroff(COLOR_PAIR(1));
        }

        mvaddch(i, width * 2 + PONG_OFFSET_Y + 1, '|');
    }

    const char* name = " Pongy ";
    mvprintw(0, (width - (cround((float)strlen(name) / 2.0f) - PONG_OFFSET_X - 1)), name);

    mvprintw(0, cround((float)width / 2.0f) - 1, " %d ", 0);
    mvprintw(0, width + cround((float)width / 2.0f) + 1, " %d ", 0);

    if(condition == PONG_DEBUG)
    {
        mvprintw(0, 0, "h:%d w:%d t: %d e: %f r: %f, my: %d", height, width, height * width, (float)((float)height * (float)PONG_RATIO), PONG_RATIO, cround((float)height / 2.0f) + PONG_OFFSET_Y);
    }

    frame f = create_frame(height, width);

    _return.p1pos = 0;
    _return.p2pos = 6;
    _return.framecount = 0;
    _return.framebuffer = f;
    _return.ballpos = create_point(PONG_COSYS_XMAX / 2.0f, PONG_COSYS_YMAX / 2.0f);

    bool random = rand() % 2;

    _return.ballvec = create_vector(create_point(rand() % cround(PONG_COSYS_XMAX), rand() % cround(PONG_COSYS_YMAX)), _return.ballpos);

    _return.condition = condition;

    _return.alive = true;

    return _return;
}

frame create_frame(size_t height, size_t width)
{
    frame _return;
    _return.width = width;
    _return.height = height;

    _return.pxlbuf = (char**)malloc(height * sizeof(char*));
    for(int i = 0; i < height; i++)
        _return.pxlbuf[i] = (char*)malloc(width * sizeof(char));

    for(int i = 0; i < _return.height; i++)
        for(int j = 0; j < _return.width; j++)
            _return.pxlbuf[i][j] =  ' ';

    return _return;
}

error free_frame(frame f)
{
    for(int i = 0; i < f.height; i++)
        free(f.pxlbuf[i]);

    return NOERR;
}

input get_input()
{
    char c = getch();

    switch(c)
    {
        case 'q': return QUIT;
        case 'p': return PAUSE;
        case ' ': return SELECT;
        case 'r': return RESTART;
        case 'i': return UP_P2;
        case 'k': return DOWN_P2;
        case 'w': return UP_P1;
        case 's': return DOWN_P1;
    }

    return INVALID;
}

error render(gamestate gs)
{
    int ballx, bally;

    // Ball computation
    {

        bally = cround((float)gs.ballpos.y / PONG_COSYS_YMAX * gs.framebuffer.height); 
        ballx = cround((float)gs.ballpos.x / PONG_COSYS_XMAX * gs.framebuffer.width * 2 + PONG_OFFSET_X);

        mvaddch(bally + PONG_OFFSET_Y, ballx, '@');
    }
    
    // p1 slider
    {
        for(int i = 0; i < gs.framebuffer.height; i++)
            mvaddch(i + PONG_OFFSET_Y, 0 + PONG_OFFSET_X, ' ');
        for(int i = 0; i < 2; i++)
            mvaddch(gs.p1pos + PONG_OFFSET_Y + i, 0 + PONG_OFFSET_X, '|');
    }

    // p2 slider
    {

        for(int i = 0; i < gs.framebuffer.height; i++)
            mvaddch(i + PONG_OFFSET_Y, gs.framebuffer.width * 2 + PONG_OFFSET_X, ' ');
        for(int i = 0; i < 2; i++)
            mvaddch(gs.p2pos + PONG_OFFSET_Y + i, gs.framebuffer.width * 2 + PONG_OFFSET_X, '|');
    }

    // debug computation
    if(gs.condition == PONG_DEBUG)
    {

        int desty;
        int destx;

        // vector computation
        {

            desty = cround((float)gs.ballvec.destination.y / PONG_COSYS_YMAX * gs.framebuffer.height); 
            destx = cround((float)gs.ballvec.destination.x / PONG_COSYS_XMAX * gs.framebuffer.width * 2 + PONG_OFFSET_X);

            mvaddch(desty + PONG_OFFSET_Y, destx, 'O');
        }

        mvprintw(gs.framebuffer.height + 2, 0, "@: ");

        mvprintw
        (
            gs.framebuffer.height + 2, 3,
            "y: (%d / %f) * %d = %d",
            gs.ballpos.y, PONG_COSYS_YMAX, gs.framebuffer.height, bally
        );

        mvprintw
        (
            gs.framebuffer.height + 3, 3,
            "x: (%d / %f) * %d = %d",
            gs.ballpos.x, PONG_COSYS_XMAX, gs.framebuffer.width, ballx
        );

        mvprintw(gs.framebuffer.height + 4, 0, "O: ");

        mvprintw
        (
            gs.framebuffer.height + 4, 3,
            "y: (%d / %f) * %d = %d",
            gs.ballvec.destination.y, PONG_COSYS_YMAX, gs.framebuffer.height, desty
        );


        mvprintw
        (
            gs.framebuffer.height + 5, 3,
            "x: (%d / %f) * %d = %d",
            gs.ballvec.destination.x, PONG_COSYS_XMAX, gs.framebuffer.width, destx
        );

        mvprintw
        (
            gs.framebuffer.height + 6, 0,
            "framecount: %d",
            gs.framecount
        );
    }

    refresh();

    return NOERR;
}

error run(gamestate* gsptr)
{
    switch(get_input())
    {
        case INVALID:
        break;

        case QUIT:
        {
            gsptr->alive = false;
        }
        break;

        case UP_P1:
        {
            if(gsptr->p1pos <= 0)
            {
                gsptr->p1pos = 0;
            }
            else
            {
                gsptr->p1pos -= 1;
            }
        }
        break;

        case DOWN_P1:
        {
            if(gsptr->p1pos >= 6)
            {
                gsptr->p1pos = 6;
            }
            else
            {
                gsptr->p1pos += 1;
            }
        }
        break;

        case UP_P2:
        {
            if(gsptr->p2pos <= 0)
            {
                gsptr->p2pos = 0;
            }
            else
            {
                gsptr->p2pos -= 1;
            }
        }
        break;

        case DOWN_P2:
        {
            if(gsptr->p2pos >= 6)
            {
                gsptr->p2pos = 6;
            }
            else
            {
                gsptr->p2pos += 1;
            }
        }
        break;
    }

    return NOERR;
}

void cleanup()
{
    endwin();
}

int main(int argc, char** argv)
{
    int condition = PONG_REGULAR;

    if(argc > 1)
    {
        char* s = argv[1];
        int eval = strcmp(s, "debug");
        if(eval == 0)
        {
            condition = PONG_DEBUG;            
        }
    }

    gamestate gs = init_game(8, condition);

    while(gs.alive)
    {
        render(gs);
        gs.framecount++;
        run(&gs);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    free_frame(gs.framebuffer);

    cleanup();
}