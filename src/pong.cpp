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
#define PONG_RATIO (float)(4.0f / 3.0f)
#define PONG_COSYS_XMAX 11
#define PONG_COSYS_YMAX 8
#define DEBUG_CPAIRRED 1
#define DEBUG_CPAIRBLUE 2

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
    char** pxlbuf;
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

gamestate init_game(int condition);
frame create_frame();
error free_frame(frame);
error render(gamestate);
error run(gamestate*);
input get_input();
void cleanup();

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
        for(int i = 0; i < PONG_COSYS_XMAX * 2 + PONG_OFFSET; i++)
            addch('-');
        addch('x');
    }

    // Prints y debug center
    if(condition == PONG_DEBUG)
    {
        for(int i = 1; i < PONG_COSYS_XMAX * 2; i++)
        {
            attron(COLOR_PAIR(DEBUG_CPAIRRED));
            mvaddch(cround((float)PONG_COSYS_YMAX / 2.0f) + PONG_OFFSET, 0, ' ');
            for(int i = 0; i < PONG_COSYS_XMAX * 2 + PONG_OFFSET; i++)
                addch('-');
            attroff(COLOR_PAIR(DEBUG_CPAIRRED));
        }
    }

    // prints lower border
    {
        mvaddch(PONG_COSYS_YMAX + PONG_OFFSET, 0, 'x');
        for(int i = 0; i < PONG_COSYS_XMAX * 2 + PONG_OFFSET; i++)
            addch('-');
        addch('x'); 
    }

    // prints side borders
    for(int i = 0; i < PONG_COSYS_YMAX; i++)
    {
        mvaddch(i + PONG_OFFSET, 0, '|');

        // prints y center border
        if(condition == PONG_DEBUG)
        {
            attron(COLOR_PAIR(DEBUG_CPAIRBLUE));
            mvaddch(i + PONG_OFFSET, cround((PONG_COSYS_XMAX / 2.0f) / PONG_COSYS_XMAX * PONG_COSYS_XMAX * 2 + PONG_OFFSET), '|');
            attroff(COLOR_PAIR(DEBUG_CPAIRBLUE));
        }

        mvaddch(i + PONG_OFFSET, PONG_COSYS_XMAX * 2 + PONG_OFFSET * 2, '|');
    }

    // Prints headline + intial points position
    {
        const char* name = " Pongy ";
        mvprintw(0, (PONG_COSYS_XMAX - (cround((float)strlen(name) / 2.0f) - PONG_OFFSET - 1)), name);

        mvprintw(0, cround((float)PONG_COSYS_XMAX / 2.0f) - 1, " %d ", 0);
        mvprintw(0, PONG_COSYS_XMAX + cround((float)PONG_COSYS_XMAX / 2.0f) + 1, " %d ", 0);
    }

    // prints various debug info
    if(condition == PONG_DEBUG)
    {
        mvprintw
        (
            0, 0,
            "h:%d w:%d t: %d e: %f r: %f, my: %d",
            PONG_COSYS_YMAX, PONG_COSYS_XMAX, PONG_COSYS_YMAX * PONG_COSYS_XMAX, (float)((float)PONG_COSYS_YMAX * (float)PONG_RATIO), PONG_RATIO, cround((float)PONG_COSYS_YMAX / 2.0f) + PONG_OFFSET
        );
    }

    // creates initial frame
    frame f = create_frame();

    // copies calculated data into our return obj
    _return.p1pos = 1;
    _return.p2pos = 6;
    _return.framecount = 0;
    _return.framebuffer = f;
    _return.ballpos = create_point(cround((float)PONG_COSYS_XMAX / 2.0f), cround((float)PONG_COSYS_YMAX / 2.0f));
    _return.condition = condition;

    // seeds rand function
    time_t *tptr;
    time(tptr);
    rand_r((unsigned int*)tptr);

    // creates random between 0, 1
    bool random = rand() % 2;

    // calculates random ball vector
    _return.ballvec = create_vector
    (
        create_point(rand() % PONG_COSYS_XMAX, rand() % PONG_COSYS_YMAX),
        _return.ballpos
    );

    // defines gamestate as alive
    _return.alive = true;

    // returns
    return _return;
}

frame create_frame()
{
    frame _return;

    // allocates memory
    _return.pxlbuf = (char**)malloc(PONG_COSYS_YMAX * sizeof(char*));
    for(int i = 0; i < PONG_COSYS_YMAX; i++)
        _return.pxlbuf[i] = (char*)malloc(PONG_COSYS_XMAX * sizeof(char));

    // fills memory with ' ' chars
    for(int i = 0; i < PONG_COSYS_YMAX; i++)
        for(int j = 0; j < PONG_COSYS_XMAX; j++)
            _return.pxlbuf[i][j] =  ' ';

    return _return;
}

// not correctly implemented. Do Not Use!
error free_frame(frame f)
{
    for(int i = 0; i < PONG_COSYS_YMAX; i++)
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
    // Ball computation
    {
        mvaddch(gs.ballpos.y + PONG_OFFSET, gs.ballpos.x * 2 + PONG_OFFSET, '@');
    }

    // p1 slider
    {
        for(int i = 0; i < PONG_COSYS_YMAX; i++)
            mvaddch(i + PONG_OFFSET, 0 + PONG_OFFSET, ' ');
        for(int i = 0; i < 2; i++)
            mvaddch(gs.p1pos + PONG_OFFSET + i, 0 + PONG_OFFSET, '|');
    }

    // p2 slider
    {

        for(int i = 0; i < PONG_COSYS_YMAX; i++)
            mvaddch(i + PONG_OFFSET, PONG_COSYS_XMAX * 2 + PONG_OFFSET, ' ');
        for(int i = 0; i < 2; i++)
            mvaddch(gs.p2pos + PONG_OFFSET + i, PONG_COSYS_XMAX * 2 + PONG_OFFSET, '|');
    }

    // debug computation
    if(gs.condition == PONG_DEBUG)
    {
        int desty;
        int destx;

        /* vector computation
        {
            desty = cround((float)gs.ballvec.destination.y / PONG_COSYS_YMAX * PONG_COSYS_YMAX); 
            destx = cround((float)gs.ballvec.destination.x / PONG_COSYS_XMAX * PONG_COSYS_XMAX * 2 + PONG_OFFSET);

            mvaddch(desty + PONG_OFFSET, destx, 'O');

            mvprintw(gs.framebuffer.PONG_COSYS_YMAX + 4, 0, "O: ");

            mvprintw
            (
                gs.framebuffer.PONG_COSYS_YMAX + 4, 3,
                "y: (%f / %f) * %d = %d",
                gs.ballvec.destination.y, PONG_COSYS_YMAX, gs.framebuffer.PONG_COSYS_YMAX, desty
            );


            mvprintw
            (
                gs.framebuffer.PONG_COSYS_YMAX + 5, 3,
                "x: (%f / %f) * %d = %d",
                gs.ballvec.destination.x, PONG_COSYS_XMAX, gs.framebuffer.PONG_COSYS_XMAX, destx
            );
        }*/

        // prints framecount
        mvprintw
        (
            PONG_COSYS_YMAX + 6, 0,
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
}