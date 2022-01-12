#include "cround.h"

int cround(double d)
{
    int dint = d;
    d -= dint;

    if(d > 0)
    {
        if(d >= .5)
            return dint+1;
    }
    else if(d < 0)
    {
        if(d <= -.5)
            return dint-1;
    }

    return dint;
}