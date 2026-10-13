/*===========================================================================
    basecontext.hpp
    10-Oct-99 Created by Neall

    Copyright (c)1999  Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _BASETYPE_HPP
#define _BASETYPE_HPP

#include <pddi/base/debug.hpp>

template <class T> class pddiStack
{
public:
    pddiStack(int d)     { stack = new T[d]; top = 0; depth = d; }
    virtual ~pddiStack() { delete[] stack; }
    T* Top()             { return &stack[top]; }
#ifdef RAD_PICA
    bool Push()          { if(top >= depth - 1) return false; top++; stack[top] = stack[top-1]; return true; }
    bool Pop()           { if(top <= 0) return false; top--; return true; }
#else
    void Push()          { PDDIASSERT(top < (depth - 1)); top++; stack[top] = stack[top-1]; }
    void Pop()           { PDDIASSERT(top > 0); top--; }
#endif

protected:
    int depth;
    int top;
    T* stack;
};

#endif /* _BASETYPE_HPP */

