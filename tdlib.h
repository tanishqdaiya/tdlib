/* tdlib -- v0.1 - a small, opinionated c utility library

   To create the implementation, define:
       #define TDLIB_IMPLEMENTATION
   in exactly one C file before including the header.

   A typical setup looks like this
   #include ...
   #include ...
   #include ...
   #define TDLIB_IMPLEMENTATION
   #include "tdlib.h"

   All other files should include this header normally, without defining
   TDLIB_IMPLEMENTATION.

   This library does not imply warranty.

   A guided overview of the library is provided in PRIMER. This is not an
   exhaustive documentation. Reading the code is expected.

   LICENSE

   This software is released into the public domain.

   Where public domain is not recognized, you may use this software under
   the terms of the MIT License.

   See LICENSE for details.
*/

#ifndef TDLIB_H
#define TDLIB_H

/* PRIMER

   This library provides a small set of utilities for writing C programs. It
   includes compiler and os context cracking, fixed-width and intent-oriented
   typedefs and a macro-based dynamic array (we use the name 'vector' because
   that's what it is), and a string implementation.

   Everything here is opinionated. That's not a flaw. It is the intent. The code
   does exactly what I need it to do, the way I want it done. If this matches
   your style, great. If not, don't.

   This is a "PRIMER", not "DOCUMENTATION". You get a guided walkthrough. After
   that, you're on your own.

   Compiler and OS Context Cracking
   --------------------------------
   This library defines compile-time macros describing the compiler and
   operating system. This is a one and done thing so that the rest of the code
   doesn't have to play twenty questions with the preprocessor.

   Windows, Linux, the BSD family and macOS are supported. The rest get an
   error. That's intentional. No silent fallbacks.

   A POSIX macro is also defined. In this library, POSIX means Linux, BSD and
   macOS. If your code works on those systems, you can gate it once instead of
   copy pasting conditions everywhere.

   The supported compilers are GCC, Clang and MSVC. If you're using something
   else, you're on your own.

   All these macros are part of the public interface. If you don't like it, nuke
   the section.
   
   Typedefs
   --------
   The header defines a set of fixed-width integer typedefs and a few intent
   based aliases. You can read the typedefs directly; it is short.

   Fixed-width types are encouraged because ambiguity is a liability. This does
   not prohibit the use of the standard C types.

   There are also aliases like global_variable, local_persist and internal. They
   all expand to static. They don't change any behavior. They make the intent
   obvious.
   
   Vector
   ------
   
   struct vec_T {
       T *data;
       size_t size, alloc;
   };

   T is the element type.  data points to the contiguous heap allocation.  size
   is how many elements the vector contains.  alloc is how many elements the
   vector CAN contain until the next reallocation.

   You can add fields if you want; you can't remove any.

   Initialization:
   struct vec_T my_vector = {0};

   Append to end with td_vec_append:
   td_vec_append(&my_vector, 5);

   You pass the pointer because the vector owns its state. Appending may
   reallocate and invalidate pointers. Nothing surprising.

   The vector grows to twice its previous capacity. On first use, it allocates
   an initial capacity of TD_VECINITSZ elements. If you don't like that number,
   define it yourself before including the header.

   If you want a different allocation strategy. Redefine TD_MALLOC, TD_REALLOC
   and TD_FREE to suit your needs.

   Removing elements from the end is done by decrementing the size:
   my_vector.size -= 1; // Removes 1 element

   This is fast because no memory is freed. That is not a leak. The next append
   overwrites the unused space.

   Bulk appends exist because copying data one at a time is pointless when you
   already know the size. The implementation is short. Read it.

   Strings
   -------
   There are two string types: TD_String and TD_String_View. They do different
   things.

   TD_String is a growable byte buffer. It is not null-terminated by
   default. That's intentional.

   Appending works exactly the same as in vectors. You can append your c-strings
   with td_string_append_cstr.

   You can also slice this string from [start, end) by td_string_slice. This
   returns a TD_String_View, which we dicuss now.

   TD_String_View doesn't allocate. It doesn't free. It doesn't modify
   anything. It is a non-owning view. It just points to something and knows how
   long it is.
   
   Views can be created from TD_String by td_string_view_from_string. It can
   also be created from c-strings td_string_view_from_cstr. You can compare two
   string views by td_string_view_equal.

   You can chop a portion by a given delimiter. You can trim from left, right
   and both the sides. You can also slice a string from [start, end).
   [td_string_view_chop, td_string_view_trim_left, td_string_view_trim_right,
   td_string_view_trim, td_string_view_slice]

   You can read a complete file into a string buffer. [td_read_file_to_string]
 */

#ifndef TD_LIBDEF
#    ifdef TDLIB_STATIC
#        define TD_LIBDEF static
#    else
#        define TD_LIBDEF extern
#    endif
#endif

#if defined __clang__
#    define COMPILER_CLANG
#elif defined _MSC_VER
#    define COMPILER_MS
#elif defined __GNUC__
#    define COMPILER_GNU
#endif

#if defined _WIN32
#    define PLATFORM_WIN
#elif defined __linux__ || defined __gnu_linux__
#    define PLATFORM_LINUX
#elif defined __APPLE__ && defined __MACH__
#    define PLATFORM_MACOS
#elif defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__
#    define PLATFORM_BSD
#else
#    error "fatal: unsupported platform"
#endif

#if defined PLATFORM_LINUX || defined PLATFORM_MACOS || defined PLATFORM_BSD
#    define PLATFORM_POSIX
#endif

#ifdef COMPILER_MS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8  b8;
typedef i32 b32;
typedef i64 b64;

typedef float  f32;
typedef double f64;

#define global_variable static
#define local_persist   static
#define internal        static

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MALLOC
#    define TD_MALLOC(sz) malloc(sz)
#endif

#ifndef TD_REALLOC
#    define TD_REALLOC(ptr, sz) realloc((ptr), (sz))
#endif

#ifndef TD_FREE
#    define TD_FREE(p) free(p)
#endif

#ifndef TD_VECINITSZ
#    define TD_VECINITSZ 1024
#endif

#ifndef TD_PANIC
#define TD_PANIC(msg)                                           \
    do {                                                        \
        fprintf(stderr, "%s\n", (msg));                         \
        exit(EXIT_FAILURE);                                     \
    } while (0)
#endif
        

/* It is a resizing function which required the vector in its right structural
   format and the required capacity. If the capacity exceeds the pre-allocated
   size of the vector, we resize. This function is unsafe and provides no
   guaranteed successful reallocation */
#define td__vec_alloc(vector, capacity)                                 \
    do {                                                                \
        if ((capacity) > (vector)->alloc) {                             \
            if ((vector)->alloc == 0) (vector)->alloc = TD_VECINITSZ;   \
            while ((capacity) > (vector)->alloc) (vector)->alloc *= 2;  \
            (vector)->data = TD_REALLOC((vector)->data,                 \
                                     (vector)->alloc * sizeof(*(vector)->data)); \
            if ((vector)->data == NULL) {                               \
                TD_PANIC("TD_REALLOC: out of memory");                  \
            }                                                           \
        }                                                               \
    } while (0)

#define td_vec_append(vector, item)                     \
    do {                                                \
        td__vec_alloc((vector), (vector)->size + 1);    \
        (vector)->data[(vector)->size++] = (item);      \
    } while (0)

#define td_vec_append_bulk(vector, items, count)                \
    do {                                                        \
        td__vec_alloc((vector), (vector)->size + count);        \
        memcpy((vector)->data + (vector)->size,                 \
               (items),                                         \
               (count)*sizeof(*(vector)->data));                \
        (vector)->size += (count);                              \
    } while (0)

/* Not null-terminated by default behavior */
typedef struct {
    char   *data;
    size_t  size, alloc;
} TD_String;

typedef struct {
    char   *data;
    size_t  size;
} TD_String_View;

#define td_string_append_cstr(string, cstr)             \
    do {                                                \
        const char *str = cstr;                         \
        size_t size = strlen(cstr);                     \
        td_vec_append_bulk((string), str, size);        \
    } while (0)

#define td_string_clear(string)                 \
    do {                                        \
        TD_FREE((string)->data);                \
        (string)->data = NULL;                  \
        (string)->size = (string)->alloc = 0;   \
    } while (0)

TD_LIBDEF TD_String_View td_string_view_from_string(TD_String*);
TD_LIBDEF TD_String_View td_string_view_from_cstr(char*);
TD_LIBDEF bool           td_string_view_equal(TD_String_View, TD_String_View);
TD_LIBDEF TD_String_View td_string_view_slice(const TD_String_View, size_t, size_t);
TD_LIBDEF TD_String_View td_string_view_chop(TD_String_View*, char);
TD_LIBDEF TD_String_View td_string_view_trim_left(TD_String_View);
TD_LIBDEF TD_String_View td_string_view_trim_right(TD_String_View);
TD_LIBDEF TD_String_View td_string_view_trim(TD_String_View);

TD_LIBDEF TD_String_View td_string_slice(const TD_String*, size_t, size_t);

TD_LIBDEF bool		 td_read_file_to_string(TD_String*, FILE*);
#endif /* TDLIB_H */


#ifdef TDLIB_IMPLEMENTATION

#include <ctype.h>

TD_LIBDEF TD_String_View
td_string_view_from_string(TD_String *str)
{
    return (TD_String_View){ str->data, str->size };
}
    
TD_LIBDEF TD_String_View
td_string_view_from_cstr(char *cstr)
{
    return (TD_String_View){ cstr, strlen(cstr) };
}

TD_LIBDEF bool
td_string_view_equal(TD_String_View a, TD_String_View b)
{
    if (a.size != b.size) return false;
    for (size_t i = 0; i < a.size; i++)
        if (a.data[i] != b.data[i]) return false;
    return true;
}

TD_LIBDEF TD_String_View
td_string_view_slice(const TD_String_View v, size_t start, size_t end)
{
    if (start > end) start = end;
    if (end > v.size) end = v.size;

    return (TD_String_View) { v.data + start, end - start };
}

TD_LIBDEF TD_String_View
td_string_view_chop(TD_String_View *v, char delim)
{
    size_t i = 0;
    while (i < v->size && v->data[i] != delim) i++;

    TD_String_View out = { v->data, i };

    if (i < v->size) {
        v->data += i + 1;
        v->size -= i + 1;
    } else {
        v->data += i;
        v->size = 0;
    }

    return out;
}

TD_LIBDEF TD_String_View
td_string_view_trim_left(TD_String_View v)
{
    size_t i = 0;
    while (i < v.size && isspace((unsigned char)v.data[i]))
        ++i;
    return (TD_String_View){ v.data + i, v.size - i };
}

TD_LIBDEF TD_String_View
td_string_view_trim_right(TD_String_View v)
{
    size_t end = v.size;
    while (end > 0 && isspace((unsigned char)v.data[end - 1]))
        --end;
    return (TD_String_View){ v.data, end };
}

TD_LIBDEF TD_String_View
td_string_view_trim(TD_String_View v)
{
    return td_string_view_trim_right(td_string_view_trim_left(v));
}

TD_LIBDEF TD_String_View
td_string_slice(const TD_String *str,
                size_t           start,
                size_t           end)
{
    if (start > str->size) start = str->size;
    if (end > str->size) end = str->size;
    if (end < start) end = start;

    return (TD_String_View) { str->data + start, end - start };
}

TD_LIBDEF bool
td_read_file_to_string(TD_String *str, FILE *fp)
{
    i32 file_size;
    size_t read;

    if (fseek(fp, 0, SEEK_END) != 0)
        return false;

    file_size = ftell(fp);
    if (file_size < 0)
        return false;

    rewind(fp);

    td__vec_alloc(str, (size_t)file_size);
    read = fread(str->data, 1, (size_t)file_size, fp);
    if (read != (size_t)file_size)
        return false;

    str->size = read;
    return true;
}

#endif /* TDLIB_IMPLEMENTATION */
