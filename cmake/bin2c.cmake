if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
    message(FATAL_ERROR "bin2c.cmake needs -DINPUT= -DOUTPUT= -DSYMBOL=")
endif()

file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" hexLength)
math(EXPR byteCount "${hexLength} / 2")

set(body "")
set(i 0)
set(column 0)
while(i LESS hexLength)
    string(SUBSTRING "${hex}" ${i} 2 byte)
    string(APPEND body "0x${byte},")
    math(EXPR column "${column} + 1")
    if(column EQUAL 16)
        string(APPEND body "\n    ")
        set(column 0)
    endif()
    math(EXPR i "${i} + 2")
endwhile()

file(WRITE "${OUTPUT}"
"/* Generated from ${INPUT} - do not edit. */\n"
"#include <stddef.h>\n"
"\n"
"__attribute__((aligned(4)))\n"
"const unsigned char ${SYMBOL}[] = {\n    ${body}\n};\n"
"const size_t ${SYMBOL}_size = ${byteCount};\n")
