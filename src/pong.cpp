#include <ncurses.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cround/cround.hpp"

#define PONG_OFFSET 1
#define PONG_REGULAR 0
#define PONG_DEBUG 1
#define PONG_RATIO (float)(21.0f / 11.0f)
#define PONG_COSYS_XMAX 20
#define PONG_COSYS_YMAX 10
#define PONG_COSYS_WIDTH 21
#define PONG_COSYS_HEIGHT 11
#define DEBUG_CPAIRRED 1
#define DEBUG_CPAIRBLUE 2
#define PONG_SLIDEROFFSET 2

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
    char pxlbuf[PONG_COSYS_HEIGHT][PONG_COSYS_WIDTH];
} frame;

typedef struct point
{
    int x;
    int y;
} point;

typedef struct vector
{
    point destination;
    point origin;
} vector;

typedef struct gamestate
{
    bool alive;
    int condition;
    int p1pos, p1points;
    int p2pos, p2points;
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

gamestate init_game(int condition);
error render(gamestate);
error run(gamestate*);
input get_input();
void cleanup();
error push_frame(frame);
point create_point(int, int);
vector create_vector(point, point);
error prepare_frame(frame*);

gamestate init_game(int condition)
{
    {
        initscr();
        timeout(0);
        curs_set(0);
        noecho();
        start_color();
        init_pair(DEBUG_CPAIRBLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(DEBUG_CPAIRRED, COLOR_RED, COLOR_BLACK);
    }

    gamestate _return;

    // Prints uppper border
    {
        mvaddch(0, 0, 'x');
        for(int i = 0; i < PONG_COSYS_WIDTH * 2 + PONG_OFFSET; i++)
            addch('-');
        addch('x');
    }

    // prints lower border
    {
        mvaddch(PONG_COSYS_HEIGHT + PONG_OFFSET, 0, 'x');
        for(int i = 0; i < PONG_COSYS_WIDTH * 2 + PONG_OFFSET; i++)
            addch('-');
        addch('x'); 
    }

    // prints side borders
    for(int i = 0; i < PONG_COSYS_HEIGHT; i++)
    {
        mvaddch(i + PONG_OFFSET, 0, '|');
        mvaddch(i + PONG_OFFSET, PONG_COSYS_WIDTH * 2 + PONG_OFFSET * 2, '|');
    }

    // Prints headline + intial points position
    {
        const char* name = " Pongy ";
        mvprintw(0, (PONG_COSYS_WIDTH - (cround((float)strlen(name) / 2.0f) - PONG_OFFSET - 1)), name);

        mvprintw(0, cround((float)PONG_COSYS_HEIGHT / 2.0f) - 1, " %d ", 0);
        mvprintw(0, PONG_COSYS_HEIGHT + cround((float)PONG_COSYS_HEIGHT / 2.0f) + 1, " %d ", 0);
    }

    // prints various debug info
    if(condition == PONG_DEBUG)
    {
        mvprintw
        (
            0, 0,
            "h:%d w:%d t: %d e: %f r: %f, my: %d",
            PONG_COSYS_HEIGHT, PONG_COSYS_WIDTH, PONG_COSYS_HEIGHT * PONG_COSYS_WIDTH, (float)((float)PONG_COSYS_HEIGHT * (float)PONG_RATIO), PONG_RATIO, cround((float)PONG_COSYS_WIDTH / 2.0f) + PONG_OFFSET
        );
    }

    // copies calculated data into our return obj
    _return.p1pos = 1;
    _return.p2pos = PONG_COSYS_YMAX - PONG_SLIDEROFFSET - 1;
    _return.framecount = 0;
    _return.ballpos = create_point(cround((float)PONG_COSYS_XMAX / 2.0f), cround((float)PONG_COSYS_YMAX / 2.0f));
    _return.condition = condition;
    prepare_frame(&_return.framebuffer);

    // seeds rand function
    srand(time(NULL));

    // creates random between 0, 1
    bool random = rand() % 2;

    // calculates random ball vector
    _return.ballvec = create_vector
    (
        create_point(rand() % PONG_COSYS_WIDTH, rand() % PONG_COSYS_HEIGHT),
        _return.ballpos
    );

    // defines gamestate as alive
    _return.alive = true;

    // returns
    return _return;
}

error prepare_frame(frame* fptr)
{
    for(int y = 0; y < PONG_COSYS_HEIGHT; y++)
    {
        for(int x = 0; x < PONG_COSYS_WIDTH; x++)
        {
            fptr->pxlbuf[y][x] = ' ';
        }
    }

    return NOERR;
}

point create_point(int x, int y)
{
    point _return;
    _return.x = x;
    _return.y = y;

    return _return;
}

vector create_vector(point dest, point orig)
{
    vector _return;
    _return.destination = dest;
    _return.origin = orig;

    return _return;
}

error push_frame(frame f)
{
    for(int y = 0; y < PONG_COSYS_HEIGHT; y++)
    {
        for(int x = 0; x < PONG_COSYS_WIDTH; x++)
        {
            mvaddch(y + PONG_OFFSET, (x * 2) + PONG_OFFSET * 2, f.pxlbuf[y][x]);
        }
    }

    refresh();

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
    // Ball computation
    {
        gs.framebuffer.pxlbuf[gs.ballpos.y][gs.ballpos.x] = '@';
    }

    // p1 slider
    {
        for(int i = 0; i < PONG_COSYS_HEIGHT; i++)
            gs.framebuffer.pxlbuf[i][0] = ' ';
        for(int i = 0; i <= PONG_SLIDEROFFSET; i++)
            gs.framebuffer.pxlbuf[gs.p1pos + i][0] = '|';
    }

    // p2 slider
    {

        for(int i = 0; i < PONG_COSYS_HEIGHT; i++)
            gs.framebuffer.pxlbuf[i][PONG_COSYS_XMAX] = ' ';
        for(int i = 0; i <= PONG_SLIDEROFFSET; i++)
            gs.framebuffer.pxlbuf[gs.p2pos + i][PONG_COSYS_XMAX] = '|';
    }

    if(gs.condition = PONG_DEBUG)
    {
        mvprintw
        (
            PONG_COSYS_YMAX + PONG_OFFSET * 2, 0,
            "framecount: %d",
            gs.framecount
        );
    }

    push_frame(gs.framebuffer);

    return NOERR;
}

error run(gamestate* gsptr)
{
    switch(get_input())
    {
        case INVALID:
        break;

        // q pressed -> exits game
        case QUIT:
        {
            gsptr->alive = false;
        }
        break;

        // w pressed -> moves slider p1 up
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

        // s pressed -> moves slider p1 up
        case DOWN_P1:
        {
            if(gsptr->p1pos >= PONG_COSYS_YMAX - PONG_SLIDEROFFSET)
            {
                gsptr->p1pos = PONG_COSYS_YMAX - PONG_SLIDEROFFSET;
            }
            else
            {
                gsptr->p1pos += 1;
            }
        }
        break;

        // i pressed -> moves slider p2 up
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

        // k pressed -> moves slider p2 down
        case DOWN_P2:
        {
            if(gsptr->p2pos >= PONG_COSYS_YMAX - PONG_SLIDEROFFSET)
            {
                gsptr->p2pos = PONG_COSYS_YMAX - PONG_SLIDEROFFSET;
            }
            else
            {
                gsptr->p2pos += 1;
            }
        }
        break;
    }

    // evaluate if ballpos has left the coordinate system on x
    if(gsptr->ballpos.x > PONG_COSYS_XMAX)
    {
        gsptr->p1points++;

        // creates new ball vector & position
        gsptr->ballpos = create_point(cround((float)PONG_COSYS_XMAX / 2.0f), cround((float)PONG_COSYS_YMAX / 2.0f));
        gsptr->ballvec = create_vector
        (
            create_point(rand() % PONG_COSYS_XMAX, rand() % PONG_COSYS_YMAX),
            gsptr->ballpos
        );
    }

    else if(gsptr->ballpos.x < 0)
    {
        gsptr->p2points++;

        // creates new ball vector & position
        gsptr->ballpos = create_point(cround((float)PONG_COSYS_XMAX / 2.0f), cround((float)PONG_COSYS_YMAX / 2.0f));
        gsptr->ballvec = create_vector
        (
            create_point(rand() % PONG_COSYS_XMAX, rand() % PONG_COSYS_YMAX),
            gsptr->ballpos
        );
    }

    return NOERR;
}

void cleanup()
{
    endwin();
}

int main(int argc, char** argv)
{
    // standard condition is PONG_REGULAR
    int condition = PONG_REGULAR;

    // evaluates additional arguments
    if(argc > 1)
    {
        char* s = argv[1];
        int eval = strcmp(s, "debug");
        if(eval == 0)
        {
            condition = PONG_DEBUG;            
        }
    }

    // creates initial gamestate
    gamestate gs = init_game(condition);

    // input-calculation-output-loop
    while(gs.alive)
    {
        // ouputs gamestate gs on terminal
        render(gs);
        // increments framecount
        gs.framecount++;
        // considers input and calculates based on that input
        run(&gs);
        // sleeps for 40 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }   // exists when gs.alive is false

    cleanup();

    return 0;
}