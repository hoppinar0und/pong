#ifndef SIMON_CROUND_H
#define SIMON_CROUND_H

int cround(double d)
{
    int d_int = d;
    d -= d_int;

    if(d > 0)
    {
        if(d >= .5)
            return d_int+1;
    }
    else if(d < 0)
    {
        if(d <= -.5)
            return d_int-1;
    }

    return d_int;
}

#endif // SIMON_CROUND_H