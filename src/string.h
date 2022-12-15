#ifndef STRING_H
#define STRING_H

#include <base.h>


struct string
{
    char const *data;
    usize size;
};


int cstring_size_no0(char const *s);
int cstring_size_with0(char const *s);

struct string make_string_from_cstring(char const *s);


#endif // STRING_H
