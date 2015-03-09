// pbftoosm 2011-05-28 16:10
#define VERSION "0.10"
// (c) Markus Weber, Nuernberg
//
// compile this source with option -lz
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Affero General Public License
// as published by the Free Software Foundation.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
// Other licenses are available on request; please ask the author.

// just a note (for tests): -b=8.748,53.052,8.749,53.053
#define MAXLOGLEVEL 2
const char* helptext=
"\npbftoosm " VERSION "\n"
"\n"
"This program reads a .pbf file from standard input and writes it to\n"
"standard output using .osm XML format. For example:\n"
"\n"
"  ./pbftoosm <switzerland.pbf >switzerland.osm\n"
"\n"
"For most applications the history tags are not needed. If you decide\n"
"to exclude version, changeset, user and timestamp information, add\n"
"the command line argument \'--drop-history\'. For example:\n"
"\n"
"  ./pbftoosm --drop-history <a.pbf >a.osm\n"
"\n"
"Similar to this, you can drop whole sections of the file:\n"
"  --drop-nodes\n"
"  --drop-ways\n"
"  --drop-relations\n"
"\n"
"If you want to limit the region, you can use a bounding box. To do\n"
"this, enter the southwestern and the northeastern corners of the box.\n"
"For example:\n"
"\n"
"  ./pbftoosm -b=-0.5,51,0.5,52 <a.pbf >a.osm\n"
"\n"
"For another option to limit the region, you can use a border polygon\n"
"file:\n"
"\n"
"  ./pbftoosm -B=mycity.poly <europe.pbf >mycity.osm\n"
"\n"
"The format of a border polygon file can be found in the OSM Wiki:\n"
"http://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format\n"
"You do not need to follow strictly the format description, but you\n"
"must ensure that every line of coordinates starts with blanks.\n"
"\n"
"Usually, the output file will contain some empty relations because\n"
"their nodes do not lie inside the borders you have applied. If you\n"
"need to prevent this, please use the parameter \'-i\' to allow the\n"
"program random file access. The processing time will increase a\n"
"little. For example:\n"
"\n"
"  ./pbftoosm -b=-0.5,51,0.5,52 -i=a.pbf >a.osm\n"
"  ./pbftoosm -B=mycity.poly -i=europe.pbf >mycity.osm\n"
"\n"
"If you need to delete references to nodes which have been excluded\n"
"because lying outside the borders, use this option:\n"
"\n"
"  --drop-brokenrefs\n"
"\n"
"Tuning\n"
"\n"
"To speed-up the process, the program uses some main memory for a\n"
"hash table. By default, it uses 320 MiB for storing a flag for every\n"
"possible node, 60 for the way flags, and 20 relation flags.\n"
"Every byte holds the flag for 8 ID numbers, i.e., in 320 MiB the\n"
"program can store 2684 million flags. As there are less than 1000\n"
"million IDs for nodes at present (Oct 2010), 120 MiB would suffice.\n"
"So, for example, you can decrease the hash sizes to e.g. 130, 12 and\n"
"2 MiB using this option:\n"
"\n"
"  -h=130-12-2\n"
"\n"
"But keep in mind that the OSM database is continuously expanding. For\n"
"this reason the program-own default value is higher than shown in the\n"
"example, and it may be appropriate to increase it in the future.\n"
"If you do not want to bother with the details, you can enter the\n"
"amount of memory as a sum, and the program will divide it by itself.\n"
"For example:\n"
"\n"
"  -h=1000\n"
"\n"
"These 1000 MiB will be split in three parts: 800 for nodes, 150 for\n"
"ways, and 50 for relations.\n"
"\n"
"Because we are taking hashes, it is not necessary to provide all the\n"
"suggested memory; the program will operate with less hash memory too.\n"
"But, in this case, the border filter will be less effective, i.e.,\n"
"some ways and some relations will be left in the output file although\n"
"they should have been excluded.\n"
"The maximum value the program accepts for the hash size is 4000 MiB;\n"
"If you exceed the maximum amount of memory available on your system,\n"
"the program will try to reduce this amount and display a warning\n"
"message.\n"
"\n"
"Limitations\n"
"\n"
"When extracting a geographical region (using -b or -B), the input\n"
"file must contain the objects ordered by their type: first, all\n"
"nodes, next, all ways, followed by all relations. If you read the\n"
"data from standard input, the relations section must be sorted by \n"
"id in ascending order. Otherwise relations containing other relations\n"
"might be excluded although lying inside the specified region.\n"
"\n"
".pbf files usually adhere to both of these conditions. This means\n"
"that you do not have to worry about these limitations. pbftoosm will\n"
"display an error message if this sequence is broken. You may also\n"
"test the sequence within a file without doing any other processing:\n"
"\n"
"  ./pbftoosm -t file_to_test.osm\n"
"\n"
"If a polygon file for borders is supplied, there are two further\n"
"limitations:\n"
"First, the maximum number of polygon points is about 40,000.\n"
"Second, ways without a node inside the border polygon will be filtered\n"
"out as well as relations which refer only to such ways. Relations\n"
"which refer solely to unused relations with a lower ID will also be\n"
"filtered out, whereas every other relation will stay. For this reason,\n"
"there might be a few unwanted relations in the program\'s output; but\n"
"usually, this does not matter because they do not have georeferences.\n"
"\n"
"This help can be displayed by using the command line argument \'-h\'.\n"
"  ./pbftoosm -h\n"
"\n"
"Presently, this program is in an experimental state. Please expect\n"
"errors and do not use the program in productive or commercial systems.\n"
"\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"Please send any bug reports to markus.weber@gmx.com\n\n";

#define _FILE_OFFSET_BITS 64
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

typedef enum {false= 0,true= 1} bool;
typedef uint8_t byte;
typedef unsigned int uint;
#define isdig(x) isdigit((unsigned char)(x))
static int loglevel= 0;  // logging to stderr;
  // 0: no logging; 1: small logging; 2: normal logging;
  // 3: extended logging;
#ifdef _WIN32
  #define NL "\r\n"  // use CR/LF as new-line sequence
  #define off_t off64_t
  #define lseek lseek64
#else
  #define NL "\n"  // use LF as new-line sequence
  #define O_BINARY 0
#endif

static inline char *strmcpy(char *dest, const char *src, size_t maxlen) {
  // similar to strcpy(), this procedure copies a character string;
  // here, the length is cared about, i.e. the target string will
  // be limited in case it is too long;
  // src[]: source string which is to be copied;
  // maxlen: maximum length of the destination string
  //         (including terminator null);
  // return:
  // dest[]: destination string of the copy; this is the
  //         function's return value too;
  char* d;

  if(maxlen==0)
return dest;
  d= dest;
  while(--maxlen>0 && *src!=0)
    *d++= *src++;
  *d= 0;
  return dest;
  }  // end   strmcpy()
#define strMcpy(d,s) strmcpy((d),(s),sizeof(d))

static inline int strzcmp(const char* s1,const char* s2) {
  // similar to strcmp(), this procedure compares two character strings;
  // here, the number of characters which are to be compared is limited
  // to the length of the second string;
  // i.e., this procedure can be used to identify a short string s2
  // within a long string s1;
  // s1[]: first string;
  // s2[]: string to compare with the first string;
  // return:
  // 0: both strings are identical; the first string may be longer than
  //    the second;
  // -1: the first string is alphabetical smaller than the second;
  // 1: the first string is alphabetical greater than the second;
  while(*s1==*s2 && *s1!=0) { s1++; s2++; }
  if(*s2==0)
    return 0;
  return *(unsigned char*)s1 < *(unsigned char*)s2? -1: 1;
  }  // end   strzcmp()

static inline int strzlcmp(const char* s1,const char* s2) {
  // similar to strzcmp(), this procedure compares two character strings;
  // and accepts the first string to be longer than the second;
  // other than strzcmp(), this procedure returns the length of s2[] in
  // case both string contents are identical, and returns 0 otherwise;
  // s1[]: first string;
  // s2[]: string to compare with the first string;
  // return:
  // >0: both strings are identical, the length of the second string is
  //     returned; the first string may be longer than the second;
  // 0: the string contents are not identical;
  const char* s2a;

  s2a= s2;
  while(*s1==*s2 && *s1!=0) { s1++; s2++; }
  if(*s2==0)
    return s2-s2a;
  return 0;
  }  // end   strzlcmp()



//------------------------------------------------------------
// Module pbf_   protobuf conversions module
//------------------------------------------------------------

// this module provides procedures for conversions from
// protobuf formats to regular numbers;
// as usual, all identifiers of a module have the same prefix,
// in this case 'pbf'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----
// many procedures have a parameter 'pp'; here, the address of
// a buffer pointer is expected; this pointer will be incremented
// by the number of bytes the converted protobuf element consumes;

//------------------------------------------------------------

static inline uint32_t pbf_uint32(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  uint32_t i;
  uint32_t fac;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
return i;
    }
  i&= 0x7f;
  fac= 0x80;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
  return i;
  }  // end   pbf_uint32()

static inline int32_t pbf_sint32(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  int32_t i;
  int32_t fac;
  int sig;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
    if(i & 1)  // negative
return -1-(i>>1);
    else
return i>>1;
    }
  sig= i & 1;
  i= (i & 0x7e)>>1;
  fac= 0x40;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
    if(sig)  // negative
return -1-i;
    else
return i;
  }  // end   pbf_sint32()

static inline uint64_t pbf_uint64(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  uint64_t i;
  uint64_t fac;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
return i;
    }
  i&= 0x7f;
  fac= 0x80;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
  return i;
  }  // end   pbf_uint64()

static inline int64_t pbf_sint64(byte** pp) {
  // get the value of an unsigned integer;
  // pp: see module header;
  byte* p;
  int64_t i;
  int64_t fac;
  int sig;

  p= *pp;
  i= *p;
  if((*p & 0x80)==0) {  // just one byte
    (*pp)++;
    if(i & 1)  // negative
return -1-(i>>1);
    else
return i>>1;
    }
  sig= i & 1;
  i= (i & 0x7e)>>1;
  fac= 0x40;
  while(*++p & 0x80) {  // more byte(s) will follow
    i+= (*p & 0x7f)*fac;
    fac<<= 7;
    }
  i+= *p++ *fac;
  *pp= p;
    if(sig)  // negative
return -1-i;
    else
return i;
  }  // end   pbf_sint64()

static inline bool pbf_jump(byte** pp) {
  // jump over a protobuf formatted element - no matter
  // which kind of element;
  // pp: see module header;
  // return: the data do not meet protobuf specifications (error);
  byte* p;
  int type;

  p= *pp;
  type= *p & 0x07;
  switch(type) {  // protobuf type
  case 0:  // Varint
    while(*p & 80) p++; p++;  // jump over id
    while(*p & 80) p++; p++;  // jump over data
    break;
  case 1: // fixed 64 bit;
    while(*p & 80) p++; p++;  // jump over id
    p+= 4;  // jump over data
    break;
  case 2:  // String
    while(*p & 80) p++; p++;  // jump over id
    p+= pbf_uint32(&p);  // jump over string contents
    break;
  case 5: // fixed 32 bit;
    while(*p & 80) p++; p++;  // jump over id
    p+= 2;  // jump over data
    break;
  default:  // unknown id
    fprintf(stderr,"pbftoosm: Format error: 0x%02X.\n",*p);
    (*pp)++;
return true;
    }  // end   protobuf type
  *pp= p;
  return false;
  }  // end   pbf_jump()

static inline void pbf_intjump(byte** pp) {
  // jump over a protobuf formatted integer;
  // pp: see module header;
  // we do not care about a possibly existing identifier,
  // therefore as the start address *pp the address of the
  // integer value is expected;
  byte* p;

  p= *pp;
  while(*p & 0x80) p++; p++;
  *pp= p;
  }  // end   pbf_intjump()

//------------------------------------------------------------
// end   Module pbf_   protobuf conversions module
//------------------------------------------------------------



//------------------------------------------------------------
// Module hash_   OSM hash module
//------------------------------------------------------------

// this module provides three hash tables with default sizes
// of 320, 60 and 20 MB;
// the procedures hash_seti() and hash_geti() allow bitwise
// access to these tables;
// as usual, all identifiers of a module have the same prefix,
// in this case 'hash'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static bool hash__initialized= false;
#define hash__M 3
static unsigned char* hash__mem[hash__M]= {NULL,NULL,NULL};
  // start of the hash fields for each object type (node, way, relation);
static uint32_t hash__max[hash__M]= {0,0,0};
  // size of the hash fields for each object type (node, way, relation);
static int hash__error_number= 0;
  // 1: object too large

static void hash__end() {
  // clean-up for hash module;
  // will be called at program's end;
  int o;  // object type

  for(o= 0;o<hash__M;o++) {
    hash__max[o]= 0;
    if(hash__mem[o]!=NULL) {
      free(hash__mem[o]); hash__mem[o]= NULL; }
    }
  hash__initialized= false;
  }  // end   hash__end()

//------------------------------------------------------------

static int hash_ini(int n,int w,int r) {
  // initializes the hash module;
  // n: amount of memory which is to be allocated for nodes;
  // w: amount of memory which is to be allocated for ways;
  // r: amount of memory which is to be allocated for relations;
  // range for all input parameters: 1..4000, unit: MiB;
  // the second and any further call of this procedure will be ignored;
  // return: 0: initialization has been successful (enough memory);
  //         1: memory request had to been reduced to fit the system's
  //            resources (warning);
  //         2: memory request was unsuccessful (error);
  // general note concerning OSM database:
  // number of objects at Oct 2010: 950M nodes, 82M ways, 1.3M relations;
  int o;  // object type
  bool warning,error;

  warning= error= false;
  if(hash__initialized)  // already initialized
    return 0;  // ignore the call of this procedure
  // check parameters and store the values
  #define D(x,o) if(x<1) x= 1; else if(x>4000) x= 4000; \
    hash__max[o]= x*(1024*1024);
  D(n,0) D(w,1) D(r,2)
  #undef D
  // allocate memory for each hash table
  for(o= 0;o<hash__M;o++) {  // for each hash table
    do {
      hash__mem[o]= (unsigned char*)malloc(hash__max[o]);
      if(hash__mem[o]!=NULL) {  // allocation successful
        memset(hash__mem[o],0,hash__max[o]);  // clear all flags
    break;
        }
      // here: allocation unsuccessful
      // reduce amount by 50%
      hash__max[o]/=2;
      warning= true;
        // memorize that the user should be warned about this reduction
      // try to allocate the reduced amount of memory
      } while(hash__max[o]>=1024);
    if(hash__mem[o]==NULL)  // allocation unsuccessful at all
      error= true;  // memorize that the program should be aborted
    }  // end   for each hash table
  atexit(hash__end);  // chain-in the clean-up procedure
  if(!error) hash__initialized= true;
  return error? 2: warning? 1: 0;
  }  // end   hash_ini()

static void hash_seti(int o,int64_t idi) {
  // set a flag for a specific object type and ID;
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries
  //    are not checked;
  // id: id of the object;
  unsigned char* mem;  // address of byte in hash table
  unsigned int ido;  // bit offset to idi;

  if(!hash__initialized) return;  // error prevention
  idi+= hash__max[o]<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  *mem|= (1<<ido);  // set bit
  }  // end   hash_seti()

static bool hash_geti(int o,int64_t idi) {
  // get the status of a flag for a specific object type and ID;
  // (same as previous procedure, but id must be given as number);
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries are not checked;
  // id: id of the object; the id is given as a string of decimal digits;
  //     a specific string terminator is not necessary, it is assumed
  //     that the id number ends with the first non-digit character;
  unsigned char* mem;
  unsigned int ido;  // bit offset to idi;
  bool flag;

  if(!hash__initialized) return 0;  // error prevention
  idi+= hash__max[o]<<3;  // consider small negative numbers
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  flag= (*mem&(1<<ido))!=0;  // get status of the addressed bit
  return flag;
  }  // end   hash_geti();

static int hash_queryerror() {
  // determine if an error has occurred;
  return hash__error_number;
  }  // end   hash_queryerror()

//------------------------------------------------------------
// end   Module hash_   OSM hash module
//------------------------------------------------------------



//------------------------------------------------------------
// Module border_   OSM border module
//------------------------------------------------------------

// this module provides procedures for reading the border file
// (.poly) and determine if a point lies inside or outside the
// border polygon;
// as usual, all identifiers of a module have the same prefix,
// in this case 'border'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static const int32_t border__nil= 2000000000L;
static int32_t border__bx1= 2000000000L,border__by1,
  border__bx2,border__by2;
  // in case of a border box:
  // coordinates of southwest and northeast corner;
// in case of a border polygon:
// for the border polygon, every edge is stored in a list;
// to speed-up the inside/outside question we need to sort the edges
// by x1; subsequently, for every edge there must be stored references
// which refer to all that edges which overlap horizontally with
// that region between x1 and the next higher x1 (x1 of the next edge
// in the sorted list);
#define border__edge_M 60004
typedef struct border__edge_t {
  int32_t x1,y1,x2,y2;  // coordinates of the edge; always: x1<x2;
  struct border__chain_t* chain;
  } border__edge_t;
  // the last element in this list will have x1==border__nil;
static border__edge_t* border__edge;
static int border__edge_n= 0;  // number of elements in border__edge[0]
#define border__chain_M (border__edge_M*8)
typedef struct border__chain_t {
  border__edge_t* edge;
  struct border__chain_t* next;
  } border__chain_t;
  // the last element in this list will have edge==NULL;
  // the last element of each chain will be terminated with next==NULL;
static border__chain_t* border__chain;

static void border__end(void) {
  // close this module;
  // this procedure has no parameters because we want to be able
  // to call it via atexit();

  if(border__edge!=NULL)
    free(border__edge);
  border__edge= NULL;
  border__edge_n= 0;
  if(border__chain!=NULL)
    free(border__chain);
  border__chain= NULL;
  }  // end   border__end()

static inline bool border__ini(void) {
  // initialize this module;
  // you may call this procedure repeatedly; only the first call
  // will have effect; subsequent calls will be ignored;
  // return: ==true: success, or the call has been ignored;
  //         ==false: an error occurred during initialization;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;
    atexit(border__end);
    border__edge= (border__edge_t*)
      malloc((border__edge_M+4)*sizeof(border__edge_t));
    if(border__edge==NULL)
return false;
    border__chain= (border__chain_t*)
      malloc((border__chain_M+4)*sizeof(border__chain_t));
    if(border__chain==NULL)
return false;
    }
  return true;
  }  // end   border__ini()

static int border__qsort_edge(const void* a,const void* b) {
  // edge comparison for qsort()
  int32_t ax,bx;

  ax= ((border__edge_t*)a)->x1;
  bx= ((border__edge_t*)b)->x1;
  if(ax>bx)
return 1;
  if(ax==bx)
return 0;
  return -1;
  }  // end   border__qsort()

//------------------------------------------------------------

static bool border_active= false;  // borders are to be considered;
  // this variable must not be written from outside of the module;

static bool border_box(const char* s) {
  // read coordinates of a border box;
  // s[]: coordinates as a string; example: "11,49,11.3,50"
  // return: success;
  double x1f,y1f;  // coordinates of southwestern corner
  double x2f,y2f;  // coordinates of northeastern corner
  int r;

  x1f= y1f= x2f= y2f= 200.1;
  r= sscanf(s,"%lG,%lG,%lG,%lG",&x1f,&y1f,&x2f,&y2f);
  if(r!=4 || x1f<-180.1 || x1f>180.1 || y1f<-90.1 || y1f>90.1 ||
      x2f<-180.1 || x2f>180.1 || y2f<-90.1 || y2f>90.1)
return false;
  border_active=true;
  border__bx1= x1f*10000000L;  // convert floatingpoint to fixpoint
  border__by1= y1f*10000000L;
  border__bx2= x2f*10000000L;
  border__by2= y2f*10000000L;
  return true;
  }  // end   border_box()

static bool border_file(const char* fn) {
  // read border polygon file, store the coordinates, and determine
  // an enclosing border box to speed-up the calculations;
  // fn[]: file name;
  // return: success;
  static int32_t nil;

  if(!border__ini())
return false;
  nil= border__nil;

  /* get border polygon */ {
    border__edge_t* bep;  // growing pointer in border__edge[]
    border__edge_t* bee;  // memory end of border__edge[]
    FILE* fi;
    char s[80],*sp;
    int32_t x0,y0;  // coordinate of the first point in a section;
      // this is used to close an unclosed polygon;
    int32_t x1,y1;  // last coordinates
    int32_t x,y;

    border__edge[0].x1= nil;
    fi= fopen(fn,"rb");
    if(fi==NULL)
return false;
    bee= border__edge+(border__edge_M-2);
    bep= border__edge;
    x0= nil;  // (sign that there is no first coordinate at the moment)
    x1= nil;  // (sign that there is no last coordinate at the moment)
    for(;;) {  // for every line in border file
      s[0]= 0;
      sp= fgets(s,sizeof(s),fi);
      if(bep>=bee)
return false;
      if(s[0]!=' ' && s[0]!='\t') {  // not inside a section
        if(x0!=nil && x1!=nil && (x1!=x0 || y1!=y0)) {
            // last polygon was not closed
          if(x1==x0) {  // the edge would be vertical
            // we have to insert an additional edge
            x0+= 3;
            if(x0>x1)
              { bep->x1= x1; bep->y1= y1; bep->x2= x0; bep->y2= y0; }
            else
              { bep->x1= x0; bep->y1= y0; bep->x2= x1; bep->y2= y1; }
            bep->chain= NULL;
            if(loglevel>=1)
              fprintf(stderr,
                "+ %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                (int)(bep-border__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            bep++;
            x1= x0; y1= y0;
            x0-= 3;
            }  // the edge would be vertical
          // close the polygon
          if(x0>x1)
            { bep->x1= x1; bep->y1= y1; bep->x2= x0; bep->y2= y0; }
          else
            { bep->x1= x0; bep->y1= y0; bep->x2= x1; bep->y2= y1; }
          bep->chain= NULL;
          if(loglevel>=1)
            fprintf(stderr,
              "c %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
              (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
          bep++;
          }  // end   last polygon was not closed
        x0= x1= nil;
        }  // end   not inside a section
      else {  // inside a section
        double xf,yf;

        xf= yf= 200.1;
        sscanf(s+1,"%lG %lG",&xf,&yf);
        if(xf<-180.1 || xf>180.1 || yf<-90.1 || yf>90.1) x= nil;
        else
          { x= xf*10000000+0.5; y= yf*10000000+0.5; }
        if(x!=nil) {  // data plausible
          if(x1!=nil) {  // there is a preceding coordinate
            if(x==x1) x+= 2;  // do not accept exact north-south
              // lines, because then we may not be able to determine
              // if a point lies inside or outside the polygon;
            if(x>x1)
              { bep->x1= x1; bep->y1= y1; bep->x2= x; bep->y2= y; }
            else
              { bep->x1= x; bep->y1= y; bep->x2= x1; bep->y2= y1; }
            bep->chain= NULL;
            if(loglevel>=1)
              fprintf(stderr,
                "- %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                (int)(bep-border__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            bep++;
            }  // end   there is a preceding coordinate
          x1= x; y1= y;
          if(x0==nil)
            { x0= x; y0= y; }
          }  // end   data plausible
        }  // end   inside a section
      if(sp==NULL)  // end of border file
    break;
      }  // end   for every line in border file
    bep->x1= nil;  // set terminator of edge list
    border__edge_n= bep-border__edge;  // set number of edges
    }  // end   get border polygon

  // sort edges ascending by x1 value
  if(loglevel>=1)
    fprintf(stderr,"Border polygons: %i. Now sorting.\n",
      border__edge_n);
  qsort(border__edge,border__edge_n,sizeof(border__edge_t),
    border__qsort_edge);

  /* generate chains for each edge */ {
    int32_t /*x1,*/x2;
    border__chain_t* bcp;  // growing pointer in chain storage
    border__edge_t* bep;  // pointer in border__edge[]
    border__edge_t* bep2;  // referenced edge
    border__chain_t* bcp2;  // chain of referenced edge;

    bep= border__edge;
    bcp= border__chain;
    while(bep->x1!=nil) {  // for each edge in list
      if(loglevel>=1)
        fprintf(stderr,
          "> %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
          (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      /*x1= bep->x1;*/ x2= bep->x2;
      bep2= bep;
      while(bep2>border__edge && (bep2-1)->x1==bep2->x1) bep2--;
        // we must examine previous edges having same x1 too;
      while(bep2->x1!=nil && bep2->x1 <= x2) {
          // for each following overlapping edge in list
        if(bep2==bep) {  // own edge
          bep2++;  // (needs not to be chained to itself)
      continue;
          }
        if(bcp>=border__chain+border__chain_M)
            // no more space in chain storage
return false;
        if(loglevel>=2)
          fprintf(stderr,"+ add to chain of %i\n",
            (int)(bep2-border__edge));
        bcp2= bep2->chain;
        if(bcp2==NULL)  // no chain yet
          bep2->chain= bcp;  // add first chain link
        else {  // edge already has a chain
          // go to the chain's end and add new chain link there

          while(bcp2->next!=NULL) bcp2= bcp2->next;
          bcp2->next= bcp;
          }  // end   edge already has a chain
        bcp->edge= bep;
          // add source edge to chain of overlapping edges
        bcp->next= NULL;  // new chain termination
        bcp++;
        bep2++;
        }  // for each following overlapping  edge in list
      bep++;
      }  // end   for each edge in list
    }  // end   generate chains for each edge

  // test output
  if(loglevel>=2) {
    border__edge_t* bep,*bep2;  // pointers in border__edge[]
    border__chain_t* bcp;  // pointer in chain storage

    fprintf(stderr,"Chains:\n");
    bep= border__edge;
    while(bep->x1!=nil) {  // for each edge in list
      fprintf(stderr,
        "> %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
        (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      bcp= bep->chain;
      while(bcp!=NULL) {  // for each chain link in edge
        bep2= bcp->edge;
        fprintf(stderr,
          "  %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
          (int)(bep2-border__edge),
          bep2->x1,bep2->y1,bep2->x2,bep2->y2);
        bcp= bcp->next;
        }  // end   for each chain link in edge
      bep++;
      }  // end   for each edge in list
    }  // end   test output

  /* determine enclosing border box */ {
    border__edge_t* bep;  // pointer in border__edge[]

    border__bx1= border__edge[0].x1;
    border__bx2= -2000000000L;  // (default)
    border__by1= 2000000000L; border__by2= -2000000000L;  // (default)
    bep= border__edge;
    while(bep->x1!=nil) {  // for each coordinate of the polygon
      if(bep->x2>border__bx2) border__bx2= bep->x2;
      if(bep->y1<border__by1) border__by1= bep->y1;
      if(bep->y2<border__by1) border__by1= bep->y2;
      if(bep->y1>border__by2) border__by2= bep->y1;
      if(bep->y2>border__by2) border__by2= bep->y2;
      bep++;
      }  // end   for each coordinate of the polygon
    }  // end   determine enclosing border box
  border_active=true;
  if(loglevel>=1)
    fprintf(stderr,"End of border initialization.\n");
  return true;
  }  // end   border_file()

static bool border_queryinside(int32_t x,int32_t y) {
  // determine if the given coordinate lies inside or outside the
  // border polygon(s);
  // x,y: coordinates of the given point in 0.0000001 degrees;
  // return: point lies inside the border polygon(s);
  static int32_t nil;

  nil= border__nil;

  #if MAXLOGLEVEL>=3
  if(loglevel>=3)
    fprintf(stderr,"# %li,%li\n",x,y);
  #endif
  // first, consider border box (if any)
  if(border__bx1!=nil) {  // there is a border box
    if(x<border__bx1 || x>border__bx2 ||
        y<border__by1 || y>border__by2)
        // point lies outside the border box
return false;
    }  // end   there is a border box

  /* second, consider border polygon (if any) */ {
    border__edge_t* bep;  // pointer in border__edge[]
    border__chain_t* bcp;  // pointer in border__chain[]
    int cross;  // number of the crossings a line from the point
      // to the north pole would have ageinst the border lines
      // in border__coord[][];

    if(border__edge==NULL)
return true;
    cross= 0;

    /* binary-search the edge with the closest x1 */ {
      int i,i1,i2;  // iteration indexes

      i1= 0; i2= border__edge_n;
      while(i2>i1+1) {
        i= (i1+i2)/2;
        bep= border__edge+i;
//fprintf(stderr,"s %i %i %i   %li\n",i1,i,i2,bep->x1); ///
        if(bep->x1 > x) i2= i;
        else i1= i;
//fprintf(stderr,"  %i %i %i\n",i1,i,i2); ///
        }
      bep= border__edge+i1;
      }  // end   binary-search the edge with the closest x1

    bcp= NULL;
      // (default, because we want to examine the own edge first)
    for(;;) {  // for own edge and each edge in chain
      if(bep->x1 <= x && bep->x2 > x) {  // point lies inside x-range
        if(bep->y1 > y && bep->y2 > y) {
            // line lies completely north of point
          cross++;
          #if MAXLOGLEVEL>=3
          if(loglevel>=3)
            fprintf(stderr,"= %i %li,%li,%li,%li\n",
              (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
          #endif
          }
        else if(bep->y1 > y || bep->y2 > y) {
            // one line end lies north of point
          if( (int64_t)(y-bep->y1)*(int64_t)(bep->x2-bep->x1) <
              (int64_t)(x-bep->x1)*(int64_t)(bep->y2-bep->y1) ) {
              // point lies south of the line
            cross++;
            #if MAXLOGLEVEL>=3
            if(loglevel>=3)
              fprintf(stderr,"/ %i %li,%li,%li,%li\n",
                (int)(bep-border__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            #endif
            }
          #if MAXLOGLEVEL>=3
          else if(loglevel>=3)
            fprintf(stderr,". %i %li,%li,%li,%li\n",
              (int)(bep-border__edge),
              bep->x1,bep->y1,bep->x2,bep->y2);
          #endif
          }  // end   one line end north of point
        #if MAXLOGLEVEL>=3
        else if(loglevel>=3)
            fprintf(stderr,"_ %i %li,%li,%li,%li\n",
              (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
        #endif
        }  // end   point lies inside x-range
      if(bcp==NULL)  // chain has not been examined
        bcp= bep->chain;  // get the first chain link
      else
        bcp= bcp->next;  // get the next chain link
      if(bcp==NULL)  // no more chain links
    break;
      bep= bcp->edge;
      }  // end   for own edge and each edge in chain
//if(loglevel>=3) fprintf(stderr,"# %li,%li cross %i\n",x,y,cross);
return (cross&1)!=0;  // odd number of crossings
    }  // end   second, consider border polygon (if any)
  }  // end   border_queryinside()

static void border_querybox(int32_t* x1,int32_t* y1,
    int32_t* x2,int32_t* y2) {
  // get the values of a previously defined border box;
  // border_box() or border_file() must have been called;
  // return values are valid only if border_active==true;
  // x1,y1;  // coordinates of southwestern corner;
  // x2,y2;  // coordinates of northeastern corner;
  if(!border_active)
    *x1= *y1= *x2= *y2= 0;
  else {
    *x1= border__bx1;
    *y1= border__by1;
    *x2= border__bx2;
    *y2= border__by2;
    }
  }  // end   border_querybox()

//------------------------------------------------------------
// end Module border_   OSM border module
//------------------------------------------------------------



//------------------------------------------------------------
// Module read_   OSM stdin read module
//------------------------------------------------------------

// this module provides procedures for buffered reading of
// standard input;
// as usual, all identifiers of a module have the same prefix,
// in this case 'read'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int read__fd= 0;  // file descriptor
static int read__bufl= 0;  // length of the buffer;
  // will be calculated at first call of read_input();
static byte* read__buf= NULL;  // read buffer
static bool read__eof= false;  // we are at the end of input file

static void read__end() {
  // terminate the services of this module;
  if(read__fd>0) {
    close(read__fd); read__fd= 0; }
  if(read__buf!=NULL) {
    free(read__buf); read__buf= NULL; }
  }  // end   read__end()

//------------------------------------------------------------

static int read_prefetch= 0;  // number of bytes which will
  // be available in the buffer after every call of read_input();
  // may not be changed from external; please use read_ini()
  // if you need to determine the bytes which are to be prefetched;
static byte* read_bufe= NULL;  // may not be changed from external
static byte* read_bufp= NULL;  // may be incremented by external
  // up to the number of read_prefetch bytes before read_input() is
  // called again;
static int64_t read__counter= 0;
  // byte counter to get the read position in input file

static int read_ini(int prefetch,const char* filename) {
  // initialize this module;
  // filename[]: path and name of input file;
  //             ==NULL: standard input;
  // prefetch: number of bytes which are to be read ahead at least
  //           at every call of read_input();
  //           0: take a default value;
  // return: 0: ok; !=0: error;
  if(read__buf!=NULL)  // already initialized
return 0;  // ignore this call;
  if(filename==NULL)  // we are to read from standard input
    read__fd= 0;
  else {  // real file shall be opened
    read__fd= open(filename,O_RDONLY|O_BINARY);
    if(read__fd<0) {
      fprintf(stderr,"pbftoosm Error: could not open input file.\n");
return 1;
      }
    }
  if(prefetch<=0)  // caller wants us to take a default value
    prefetch= 10*1024*1024;
  read_prefetch= prefetch;
  read__bufl= prefetch*5;
  read__buf= (byte*)malloc(read__bufl);
  if(read__buf==NULL) {
    fprintf(stderr,"pbftoosm Error: could not get "
      "%i bytes of memory.\n",read__bufl);
return 1;
    }
  read_bufp= read_bufe= read__buf;
  atexit(read__end);
return 0;
  }  // end   read_ini()

static inline bool read_input() {
  // read data from standard input file, use an internal buffer;
  // make data available at read_bufp;
  // read_ini() must have been called before calling this procedure;
  // return: there are no (more) bytes to read;
  // read_bufp: start of next bytes available;
  //            may be incremented by the caller, up to read_bufe;
  // read_bufe: end of bytes in buffer;
  //            must not be changed by the caller;
  // after having called this procedure, the caller may rely on
  // having available at least read_prefetch bytes at address
  // read_bufp - with one exception: if there are not enough bytes
  // left to read from standard input, every byte after the end of
  // the reminding part of the file in the buffer will be set to
  // 0x00 - up to read_bufp+read_prefetch;
  int l,r;

  if(read_bufp+read_prefetch>=read_bufe) {  // read buffer is too low
    if(!read__eof) {  // still bytes in the file
      if(read_bufe>read_bufp) {  // bytes remaining in buffer
        memmove(read__buf,read_bufp,read_bufe-read_bufp);
          // move remaining bytes to start of buffer
        read_bufe= read__buf+(read_bufe-read_bufp);
          // protect the remaining bytes at buffer start
        }
      else  // no remaining bytes in buffer
        read_bufe= read__buf;  // no bytes remaining to protect
        // add read bytes to debug counter
      read_bufp= read__buf;
      do {  // while buffer has not been filled
        l= (read__buf+read__bufl)-read_bufe-4;
          // number of bytes to read
        r= read(read__fd,read_bufe,l);
        if(r<=0) {  // no more bytes in the file
          read__eof= true;  // memorize that there we are at end of file
          l= (read__buf+read__bufl)-read_bufe;
            // reminding space in buffer
          if(l>read_prefetch) l= read_prefetch;
          memset(read_bufe,l,0);
            // set reminding space up to prefetch bytes in buffer to 0
      break;
          }
        read__counter+= r;
        read_bufe+= r;  // set new mark for end of data
        read_bufe[0]= 0; read_bufe[1]= 0;  // set 4 null-terminators
        read_bufe[2]= 0; read_bufe[3]= 0;
        } while(r<l);  // end   while buffer has not been filled
      }  // end   still bytes to read
    }  // end   read buffer is too low
  return read__eof && read_bufp>=read_bufe;
  }  // end   read__input()

static inline int read_jump(bool jump) {
  // memorize the current position in the file or jump to it;
  // jump: jump to a previously stored position;
  // return: 0: OK; !=0: error;
  static off_t pos= -1;

  if(jump) {
    if(pos==-1 || lseek(read__fd,pos,SEEK_SET)<0) {
      fprintf(stderr,"pbftoosm Error: could not rewind input file.\n");
return 1;
      }
    read__counter= pos;
    read_bufp= read_bufe;  // force refetch
    read__eof= false;  // force retest for end of file
    read_input();  // ensure prefetch
    }
  else {
    pos= read__counter-(read_bufe-read_bufp);
      // get current position, take buffer pointer into account;
    }
return 0;
  }  // end   read_jump()

static uint64_t read_count() {
  // determine the number of bytes which have been read;
  return read__counter-(read_bufe-read_bufp);
  }

//------------------------------------------------------------
// end Module read_   OSM read module
//------------------------------------------------------------



//------------------------------------------------------------
// Module write_   write module
//------------------------------------------------------------

// this module provides a procedure which writes a byte to
// standard output;
// as usual, all identifiers of a module have the same prefix,
// in this case 'write'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static char write__buf[UINT64_C(16000000)];
static char* write__bufe= write__buf+sizeof(write__buf);
static char* write__bufp= write__buf;

//------------------------------------------------------------

static bool write_testmode= false;  // no standard output

static inline void write_char(int c) {
  // write one byte to stdout, use a buffer;
  if(write__bufp>=write__bufe) {  // the write buffer is full
    if(!write_testmode)
      write(1,write__buf,write__bufp-write__buf);
    write__bufp= write__buf;
    }
  *write__bufp++= (char)c;
  }  // end   write_char();

static inline void write_str(char* s) {
  // write a string to stdout, use a buffer;
  while(*s!=0) {
    if(write__bufp>=write__bufe) {  // the write buffer is full
      if(!write_testmode)
        write(1,write__buf,write__bufp-write__buf);
      write__bufp= write__buf;
      }
    *write__bufp++= (char)(*s++);
    }
  }  // end   write_str();

static inline void write_xmlstr(char* s) {
  // write an XML string to stdout, use a buffer;
  // every character which is not allowed within an XML string
  // will be replaced by the appropriate sequence;
  static byte allowedchar[]= {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,  // \"&'
    0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,  // <>
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // @ - changed
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,  // {}~DEL
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 0xa0...
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte c,i;
  #define write__char_D(c) { \
    if(write__bufp>=write__bufe) { \
      if(!write_testmode) \
        write(1,write__buf,write__bufp-write__buf); \
      write__bufp= write__buf; \
      } \
    *write__bufp++= (char)(c); }

  for(;;) {
    c= *s++;
    if(c==0)
  break;
    if(allowedchar[c]==0)
      write__char_D(c)
    else {
      write__char_D('&') write__char_D('#') //write__char_D('x')
      i= c/100; if(i) write__char_D(i+'0')
      i= c/10%10; if(i) write__char_D(i+'0')
      write__char_D(c%10+'0')
      //write__char_D("0123456789abcdef"[c/16])
      //write__char_D("0123456789abcdef"[c%16])
      write__char_D(';')
      }
    }
  #undef write__char_D
  }  // end   write_xmlstr();

static inline void write_uint32(uint32_t v) {
  // write an unsigned 32 bit integer number to standard output;
  char s[20],*s1,*s2,c;

  s1= s;
  if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
      { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_uint32()

static inline void write_sint32(int32_t v) {
  // write a signed 32 bit integer number to standard output;
  char s[20],*s1,*s2,c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sint32()

static inline void write_sint64(int64_t v) {
  // write a signed 64 bit integer number to standard output;
  static char s[30],*s1,*s2,c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sint64()

static inline void write_sfix7(int32_t v) {
  // write a signed 7 decimals fixpoint value to standard output;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 7;
  while((v%10)==0 && i>0)  // trailing zeroes
    { v/= 10;  i--; }
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix7()

static inline void write_sfix7o(int32_t v) {
  // write a signed 7 decimals fixpoint value to standard output;
  // keep trailing zeroes;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 7;
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix7o()

static inline void write_sfix9(int64_t v) {
  // write a signed 9 decimals fixpoint value to standard output;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 9;
  while(--i>=0)
    { *s2++= (v%10)+'0'; v/= 10; }
  *s2++= '.';
  do
    { *s2++= (v%10)+'0'; v/= 10; }
    while(v>0);
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  write_str(s);
  }  // end write_sfix9()

static void write_timestamp(uint64_t v) {
  // write a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z"
  time_t vtime;
  struct tm tm;
  char s[30],*sp;
  int i;

  vtime= v;
  #ifdef _WIN32
  memcpy(&tm,gmtime(&vtime),sizeof(tm));
  #else
  gmtime_r(&vtime,&tm);
  #endif
  i= tm.tm_year+1900;
  sp= s+3; *sp--= i%10+'0';
  i/=10; *sp--= i%10+'0';
  i/=10; *sp--= i%10+'0';
  i/=10; *sp= i%10+'0';
  sp+= 4; *sp++= '-';
  i= tm.tm_mon+1;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= '-';
  i= tm.tm_mday;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= 'T';
  i= tm.tm_hour;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= ':';
  i= tm.tm_min;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= ':';
  i= tm.tm_sec%60;
  *sp++= i/10+'0'; *sp++= i%10+'0'; *sp++= 'Z'; *sp= 0;
  write_str(s);
  }  // end   write_timestamp()

static inline void write_flush() {
  if(write__bufp>write__buf && !write_testmode)
      // at least one byte in buffer AND not test mode
    write(1,write__buf,write__bufp-write__buf);
  write__bufp= write__buf;
  }  // end   write_flush();

//------------------------------------------------------------
// end   Module write_   write module
//------------------------------------------------------------
  
  

//------------------------------------------------------------
// Module po_   pbf-to-osm module
//------------------------------------------------------------

// this module provides procedures which convert pbf data to
// osm data;
// the pbf data are read from standard input whereas the osm
// data will be written to standard output;
// as usual, all identifiers of a module have the same prefix,
// in this case 'po'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int po__decompress(byte* ibuf,uint isiz,byte* obuf,uint osizm,
  uint* osizp) {
  // decompress a block of data;
  // return: 0: decompression was successful;
  //         !=0: error number from zlib;
  // *osizp: size of uncompressed data;
  z_stream strm;
  int r,i;

  // initialization
  strm.zalloc= Z_NULL;
  strm.zfree= Z_NULL;
  strm.opaque= Z_NULL;
  strm.next_in= Z_NULL;
  strm.total_in= 0;
  strm.avail_out= 0;
  strm.next_out= Z_NULL;
  strm.total_out= 0;
  strm.msg= NULL;
  r= inflateInit(&strm);
  if(r!=Z_OK)
return r;

  // read data
  strm.next_in = ibuf;
  strm.avail_in= isiz;

  // decompress
  strm.next_out= obuf;
  strm.avail_out= osizm;
  r= inflate(&strm,Z_FINISH);
  if(r!=Z_OK && r!=Z_STREAM_END) {
    inflateEnd(&strm);
    *osizp= 0;
return r;
    }

  inflateEnd(&strm);
  obuf+= *osizp= osizm-(i= strm.avail_out);
  // add some zero bytes
  if(i>4) i= 4;
  while(--i>=0) *obuf++= 0;
  return 0;
  }  // end   po__decompress()

#define po__blockM (32*1024*1024)  // maximum block size
#define po__strM (4*1024*1024)
  // maximum number of strings within each block
static bool po_drophistory= false;  // exclude history information
static bool po_dropbrokenrefs= false;  // exclude broken references
static bool po_dropnodes= false;  // exclude nodes section
static bool po_dropways= false;  // exclude ways section
static bool po_droprelations= false;  // exclude relations section
static bool po_emulateosmosis= false;
  // emulate osmosis compatible output
static bool po_emulatepbf2osm= false;
  // emulate pbf2osm compatible output
static bool po_testsequence= false;  // test for sequence:
  // first, all nodes, then all way, and finally all relations;
  // within each of these groups, the ids must be ordered in
  // ascending sequence;
  // any errors in this sequence will be reported to standard output;
static int po__sequencetype= -1;
  // type of last object which has been processed;
  // -1: no object yet; 0: node; 1: way; 2: relation;
static int64_t po__sequenceid= INT64_C(-0x7fffffffffffffff);
  // id of last object which has been processed;
static int po__stage= 0;  // stage of processing;
  // only valid if borders are to be applied and relation shall be
  // processed recursively;
  // 0: no recursive processing at all;
  // 1: write nodes and ways, change stage to 2 as soon as
  //    first relation has been encountered;
  // 2: do not write anything, just process relations;
  // 3: write only relations;
static bool po__changed= false;  // only valid if po__stage==2:
  // the flag for a relation has been changed, i.e., the recursive
  // processing will continue; if none of the relations' flags
  // has been changed, the stage will increase to 3;

static int po__h(byte* buf,byte* bufe) {
  // decode compressed part of Header block
  bool osmschema,densenodes;
  byte* bp;
  uint l;
  byte* bboxe;
  int64_t minlon,maxlon,maxlat,minlat;
  int bboxflags;

  osmschema= false;
  densenodes= false;
  bboxflags= 0;
  bp= buf;
  while(bp<bufe) {  // for every element in this loop
    switch(bp[0]) {  // first byte of element
    case 0x0a:  // S 1, bbox
      bp++;
      l= pbf_uint32(&bp);
      if(l>=100) {  // group too large
        fprintf(stderr,"pbftoosm Error: bbox group too large: "
          "%u\n",l);
return 1;
        }
      bboxe= bp+l;
      while(bp<bboxe) {  // for every element in bbox
        switch(bp[0]) {  // first byte of element in bbox
        case 0x08:  // V 1, minlon
          bp++;
          minlon= pbf_sint64(&bp);
          bboxflags|= 0x01;
          break;
        case 0x10:  // V 2, maxlon
          bp++;
          maxlon= pbf_sint64(&bp);
          bboxflags|= 0x02;
          break;
        case 0x18:  // V 3, maxlat
          bp++;
          maxlat= pbf_sint64(&bp);
          bboxflags|= 0x04;
          break;
        case 0x20:  // V 4, minlat
          bp++;
          minlat= pbf_sint64(&bp);
          bboxflags|= 0x08;
          break;
        default:
          /* block */ {
            static int msgn= 3;
            if(--msgn>=0) {
              fprintf(stderr,
                "pbftoosm Warning: bbox element type unknown: "
                "0x%02X 0x%02X.\n",bp[0],bp[1]);
              }
            }
          if(pbf_jump(&bp))
return 1;
          }  // end   first byte of element
        if(bp>bboxe) {
          fprintf(stderr,"pbftoosm Error: bbox format length.\n");
return 1;
          }
        }  // end   for every element in bbox
      bp= bboxe;
      break;
    case 0x22:  // S 4, required features
      bp++;
      l= pbf_uint32(&bp);
      if(memcmp(bp-1,"\x0e""OsmSchema-V0.6",15)==0)
        osmschema= true;
      else if(memcmp(bp-1,"\x0a""DenseNodes",11)==0)
        densenodes= true;
      else {  // unsupported feature
        if(l>=31) l= 32;
        fprintf(stderr,"pbftoosm Error: unsupported feature: "
          "%.*s\n",l,bp);
return 1;
        }
      bp+= l;
      break;
    case 0x82:  // 0x01 S 16, writing program
      if(bp[1]!=0x01) goto po__h_unknown;
      bp+= 2;
      l= pbf_uint32(&bp);
      bp+= l;  // (ignore this element)
      break;
    case 0x8a:  // 0x01 S 17, source
      if(bp[1]!=0x01) goto po__h_unknown;
      bp+= 2;
      l= pbf_uint32(&bp);
      bp+= l;  // (ignore this element)
      break;
    default:
    po__h_unknown:  
      /* block */ {
        static int msgn= 3;
        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: header block element type unknown: "
            "0x%02X 0x%02X.\n",bp[0],bp[1]);
          }
        }
      if(pbf_jump(&bp))
return 1;
      }  // end   first byte of element
    if(bp>bufe) {
      fprintf(stderr,"pbftoosm Error: header block format length.\n");
return 1;
      }
    }  // end   for every element in this loop
  if(!osmschema) {
    fprintf(stderr,"pbftoosm Error: expected feature: "
      "OsmSchema-V0.6\n");
return 1;
    }
  if(!densenodes) {
    fprintf(stderr,"pbftoosm Error: expected feature: "
      "DenseNodes\n");
return 1;
    }
  write_str("<?xml version=\'1.0\' encoding=\'UTF-8\'?>"NL);
  if(po_emulateosmosis)
    write_str("<osm version=\"0.6\" generator=\"Osmosis 0.39\">"NL);
  else if(po_emulatepbf2osm)
    write_str("<osm version=\"0.6\" generator=\"pbf2osm\">"NL);
  else
    write_str("<osm version=\"0.6\" generator=\"pbftoosm "
      VERSION"\">"NL);
  if(po_emulatepbf2osm)
    ;
  else if(border_active) {  // borders are to be applied
    int32_t x1,y1,x2,y2;

    border_querybox(&x1,&y1,&x2,&y2);
    write_str("\t<bounds minlat=\""); write_sfix7(y1);
    write_str("\" minlon=\""); write_sfix7(x1);
    write_str("\" maxlat=\""); write_sfix7(y2);
    write_str("\" maxlon=\""); write_sfix7(x2);
    write_str("\"/>"NL);
    }
  else if(bboxflags==0x0f) {  // bbox is complete
    write_str("\t<bounds minlat=\""); write_sfix9(minlat);
    write_str("\" minlon=\""); write_sfix9(minlon);
    write_str("\" maxlat=\""); write_sfix9(maxlat);
    write_str("\" maxlon=\""); write_sfix9(maxlon);
    write_str("\"/>"NL);
    }
  return 0;
  }  // end   po__h()

static int po__d_pg_dn(byte* buf,byte* bufe,char** str,uint strm) {
  // decode dense node part of primitive group of Data block
  byte* bp;
  uint l;
  byte* bhise;  // end of history section in buf[]
  byte* nodeid,*nodeide;  // start and end of node ids
  byte* nodever,*nodevere;  // start and end of version
  byte* nodetime,*nodetimee;  // start and end of times
  byte* nodecset,*nodecsete;  // start and end of change sets
  byte* nodeuid,*nodeuide;  // start and end of user ids
  byte* nodeuser,*nodeusere;  // start and end of user names
  byte* nodelat,*nodelate;  // start and end of node latitudes
  byte* nodelon,*nodelone;  // start and end of node longitudes
  byte* nodetags,*nodetagse;  // start and end of node tag pairs
  int64_t id;  // (delta coded)
  int32_t lat,lon;  // (delta coded)
  int64_t histime;  // time (dalta coded)
  int32_t hiscset;  // changeset (delta coded)
  uint32_t hisuid;  // uid (delta coded)
  uint32_t hisuser;  // string index of user name (delta coded)
  uint key,val;

  // place pointers at every first element
  // of the supplied data tables
  nodeid= nodeide= NULL;
  nodever= nodevere= NULL;
  nodetime= nodetimee= NULL;
  nodecset= nodecsete= NULL;
  nodeuid= nodeuide= NULL;
  nodeuser= nodeusere= NULL;
  nodelat= nodelate= NULL;
  nodelon= nodelone= NULL;
  nodetags= nodetagse= NULL;
  bp= buf;
  while(bp<bufe) {  // for every element in this loop
    switch(bp[0]) {  // first byte of element
    case 0x0a:  // S 1, ids
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,"pbftoosm Error: node id table too large: "
          "%u\n",l);
return 1;
        }
      nodeid= bp;
      nodeide= (bp+= l);
      break;
    case 0x2a:  // S 5, history - with subelements
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: node history section too large: "
          "%u\n",l);
return 1;
        }
      if(po_drophistory) {  // history information is not required
        bp+= l;  // jump over this section and ignore it
        break;
        }
      bhise= bp+l;
      while(bp<bhise) {  // for each history subelement
        switch(bp[0]) {  // first byte of element in history section
        case 0x0a:  // S 1, versions
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bhise) {
            fprintf(stderr,
              "pbftoosm Error: node version table too large: "
              "%u\n",l);
return 1;
            }
          nodever= bp;
          nodevere= (bp+= l);
          break;
        case 0x12:  // S 2, times
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bhise) {
            fprintf(stderr,
              "pbftoosm Error: node time table too large: "
              "%u\n",l);
return 1;
            }
          nodetime= bp;
          nodetimee= (bp+= l);
          break;
        case 0x1a:  // S 3, change sets
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bhise) {
            fprintf(stderr,
              "pbftoosm Error: node change set table too large: "
              "%u\n",l);
return 1;
            }
          nodecset= bp;
          nodecsete= (bp+= l);
          break;
        case 0x22:  // S 4, user ids
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bhise) {
            fprintf(stderr,
              "pbftoosm Error: node user id table too large: "
              "%u\n",l);
return 1;
            }
          nodeuid= bp;
          nodeuide= (bp+= l);
          break;
        case 0x2a:  // S 5, user names
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bhise) {
            fprintf(stderr,
              "pbftoosm Error: node user name table too large: "
              "%u\n",l);
return 1;
            }
          nodeuser= bp;
          nodeusere= (bp+= l);
          break;
        default:
          /* block */ {
            static int msgn= 3;
            if(--msgn>=0) {
              fprintf(stderr,
                "pbftoosm Warning: node history element type unknown: "
                "0x%02X 0x%02X.\n",bp[0],bp[1]);
              }
            }
          if(pbf_jump(&bp))
return 1;
          }  // end   first byte of element
        }  // end   for each history subelement
      if(bp>bhise) {
        fprintf(stderr,"pbftoosm Error: node history format length.\n");
return 1;
        }
      bp= bhise;
      break;  // end   history - with subelements
    case 0x42:  // S 8, latitudes
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: node latitude table too large: "
          "%u\n",l);
return 1;
        }
      nodelat= bp;
      nodelate= (bp+= l);
      break;
    case 0x4a:  // S 9, longitudes
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: node longitude table too large: "
          "%u\n",l);
return 1;
        }
      nodelon= bp;
      nodelone= (bp+= l);
      break;
    case 0x52:  // S 10, tag pairs
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: node tag pair table too large: "
          "%u\n",l);
return 1;
        }
      nodetags= bp;
      nodetagse= (bp+= l);
      break;
    default:
      /* block */ {
        static int msgn= 3;
        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: dense node element type unknown: "
            "0x%02X 0x%02X.\n",bp[0],bp[1]);
          }
        }
      if(pbf_jump(&bp))
return 1;
      }  // end   first byte of element
    if(bp>bufe) {
      fprintf(stderr,"pbftoosm Error: dense node format length.\n");
return 1;
      }
    }  // end   for every element in this loop

  // write all the supplied data to standard output
  id= 0;
  lat= lon= 0;
  histime= 0;
  hiscset= 0;
  hisuid= hisuser= 0;
  if(nodeid!=NULL && nodelat!=NULL && nodelon!=NULL) {
      // minimum contents available
    while(nodeid<nodeide && nodelat<nodelate &&
        nodelon<nodelone) {  // still nodes to write
      // get node data
      id+= pbf_sint64(&nodeid);
      lat+= pbf_sint32(&nodelat);
      lon+= pbf_sint32(&nodelon);

      // check sequence, if necessary
      if(po_testsequence) {
        static int64_t oldid= INT64_C(-0x7fffffffffffffff);
        static int msgn= 3;

        if((po__sequencetype>0 || id<=oldid) && --msgn>=0)
          fprintf(stderr,"pbftoosm Warning: wrong sequence at "
            "node %"PRIi64".\n",id);
        oldid= id;
        }
      po__sequencetype= 0; po__sequenceid= id;

      // write the node
      if(border_active && !border_queryinside(lon,lat)) {
          // node lies outside a predefined border
        // jump over unused node information
        if(nodever<nodevere && nodetime<nodetimee &&
            nodecset<nodecsete && nodeuid<nodeuide &&
            nodeuser<nodeusere) {
            // history information available
          pbf_intjump(&nodever);
          hiscset+= pbf_sint32(&nodecset);
          hisuser+= pbf_sint32(&nodeuser);
          hisuid+= pbf_sint32(&nodeuid);
          histime+= pbf_sint64(&nodetime);
          }  // end   history information available
        if(nodetags<nodetagse && (key= pbf_uint32(&nodetags))!=0) {
            // there are key/val pairs
          do {  // for every key/val pair
            pbf_intjump(&nodetags);
            key= pbf_uint32(&nodetags);
            } while(key!=0);  // end   for every key/val pair
          }  // end   there are key/val pairs
        }  // end   node lies outside a predefined border
      else {  // no border to be applied OR node lies inside
        if(border_active)  // border are to be applied
          hash_seti(0,id);  // mark this node id as 'inside'
        if(!po_dropnodes) {  // not to drop
          if(po_emulateosmosis) {  // emulate osmosis
            write_str("  <node id=\""); write_sint64(id);
            if(nodever<nodevere && nodetime<nodetimee &&
                nodecset<nodecsete && nodeuid<nodeuide &&
                nodeuser<nodeusere) {
                // history information available
              write_str("\" version=\"");
              write_uint32(pbf_uint32(&nodever));
              histime+= pbf_sint64(&nodetime);
              write_str("\" timestamp=\""); write_timestamp(histime);
              hisuser+= pbf_sint32(&nodeuser);
              if(hisuser>=strm) {
                fprintf(stderr,
                  "pbftoosm Error: node user string index overflow: "
                  "%u->%u\n",strm,hisuser);
    return 1;
                }  // end   user name available
              hisuid+= pbf_sint32(&nodeuid);
              if(hisuser!=0 && str[hisuser][0]!=0) {
                  // user name available
                write_str("\" uid=\""); write_uint32(hisuid);
                write_str("\" user=\""); write_xmlstr(str[hisuser]);
                }
              hiscset+= pbf_sint32(&nodecset);
              write_str("\" changeset=\""); write_uint32(hiscset);
              }  // end   history information available
            write_str("\" lat=\""); write_sfix7(lat);
            write_str("\" lon=\""); write_sfix7(lon);
            }  // end   emulate osmosis
          else {  // not emulate osmosis
            write_str("\t<node id=\""); write_sint64(id);
            if(po_emulatepbf2osm) {
              write_str("\" lat=\""); write_sfix7o(lat);
              write_str("\" lon=\""); write_sfix7o(lon);
              }
            else {
              write_str("\" lat=\""); write_sfix7(lat);
              write_str("\" lon=\""); write_sfix7(lon);
              }
            if(nodever<nodevere && nodetime<nodetimee &&
                nodecset<nodecsete && nodeuid<nodeuide &&
                nodeuser<nodeusere) {
                // history information available
              write_str("\" version=\"");
              write_uint32(pbf_uint32(&nodever));

              hiscset+= pbf_sint32(&nodecset);
              write_str("\" changeset=\""); write_uint32(hiscset);

              hisuser+= pbf_sint32(&nodeuser);
              if(hisuser>=strm) {
                fprintf(stderr,
                  "pbftoosm Error: node user string index overflow: "
                  "%u->%u\n",strm,hisuser);
    return 1;
                }  // end   user name available
              hisuid+= pbf_sint32(&nodeuid);
              if(hisuser!=0 && str[hisuser][0]!=0) {
                  // user name available
                write_str("\" user=\""); write_xmlstr(str[hisuser]);
                write_str("\" uid=\""); write_uint32(hisuid);
                }
              histime+= pbf_sint64(&nodetime);
              write_str("\" timestamp=\""); write_timestamp(histime);
              }  // end   history information available
            }  // end   not emulate osmosis
          if(nodetags>=nodetagse || (key= pbf_uint32(&nodetags))==0)
              // no (more) tag pairs
            write_str("\"/>"NL);  // make node tag a short tag
          else {  // there are key/val pairs
            write_str("\">"NL);  // do not terminate node tag
            do {  // for every key/val pair
              val= pbf_uint32(&nodetags);
              if(key>=strm || val>=strm) {
                fprintf(stderr,
                  "pbftoosm Error: key/val string index overflow: "
                  "%u->%u,%u\n",strm,key,val);
  return 1;
                }
              write_str("\t\t<tag k=\""); write_xmlstr(str[key]);
              write_str("\" v=\""); write_xmlstr(str[val]);
              write_str("\" />"NL);
              key= pbf_uint32(&nodetags);
              } while(key!=0);  // end   for every key/val pair
            write_str("\t</node>"NL);  // terminate node tag
            }  // end   there are key/val pairs

          }  // end   not to drop

        }  // end   no border to be applied OR node lies inside
      }  // end   still nodes to write
    }  // minimum contents available
  return 0;
  }  // end   po__d_pg_dn()

static int po__d_pg_ways(byte* buf,byte* bufe,char** str,uint strm) {
  // ways-part of primitive group of Data block
  byte* bp;
  uint l;
  byte* bhise;  // end of history section in buf[]
  int64_t id;
  byte* waynode,*waynodee;  // start and end of node refs
  byte* waykey,*waykeye;  // start and end of keys
  byte* wayval,*wayvale;  // start and end of vals
  uint32_t hisver;
  int64_t histime;
  uint32_t hiscset;
  uint32_t hisuid;
  uint32_t hisuser;
  int complete;  // flags which determine if the dataset is complete
  int hiscomplete;  // flags which determine if the history is complete

  bp= buf;
  waynode= waynodee= NULL;
  waykey= waykeye= NULL;
  wayval= wayvale= NULL;
  complete= hiscomplete= 0;
  while(bp<bufe) {  // for every element in this loop
    switch(bp[0]) {  // first byte of element
    case 0x08:  // V 1, id
      bp++;
      id= pbf_uint64(&bp);
      complete|= 1;
      break;
    case 0x12:  // S 2, keys
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: way key table too large: "
          "%u\n",l);
return 1;
        }
      waykey= bp;
      waykeye= (bp+= l);
      complete|= 2;
      break;
    case 0x1a:  // S 3, vals
      bp++;
      l= pbf_uint32(&bp);
      /* deal with strange S 3 element at data set end */ {
        if(complete & (4|16)) {  // already have vals or node refs
fprintf(stderr,"pbftoosm Warning: format 0x1a found: %02X\n",complete);
//,,,
          break;  // ignore this element
          }
        }
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: way val table too large: "
          "%u\n",l);
return 1;
        }
      wayval= bp;
      wayvale= (bp+= l);
      complete|= 4;
      break;
    case 0x22:  // S 4, history - with subelements
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: way history section too large: "
          "%u\n",l);
return 1;
        }
      if(po_drophistory) {  // history information is not required
        bp+= l;  // jump over this section and ignore it
        break;
        }
      bhise= bp+l;
      while(bp<bhise) {  // for each history subelement
        switch(bp[0]) {  // first byte of element in history section
        case 0x08:  // V 1, version
          bp++;
          hisver= pbf_uint32(&bp);
          hiscomplete|= 1;
          break;
        case 0x10:  // V 2, timestamp
          bp++;
          histime= pbf_uint64(&bp);
          hiscomplete|= 2;
          break;
        case 0x18:  // V 3, cset
          bp++;
          hiscset= pbf_uint32(&bp);
          hiscomplete|= 4;
          break;
        case 0x20:  // V 4, uid
          bp++;
          hisuid= pbf_uint32(&bp);
          hiscomplete|= 8;
          break;
        case 0x28:  // V 5, user
          bp++;
          hisuser= pbf_uint32(&bp);
          hiscomplete|= 16;
          break;
        default:
          /* block */ {
            static int msgn= 3;
            if(--msgn>=0) {
              fprintf(stderr,
                "pbftoosm Warning: way history element type unknown: "
                "0x%02X 0x%02X.\n",bp[0],bp[1]);
              }
            }
          if(pbf_jump(&bp))
return 1;
          }  // end   first byte of element
        }  // end   for each history subelement
      if(bp>bhise) {
        fprintf(stderr,"pbftoosm Error: way history format length.\n");
return 1;
        }
      bp= bhise;
      complete|= 8;
      break;  // end   history - with subelements
    case 0x42:  // S 8, node refs
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: way noderef table too large: "
          "%u\n",l);
return 1;
        }
      waynode= bp;
      waynodee= (bp+= l);
      complete|= 16;
      break;
    default:
      /* block */ {
        static int msgn= 3;
        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: way element type unknown: "
            "0x%02X 0x%02X 0x%02X 0x%02X + %i.\n",
            bp[0],bp[1],bp[2],bp[3],complete);
          }
        }
      if(pbf_jump(&bp))
return 1;
      }  // end   first byte of element
    if(bp>bufe) {
      fprintf(stderr,"pbftoosm Error: way format length.\n");
return 1;
      }

    // write supplied data of this way to standard output
    if((complete & 17)==17) {  // have at least id and node refs
      int64_t noderef;
      bool inside;  // way lies inside borders, if appl.
      int64_t reftab[50000];  // table for temporarily stored noderefs
      int64_t* reftabp,*reftabe,*reftabee;
      byte* waynode_dyn;

      // check sequence, if necessary
      if(po_testsequence) {
        static int64_t oldid= INT64_C(-0x7fffffffffffffff);
        static int msgn= 3;

        if((po__sequencetype>1 || id<=oldid) && --msgn>=0)
          fprintf(stderr,"pbftoosm Warning: wrong sequence at "
            "way %"PRIi64".\n",id);
        oldid= id;
        }
      po__sequencetype= 1; po__sequenceid= id;

      reftabp= reftabe= reftab;
      noderef= 0;
      if(!border_active)  // no borders shall be applied
        inside= true;
      else {  // borders are to be applied
        reftabee= (int64_t*)(((char*)reftab)+sizeof(reftab));
        inside= false;  // (default)
        reftabe= reftab;
        noderef= 0;
        while(waynode<waynodee && reftabe<reftabee) {
            // still node refs AND space in reftab[]
          noderef+= pbf_sint64(&waynode);
          *reftabe++= noderef;
          if(hash_geti(0,noderef)) {

//{static int msgn=3; if(--msgn>=0) fprintf(stderr,"X %lli\n",id);}
            inside= true; goto po__d_pg_ways_inside; }
          }
        waynode_dyn= waynode;
        while(waynode_dyn<waynodee) {
            // still node refs which did not fit into reftab[]
          noderef+= pbf_sint64(&waynode_dyn);
          if(hash_geti(0,noderef)) {
//{static int msgn=3; if(--msgn>=0) fprintf(stderr,"Y %lli\n",id);}
            inside= true; goto po__d_pg_ways_inside; }
          }
        po__d_pg_ways_inside: ;
        }  // end   borders are to be applied
      if(inside) {
          // no borders OR at least one node inside
        if(border_active)
          hash_seti(1,id);  // memorize that this way lies inside
        if(!po_dropways) {  // not ways to drop
          write_str("\t<way id=\"");
          write_sint64(id);
          if((hiscomplete&7)==7) {  // history information is complete
            write_str("\" version=\""); write_uint32(hisver);
            write_str("\" changeset=\""); write_uint32(hiscset);
            if((hiscomplete&24)==24 && str[hisuser][0]!=0) {
                // user name available
              if(hisuser>=strm) {
                if(hisuser==32767)
                  hisuser= 0;
                else {
                  fprintf(stderr,
                    "pbftoosm Error: way user string index overflow: "
                    "%u->%u\n",strm,hisuser);
return 1;
                  }
                }
              write_str("\" uid=\""); write_uint32(hisuid);
              write_str("\" user=\""); write_xmlstr(str[hisuser]);
              }
            write_str("\" timestamp=\""); write_timestamp(histime);
            }  // end   history shall be written
          write_str("\">"NL);
          noderef= 0;
          while(reftabp<reftabe) {  // still node refs in reftab[]
            noderef= *reftabp++;
            if(!po_dropbrokenrefs || hash_geti(0,noderef)) {
                // referenced node lies inside the borders
              write_str("\t\t<nd ref=\""); write_sint64(noderef);
              write_str("\"/>"NL);
              }  // end   referenced node lies inside the borders
            }
          while(waynode<waynodee) {  // still node refs to write
            noderef+= pbf_sint64(&waynode);
            if(!po_dropbrokenrefs || hash_geti(0,noderef)) {
                // referenced node lies inside the borders
              write_str("\t\t<nd ref=\""); write_sint64(noderef);
              write_str("\"/>"NL);
              }  // end   referenced node lies inside the borders
            }
          while(waykey<waykeye && wayval<wayvale) {
              // still key/val pairs to write
            uint key,val;

            key= pbf_uint32(&waykey);
            val= pbf_uint32(&wayval);
            if(key>=strm || val>=strm) {
              fprintf(stderr,
                "pbftoosm Error: way key/val string index overflow: "
                "%u->%u,%u\n",strm,key,val);
return 1;
              }
            write_str("\t\t<tag k=\""); write_xmlstr(str[key]);
            write_str("\" v=\""); write_xmlstr(str[val]);
            write_str("\" />"NL);
            }
          write_str("\t</way>"NL);
          }  // end   not ways to drop
        }  // end   no border OR at least one node inside
      complete= hiscomplete= 0;
      }  // end   write supplied data of this way to standard output
    }  // end   for every element in this loop
  return 0;
  }  // end   po__d_pg_ways()

static int po__d_pg_rels(byte* buf,byte* bufe,char** str,uint strm) {
  // rels-part of primitive group of Data block
  byte* bp;
  uint l;
  byte* bhise;  // end of history section in buf[]
  int64_t id;
  byte* relkey,*relkeye;  // start and end of keys
  byte* relval,*relvale;  // start and end of vals
  byte* relrefrole,*relrefrolee;  // start and end of ref roles
  byte* relrefid,*relrefide;  // start and end of ref ids
  byte* relreftype,*relreftypee;  // start and end of ref types
  uint32_t hisver;
  int64_t histime;
  uint32_t hiscset;
  uint32_t hisuid;
  uint32_t hisuser;
  int complete;  // flags which determine if the dataset is complete
  int hiscomplete;  // flags which determine if the history is complete

//write_flush(); write_testmode= false;
  bp= buf;
  relkey= relkeye= NULL;
  relval= relvale= NULL;
  relrefrole= relrefrolee= NULL;
  relrefid= relrefide= NULL;
  relreftype= relreftypee= NULL;
  complete= hiscomplete= 0;
  while(bp<bufe) {  // for every element in this loop
    switch(bp[0]) {  // first byte of element
    case 0x08:  // V 1, id
      bp++;
      id= pbf_uint64(&bp);
      complete|= 1;
      break;
    case 0x12:  // S 2, keys
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rel key table too large: "
          "%u\n",l);
return 1;
        }
      relkey= bp;
      relkeye= (bp+= l);
      complete|= 2;
      break;
    case 0x1a:  // S 3, vals
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rel val table too large: "
          "%u\n",l);
return 1;
        }
      relval= bp;
      relvale= (bp+= l);
      complete|= 4;
      break;
    case 0x22:  // S 4, history - with subelements
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rel history section too large: "
          "%u\n",l);
return 1;
        }
      if(po_drophistory) {  // history information is not required
        bp+= l;  // jump over this section and ignore it
        break;
        }
      bhise= bp+l;
      while(bp<bhise) {  // for each history subelement
        switch(bp[0]) {  // first byte of element in history section
        case 0x08:  // V 1, version
          bp++;
          hisver= pbf_uint32(&bp);
          hiscomplete|= 1;
          break;
        case 0x10:  // V 2, timestamp
          bp++;
          histime= pbf_uint64(&bp);
          hiscomplete|= 2;
          break;
        case 0x18:  // V 3, cset
          bp++;
          hiscset= pbf_uint32(&bp);
          hiscomplete|= 4;
          break;
        case 0x20:  // V 4, uid
          bp++;
          hisuid= pbf_uint32(&bp);
          hiscomplete|= 8;
          break;
        case 0x28:  // V 5, user
          bp++;
          hisuser= pbf_uint32(&bp);
          hiscomplete|= 16;
          break;
        default:
          /* block */ {
            static int msgn= 3;
            if(--msgn>=0) {
              fprintf(stderr,
                "pbftoosm Warning: rel history element type unknown: "
                "0x%02X 0x%02X.\n",bp[0],bp[1]);
              }
            }
          if(pbf_jump(&bp))
return 1;
          }  // end   first byte of element
        }  // end   for each history subelement
      if(bp>bhise) {
        fprintf(stderr,"pbftoosm Error: rel history format length.\n");
return 1;
        }
      bp= bhise;
      complete|= 8;
      break;  // end   history - with subelements
    case 0x42:  // S 8, refroles
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rel role table too large: "
          "%u\n",l);
return 1;
        }
      relrefrole= bp;
      relrefrolee= (bp+= l);
      complete|= 16;
      break;
    case 0x4a:  // S 9, refids
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rel id table too large: "
          "%u\n",l);
return 1;
        }
      relrefid= bp;
      relrefide= (bp+= l);
      complete|= 32;
      break;
    case 0x52:  // S 10, reftypes
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rel type table too large: "
          "%u\n",l);
return 1;
        }
      relreftype= bp;
      relreftypee= (bp+= l);
      complete|= 64;
      break;
    default:
      /* block */ {
        static int msgn= 3;
        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: rel element type unknown: "
            "0x%02X 0x%02X 0x%02X 0x%02X + %i.\n",
            bp[0],bp[1],bp[2],bp[3],complete);
          }
        }
      if(pbf_jump(&bp))
return 1;
      }  // end   first byte of element
    if(bp>bufe) {
      fprintf(stderr,"pbftoosm Error: rel format length.\n");
return 1;
      }

    // write supplied data of this relation to standard output
    if((complete & 113)==113) {
        // have at least id and refs (1|16|32|64)
      int64_t refid;
      bool inside;  // relation lies inside borders, if appl.
      int64_t reftab[50000];  // table for temporarily stored noderefs
      byte reftypetab[50000];  // table for temp. stored noderef types;
        // must have the same maximum number of elements as reftab[];
      int64_t* reftabp,*reftabe,*reftabee;
      byte* reftypetabp,*reftypetabe;
      byte* relrefid_dyn,*relreftype_dyn;

      // check sequence, if necessary
      if(po_testsequence) {
        static int64_t oldid= INT64_C(-0x7fffffffffffffff);
        static int msgn= 3;

        if((po__sequencetype>2 || id<=oldid) && --msgn>=0)
          fprintf(stderr,"pbftoosm Warning: wrong sequence at "
            "relation %"PRIi64".\n",id);
        oldid= id;
        }
      po__sequencetype= 2; po__sequenceid= id;
      reftabp= reftabe= reftab;
      reftypetabp= reftypetab;
      refid= 0;
      if(!border_active || hash_geti(2,id))
          // no borders shall be applied OR this relation
          // has been determined to lie inside the borders
        inside= true;
      else {  // borders are to be applied
        reftabee= (int64_t*)(((char*)reftab)+sizeof(reftab));
        reftabe= reftab;
        reftypetabe= reftypetab;

        refid= 0;
//if(id==1070811) fprintf(stderr,"X %lli %lli\n",id,refid);
        while(relrefid<relrefide && relreftype<relreftypee &&
            reftabe<reftabee) {  // still refs AND space in reftab[]
          int reftype;

          refid+= pbf_sint64(&relrefid);
          *reftabe++= refid;
          reftype= pbf_uint32(&relreftype);
          *reftypetabe++= reftype;
          if((po__stage==0 && reftype==2 && refid>=id) ||
              hash_geti(reftype,refid))
              // referenced relation has higher id
              // (i.e., we do not know if that relation lies inside
              // because we did not inspect it yet, therefore we
              // decide to keep the presently processed relation)
              // OR referenced object lies inside
            goto po__d_pg_refs_inside;
          }  // end   still refs AND space in reftab[]
        relrefid_dyn= relrefid;
        relreftype_dyn= relreftype;
        while(relrefid_dyn<relrefide && relreftype_dyn<relreftypee) {
            // still refs which did not fit into reftab[]
          int reftype;

          refid+= pbf_sint64(&relrefid_dyn);
          reftype= pbf_uint32(&relreftype_dyn);
          if((po__stage==0 && reftype==2 && refid>=id) ||
              hash_geti(reftype,refid))
              // referenced relation has higher id (see above)
              // OR referenced object lies inside
            goto po__d_pg_refs_inside;
          }  // end   still refs which did not fit into reftab[]
        inside= false;
        goto po__d_pg_refs_outside;
        po__d_pg_refs_inside: ;
        inside= true;
        hash_seti(2,id);
          // memorize that this relation lies inside the borders
        po__changed= true;
          // memorize that the flag has been changed from 0 to 1;
        po__d_pg_refs_outside: ;
        }  // end   borders are to be applied
      if(inside) {  // no borders OR at least one node inside
        write_str("\t<relation id=\"");
        write_sint64(id);
        if(!po_drophistory && (hiscomplete&7)==7) {
            // history shall be written to standard output AND
            // history information is complete
          write_str("\" version=\""); write_uint32(hisver);
          write_str("\" changeset=\""); write_uint32(hiscset);
          if((hiscomplete & 24)==24 && str[hisuser][0]!=0) {
              // user name available
            if(hisuser>=strm) {
              fprintf(stderr,
                "pbftoosm Error: rel user string index overflow: "
                "%u->%u\n",strm,hisuser);
return 1;
              }
            write_str("\" uid=\""); write_uint32(hisuid);
            write_str("\" user=\""); write_xmlstr(str[hisuser]);
            }
          write_str("\" timestamp=\""); write_timestamp(histime);
          }  // end   history shall be written
        write_str("\">"NL);
        refid= 0;
        while((reftabp<reftabe ||
            (relrefid<relrefide && relreftype<relreftypee)) &&
            relrefrole<relrefrolee) {  // still refs to write
          int reftype,refrole;

          if(reftabp<reftabe) {  // still node refs in reftab[]
            refid= *reftabp++;
            reftype= *reftypetabp++;
            }
          else {
            refid+= pbf_sint64(&relrefid);
            reftype= pbf_uint32(&relreftype);
            }
          refrole= pbf_uint32(&relrefrole);
          if(refrole>=strm) {
            fprintf(stderr,
              "pbftoosm Error: rel refrole string index overflow: "
              "%u->%u\n",strm,refrole);
return 1;
            }
          if(!po_dropbrokenrefs || hash_geti(reftype,refid)) {
              // referenced object lies inside the borders
            if(reftype==0)
              write_str("\t\t<member type=\"node\" ref=\"");
            else if(reftype==1)
              write_str("\t\t<member type=\"way\" ref=\"");
            else
              write_str("\t\t<member type=\"relation\" ref=\"");
            write_sint64(refid);
            write_str("\" role=\""); write_xmlstr(str[refrole]);
            write_str("\"/>"NL);
            }  // end   referenced object lies inside the borders
          }  // end   still refs to write
        while(relkey<relkeye && relval<relvale) {
            // still key/val pairs to write
          uint key,val;

          key= pbf_uint32(&relkey);
          val= pbf_uint32(&relval);
          if(key>=strm || val>=strm) {
            fprintf(stderr,
              "pbftoosm Error: rel key/val string index overflow: "
              "%u->%u,%u\n",strm,key,val);
return 1;
            }
          write_str("\t\t<tag k=\""); write_xmlstr(str[key]);
          write_str("\" v=\""); write_xmlstr(str[val]);
          write_str("\" />"NL);
          }
        write_str("\t</relation>"NL);
        }  // end   no borders OR at least one node inside
      complete= hiscomplete= 0;
      }  // end   write supplied data of this relation to standard output
    }  // end   for every element in this loop
#if 0
fprintf(stderr,"X %p %p %p %u\n",buf,bufe,str,strm);
int i;
i= 3000;
while(--i>0 && bp<bufe) putchar(*bp++);
static int j=3;
if(--j<0) exit(0);
#endif
#if 0
int i;
i= 0;
do printf("str[%2i]: %s\n",i,str[i]);
while(++i<=40 && i<strm);
exit(0);
#endif
  return 0;
  }  // end   po__d_pg_rels()

static int po__d_pg_rels_analyze(byte* buf,byte* bufe,char** str,uint strm) {
  // analyze rels-part of primitive group of Data block;
  // same as preceding procedure po__d_pg_rels(), but no writing
  // is done; relation data are decoded and analyzed to get
  // information about which relations lie inside the borders;
  byte* bp;
  uint l;
  int64_t id;
  byte* relrefid,*relrefide;  // start and end of ref ids
  byte* relreftype,*relreftypee;  // start and end of ref types
  int complete;  // flags which determine if the dataset is complete

  bp= buf;
  relrefid= relrefide= NULL;
  relreftype= relreftypee= NULL;
  complete= 0;
  while(bp<bufe) {  // for every element in this loop
    switch(bp[0]) {  // first byte of element
    case 0x08:  // V 1, id
      bp++;
      id= pbf_uint64(&bp);
      complete|= 1;
      break;
    case 0x12:  // S 2, keys
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rela key table too large: "
          "%u\n",l);
return 1;
        }
      bp+= l;  // jump over this section and ignore it
      complete|= 2;
      break;
    case 0x1a:  // S 3, vals
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rela val table too large: "
          "%u\n",l);
return 1;
        }
      bp+= l;  // jump over this section and ignore it
      complete|= 4;
      break;
    case 0x22:  // S 4, history - with subelements
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rela history section too large: "
          "%u\n",l);
return 1;
        }
      bp+= l;  // jump over this section and ignore it
      complete|= 8;
      break;  // end   history - with subelements
    case 0x42:  // S 8, refroles
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rela role table too large: "
          "%u\n",l);
return 1;
        }
      bp+= l;  // jump over this section and ignore it
      complete|= 16;
      break;
    case 0x4a:  // S 9, refids
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rela id table too large: "
          "%u\n",l);
return 1;
        }
      relrefid= bp;
      relrefide= (bp+= l);
      complete|= 32;
      break;
    case 0x52:  // S 10, reftypes
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,
          "pbftoosm Error: rela type table too large: "
          "%u\n",l);
return 1;
        }
      relreftype= bp;
      relreftypee= (bp+= l);
      complete|= 64;
      break;
    default:
      /* block */ {
        static int msgn= 3;
        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: rela element type unknown: "
            "0x%02X 0x%02X 0x%02X 0x%02X + %i.\n",
            bp[0],bp[1],bp[2],bp[3],complete);
          }
        }
      if(pbf_jump(&bp))
return 1;
      }  // end   first byte of element
    if(bp>bufe) {
      fprintf(stderr,"pbftoosm Error: rela format length.\n");
return 1;
      }

    // analyze supplied data of this relation
    if((complete & 113)==113) {
        // have at least id and refs (1|16|32|64)
      if(!hash_geti(2,id)) {  // not already marked as 'inside'
        int64_t refid;
        int reftype;

        refid= 0;
        while(relrefid<relrefide && relreftype<relreftypee) {
            // still refs
          refid+= pbf_sint64(&relrefid);
          reftype= pbf_uint32(&relreftype);
          if(hash_geti(reftype,refid)) {
              // referenced object lies inside
            hash_seti(2,id);
              // memorize that this relation lies inside the borders
            po__changed= true;
              // memorize that the flag has been changed from 0 to 1;
        break;
            }
          }  // end   still refs
        }  // end   not already marked as 'inside'
      complete= 0;
      }  // end   analyze supplied data of this relation
    }  // end   for every element in this loop
  return 0;
  }  // end   po__d_pg_rels_analyze()

static int po__d(byte* buf,byte* bufe) {
  // decode compressed part of Data block
  byte* bp;
  static char* str[po__strM];  // string table
  char** stre;  // end of data in str[]
  char** stree;  // end of str[]
  byte* bstre;  // end of string table in buf[]
  byte* bpge;  // end of primitive group in buf[]
  uint l;

  bp= buf;
  stree= (stre= str)+po__strM;
  while(bp<bufe) {  // for every element in this loop
    switch(bp[0]) {  // first byte of element
    case 0x0a:  // S 1, string table
      bp++;
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,"pbftoosm Error: string table too large: "
          "%u\n",l);
return 1;
        }
      bstre= bp+l;
      while(bp<bstre) {  // for each element in string table
        if(bp[0]==0x0a) {  // S 1, string
          *bp++= 0;  // set null terminator for previous string
          l= pbf_uint32(&bp);
          if(bp+l>bstre) {  // string too large
            fprintf(stderr,"pbftoosm Error: string too large: "
              "%u\n",l);
return 1;
            }
          if(stre>=stree) {
            fprintf(stderr,"pbftoosm Error: too many strings: "
              "%i\n",po__strM);
return 1;
            }
          *stre++= (char*)bp;
          bp+= l;
          }  // end   S 1, string
        else {  // element type unknown
          byte* p;

          /* block */ {
            static int msgn= 3;
            if(--msgn>=0) {
              fprintf(stderr,
                "pbftoosm Warning: string table element type unknown: "
                "0x%02X 0x%02X.\n",bp[0],bp[1]);
              }
            }
          p= bp;
          if(pbf_jump(&bp))
return 1;
          *p= 0;  // set null terminator for previous string
          }  // end   element type unknown
        }  // end   for each element in string table
      bp= bstre;
      break;
    case 0x12:  // S 2, primitive group
      *bp++= 0;  // set null terminator for previous string
      l= pbf_uint32(&bp);
      if(bp+l>bufe) {
        fprintf(stderr,"pbftoosm Error: primitive group too large: "
          "%u\n",l);
return 1;
        }
      bpge= bp+l;
      while(bp<bpge) {  // for each element in primitive group
        switch(bp[0]) {  // first byte of primitive group element
        case 0x0a:  // S 1, normal nodes
          fprintf(stderr,
            "pbftoosm Error: can only process dense nodes\n");
return 1;
        case 0x12:  // S 2, dense nodes
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bufe) {
            fprintf(stderr,"pbftoosm Error: dense nodes too large: "
              "%u\n",l);
return 1;
            }
          if((border_active || !po_dropnodes)  && po__stage<=1)
            if(po__d_pg_dn(bp,bp+l,str,stre-str))
return 1;
          bp+= l;
          break;
        case 0x1a:  // S 3, ways
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bufe) {
            fprintf(stderr,"pbftoosm Error: ways too large: "
              "%u\n",l);
return 1;
            }
          if((border_active || !po_dropways) && po__stage<=1)
            if(po__d_pg_ways(bp,bp+l,str,stre-str))
return 1;
          bp+= l;
          break;
        case 0x22:  // S 4, rels
          bp++;
          l= pbf_uint32(&bp);
          if(bp+l>bufe) {
            fprintf(stderr,"pbftoosm Error: rels too large: "
              "%u\n",l);
return 1;
            }
          if(!po_droprelations) {  // relations are required
//,,,
            if(po__stage==1) {
                // write nodes and ways, change stage to 2 as soon as
                // first relation has been encountered
              po__stage= 2;
                // do not write anything, just process relations
              po__changed= false;
              }
            if(po__stage==2) {
                // do not write anything, just process relations
              if(po__d_pg_rels_analyze(bp,bp+l,str,stre-str))
return 1;
              }
            else {
              if(po__d_pg_rels(bp,bp+l,str,stre-str))
return 1;
              }
            }  // end   relations are required
          bp+= l;
          break;
        default:
          /* block */ {
            static int msgn= 3;
            if(--msgn>=0) {
              fprintf(stderr,"pbftoosm Warning: primitive group "
                "element type unknown: "
                "0x%02X 0x%02X.\n",bp[0],bp[1]);
              }
            }
          if(pbf_jump(&bp))
return 1;
          }  // end   first byte of primitive group element
        }  // end   for each element in primitive group
      bp= bpge;
      break;
    case 0x88:  // 0x01 V 17, nanodegrees
      if(bp[1]!=0x01) goto po__d_unknown;
      bp+= 2;
      l= pbf_uint32(&bp);
      if(l!=100) {
        fprintf(stderr,
          "pbftoosm Error: node nanodegrees must be 100: "
          "%u\n",l);
return 1;
        }
      break;
    case 0x90:  // 0x01 V 18, millisec
      if(bp[1]!=0x01) goto po__d_unknown;
      bp+= 2;
      l= pbf_uint32(&bp);
      if(l!=1000) {
        fprintf(stderr,
          "pbftoosm Error: node milliseconds must be 1000: "
          "%u\n",l);
return 1;
        }
      break;
    case 0x98:  // 0x01 V 19, latitude offset
      if(bp[1]!=0x01) goto po__d_unknown;
      bp+= 2;
      if(pbf_sint64(&bp)!=0) {
        fprintf(stderr,
          "pbftoosm Error: cannot process latitude offsets.\n");
return 1;
        }
      break;
    case 0xa0:  // 0x01 V 20, longitude offset
      if(bp[1]!=0x01) goto po__d_unknown;
      bp+= 2;
      if(pbf_sint64(&bp)!=0) {
        fprintf(stderr,
          "pbftoosm Error: cannot process longitude offsets.\n");
return 1;
        }
      break;
    po__d_unknown:
    default:
      /* block */ {
        static int msgn= 3;
        byte* p;

        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: data block element type unknown: "
            "0x%02X 0x%02X.\n",bp[0],bp[1]);
          }
        p= bp;
        if(pbf_jump(&bp))
return 1;
        *p= 0;  // set null terminator for previous string
        }  // end   block
      }  // end   first byte of element
    if(bp>bufe) {
      fprintf(stderr,"pbftoosm Error: data block format length.\n");
return 1;
      }
    }  // end   for every element in this loop
  return 0;
  }  // end   po__d()

//------------------------------------------------------------

static int po_main(const char* filename) {
  // convert PBF formatted standard input into OSM formatted
  // standard output;
  // filename[]: path and name of the input file;
  //             ==NULL: take standard input;
  // history: history information is not required to be written
  //          to standard output and shall be dropped,
  //          i.e. version, time, changeset, uid, user;
  // return: error number;
  //         0: conversion went ok, warnings may have occurred;
  //         !=0: an error has occurred, gave up conversion;
  int blocktype;  // -1: 0: unknown; expected; 1: Header; 2: Data;
  int datasize;  // -1: expected;
  int rawsize;  // -1: expected;
  int zdata;  // -1: expected;
    // 1: encountered section with compressed data
  uint l;
  static byte zbuf[po__blockM+1000];
  #define maxrewindINI 12
  static int maxrewind= maxrewindINI;
    // maximum number of relation-relation dependencies
  byte* p;
  int r;

  // initialization
  if(read_ini(po__blockM+3000000,filename))
return 3;
  if(border_active && filename!=NULL && !po_droprelations)
      // borders to apply AND input file is a real file AND
      // relations are required
    po__stage= 1;  // initiate recursive processing
  else
    po__stage= 0;  // no recursive processing

  // main loop
  for(;;) {  // main loop
    if(read_bufp>read_bufe) {
      fprintf(stderr,"pbftoosm Error: main format length.\n");
return 1;
      }
    read_input();  // get at least maximum block size
    #if 0
    if(read_bufe-read_bufp<100000) fprintf(stderr,
    "Pointers: %p %p %p %i\nSequence: %i %"PRIi64"\n"
    ,read__buf,read_bufp,read_bufe,read_prefetch,
    po__sequencetype,po__sequenceid);  //,,,
    #endif
    if(read_bufp>=read_bufe) {  // at end of input file
      if(po__stage!=2) {  // no recursive processing at all OR
          // write nodes and ways, change stage to 2
          // as soon as first relation has been encountered
          // (i.e., there are no relations at all) OR
          // write only relations
        if(po__stage==3 && po__changed)
          fprintf(stderr,
            "pbftoosm Warning: relation dependencies too complex\n"
            "         (more than %i hierarchy levels).\n"
            "         Some relations might have been excluded\n"
            "         although lying within the borders.\n"
            "         Please use standard input instead of \'-i\'.\n",
            maxrewindINI);
  break;
        }
      // here: po__stage==2
      // do not write anything, just process relations
      if(!po__changed || --maxrewind<=0)
          // last processing did not result in flag changes
        po__stage= 3;  // enter next stage
      read_jump(true);  // jump to previously stored location
//,,,
      po__changed= false;
      po_testsequence= false;  // (sequence testing would result
        // in unjustified warning messages)
      }  // end   at end of input file
    if(po__stage==1)  // write nodes and ways, change stage to 2
        // as soon as first relation has been encountered
      read_jump(false);
        // set jump destination for a possible input find rewind
  // 0: no recursive processing at all;
  // 1: write nodes and ways, change stage to 2 as soon as
  //    first relation has been encountered;
  // 2: do not write anything, just process relations;
  // 3: write only relations;  ,,,

    if(read_bufp>=read_bufe)
  break;
    if(read_bufp[0]!=0) {  // unknown id at outermost level
      /* block */ {
        static int msgn= 3;
        if(--msgn>=0) {
          fprintf(stderr,
            "pbftoosm Warning: main-element type unknown: "
            "0x%02X 0x%02X.\n",read_bufp[0],read_bufp[1]);
          }
        }
      if(pbf_jump(&read_bufp))
return 1;
    continue;
      }  // end   unknown id at outermost level
    if(read_bufp[1]!=0 || read_bufp[2]!=0 ||
      read_bufp[3]<11 || read_bufp[3]>17) {
      fprintf(stderr,"pbftoosm Error: format blob header %i.\n",
        read_bufp[1]*65536+read_bufp[2]*256+read_bufp[3]);
return 1;
      }
    read_bufp+= 4;
    // new block
    blocktype= datasize= rawsize= zdata= -1;
    for(;;) {  // block loop
      read_input();  // get at least maximum block size
      if(read_bufp>=read_bufe)
    break;
      if(blocktype<0) { // || zdata>0) {  
          // block type expected OR last block might have ended
        // expect the block type here
        if(read_bufp[0]!=0x0a) {  // not id S 1
          fprintf(stderr,"pbftoosm Error: block type expected at: "
            "0x%02X.\n",*read_bufp);
return 1;
          }
        read_bufp++;
        if(memcmp(read_bufp,"\x09OSMHeader",10)==0) {
          blocktype= 1;
          read_bufp+= 10;
    continue;
          }
        if(memcmp(read_bufp,"\x07OSMData",8)==0) {
          blocktype= 2;
          read_bufp+= 8;
    continue;
          }
        blocktype= 0;
        l= pbf_uint32(&read_bufp);
        if(read_bufp+l>read_bufe) {  // string too long
          fprintf(stderr,"pbftoosm Error: block type too long: "
            "%.40s\n",read_bufp);
return 1;
          }
        /* block */ {
          static int msgn= 3;
          if(--msgn>=0) {
            fprintf(stderr,"pbftoosm Warning: block type unknown: "
              "%.40s\n",read_bufp);
            }
          }
        read_bufp+= l;
    continue;
        }  // end   block type expected
      if(datasize<0) {  // data size expected
          if(read_bufp[0]!=0x18) {  // not id V 3
            fprintf(stderr,"pbftoosm Error: block data size "
              "expected at: 0x%02X.\n",*read_bufp);
return 1;
            }
        read_bufp++;
        datasize= pbf_uint32(&read_bufp);
        }  // end   data size expected
      if(blocktype==0) {  // block type unknown
        read_bufp+= datasize;  // jump over this block
    continue;
        }  // end   block type unknown
      if(rawsize<0) {  // raw size expected
          if(read_bufp[0]!=0x10) {  // not id V 2
            fprintf(stderr,"pbftoosm Error: block raw size "
              "expected at: 0x%02X.\n",*read_bufp);
return 1;
            }
        p= read_bufp;
        read_bufp++;
        rawsize= pbf_uint32(&read_bufp);
        datasize-= read_bufp-p;
        }  // end   raw size expected
      if(zdata<0) {  // compressed data expected
          if(read_bufp[0]!=0x1a) {  // not id S 3
            fprintf(stderr,"pbftoosm Error: compressed data "
              "expected at: 0x%02X.\n",*read_bufp);
return 1;
            }
        p= read_bufp;
        read_bufp++;
        l= pbf_uint32(&read_bufp);
        datasize-= read_bufp-p;
        if(datasize<0 || datasize>po__blockM ||
            read_bufp+datasize>read_bufe) {
          fprintf(stderr,"pbftoosm Error: block data size too large: "
            "%i\n",datasize);
fprintf(stderr,"Pointers: %p %p %p\n",read__buf,read_bufp,read_bufe);
return 1;
          }
        if(l!=datasize) {
          fprintf(stderr,"pbftoosm Error: compressed length: "
            "%i->%u.\n",datasize,l);
return 1;
          }
        // uncompress
        r= po__decompress(read_bufp,l,zbuf,sizeof(zbuf),&l);
        if(r!=0) {
          fprintf(stderr,"pbftoosm Error: decompression failed: "
            "%i.\n",r);
return 1;
          }
        if(l!=rawsize) {
          fprintf(stderr,"pbftoosm Error: uncompressed length: "
            "%i->%u.\n",rawsize,l);
return 1;
          }
        // block specific processing
        if(blocktype==1) {  // Header block
          if(po__h(zbuf,zbuf+rawsize))
return 2;
          }
        else {  // Data block
          if(po__d(zbuf,zbuf+rawsize))
return 2;
          }
        zdata= 1;
        read_bufp+= datasize;
        }  // end   compressed data expected
      if(read_bufp[0]==0)  // possibly a new block start
    break;
      if(pbf_jump(&read_bufp))
return 1;
      }  // end   block loop
    }  // end   main loop
  write_str("</osm>"NL);
  if(po_emulatepbf2osm || po_emulateosmosis)
    write_str("<!--End of emulated output.-->"NL);
  return 0;
  }  // end   po_main()

//------------------------------------------------------------
// end   Module po_   pbf-to-osm module
//------------------------------------------------------------



int main(int argc,const char** argv) {
  // main program;
  // for the meaning of the calling line parameters please look at the
  // contents of helptext[];
  const char* filename;  // name of the input file;
  int h_n,h_w,h_r;  // user-suggested hash size in MiB, for
    // hash tables of nodes, ways, and relations;
  int r;

  // initializations
  h_n= h_w= h_r= 0;
  filename= NULL;  // default: standard input;
  #ifdef _WIN32
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
  #endif

  // read command line parameters
  while(--argc>0) {  // for every parameter in command line
    argv++;  // switch to next parameter; as the first one is just
      // the program name, we must do this previous reading the
      // first 'real' parameter;
    if(strcmp(argv[0],"-h")==0) {
        // user wants help text
      fprintf(stderr,"%s",helptext);  // print help text
        // (strange %s just argument to prevent
        // unjustified compiler warning)
return 0;
      }
    if(strzcmp(argv[0],"--drop-his")==0) {
        // user does not want history information in standard output
      po_drophistory= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--drop-bro")==0) {
        // user does not want broken references in standard output
      po_dropbrokenrefs= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--drop-nod")==0) {
        // user does not want nodes section in standard output
      po_dropnodes= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--drop-way")==0) {
        // user does not want ways section in standard output
      po_dropways= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--drop-rel")==0) {
        // user does not want relations section in standard output
      po_droprelations= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--emulate-o")==0) {
        // emulate osmosis compatible output
      po_emulateosmosis= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"--emulate-p")==0) {
        // emulate pbf2osm compatible output
      po_emulatepbf2osm= true;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"-i=")==0 && argv[0][3]!=0) {
        // use input file instead of stdin
      filename= argv[0]+3;
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"-t")==0) {
        // test mode
      write_testmode= true;
      if(argv[0][2]==0)
        fprintf(stderr,"pbftoosm: Entering test mode.\n");
      else {
        loglevel= argv[0][2]-'0';
        if(loglevel<1) loglevel= 1;
        if(loglevel>MAXLOGLEVEL) loglevel= MAXLOGLEVEL;
        fprintf(stderr,"pbftoosm: Entering loglevel %i.\n",loglevel);
        }
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"-h=")==0 && isdig(argv[0][3])) {
        // "-h=...": user wants a specific hash size;
      const char* p;

      p= argv[0]+3;  // jump over "-h="
      h_n= h_w= h_r= 0;
      // read the up to three values for hash tables' size;
      // format examples: "-h=200-20-10", "-h=1200"
      while(isdig(*p)) { h_n= h_n*10+*p-'0'; p++; }
      if(*p!=0) { p++; while(isdig(*p)) { h_w= h_w*10+*p-'0'; p++; } }
      if(*p!=0) { p++; while(isdig(*p)) { h_r= h_r*10+*p-'0'; p++; } }
  continue;  // take next parameter
      }
    if(strzcmp(argv[0],"-b=")==0) {
        // border consideration by a bounding box
      if(!border_box(argv[0]+3)) {
        fprintf(stderr,"pbftoosm Error: use border format: "
          " -b\"x1,y1,x2,y2\"\n");
return 3;
        }  // end   border consideration by a bounding box
      continue;  // take next parameter
      }
    if(strzcmp(argv[0],"-B=")==0) {
        // border consideration by polygon file
      if(!border_file(argv[0]+3)) {
        fprintf(stderr,
          "pbftoosm Error: no polygon file or too large: %s\n",
          argv[0]);
return 4;
        }  // end   border consideration by polygon file
  continue;  // take next parameter
      }
    fprintf(stderr,
      "pbftoosm Error: unknown parameter. Please try: -h\n");
return 0;
    }  // end   for every parameter in command line

  // process parameters
  if(border_active) {  // user wants borders
    int r;

    if(h_n==0) h_n= 400;  // use standard value if not set otherwise
    if(h_w==0 && h_r==0) {
        // user chose simple form for hash memory value
      // take the one given value as reference and determine the 
      // three values using these factors: 80%, 15%, 5%
      h_w= h_n/5; h_r= h_n/20;
      h_n-= h_w; h_w-= h_r; }
    r= hash_ini(h_n,h_w,h_r);  // initialize hash table
    if(r==1)
      fprintf(stderr,"pbftoosm: Hash size had to be reduced.\n");
    else if(r==2)
      fprintf(stderr,"pbftoosm: Not enough memory for hash.\n");
    }  // end   user wants borders
  if(border_active || write_testmode)
    po_testsequence= true;

  // do the work
  r= po_main(filename);
  write_flush();
  if(r==1 || r==2)
    fprintf(stderr,"pbftoosm: Number of bytes read: %"PRIu64"\n",
      read_count());
  if(r==1 && read_bufp!=NULL)
    fprintf(stderr,"pbftoosm: Next bytes to parse:\n"
      "  %.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X\n",
      read_bufp[0],read_bufp[1],read_bufp[2],read_bufp[3],
      read_bufp[4],read_bufp[5],read_bufp[6],read_bufp[7]);
  if(hash_queryerror()!=0)
return 12;
  return r;
  }  // end   main()

