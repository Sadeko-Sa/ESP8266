#pragma once
#include "Output.h"
#include "Global.h"


#define Sb StringBuilder::ins()

#if !defined STRING_BUILDER_BUF_SIZE || STRING_BUILDER_BUF_SIZE < 32
    #define STRING_BUILDER_BUF_SIZE 1024
#endif

//#define DEBUG

class StringBuilder : public Buffer, CString { //=============================================  Class


private: //██████████████████████████████████████████████████████████████████████████████████████████  PRIVATE
inline static std::shared_ptr<StringBuilder> _ins;    // smart pointer for singleton instance
inline static size_t _defCap = STRING_BUILDER_BUF_SIZE;
inline static const char _null[1] = "";

size_t _cap;

//char buf[_CAPACITY] = "";   // primary buffer
char* buf;         // primary buffer
size_t sPtr = 0;                          // pointer to start of last string in [buf]

boolean _start = false;
boolean _end = true;            // set to true to end the string
boolean _retain = false;        // true to keep old strings in buf cause they're still in use
boolean circularRetain = false; // true to start from buf[0] once [buf] is full

// macro
#define _LockCheck()\
    _ThrowIf(isLock(), "StringBuilder: start() before end()");

/** containing start ptr and length of free space in the buffer */
struct BufInfo //==============================================  BufInfo f
{   char* start;
    int free;
} info;

FORCE_INLINE boolean isLock() { return _start; } //=============  isLock f


FORCE_INLINE size_t getBufLen() { return _cap; } //==========  getBufLen f

/** get info for catting new string
 *  @return ptr cat pos, and available size of buffer to use */
void getBufInfo() //========================================  getBufInfo f
{   char* cBuf = getCBuf();       // current buf
    int cBufLen = strlen(cBuf);   // current buf len

    info.start = cBuf + cBufLen;                    // start after current buf ends
    info.free = _cap /**sizeof(buf)*/ - sPtr - cBufLen - 1;   // available space of buf, -1 for '\0'
    //   ↓buf start             ↓'\0'              ↓buf end
    //  |aaa bbb ccccccccccccccc                   |
    //           ^sPrt         ^cBufLen
}

/** get buffer at current [sPtr] */
char* getCBuf() //=============================================  getCBuf f
{   
    #ifdef DEBUG
        Serial.printf("___buf[%d] = [%s], strlen = %d, retain = %d, free = %d\n", sPtr, buf + sPtr, strlen(buf + sPtr), _retain, info.free);
    #endif
    return buf + sPtr;
}

/** get buffer at cat pos */
char* getCat() //===============================================  getCat f
{   getBufInfo();
    return info.start;
}

FORCE_INLINE void setSPtr(int pos, boolean erase = false) //===  setSPtr f
{   sPtr = pos;
    if (erase)
        buf[sPtr] = '\0';
}

void setSPtrOffset(int offset) //========================  setSPtrOffset f
{   int newPtr = sPtr + offset;
    if (0 <= newPtr && newPtr < getBufLen())
        sPtr = newPtr;
}


FORCE_INLINE void setup(StringBuilder* i) { if (!_ins) _ins.reset(i); }

public: //██████████████████████████████████████████████████████████████████████████████████████████  PUBLIC
~StringBuilder() { delete [] buf; }
StringBuilder()           : buf(new char[_defCap]), _cap(_defCap) { setup(this); }
StringBuilder(size_t cap) : buf(new char[cap])    , _cap(cap)     { setup(this); }

FORCE_INLINE static StringBuilder &ins() //=======================  &ins f
{   if (!_ins)
        new StringBuilder();
    return *_ins;
        
}

inline static void setBufferSize(size_t size) //=========  setBufferSize f
{   _defCap = size;
}

FORCE_INLINE const char* c_str() override { return buf; } //=====  c_str f

FORCE_INLINE char* getBuffer() override { return buf; } //===  getBuffer f

StringBuilder &start() //=======================================  &start f
{   _LockCheck();

    buf[sPtr] = '\0';  // reset buf

    _start = true;
    _end = false;
    return *this;
}

StringBuilder &start(const char* s) { return start().cat(s); }


/** be extremely careful with manually writing [buf] */
char* startBuf() { return start().getCBuf(); } //=============  startBuf f


const char* end() //===============================================  end f
{   _ReturnIfn(_start, getCBuf());

    _start = false;
    _end = true;
    char* currentStr = getCBuf();
    
    if (_retain)                          // if retain
    {   size_t sLen = strlen(currentStr);
        setSPtrOffset(sLen + 1);
        #ifdef DEBUG
            Serial.printf("AAAAAAA sPtr = %d, strlen(string) = %d, string = %s\n", sPtr, strlen(currentStr), currentStr);
        #endif
    }
    else
        setSPtr(0);
    return currentStr;
}

/** raw cat, no check, not safe, use with caution */
void catRaw(const char* s) { strcat(buf, s); } //===============  catRaw f

/** concat string, memory safe */
StringBuilder &cat(const char* s, size_t len) //===================  cat f
{   _ReturnIfn(_start, *this);

    getBufInfo();

    if (len <= info.free)
        strncat(info.start, s, len);
    else
        end();

    return *this;
}

StringBuilder &cat(const char* s) { return cat(s, strlen(s)); }

StringBuilder &catIf(boolean _if, const char* s)
{   if (_if)
        cat(s);
    return *this;
}

/** cat a char */
StringBuilder &cat(const char c)
{   _ReturnIfn(_start, *this);

    getBufInfo();
    
    if (info.free >= 1)
    {   info.start[0] = c;
        info.start[1] = '\0';
    }
    else
        end();

    return *this;
}

StringBuilder &catIf(boolean _if, const char c)
{   if (_if)
        cat(c);
    return *this;
}

typedef std::function<void(char*)> Catter;
StringBuilder &cat(const Catter &catter)
{   _ReturnIfn(_start, *this);

    catter(getCat());
    return *this;
}

StringBuilder &catln() { cat("\n"); return *this; } //===========  catln f
StringBuilder &catln(const Catter &catter)
{   cat(catter);
    return catln();
}

StringBuilder &catlnIf(boolean b) { if (b) catln(); return *this; } //  catlnIf f

    
/** concat formated string, memory safe */
StringBuilder &catf(const char* format, ...) //===================  catf f
{   _ReturnIfn(_start, *this);

    getBufInfo();

    _VaList(format
        , int inputSize = vsnprintf(info.start, info.free, format, vl));

    if (inputSize > info.free)
    {   info.start[0] = '\0';
        end();
        #ifdef DEBUG
            Serial.print("+++++++++++++++++++++++++++++++++++++++++ inputSize > info.free, overflow, end()\n");
        #endif
    }

    return *this;
}
    
/** concat formated params, memory safe
 *  [len]: how many params to process */
StringBuilder &catfp(Parameter<auto> &p, const int LEN, const char* format, ...) //  catfp f
{   _ReturnIfn(_start, *this);

    _VaList(format,
        for (int paramNum, i = 0; i < LEN; i++)
        {   paramNum = va_arg(vl, int);
            getBufInfo();
            snprintf(info.start, info.free, format, p.param(paramNum));
        });
    
    return *this;
}



StringBuilder &print() //===============================  print f
{   Serial.print(c_str());
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

/** keep old strings in [buf] cause they're still in use */
using Retain = std::function<void()>;
StringBuilder &retain(Retain r) //==============================  retain f
{   retain(true);
    r();
    retain(false);
    return *this;
}

inline StringBuilder &retain(boolean b = true)
{   _retain = b;
    setSPtr(0);
    return *this;
}

size_t free() { getBufInfo(); return info.free; } //==============  free f

const char* null() { return _null; } //===========================  null f


}; //=====================================================================
