//---------------------------------------------------------------------------


#pragma hdrstop

#include "SelectSort.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

static int select_sort[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

int GetSortElem(int i)
{
    return select_sort[i];
}

void Push2First(int band)
{
    if (select_sort[0] == band)
    {
        return;
    }

    int tmp = select_sort[0];
    select_sort[0] = band;
        
    for (int i=1;i<12;i++)
    {
        if (select_sort[i] != band)
        {
            int tmp_next = select_sort[i];
            select_sort[i] = tmp;
            tmp = tmp_next;
        }
        else
        {
            select_sort[i] = tmp;
            break;
        }
    }
}

