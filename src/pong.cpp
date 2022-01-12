#include <ncurses.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <string.h>

#define PONG_OFFSET_X 1
#define PONG_OFFSET_Y 1
#define PONG_REGULAR 0
#define PONG_DEBUG 1
#define PONG_RATIO 1.334
#define PONG_COSYS_XMAX (float)((float)10 * 1.334)
#define PONG_COSYS_YMAX ((float)10)

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
    float p1pos;
    float p2pos;
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
error run_frame(gamestate);
void cleanup();


point create_point(float x, float y)
{
    point _return;
    _return.x = x;
    _return.y = y;

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

    size_t width = height * PONG_RATIO;

    gamestate _return;

    mvaddch(0, 0, 'x');
    for(int i = 0; i < width * 2 + 1; i++)
        addch('-');
    addch('x');

    if(condition == PONG_DEBUG)
    {
        for(int i = 1; i < width * 2; i++)
        {
            attron(COLOR_PAIR(2));
            mvaddch(height / 2 + 1, 0, ' ');
            for(int i = 0; i < width * 2 + 1; i++)
                addch('-');
            attroff(COLOR_PAIR(2));
        }
    }

    mvaddch(height + 1, 0, 'x');
    for(int i = 0; i < width * 2 + 1; i++)
        addch('-');
    addch('x');

    for(int i = 1; i < height + 1; i++)
    {
        mvaddch(i, 0, '|');

        if(condition == PONG_DEBUG)
        {
            attron(COLOR_PAIR(1));
            mvaddch(i, width + 1, '|');
            attroff(COLOR_PAIR(1));
        }

        mvaddch(i, width * 2 + 2, '|');
    }

    mvprintw(0, width - 6, "= Pong Curses =");

    mvprintw(0, width / 2 - 1, " %d ", 0);
    mvprintw(0, width + width / 2 + 1, " %d ", 0);

    if(condition == PONG_DEBUG)
    {
        mvprintw(0, 0, "h:%d w:%d t: %d", height, width, height * width);
    }

    frame f = create_frame(height, width);

    _return.p1pos = 2.5;
    _return.p2pos = 7.5;
    _return.framecount = 0;
    _return.framebuffer = f;
    _return.ballpos = create_point((PONG_COSYS_XMAX) / 2, PONG_COSYS_YMAX / 2);

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
        bally = (gs.ballpos.y / PONG_COSYS_YMAX) * gs.framebuffer.height; 
        ballx = (gs.ballpos.x / PONG_COSYS_XMAX) * gs.framebuffer.width;

        mvaddch(bally + PONG_OFFSET_Y, ballx * 2 + PONG_OFFSET_X, '@');
    }

    // p1 slider computation
    {
        int sliderpos = (gs.p1pos / PONG_COSYS_YMAX) * gs.framebuffer.height;
        int slidersize = gs.framebuffer.height / 3;

        int offset = slidersize / 2 - 1;

        for(int i = 0; i < gs.framebuffer.height; i++)
        {

        }

        for(int i = 0; i < slidersize; i++)
        {
            mvaddch(sliderpos - offset + i, 0 + PONG_OFFSET_X, '|');
        }
    }

    // p2 slider computation
    {
        int sliderpos = (gs.p2pos / PONG_COSYS_YMAX) * gs.framebuffer.height;
        int slidersize = gs.framebuffer.height / 3;

        int offset = slidersize / 2 - 1;

        for(int i = 0; i < slidersize; i++)
        {
            mvaddch(sliderpos - offset + i, gs.framebuffer.width * 2 + PONG_OFFSET_Y, '|');
        }
    }

    // debug computation
    if(gs.condition == PONG_DEBUG)
    {

        mvprintw(gs.framebuffer.height + 2, 0, "@: ");

        mvprintw
        (
            gs.framebuffer.height + 2, 3,
            "y: (%f / %f) * %f = %f",
            gs.ballpos.y, PONG_COSYS_YMAX, (float)gs.framebuffer.height, (float)bally
        );

        mvprintw
        (
            gs.framebuffer.height + 3, 3,
            "x: (%f / %f) * %f = %f",
            gs.ballpos.x, PONG_COSYS_XMAX, (float)gs.framebuffer.width, (float)ballx
        );

        mvprintw
        (
            0, gs.framebuffer.width * 2 + 4,
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
            gsptr->p1pos -= 0.25;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    free_frame(gs.framebuffer);

    cleanup();
}