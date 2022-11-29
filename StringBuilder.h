#pragma once
#include "Output.h"
#include "Global.h"

#define Sbb StringBuilder::ins()

#define STRING_BUILDER_DEF_SIZE 1000


class StringBuilder;
#if !defined(NO_GLOBAL_INSTANCES)
    extern StringBuilder Sb;
#endif

class StringBuilder : public Buffer, CString { //=============================================  Class


private: //�����������������������������������������������������������������������������������������  PRIVATE
char buf[STRING_BUILDER_DEF_SIZE] = "";   // primary buffer
boolean _start = false;
boolean _end = true;            // set to true to end the string
size_t sPtr = 0;                // pointer to start of free space in buf
boolean _retain = false;        // true to keep old strings in buf cause they're still in use

// macro
#define _LockCheck()\
    _ThrowIf(isLock(), "StringBuilder: start() before end()");

/** containing start ptr and length of free space in the buffer */
struct BufInfo //==============================================  BufInfo f
{   char* start;
    int free;
} info;

FORCE_INLINE boolean isLock() { return _start; } //=============  isLock f


size_t getBufLen() { return strlen(buf); } //================  getBufLen f
size_t getCBufLen() { return strlen(getCBuf()); } //========  getCBufLen f

BufInfo getBufInfo() //=====================================  getBufInfo f
{   char* cBuf = getCBuf();
    int cBufLen = strlen(cBuf);

    
    info.start = cBuf + cBufLen;
    info.free = sizeof(buf) - sPtr - cBufLen - 1; // '\0' Maybe wrong
//    Serial.printf("cBufLen = %d, info.start = %d, free = %d\n", cBufLen, info.start, info.free);
    return info;
}

char* getCBuf() //=============================================  getCBuf f
{   return buf + sPtr;
}

/** get buffer at cat pos */
char* getCat() //===============================================  getCat f
{   getBufInfo();
    return info.start;
}


public: //������������������������������������������������������������������������������������������  PUBLIC 
FORCE_INLINE static StringBuilder &ins() { return Sb; } //========  &ins f

const char* c_str() override //==================================  c_str f
{   return buf;
}

inline char* get() { return buf; } //==============================  get f

char* getBuffer() override { return buf; }

StringBuilder &start() //=======================================  &start f
{   _LockCheck();

    if (!_retain)
        buf[0] = '\0';  // reset buf

    _start = true;
    _end = false;   // remove termination
    return *this;
}

StringBuilder &start(const char* s)
{   start();
    return cat(s);
}

char* startBuf() //===========================================  startBuf f
{   start();
    return getCBuf();
}


char* end() //=====================================================  end f
{   _start = false;
    _end = true;
    char* string = getCBuf();
    if (_retain)                       // if retain
    {   sPtr += (strlen(string) + 1); // move pointer to next free location, +1 for '\0'
//        Serial.printf("sPtr = %d, strlen(string) = %d, string = %s\n", sPtr, strlen(string), string);
    }
    else
        sPtr = 0;
    return string;
}

/** raw cat, no check, not safe, use with caution */
void catRaw(const char* s) { strcat(buf, s); } //===============  catRaw f

/** concat string, memory safe */
StringBuilder &cat(const char* s) //================================  cat f
{   getBufInfo();
    int inputLen = strlen(s);
    
    if (inputLen <= info.free)
        strcat(info.start, s);
    else
        end();

    return *this;
}

typedef std::function<void(char*)> Catter;

StringBuilder &cat(const Catter &catter)
{   catter(getCat());
    return *this;
}

StringBuilder &catln() { cat("\n"); return *this; } //===========  catln f
StringBuilder &catln(const Catter &catter)
{   cat(catter);
    return catln();
}

StringBuilder &catlnIf(boolean b) { if (b) catln(); return *this; } //  &catlnIf f


/** concat formated string, memory safe */
StringBuilder &catf(const char* format, ...) //====================  catf f
{   getBufInfo();

    va_list vl;
    va_start(vl, format);
    vsnprintf(info.start, info.free, format, vl);
    va_end(vl);

    return *this;
}

/** concat formated params, memory safe
 *  [len]: how many params to process
*/
StringBuilder &catfp(Parameter &p, const int LEN, const char* format, ...) //  catfp f
{   va_list vl;
    va_start(vl, format);

    for (int paramNum, i = 0; i < LEN; i++)
    {   paramNum = va_arg(vl, int);
        getBufInfo();
        snprintf(info.start, info.free, format, p.param(paramNum));
    }

    va_end(vl);
    return *this;
}



StringBuilder &print() //=========================================  print f
{   Out.print(c_str());
    return *this;
}

StringBuilder &println() //====================================  println f
{   cat("\n");
    return print();
}


StringBuilder &printEnd() //===================================  printEnd f
{   print().end();
    return *this;
}

StringBuilder &printlnEnd() //==============================  printlnEnd f
{   println().end();
    return *this;
}

/** keep old strings in buf cause they're still in use */
using Retain = std::function<void(StringBuilder &Sb)>;
StringBuilder &retain(Retain retain) //========================  &retain f
{   _retain = true;
    retain(*this);
    _retain = false;
    return *this;
}

StringBuilder &retain(boolean b = true)
{   _retain = b;
    if (!_retain)
        sPtr = 0;
    return *this;
}

size_t free() { getBufInfo(); return info.free; } //==============  free f


}; //=====================================================================
