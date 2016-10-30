#define PROGRAM "osmassignpoly"
#define VERSION "0.0.0"
// 2016-03-30 05:15
//
// compile this file:
//   gcc osmassignpoly.c -O3 -o osmassignpoly
// there may be compiler warnings informing you about unused procedures
// and variables; both have been left in the source file intentionally,
// they are for future expansions;
//
// tests:
// gcc osmassignpoly.c -g -o osmassignpoly
// ./osmassignpoly adr.o5m -B=adr.poly -b=5.86,47.27,15.05,55.06 --use-admin-levels=";;;admin_level=" --region-key=region --subregion-key=subregion --export-strings --separate-housenumbers -o=adrr.o5m
// |csplit -f rel  %05d.poly - /^$/
// ./osmassignpoly -v mfr.o5m -B=x.poly |./osmconvert - -o=x.osm.pbf
//
// (c) 2016 Markus Weber, Nuernberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Affero General Public License
// version 3 as published by the Free Software Foundation.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.
// You should have received a copy of this license along
// with this program; if not, see http://www.gnu.org/licenses/.
//
// Other licenses are available on request; please ask the author.



//------------------------------------------------------------
// What this Program does
//------------------------------------------------------------
//
// The regular input file (.o5m-formatted) must contain only nodes.
// The .poly file (option -B=FILENAME.poly) may contain several
// polygons, each separated by one empty line from the previous
// polygon.
//
// The program will find out for each node the polygon it lies in.
// If a node lies within more than one border polygon, the polygons
// with the highest and the lowest admin_level are chosen.

// poly file example
//
// Australia
// 1
//      0.1446763E+03    -0.3825659E+02
//      0.1446693E+03    -0.3826255E+02
//      0.1446627E+03    -0.3825661E+02
//      0.1446693E+03    -0.3826255E+02
// END
// !2
//      0.1422483E+03    -0.3839481E+02
//      0.1422436E+03    -0.3839315E+02
//      0.1422436E+03    -0.3839315E+02
// END
// END
//
// Some Region
// 1
// ...
// END
// END



#define MAXLOGLEVEL 1
  // keep at 1, unless you want do debug
const char* shorthelptext=
"\n" PROGRAM " " VERSION "  Parameter Overview\n"
"(Please use  --help  to get more information.)\n"
"\n"
"<FILE>                    input file name\n"
"-                         read from standard input\n"
"-B=<border_polygons>      file with polygons\n"
"-b=<x1>,<y1>,<x2>,<y2>    manually set an all-including bounding box\n"
"--max-polygons=<n>        maximum number of polygons\n"
"--max-edges=<n>           maximum number of all border polygon edges\n"
"--use-admin-levels        admin_level values in polygon file\' names\n"
"--use-admin-levels=<s>    same, but also specifies separator\n"
"--region-key=<key>        tagging key for polygon\'s name\n"
"--subregion-key=<key>     tagging key for subpolygon\'s name\n"
"--keep-noregion           keep nodes which lie out of all polygons\n"
"--keep-noregion=<val>     tagging value for \"not in any polygon\"\n"
"--keep-boundary-nodes     keep nodes which represent boundaries\n"
"--export-strings          export all address-relevant strings\n"
"--separate-housenumbers   separate housenumber strings at semicolons\n"
"-h                        display this parameter overview\n"
"--help                    display a more detailed help\n"
"--out-o5m                 write output in .o5m format (fast binary)\n"
"--out-none                no standard output (for testing purposes)\n"
"-o=<outfile>              reroute standard output to a file\n"
"--parameter-file=<file>   param. in file, separated by empty lines\n"
"--verbose                 activate verbose mode\n";
const char* helptext=
"\n" PROGRAM " " VERSION "\n"
"\n"
"This program reads an .o5m file and assigns a suited border polygon\n"
"to each node of this file.\n"
"If a node lies within more than one border polygon, the polygons\n"
"with the highest and the lowest admin_level are chosen.\n"
"\n"
"-h\n"
"        Display a short parameter overview.\n"
"\n"
"--help\n"
"        Display this help.\n"
"\n"
"-B=<border_polygon>\n"
"        Alternatively to a bounding box you can use a border polygon\n"
"        to limit the geographical region.\n"
"        The format of a border polygon file can be found in the OSM\n"
"        Wiki: http://wiki.openstreetmap.org/wiki/Osmosis/\n"
"              Polygon_Filter_File_Format\n"
"        You do not need to strictly follow the format description,\n"
"        you must ensure that every line of coordinates starts with\n"
"        blanks.\n"
"\n"
"-b=<x1>,<y1>,<x2>,<y2>\n"
"        To limit the geographical region, you can define a bounding\n"
"        box. If you do not, the program will calculate one on its\n"
"        own (add -v parameter to see its coordinates).\n"
"        Nevertheless it is recommended to specify this box manually,\n"
"        because there might be one faulty polygon in your data which\n"
"        would increase the bounding box in an unwanted manner and.\n"
"        thereby slow down the program\'s algorithm.\n"
"        Provide southwestern and northeastern corners of that area,\n"
"        for example:\n"
"        -b=-0.5,51,0.5,52\n"
"\n"
"--max-polygons=<n>\n"
"        By default, the program can deal with up to 15000 polygons.\n"
"        Use this parameter to change this maximum.\n"
"\n"
"--max-edges=<n>\n"
"        This determines the maximum sum of edges of all border\n"
"        polygons in the polygon file. The default is 4 Millions.\n"
"\n"
"--use-admin-levels\n"
"--use-admin-levels=<separator>\n"
"        If admin_level values have been appended to the relations\'\n"
"        names in polygon file, these values can be used to\n"
"        prioritize one of the polygons in case they overlap:\n"
"        that polygon with the higher admin_level will be selected.\n"
"        In the polygon file a semicolon will be used to separate\n"
"        name and admin_level:\n"
"            Name of the region;admin_level\n"
"        To accept a different separator:\n"
"            --use-admin-level=\";\"\n"
"        or\n"
"            --use-admin-level=\", level: \"\n"
"\n"
"--region-key=<key>\n"
"        specifies a key name for region-specific node tagging.\n"
"        Default key name: \"is_in_region\".\n"
"\n"
"--subregion-key=<key>\n"
"        specifies a key name for region-specific node tagging.\n"
"        This key will be used if there is more than one polygon a\n"
"        point lies in.\n"
"        Default key name: \"is_in_subregion\".\n"
"\n"
"--keep-noregion\n"
"        Usually nodes which are not located inside of any polygon\n"
"        are ignored and therefore not written to output.\n"
"        To change this behaviour, specify this option.\n"
"\n"
"--keep-noregion=<val>\n"
"        You can specify a \"region\" tag value which will be used\n"
"        to tag every node which does not lie within any of the\n"
"        polygons.\n"
"\n"
"--keep-boundary-nodes\n"
"        Keep nodes which are representatives of boundary polygons.\n"
"        That nodes are identified by \"boundary\"=\"administrative\"\n"
"        tags.\n"
"\n"
"--export-strings\n"
"        Use this option if you want to have all address-relevant\n"
"        strings written into files named as follows:\n"
"            osmstrings_nodecount (number of node objects),\n"
"            osmstrings_region, osmstrings_subregion,\n"
"            osmstrings_city, osmstrings_street,\n"
"            osmstrings_housenumber.\n"
"        These files will not be sorted and they will contain\n"
"        doublets. You may want to sort them with \'sort -zu\'.\n"
"\n"
"--separate-housenumbers\n"
"        When exporting strings (see option above), housenumbers which\n"
"        contain semicolons will be treatet as separate housenumber\n"
"        strings.\n"
"        note that the object count in osmstrings_nodecount will be\n"
"        increased accordingly.\n"
"\n"
"--out-o5m\n"
"        The .o5m format will be used. This format has the same\n"
"        structure as the conventional .osm format, but the data are\n"
"        stored as binary numbers and are therefore much more compact\n"
"        than in .osm format. No packing is used, so you can pack .o5m\n"
"        files using every file packer you want, e.g. lzo, bz2, etc.\n"
"\n"
"--out-none\n"
"        This will be no standard output. This option is for testing\n"
"        purposes only.\n"
"\n"
"-o=<outfile>\n"
"        Standard output will be rerouted to the specified file.\n"
"\n"
"--parameter-file=FILE\n"
"        If you want to supply one ore more command line arguments\n"
"        by a parameter file, please use this option and specify the\n"
"        file name. Within the parameter file, parameters must be\n"
"        separated by empty lines. Line feeds inside a parameter will\n"
"        be converted to spaces.\n"
"        Lines starting with \"// \" will be treated as comments.\n"
"\n"
"-v\n"
"--verbose\n"
"        With activated \'verbose\' mode, some statistical data and\n"
"        diagnosis data will be displayed.\n"
"        If -v resp. --verbose is the first parameter in the line,\n"
"        " PROGRAM " will display all input parameters.\n"
"\n"
"Example\n"
"\n"
"./" PROGRAM " input.o5m -B=all.poly\n"
"\n"
"Limitations\n"
"\n"
"The input file must contain the objects ordered by their type: first,\n"
"all nodes, next, all ways, followed by all relations. Within each of\n"
"these sections, the objects section must be sorted by their id in\n"
"ascending order.\n"
"\n"
"Usual .osm, .osc, .o5m, o5c and .pbf files adhere to this condition.\n"
"This means that you do not have to worry about this limitation.\n"
"An error message will be displayed if this sequence is broken.\n"
"\n"
"This program is for experimental use. Expect malfunctions and data\n"
"loss. Do not use the program in productive or commercial systems.\n"
"\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"Please send any bug reports to marqqs@gmx.eu\n\n";

#define _FILE_OFFSET_BITS 64
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

typedef enum {false= 0,true= 1} bool;
typedef uint8_t byte;
typedef unsigned int uint;
#define isdig(x) isdigit((unsigned char)(x))
static int loglevel= 0;  // logging to stderr;
  // 0: no logging; 1: small logging; 2: normal logging;
  // 3: extended logging;
#define DP(f) fprintf(stderr,"Debug: " #f "\n");
#define DPv(f,...) fprintf(stderr,"Debug: " #f "\n",__VA_ARGS__);
#define DPM(f,p,m) { byte* pp; int i,mm; static int msgn= 3; \
  if(--msgn>=0) { fprintf(stderr,"Debug memory: " #f); \
  pp= (byte*)(p); mm= (m); if(pp==NULL) fprintf(stderr,"\n  (null)"); \
  else for(i= 0; i<mm; i++) { \
  if((i%16)==0) fprintf(stderr,"\n "); \
  fprintf(stderr," %02x",*pp++); } \
  fprintf(stderr,"\n"); } }
#define UR(x) if(x){}  // result value intentionally ignored
#if __WIN32__
  #define NL "\r\n"  // use CR/LF as new-line sequence
  #define off_t off64_t
  #define lseek lseek64
  z_off64_t gzseek64(gzFile,z_off64_t,int);
  #define gzseek gzseek64
#else
  #define NL "\n"  // use LF as new-line sequence
  #define O_BINARY 0
#endif



//------------------------------------------------------------
// Module Global   global variables for this program
//------------------------------------------------------------

// to distinguish global variable from local or module global
// variables, they are preceded by 'global_';

static bool global_dropversion= false;  // exclude version
static bool global_dropauthor= false;  // exclude author information
static bool global_fakeauthor= false;  // fake author information
static bool global_fakeversion= false;  // fake just the version number
static bool global_outo5c= false;  // output shall have .o5c format
static bool global_outnone= false;  // no standard output at all
static const char* global_polygonfile= NULL;  // name of polygon file
static int64_t global_maxpolygons= 15000;
static int64_t global_maxedges= 4000000;
static const char* global_useadminlevel= NULL;
  // separator for admin_level;
  // NULL: do not expect an admin_level in region's name
static const char* global_regionkey= "is_in_region";
  // tag key to label the node objects region;
static const char* global_subregionkey= "is_in_subregion";
  // tag key to label the node objects subregion;
static const char* global_keepnoregion= NULL;
  // tagging value for key 'global_regionkey' if a node lies outside
  // of all polygons;  "": no tagging for that nodes,
  //                       just keep them and write them to output;
static bool global_keepboundarynodes= false;
  // keep nodes which represent boundary polygons;
static bool global_exportstrings= false;
  // this switch activates string export to separate files
  // (see help text for  --export-strings  option);
static bool global_separatehousenumbers= false;
  // separate housnumbers which contain semicolons
  // (see help text for  --separate-housenumbers  option);
#define PERR(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,PROGRAM " Error: " f "\n"); }
  // print error message
#define PERRv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,PROGRAM " Error: " f "\n",__VA_ARGS__); }
  // print error message with value(s)
#define PWARN(f) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,PROGRAM " Warning: " f "\n"); }
  // print a warning message, do it maximal 3 times
#define PWARNv(f,...) { static int msgn= 3; if(--msgn>=0) \
  fprintf(stderr,PROGRAM " Warning: " f "\n",__VA_ARGS__); }
  // print a warning message with value(s), do it maximal 3 times
#define PINFO(f) \
  fprintf(stderr,PROGRAM ": " f "\n"); // print info message
#define PINFOv(f,...) \
  fprintf(stderr,PROGRAM ": " f "\n",__VA_ARGS__);
#define PLOG(ll,f,...) { \
  if(ll<=MAXLOGLEVEL && loglevel>=ll) PINFO(f) }
#define PLOGv(ll,f,...) { \
  if(ll<=MAXLOGLEVEL && loglevel>=ll) PINFOv(f,__VA_ARGS__) }
#define FREE(x) if(x!=NULL){free(x);x= NULL;}
#define ONAME(i) \
  (i==0? "node": i==1? "way": i==2? "relation": "unknown object")
#define global_fileM 1002  // maximum number of input files

//------------------------------------------------------------
// end   Module Global   global variables for this program
//------------------------------------------------------------



#define COORDINATE_NIL INT32_C(2000000000)
  // nil value for geocoordinates which are stored as
  // 10^-7 fixpoint numbers in int32_t

static inline int32_t strtosint(const char* s) {
  // read a number and convert it to a signed integer;
  // return: number;
  int sign;
  int i;
  byte b;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  i= 0;
  for(;;) {
    b= (byte)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i*sign;
  }  // end   strtosint()

static inline char* uint32toa(uint32_t v,char* s) {
  // convert uint32_t integer into string;
  // v: long integer value to convert;
  // return: s;
  // s[]: digit string;
  char* s1,*s2;
  char c;

  s1= s;
  if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= "0123456789"[v%10]; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return s;
  }  // end   uint32toa()

static inline char* int64toa(int64_t v,char* s) {
  // convert int64_t integer into string;
  // v: long integer value to convert;
  // return: s;
  // s[21]: digit string;
  char* s1,*s2;
  char c;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  else if(v==0)
    *s1++= '0';
  s2= s1;
  while(v>0)
    { *s2++= "0123456789"[v%10]; v/= 10; }
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return s;
  }  // end   int64toa()

static inline char* int32fix7topa(int32_t v,char* s) {
  // convert a signed 7 decimals fixpoint value into a string;
  // v: fixpoint value
  // return: pointer to string terminator;
  // s[13]: destination string;
  char* s1,*s2,*sterm,c;
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
  sterm= s2;
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return sterm;
  }  // end int32fix7topa()

static inline char *stpcpy0(char *dest, const char *src) {
  // redefinition of C99's stpcpy() because it's missing in MinGW,
  // and declaration in Linux seems to be wrong;
  while(*src!=0)
    *dest++= *src++;
  *dest= 0;
  return dest;
  }  // end stpcpy0()

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

static inline char *stpmcpy(char *dest, const char *src, size_t maxlen) {
  // similar to strmcpy(), this procedure copies a character string;
  // however, it returns the address of the destination string's
  // terminating zero character;
  // this makes it easier to concatenate strings;
  char* d;

  if(maxlen==0)
return dest;
  d= dest;
  while(--maxlen>0 && *src!=0)
    *d++= *src++;
  *d= 0;
  return d;
  }  // end stpmcpy()
#define stpMcpy(d,s) stpmcpy(d,s,sizeof(d))

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

static inline int strycmp(const char* s1,const char* s2) {
  // similar to strcmp(), this procedure compares two character strings;
  // here, both strings are end-aligned;
  // not more characters will be compared than are existing in string s2;
  // i.e., this procedure can be used to identify a file name extension;
  const char* s1e;
  int l;

  l= strchr(s2,0)-s2;
  s1e= strchr(s1,0);
  if(s1e-s1<l)
return 1;
  s1= s1e-l;
  while(*s1==*s2 && *s1!=0) { s1++; s2++; }
  if(*s2==0)
    return 0;
  return *(unsigned char*)s1 < *(unsigned char*)s2? -1: 1;
  }  // end   strycmp()

static inline bool file_exists(const char* file_name) {
  // query if a file exists;
  // file_name[]: name of the file in question;
  // return: the file exists;
  return access(file_name,R_OK)==0;
  }  // file_exists()

static inline int32_t msbit(int64_t v) {
  // gets the most significant 1-bit of a 64 bit integer value;
  int32_t msb;

  msb= 0;
  if(v>=0x100000000LL) {
    v/= 0x100000000LL;
    msb+= 32;
    }
  if(v>=0x10000L) {
    v/= 0x10000L;
    msb+= 16;
    }
  if(v>=0x100) {
    v/= 0x100;
    msb+= 8;
    }
  if(v>=0x10) {
    v/= 0x10;
    msb+= 4;
    }
  if(v>=0x4) {
    v/= 0x4;
    msb+= 2;
    }
  if(v>=0x2) {
    v/= 0x2;
    msb+= 1;
    }
  if(v!=0) {
    msb+= 1;
    }
  return msb;
  }  // msbit()

static int32_t sqrt32(int32_t radicand) {
  // approximate the square root;
  // radicand: radicand;
  // return: square root of x;
  // uses Heron's method;
  int32_t x;

  x= radicand;
  x= (x+radicand/x)/2; x= (x+radicand/x)/2; x= (x+radicand/x)/2;
  x= (x+radicand/x)/2; x= (x+radicand/x)/2; x= (x+radicand/x)/2;
  if(radicand<1024)
return x;
  x= (x+radicand/x)/2; x= (x+radicand/x)/2; x= (x+radicand/x)/2;
  if(radicand<INT32_C(65536))
return x;
  x= (x+radicand/x)/2; x= (x+radicand/x)/2; x= (x+radicand/x)/2;
  if(radicand<INT32_C(1048576))
return x;
  x= (x+radicand/x)/2; x= (x+radicand/x)/2; x= (x+radicand/x)/2;
  return x;
  }  // sqrt32()

static inline int64_t cosrk(int32_t lat) {
  // this procedure calculates the cosine of the given latitude,
  // multiplies it with 40000k/(360*10^7)==0.00012345679,
  // and takes the reciprocal value of it;
  // lat: latitude in 100 nano degrees;
  // return: constant k needed to approximate the area of a
  //         coordinte-defined bbox:
  //         (lonmax-lonmin)*(latmax-latmin)/k
  // example: a= (int64_t)(x_max-x_min)*(int64_t)(y_max-y_min)/
  //             cosrk((y_min+y_max)/2);
  static const int32_t cosrktab[901]= {
    8100,8100,8100,8100,8100,8100,8100,8100,
    8100,8100,8101,8101,8101,8102,8102,8102,
    8103,8103,8103,8104,8104,8105,8105,8106,
    8107,8107,8108,8109,8109,8110,8111,8111,
    8112,8113,8114,8115,8116,8116,8117,8118,
    8119,8120,8121,8122,8123,8125,8126,8127,
    8128,8129,8130,8132,8133,8134,8136,8137,
    8138,8140,8141,8143,8144,8146,8147,8149,
    8150,8152,8154,8155,8157,8159,8160,8162,
    8164,8166,8168,8169,8171,8173,8175,8177,
    8179,8181,8183,8185,8187,8189,8192,8194,
    8196,8198,8200,8203,8205,8207,8210,8212,
    8215,8217,8219,8222,8224,8227,8230,8232,
    8235,8237,8240,8243,8246,8248,8251,8254,
    8257,8260,8263,8265,8268,8271,8274,8277,
    8280,8284,8287,8290,8293,8296,8299,8303,
    8306,8309,8313,8316,8319,8323,8326,8330,
    8333,8337,8340,8344,8347,8351,8355,8358,
    8362,8366,8370,8374,8377,8381,8385,8389,
    8393,8397,8401,8405,8409,8413,8418,8422,
    8426,8430,8434,8439,8443,8447,8452,8456,
    8461,8465,8470,8474,8479,8483,8488,8493,
    8497,8502,8507,8512,8516,8521,8526,8531,
    8536,8541,8546,8551,8556,8561,8566,8571,
    8577,8582,8587,8592,8598,8603,8608,8614,
    8619,8625,8630,8636,8642,8647,8653,8658,
    8664,8670,8676,8682,8687,8693,8699,8705,
    8711,8717,8723,8729,8736,8742,8748,8754,
    8761,8767,8773,8780,8786,8793,8799,8806,
    8812,8819,8825,8832,8839,8846,8852,8859,
    8866,8873,8880,8887,8894,8901,8908,8915,
    8922,8930,8937,8944,8951,8959,8966,8974,
    8981,8989,8996,9004,9012,9019,9027,9035,
    9043,9050,9058,9066,9074,9082,9090,9098,
    9107,9115,9123,9131,9140,9148,9156,9165,
    9173,9182,9190,9199,9208,9216,9225,9234,
    9243,9252,9261,9270,9279,9288,9297,9306,
    9315,9325,9334,9343,9353,9362,9372,9381,
    9391,9400,9410,9420,9430,9439,9449,9459,
    9469,9479,9489,9499,9510,9520,9530,9540,
    9551,9561,9572,9582,9593,9604,9614,9625,
    9636,9647,9658,9669,9680,9691,9702,9713,
    9724,9736,9747,9758,9770,9781,9793,9805,
    9816,9828,9840,9852,9864,9876,9888,9900,
    9912,9924,9937,9949,9961,9974,9986,9999,
    10012,10024,10037,10050,10063,10076,10089,10102,
    10115,10128,10142,10155,10169,10182,10196,10209,
    10223,10237,10251,10265,10279,10293,10307,10321,
    10335,10350,10364,10378,10393,10408,10422,10437,
    10452,10467,10482,10497,10512,10527,10542,10558,
    10573,10589,10604,10620,10636,10652,10668,10684,
    10700,10716,10732,10748,10765,10781,10798,10815,
    10831,10848,10865,10882,10899,10916,10934,10951,
    10968,10986,11003,11021,11039,11057,11075,11093,
    11111,11129,11148,11166,11185,11203,11222,11241,
    11260,11279,11298,11317,11337,11356,11375,11395,
    11415,11435,11455,11475,11495,11515,11535,11556,
    11576,11597,11618,11639,11660,11681,11702,11724,
    11745,11767,11788,11810,11832,11854,11876,11899,
    11921,11944,11966,11989,12012,12035,12058,12081,
    12105,12128,12152,12176,12200,12224,12248,12272,
    12297,12321,12346,12371,12396,12421,12446,12472,
    12497,12523,12549,12575,12601,12627,12654,12680,
    12707,12734,12761,12788,12815,12843,12871,12898,
    12926,12954,12983,13011,13040,13069,13098,13127,
    13156,13186,13215,13245,13275,13305,13336,13366,
    13397,13428,13459,13490,13522,13553,13585,13617,
    13649,13682,13714,13747,13780,13813,13847,13880,
    13914,13948,13982,14017,14051,14086,14121,14157,
    14192,14228,14264,14300,14337,14373,14410,14447,
    14485,14522,14560,14598,14637,14675,14714,14753,
    14792,14832,14872,14912,14952,14993,15034,15075,
    15116,15158,15200,15242,15285,15328,15371,15414,
    15458,15502,15546,15591,15636,15681,15726,15772,
    15818,15865,15912,15959,16006,16054,16102,16151,
    16200,16249,16298,16348,16398,16449,16500,16551,
    16603,16655,16707,16760,16813,16867,16921,16975,
    17030,17085,17141,17197,17253,17310,17367,17425,
    17483,17542,17601,17660,17720,17780,17841,17903,
    17964,18027,18090,18153,18217,18281,18346,18411,
    18477,18543,18610,18678,18746,18814,18883,18953,
    19023,19094,19166,19238,19310,19384,19458,19532,
    19607,19683,19759,19836,19914,19993,20072,20151,
    20232,20313,20395,20478,20561,20645,20730,20815,
    20902,20989,21077,21166,21255,21346,21437,21529,
    21622,21716,21811,21906,22003,22100,22199,22298,
    22398,22500,22602,22705,22810,22915,23021,23129,
    23237,23347,23457,23569,23682,23796,23912,24028,
    24146,24265,24385,24507,24630,24754,24879,25006,
    25134,25264,25395,25527,25661,25796,25933,26072,
    26212,26353,26496,26641,26788,26936,27086,27238,
    27391,27547,27704,27863,28024,28187,28352,28519,
    28688,28859,29033,29208,29386,29566,29748,29933,
    30120,30310,30502,30696,30893,31093,31295,31501,
    31709,31920,32134,32350,32570,32793,33019,33249,
    33481,33717,33957,34200,34447,34697,34951,35209,
    35471,35737,36007,36282,36560,36843,37131,37423,
    37720,38022,38329,38641,38958,39281,39609,39943,
    40282,40628,40980,41337,41702,42073,42450,42835,
    43227,43626,44033,44447,44870,45301,45740,46188,
    46646,47112,47588,48074,48570,49076,49594,50122,
    50662,51214,51778,52355,52946,53549,54167,54800,
    55447,56111,56790,57487,58200,58932,59683,60453,
    61244,62056,62890,63747,64627,65533,66464,67423,
    68409,69426,70473,71552,72665,73814,75000,76225,
    77490,78799,80153,81554,83006,84510,86071,87690,
    89371,91119,92937,94828,96799,98854,100998,103238,
    105580,108030,110598,113290,116118,119090,122220,125518,
    129000,132681,136578,140712,145105,149781,154769,160101,
    165814,171950,178559,185697,193429,201834,211004,221047,
    232095,244305,257873,273037,290097,309432,331529,357027,
    386774,421931,464119,515683,580138,663010,773507,928203,
    1160248,1546993,2320483,4640960,
    2147483647 };  // cosrk values for 10th degrees from 0 to 90

  lat/= 1000000;
    // transform unit 100 nano degree into unit 10th degree
  if(lat<0) lat= -lat;  // make it positive
  if(lat>900) lat= 900; // set maximum of 90 degree
  return cosrktab[lat];
  }  // cosrk()
// the table in the previous procedure has been generated by this
// program:
#if 0  // file cosrk.c, run it with: gcc cosrk.c -lm -o cosrk && ./cosrk
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
int main() {
  int i;
  printf("  static const int32_t cosrktab[901]= {");
  i= 0;
  for(i= 0;i<900;i++) {
    if(i%8==0)
      printf("\n    ");
    printf("%"PRIi32",",(int32_t)(
      1/( cos(i/1800.0*3.14159265359) *0.00012345679)
      ));
    }
  printf("\n    2147483647");
  printf(" };  // cosrk values for 10th degrees from 0 to 90\n");
  return 0; }
#endif

static inline int64_t geoarea(int32_t x_min,int32_t y_min,
    int32_t x_max,int32_t y_max) {
  // calculates the area size of an earth-surfaced rectangle;
  // x_min,y_min,x_max,y_max:
  //   spanning-point coordinates of the rectangle;
  //   Southwest and Nordeast corner; unit: 10^-7 degrees;
  // return: rectangle area, unit: square meters;
  return (int64_t)(x_max-x_min)*(int64_t)(y_max-y_min)/
      cosrk((y_min+y_max)/2);
  }  // geoarea()



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
  // get the value of a signed integer;
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
  uint32_t u;

  p= *pp;
  type= *p & 0x07;
  switch(type) {  // protobuf type
  case 0:  // Varint
    while(*p & 0x80) p++; p++;  // jump over id
    while(*p & 0x80) p++; p++;  // jump over data
    break;
  case 1: // fixed 64 bit;
    while(*p & 0x80) p++; p++;  // jump over id
    p+= 4;  // jump over data
    break;
  case 2:  // String
    while(*p & 0x80) p++; p++;  // jump over id
    u= pbf_uint32(&p);
    p+= u;  // jump over string contents
    break;
  case 5: // fixed 32 bit;
    while(*p & 0x80) p++; p++;  // jump over id
    p+= 2;  // jump over data
    break;
  default:  // unknown id
    PERRv("Format 0x%02X.",*p)
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
// Module poly_   OSM border polygon module
//------------------------------------------------------------

// this module provides procedures for reading a file which
// contains one or more border polygons, and also provides
// methods which determine in what polygon a pair of given
// geocoordinate is located;
// as usual, all identifiers of a module have the same prefix,
// in this case 'poly'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static const int32_t poly__nil= COORDINATE_NIL;
static int32_t poly__bbx1= COORDINATE_NIL,poly__bby1,
  poly__bbx2,poly__bby2;
  // coordinates of southwest and northeast corner of the bounding box
  // for all polygons;
static int32_t poly__mbbx1= COORDINATE_NIL,poly__mbby1,
  poly__mbbx2,poly__mbby2;
  // same as before, but manually set by the user;
// in case of a border polygon:
// for each polygon, every edge is stored in a list;
// to speed-up the inside/outside determination we need to sort the
// edges by x1; subsequently, for every edge there must be stored
// references which refer to all that edges which overlap horizontally
// with that region between x1 and the next higher x1 (x1 of the next
// edge in the sorted list);
typedef struct poly__edge_t {
  int32_t x1,y1,x2,y2;  // coordinates of the edge; always: x1<x2;
  struct poly__chain_t* chain;
  } poly__edge_t;
  // the last element in this list will have x1==poly__nil;
static poly__edge_t* poly__edge= NULL;
static int poly__edge_n= 0;  // number of elements in poly__edge[]
static int poly__chain_n= 0;  // number of elements in poly__chain[]
#define poly__chain_M ((global_maxedges+4)*8)
typedef struct poly__chain_t {
  poly__edge_t* edge;
  struct poly__chain_t* next;
  } poly__chain_t;
  // the last element in this list will have edge==NULL;
  // the last element of each chain will be terminated with next==NULL;
static poly__chain_t* poly__chain= NULL;

// for each polygon, header information is stored in this structure
typedef struct poly__poly_t {
  int32_t bbx1,bby1,bbx2,bby2;  // bounding box of this polygon
  poly__edge_t* edge,*edgee;  // address of first edge and
    // address after last edge of this polygon
  int8_t admin_level;
  char name[128-sizeof(poly__edge_t*)*2-4*4-1]; 
    // dimension string size to make the structure 128 Bytes large;
  } poly__poly_t;
static poly__poly_t* poly__poly= NULL;
static poly__poly_t* poly__polye= NULL;  // logical end in poly__poly[]
static int poly__poly_n= 0;  // number of elements in poly__poly[]

static poly__poly_t*** poly__matrix= NULL;
  // quadratic matrix for accelerating the border-polygon
  // determination algorithm;
  // this matrix divides the total region (defined by
  // poly__bbx1..bby2) in a number of cells;
  // each cell refers to a NULL-terminated list of polygons which
  // intersect with the cell;
  // note: row/column grid is turned 90 degree counter-clockwise
static poly__poly_t** poly__list= NULL;
  // NULL-terminated lists of polygons; used by 'poly__matrix';
static int32_t poly__matrix_cellwidth= 0;  // unit: geocoordinates
static int32_t poly__matrix_cellheight= 0;  // unit: geocoordinates
static int32_t poly__matrix_width= 0;  // unit: number of cells

static bool poly__calculatematrix() {
  // calculate acceleration matrix for polygon determinations;
  // poly__bbx1..poly__bby2: global borders, and
  // poly__poly_n: number of polygons, both will be used to
  //                                   calculate the matrix size;
  // return: success;
  // allocated memory will be freed by poly__end();
  static int32_t listn= 0;  // number of elements in list
  poly__poly_t* ppp;  // pointer in poly__poly
  int i,j;  // matrix indexes
  int32_t x1,y1,x2,y2;  // coordinates
  poly__poly_t*** mp;  // pointer in poly__matrix;
  poly__poly_t** lp;  // pointer in poly__list;

  // calculate matrix width
  poly__matrix_width= poly__poly_n;
  if(poly__matrix_width<4) poly__matrix_width= 4;
  poly__matrix_width= sqrt32(poly__matrix_width)*2;
    // will result in 200 x 200 matrix cells for 10000 polygons;
    // this seems to be a reasonable partitioning
  #if MAXLOGLEVEL>=3
    if(loglevel>=3)
      fprintf(stderr,"matrix width: %"PRIi32"\n",poly__matrix_width);
  #endif

  // allocate space for matrix
  poly__matrix= malloc(sizeof(poly__poly_t**)*
    poly__matrix_width*poly__matrix_width+2);
  if(poly__matrix==NULL) {
    PERR("not enough memory space for matrix.")
    return false;
    }

  // calculate cell width and height
  poly__matrix_cellwidth= (poly__bbx2-poly__bbx1)/poly__matrix_width;
  poly__matrix_cellheight= (poly__bby2-poly__bby1)/poly__matrix_width;

  // calculate necessary length of polygon lists
  x1= poly__bbx1;
  i= poly__matrix_width;
  while(--i>=0) {  // for each matrix row
    if(i==0)  // last row
      x2= poly__bbx2;
    else
      x2= x1+poly__matrix_cellwidth;
    y1= poly__bby1;
    j= poly__matrix_width;
    while(--j>=0) {  // for each matrix column
      if(j==0)  // last column
        y2= poly__bby2;
      else
        y2= y1+poly__matrix_cellheight;
      for(ppp= poly__poly;ppp<poly__polye;ppp++) {
          // for each polygon
        // count if bounding box intersects with cell
        if(ppp->bbx2>=x1 && ppp->bbx1<=x2 &&
            ppp->bby2>=y1 && ppp->bby1<=y2)
          listn++;
        } // for each polygon
      listn++;  // for NULL-terminator
      y1= y2;
      }  // for each matrix column
    x1= x2;
    }  // for each matrix row
  #if MAXLOGLEVEL>=3
    if(loglevel>=3)
      fprintf(stderr,"list length: %"PRIi32"\n",listn);
  #endif

  // allocate memory space for matrix
  poly__list= malloc(sizeof(poly__poly_t**)*listn+2);
  if(poly__list==NULL) {
    PERR("not enough memory space for matrix lists.")
    return false;
    }

  // assign polygons to polygon lists of matrix cells
  lp= poly__list;
  mp= poly__matrix;
  x1= poly__bbx1;
  i= poly__matrix_width;
  while(--i>=0) {  // for each matrix row
    if(i==0)  // last row
      x2= poly__bbx2;
    else
      x2= x1+poly__matrix_cellwidth;
    y1= poly__bby1;
    j= poly__matrix_width;
    while(--j>=0) {  // for each matrix column
      if(j==0)  // last column
        y2= poly__bby2;
      else
        y2= y1+poly__matrix_cellheight;
      *mp= lp;  // default: no polygon intersects with this cell
      for(ppp= poly__poly;ppp<poly__polye;ppp++) {
          // for each polygon
        // assign polygon to cell's list
        // if bounding box intersects with cell
        if(ppp->bbx2>=x1 && ppp->bbx1<=x2 &&
            ppp->bby2>=y1 && ppp->bby1<=y2)
          *lp++= ppp;
        } // for each polygon
      *lp++= NULL;  // terminator for this cell's list
      mp++;  // next matrix cell
      y1= y2;
      }  // for each matrix column
    x1= x2;
    }  // for each matrix row
  #if MAXLOGLEVEL>=3
  if(loglevel>=3)
    fprintf(stderr,"End of matrix initialization.\n");
  #endif
  return true;
  }  // poly__calculatematrix()

static void poly__end() {
  // shut-down this module;
  // this procedure has no parameters because we want to be able
  // to call it via atexit();

  if(poly__edge!=NULL) {
    PLOGv(2,"number of polygon edges: %d",poly__edge_n)
    free(poly__edge);
    }
  poly__edge= NULL;
  poly__edge_n= 0;
  if(poly__chain!=NULL) {
    PLOGv(2,"number of polygon chain links: %d",poly__chain_n)
    free(poly__chain);
    }
  poly__chain= NULL;
  poly__chain_n= 0;
  if(poly__poly!=NULL) {
    PLOGv(2,"number of polygons: %d",poly__poly_n)
    free(poly__poly);
    }
  poly__poly= poly__polye= NULL;
  poly__poly_n= 0;
  FREE(poly__matrix)
  FREE(poly__list)
  }  // poly__end()

static inline bool poly__ini() {
  // initialize this module;
  // you may call this procedure repeatedly; only the first call
  // will have effect; subsequent calls will be ignored;
  // return: ==true: success, or the call has been ignored;
  //         ==false: an error occurred during initialization;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;
    atexit(poly__end);
    poly__edge= (poly__edge_t*)
      malloc((global_maxedges+4+4)*sizeof(poly__edge_t));
    poly__chain= (poly__chain_t*)
      malloc((poly__chain_M+4)*sizeof(poly__chain_t));
    poly__poly= poly__polye= (poly__poly_t*)
      malloc((global_maxpolygons+4)*sizeof(poly__poly_t));
    }
  return poly__edge!=NULL && poly__chain!=NULL && poly__poly!=NULL;
  }  // poly__ini()

static int poly__qsort_edge(const void* a,const void* b) {
  // edge comparison for qsort()
  int32_t ax,bx;

  ax= ((poly__edge_t*)a)->x1;
  bx= ((poly__edge_t*)b)->x1;
  if(ax>bx)
return 1;
  if(ax==bx)
return 0;
  return -1;
  }  // poly__qsort_edge()

//------------------------------------------------------------

static bool poly_active= false;  // borders are to be considered;
  // this variable must not be written from outside of the module;

static bool poly_box(const char* s) {
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
  poly__mbbx1= (int32_t)(x1f*10000000L);
    // convert floatingpoint to fixpoint
  poly__mbby1= (int32_t)(y1f*10000000L);
  poly__mbbx2= (int32_t)(x2f*10000000L);
  poly__mbby2= (int32_t)(y2f*10000000L);
  if(loglevel>=1) {
    char s[13*4+50],*sp;

    sp= stpcpy(s,"manually-set border: ");
    sp= int32fix7topa(poly__mbbx1,sp); *sp++= ',';
    sp= int32fix7topa(poly__mbby1,sp); *sp++= ',';
    sp= int32fix7topa(poly__mbbx2,sp); *sp++= ',';
    sp= int32fix7topa(poly__mbby2,sp); *sp= 0;
    PINFOv("%s",s)
    }
  return true;
  }  // poly_box()

static bool poly_file(const char* fn) {
  // read border polygon file, store the coordinates, and determine
  // an enclosing border box to speed-up the calculations;
  // fn[]: file name;
  // return: success;
  static int32_t nil;
  poly__poly_t* ppp;  // growing pointer in poly__poly[]
  poly__poly_t* ppe;  // logical end of data in poly__poly[]
  poly__poly_t* ppee;  // physical end of data in poly__poly[]

  if(!poly__ini()) {
    PERR("not enough memory.")
    PINFO("try to decrease  --max-polygons=  and  --max-edges=")
return false;
    }
  nil= poly__nil;

  /* get border polygon */ {
    poly__edge_t* bep;  // growing pointer in poly__edge[]
    poly__edge_t* bee;  // memory end of poly__edge[]
    FILE* fi;
    char s[80],*sp;
    int32_t x0,y0;  // coordinate of the first point in a section;
      // this is used to close an unclosed polygon;
    int32_t x1,y1;  // last coordinates
    int32_t x,y;
    int endn;  // number of sequent "END" lines
    int32_t bbx1,bby1,bbx2,bby2;
      // border box of currently inspected polygon;
      // coordinates of southwest and northeast corner;
    int adminlevel_len;

    adminlevel_len= 0;
    if(global_useadminlevel!=NULL)
      adminlevel_len= strlen(global_useadminlevel);
    ppp= ppe= poly__poly;
    ppee= poly__poly+global_maxpolygons;
    endn= 2;
    poly__edge[0].x1= nil;
    fi= fopen(fn,"rb");
    if(fi==NULL)
return false;
    bee= poly__edge+(global_maxedges-2);
    bep= poly__edge;
    x0= nil;  // (sign that there is no first coordinate at the moment)
    x1= nil;  // (sign that there is no last coordinate at the moment)
    for(;;) {  // for every line in border file
      do {  // read one line, ignoring empty lines
        s[0]= 0;
        sp= fgets(s,sizeof(s),fi);
        } while(sp!=NULL && (s[0]==0 || s[0]=='\n' || s[0]=='\r'));
      if(bep>=bee) {
        fclose(fi);
        PERR("too many polygon edges.")
        PINFO("increase  --max-edges=  parameter.")
return false;
        }
      if(s[0]==' ' || s[0]=='\t') {  // inside a section
        double xf,yf;

        endn= 0;
        xf= yf= 200.1;
        sscanf(s+1,"%lG %lG",&xf,&yf);
        if(xf<-180.1 || xf>180.1 || yf<-90.1 || yf>90.1) x= nil;
        else {
          x= (int32_t)(xf*10000000+0.5);
          y= (int32_t)(yf*10000000+0.5);
          }
        /* update size of border box */ {
          if(x<bbx1) bbx1= x;
          if(y<bby1) bby1= y;
          if(x>bbx2) bbx2= x;
          if(y>bby2) bby2= y;
          }
        if(x!=nil) {  // data plausible
          if(x1!=nil) {  // there is a preceding coordinate
            if(x1!=x) {  // new edge not in North-South direction;
                // we do not accept exact north-south lines,
                // because then we may not be able to determine
                // if a point lies inside or outside the polygon;
              if(x>x1)
                { bep->x1= x1; bep->y1= y1; bep->x2= x; bep->y2= y; }
              else
                { bep->x1= x; bep->y1= y; bep->x2= x1; bep->y2= y1; }
              bep->chain= NULL;
                // initialize; there are not any chains yet
              #if MAXLOGLEVEL>=4
              if(loglevel>=4)
                fprintf(stderr,
                  "- %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                  (int)(bep-poly__edge),
                  bep->x1,bep->y1,bep->x2,bep->y2);
              #endif
              bep++;
              }  // new edge not in North-South direction
            #if MAXLOGLEVEL>=3
            else {  // missing edge in North-South direction
              if(loglevel>=3)
                fprintf(stderr,
                  "x %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                  (int)(bep-poly__edge),
                  bep->x1,bep->y1,bep->x2,bep->y2);
              }  // missing edge in North-South direction
            #endif
            }  // there is a preceding coordinate
          x1= x; y1= y;
          if(x0==nil)
            { x0= x; y0= y; }
          }  // data plausible
        }  // inside a section
      else {  // not inside a section
        if(x0!=nil && x1!=nil && (x1!=x0 || y1!=y0)) {
            // last polygon was not closed
          if(x1!=x0) {  // missing edge not in North-South direction
            // close the polygon
            if(x0>x1)
              { bep->x1= x1; bep->y1= y1; bep->x2= x0; bep->y2= y0; }
            else
              { bep->x1= x0; bep->y1= y0; bep->x2= x1; bep->y2= y1; }
            bep->chain= NULL;
            #if MAXLOGLEVEL>=4
            if(loglevel>=4)
              fprintf(stderr,
                "c %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                (int)(bep-poly__edge),bep->x1,bep->y1,bep->x2,bep->y2);
            #endif
            bep++;
            }  // missing edge not in North-South direction
            #if MAXLOGLEVEL>=3
            else {  // missing edge not in North-South direction
              if(loglevel>=3)
                fprintf(stderr,
                  "d %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
                  (int)(bep-poly__edge),
                  bep->x1,bep->y1,bep->x2,bep->y2);
              }  // missing edge not in North-South direction
            #endif
          }  // last polygon was not closed
        x0= x1= nil;  // remember that this section has been closed
        if(strzcmp(s,"END")==0)
          endn++;
        if(endn>=2) {  // there have been 2 sequent "END" lines
          if(ppp<ppe) {  // there is an open polygon dataset
            // set terminator of edge list
            bep->x1= nil;
            bep++;
            // close this polygon dataset
            ppp->bbx1= bbx1;
            ppp->bby1= bby1;
            ppp->bbx2= bbx2;
            ppp->bby2= bby2;
            ppp->edgee= bep;
            ppp++;
            }  // there is an open polygon dataset
          if(ppp==ppe && strzcmp(s,"END")!=0 && s[0]!=0) {
              // there is no open polygon-dataset AND
              // line read must be a new headline
            // open and initialize a new polygon dataset
            ppe++;
            if(ppe>=ppee) {
              fclose(fi);
              PERR("too many polygons.")
              PINFO("increase  --max-polygons=  parameter.")
return false;
              }
            ppp->edge= bep;
            /* get rid of line-feed characters at end of line */ {
              char* p;
              p= strchr(s,0)-1;
              while(p>=s && (p[0]=='\n' || p[0]=='\r')) *p--= 0;
              }
            ppp->admin_level= 0;
            if(global_useadminlevel==NULL)
                // expect just the name, not an admin_level
              strMcpy(ppp->name,s);
            else {  // expect name AND admin_level
              // separate name and admin_level
              char* p;
              p= strstr(s,global_useadminlevel);
              if(p!=NULL) {  // found the separator
                *p= 0;  // delimit the name
                ppp->admin_level=
                  strtosint(p+adminlevel_len);
                }  // found the separator
              strMcpy(ppp->name,s);
              }  // expect name AND admin_level
            bbx1= bby1= 2000000000L;
            bbx2= bby2= -2000000000L;
            endn= 0;
            #if MAXLOGLEVEL>=2
            if(loglevel>=2)
              fprintf(stderr,
                "polygon %s (%i)\n",ppp->name,ppp->admin_level);
            #endif
            }  // no open dataset AND new headline
          }  // there have been 2 sequent "END" lines
        }  // not inside a section
      if(sp==NULL)  // end of border file
    break;
      }  // for every line in border file
    fclose(fi);
    bep->x1= nil;  // set terminator of edge list
    poly__edge_n= bep-poly__edge;  // set number of edges
    poly__poly_n= ppe-poly__poly;  // set number of polygons
    poly__polye= ppe;  // store logical end in poly__poly[]
    }  // get border polygon

  // for each polygon, sort edges ascending by x1 value
  for(ppp= poly__poly;ppp<ppe;ppp++) {  // for each polygon
    #if MAXLOGLEVEL>=3
      if(loglevel>=3)
        fprintf(stderr,"Sorting polygon: %s.\n",ppp->name);
    #endif
    qsort(ppp->edge,ppp->edgee-ppp->edge,sizeof(poly__edge_t),
      poly__qsort_edge);
    }  // for each polygon

  /* for each polygon, generate chains for each edge */ {
    int32_t x2;
    poly__chain_t* bcp;  // growing pointer in chain storage
    poly__chain_t* bcee;  // physical end of chain storage
    poly__edge_t* be;  // start of edges of currently examined polygon
    poly__edge_t* bep;  // pointer in poly__edge[]
    poly__edge_t* bep2;  // referenced edge
    poly__chain_t* bcp2;  // chain of referenced edge;

    bcp= poly__chain;
    bcee= poly__chain+poly__chain_M;
    for(ppp= poly__poly;ppp<ppe;ppp++) {  // for each polygon
      // generate chains for each edge
      bep= be= ppp->edge;
      while(bep->x1!=nil) {  // for each edge in list
        #if MAXLOGLEVEL>=3
        if(loglevel>=3)
          fprintf(stderr,
            "> %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
            (int)(bep-poly__edge),bep->x1,bep->y1,bep->x2,bep->y2);
        #endif
        x2= bep->x2;
        bep2= bep;
        while(bep2>be && (bep2-1)->x1==bep2->x1) bep2--;
          // we must examine previous edges having same x1 too;
        while(bep2->x1!=nil && bep2->x1 <= x2) {
            // for each following overlapping edge in list
          if(bep2==bep) {  // own edge
            bep2++;  // (needs not to be chained to itself)
        continue;
            }
          if(bcp>=bcee) {
              // no more space in chain storage
            PERR("too many polygon chains.")
            PINFO("increase  --max-edges=  parameter.")
return false;
            }
          #if MAXLOGLEVEL>=4
          if(loglevel>=4)
            fprintf(stderr,"+ add to chain of %i\n",
              (int)(bep2-poly__edge));
          #endif
          bcp2= bep2->chain;
          if(bcp2==NULL)  // no chain yet
            bep2->chain= bcp;  // add first chain link
          else {  // edge already has a chain
            // go to the chain's end and add new chain link there

            while(bcp2->next!=NULL) bcp2= bcp2->next;
            bcp2->next= bcp;
            }  // edge already has a chain
          bcp->edge= bep;
            // add source edge to chain of overlapping edges
          bcp->next= NULL;  // new chain termination
          bcp++;
          bep2++;
          }  // for each following overlapping  edge in list
        bep++;
        }  // for each edge in list
      }  // for each polygon
    poly__chain_n= bcp-poly__chain;  // set number of chains links
    }  // for each polygon, generate chains for each edge

  // test output
  #if MAXLOGLEVEL>=4
  if(loglevel>=4) {
    poly__edge_t* bep,*bep2;  // pointers in poly__edge[]
    poly__chain_t* bcp;  // pointer in chain storage

    fprintf(stderr,"Chains:\n");
    bep= poly__edge;
    while(bep->x1!=nil) {  // for each edge in list
      fprintf(stderr,
        "> %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
        (int)(bep-poly__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      bcp= bep->chain;
      while(bcp!=NULL) {  // for each chain link in edge
        bep2= bcp->edge;
        fprintf(stderr,
          "  %i %"PRIi32",%"PRIi32",%"PRIi32",%"PRIi32"\n",
          (int)(bep2-poly__edge),
          bep2->x1,bep2->y1,bep2->x2,bep2->y2);
        bcp= bcp->next;
        }  // for each chain link in edge
      bep++;
      }  // for each edge in list
    }  // test output
  #endif

  // determine the all-including bounding box
  poly__bbx1= poly__bby1= 2000000000L;
  poly__bbx2= poly__bby2= -2000000000L;
  for(ppp= poly__poly;ppp<ppe;ppp++) {  // for each polygon
    if(ppp->edgee!=ppp->edge &&
        ppp->bbx1<ppp->bbx2 &&
        ppp->bby1<ppp->bby2) {  // at least one point in polygon
      if(ppp->bbx1<poly__bbx1) poly__bbx1= ppp->bbx1;
      if(ppp->bby1<poly__bby1) poly__bby1= ppp->bby1;
      if(ppp->bbx2>poly__bbx2) poly__bbx2= ppp->bbx2;
      if(ppp->bby2>poly__bby2) poly__bby2= ppp->bby2;
      }  // at least one point in polygon
    }  // for each polygon

  #if MAXLOGLEVEL>=3
  if(loglevel>=3)
    fprintf(stderr,"End of border initialization.\n");
  #endif

  if(loglevel>=1) {
    char s[13*4+50],*sp;

    sp= stpcpy(s,"calculated border: ");
    sp= int32fix7topa(poly__bbx1,sp); *sp++= ',';
    sp= int32fix7topa(poly__bby1,sp); *sp++= ',';
    sp= int32fix7topa(poly__bbx2,sp); *sp++= ',';
    sp= int32fix7topa(poly__bby2,sp); *sp= 0;
    PINFOv("%s",s)
    }
  return true;
  }  // poly_file()

static bool poly_prepare() {
  // prepare this module for work;
  // return: success;
  // must be called after having called both other
  // initialize procedures, poly_file() and poly_box();
  if(poly__mbbx1!=COORDINATE_NIL) {
      // bounding box has been specified manually
    PLOG(1,"manually specified bounding box will be used.")
    poly__bbx1= poly__mbbx1;
    poly__bby1= poly__mbby1;
    poly__bbx2= poly__mbbx2;
    poly__bby2= poly__mbby2;
    }  // bounding box has been specified manually
  if(!poly__calculatematrix())
return false;
  poly_active=true;
  return true;
  }  // poly_prepare()

static bool poly_querypolygon(int32_t x,int32_t y,
    const char** regionp,const char** subregionp) {
  // determines if the given coordinate lies inside one of the
  // polygons;
  // x,y: coordinates of the given point in 0.0000001 degrees;
  // return: there is at least one polygon the point lies in;
  // *regionp: string pointer to the name of the enclosing polygon;
  //           if there is more than one suiting polygon, that one
  //           with the lowest admin_level will be chosen;
  //           NULL: point lies outside of all polygons;
  // *subregionp: same as before, bit the polygon with the highest
  //           admin_level is chosen;
  //           NULL: there is not more than one suiting polygon;
  const char* region;  // name of the polygon the point lies inside;
    // NULL: no such polygon OR point is outside;
  const char* subregion;  // name of a second polygon with a higher
    // admin_level the point lies inside, if applicable;
    // NULL: no such second polygon;
  int level;  // admin_level of the polygon the point is inside
  int sublevel;  // admin_level of a second polygon the point is inside
  static int32_t nil;
  poly__poly_t** plp;  // growing pointer in poly__list[]
  poly__poly_t* ppp;  // pointer in poly__poly[]

  nil= poly__nil;
  #if MAXLOGLEVEL>=5
  if(loglevel>=5)
    fprintf(stderr,"## %"PRIi32",%"PRIi32"\n",x,y);
  #endif

  // consider the all-including border box (if any)
  if(!poly_active || poly__polye==poly__poly || poly__bbx1==nil ||
      x<poly__bbx1 || x>poly__bbx2 ||
      y<poly__bby1 || y>poly__bby2) {
      // there is no all-including bounding box OR
      // point lies outside of it
    *regionp= *subregionp= NULL;
return false;
    }

  // initialization for border polygon search
  region= subregion= NULL;  // default
  level= INT_MAX; sublevel= -1;

  /* determine matrix cell the point lies in */ {
    int i,j;  // matrix indexes

    i= (x-poly__bbx1)/poly__matrix_cellwidth;  // matrix row
    if(i>=poly__matrix_width) i= poly__matrix_width-1;
    j= (y-poly__bby1)/poly__matrix_cellheight;  // matrix column
    if(j>=poly__matrix_width) j= poly__matrix_width-1;
    plp= *(poly__matrix+(i*poly__matrix_width+j));
      // start of polygon list for this matrix cell
    }

  // examine each polygon in the list of this matrix cell
  for(ppp= *plp;ppp!=NULL;ppp= *++plp) {
      // for each polygon in matrix-cell list
    poly__edge_t* be;
      // start of poly__edge[] data for this polygon
    poly__edge_t* bep;  // pointer in poly__edge[]
    int ben;  // number of edges of this polygon
    poly__chain_t* bcp;  // pointer in poly__chain[]
    int cross;  // number of line crossings from given point
      // to the north pole would have against the border lines
      // of this polygon in poly__edge[];

    // check if point lies inside the polygon's bounding box
    if(x<ppp->bbx1 || x>ppp->bbx2 || y<ppp->bby1 || y>ppp->bby2)
        // point lies outside of bounding box
  continue;

    // initialize search for edges which lie north of the point
    be= ppp->edge;
    if(be==NULL)  // there is no border polygon
  continue;  // take next polygon
    ben= ppp->edgee-ppp->edge;
    cross= 0;

    /* binary-search the edge with the closest x1 | x1<=x */ {
      int i,i1,i2;  // iteration indexes

      i1= 0; i2= ben;
      while(i2>i1+1) {
        i= (i1+i2)/2;
        bep= be+i;
        if(bep->x1 > x) i2= i;
        else i1= i;
        }
      bep= be+i1;
      }  // binary-search the edge with the closest x1

    bcp= NULL;
      // (default, because we want to examine the own edge first)
    for(;;) {  // for own edge and each edge in chain
      if(bep->x1 <= x && bep->x2 > x) {  // point lies inside x-range
        if(bep->y1 > y && bep->y2 > y) {
            // line lies completely north of point
          cross++;
          #if MAXLOGLEVEL>=5
          if(loglevel>=5)
            fprintf(stderr,"= %i %"PRIi32",%"PRIi32
              ",%"PRIi32",%"PRIi32"\n",
              (int)(bep-poly__edge),bep->x1,bep->y1,bep->x2,bep->y2);
          #endif
          }
        else if(bep->y1 > y || bep->y2 > y) {
            // one line end lies north of point
          if( (int64_t)(y-bep->y1)*(int64_t)(bep->x2-bep->x1) <
              (int64_t)(x-bep->x1)*(int64_t)(bep->y2-bep->y1) ) {
              // point lies south of the line
            cross++;
            #if MAXLOGLEVEL>=5
            if(loglevel>=5)
              fprintf(stderr,"/ %i %"PRIi32",%"PRIi32
                ",%"PRIi32",%"PRIi32"\n",
                (int)(bep-poly__edge),
                bep->x1,bep->y1,bep->x2,bep->y2);
            #endif
            }
          #if MAXLOGLEVEL>=5
          else if(loglevel>=5)
            fprintf(stderr,". %i %"PRIi32",%"PRIi32
              ",%"PRIi32",%"PRIi32"\n",
              (int)(bep-poly__edge),
              bep->x1,bep->y1,bep->x2,bep->y2);
          #endif
          }  // one line end north of point
        #if MAXLOGLEVEL>=5
        else if(loglevel>=5)
          fprintf(stderr,"_ %i %"PRIi32",%"PRIi32
            ",%"PRIi32",%"PRIi32"\n",
            (int)(bep-poly__edge),bep->x1,bep->y1,bep->x2,bep->y2);
        #endif
        }  // point lies inside x-range
      if(bcp==NULL)  // chain has not been examined
        bcp= bep->chain;  // get the first chain link
      else
        bcp= bcp->next;  // get the next chain link
      if(bcp==NULL)  // no more chain links
    break;
      bep= bcp->edge;
      }  // for own edge and each edge in chain
    #if MAXLOGLEVEL>=5
    if(loglevel>=5)
      fprintf(stderr,"# %"PRIi32",%"PRIi32" cross %i\n",x,y,cross);
    #endif
    if(cross&1) {  // odd number of crossings => point is inside
      if(ppp->admin_level<level) {  // new polygon has lower level
        if(region!=NULL && subregion==NULL) {
            // until now: there is a region, but no subregion
          // old region becomes subregion
          sublevel= level;
          subregion= region;
          }
        // store new region
        level= ppp->admin_level;
        region= ppp->name;
        }  // new polygon has higher level
      else if(ppp->admin_level>level && ppp->admin_level>sublevel) {
          // new polygon has higher level and higher sublevel
        // store new subregion
        sublevel= ppp->admin_level;
        subregion= ppp->name;
        }  // new polygon has lower level
      }  // point is inside
    }  // for each polygon in matrix-cell list
  *regionp= region; *subregionp= subregion;
  return region!=NULL;
  }  // poly_querypolygon()

static bool poly_querybox(int32_t* x1p,int32_t* y1p,
    int32_t* x2p,int32_t* y2p) {
  // get the values of a previously calculated all-including
  // bounding box;
  // poly_box() or poly_file() must have been called;
  // return values are valid only if poly_active==true;
  // *x1p,*y1p;  // coordinates of southwestern corner;
  // *x2p,*y2p;  // coordinates of northeastern corner;
  // return: bounding box is valid;
  int32_t x1,y1,x2,y2;

  if(!poly_active) {
    *x1p= *y1p= *x2p= *y2p= 0;
return false;
    }
  x1= poly__bbx1; y1= poly__bby1;
  x2= poly__bbx2; y2= poly__bby2;
  // round coordinates a bit
  #define D(x) { if(x%1000==1) { if(x>0) x--; else x++; } \
    else if((x)%1000==999) { if((x)>0) x++; else x--; } }
  D(x1) D(y1) D(x2) D(y2)
  #undef D
  *x1p= x1; *y1p= y1; *x2p= x2; *y2p= y2;
return true;
  }  // poly_querybox()

//------------------------------------------------------------
// end Module poly_   OSM border polygon module
//------------------------------------------------------------



//------------------------------------------------------------
// Module read_   OSM file read module
//------------------------------------------------------------

// this module provides procedures for buffered reading of
// standard input;
// as usual, all identifiers of a module have the same prefix,
// in this case 'read'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define read_PREFETCH ((32+3)*1024*1024)
  // number of bytes which will be available in the buffer after
  // every call of read_input();
  // (important for reading .pbf files:
  //  size must be greater than pb__blockM)
#define read__bufM (read_PREFETCH*5)  // length of the buffer;
#define read_GZ 0  // determines which read procedure set will be used;
  // ==0: use open();  ==1: use fopen();
  // ==2: use gzopen() (accept gzip compressed input files);
  // ==3: use gzopen() with increased gzip buffer;
typedef struct {  // members may not be accessed from external
  #if read_GZ==0
    int fd;  // file descriptor
    off_t jumppos;  // position to jump to; -1: invalid
  #elif read_GZ==1
    FILE* fi;  // file stream
    off_t jumppos;  // position to jump to; -1: invalid
  #else
    gzFile fi;  // gzip file stream
    #if __WIN32__
      z_off64_t jumppos;  // position to jump to; -1: invalid
    #else
      z_off_t jumppos;  // position to jump to; -1: invalid
    #endif
  #endif
  int64_t counter;
    // byte counter to get the read position in input file;
  char filename[300];
  bool isstdin;  // is standard input
  bool eof;  // we are at the end of input file
  byte* bufp;  // pointer in buf[]
  byte* bufe;  // pointer to the end of valid input in buf[]
  uint64_t bufferstart;
    // dummy variable which marks the start of the read buffer
    // concatenated  with this instance of read info structure;
  } read_info_t;
static bool read__jumplock= false;  // do not change .jumppos anymore;

//------------------------------------------------------------

static read_info_t* read_infop= NULL;
  // presently used read info structure, i.e. file handle
#define read__buf ((byte*)&read_infop->bufferstart)
  // start address of the file's input buffer
static byte* read_bufp= NULL;  // may be incremented by external
  // up to the number of read_PREFETCH bytes before read_input() is
  // called again;
static byte* read_bufe= NULL;  // may not be changed from external

static int read_open(const char* filename) {
  // open an input file;
  // filename[]: path and name of input file;
  //             ==NULL: standard input;
  // return: 0: ok; !=0: error;
  // read_infop: handle of the file;
  // note that you should close every opened file with read_close()
  // before the program ends;

  // save status of presently processed input file (if any)
  if(read_infop!=NULL) {
    read_infop->bufp= read_bufp;
    read_infop->bufp= read_bufe;
    }

  // get memory space for file information and input buffer
  read_infop= (read_info_t*)malloc(sizeof(read_info_t)+read__bufM);
  if(read_infop==NULL) {
    PERRv("could not get %i bytes of memory.",read__bufM)
return 1;
    }

  // initialize read info structure
  #if read_GZ==0
    read_infop->fd= 0;  // (default) standard input
  #else
    read_infop->fi= NULL;  // (default) file not opened
  #endif
  if((read_infop->isstdin= filename==NULL))
    strcpy(read_infop->filename,"standard input");
  else
    strMcpy(read_infop->filename,filename);
  read_infop->eof= false;  // we are not at the end of input file
  read_infop->bufp= read_infop->bufe= read__buf;  // pointer in buf[]
    // pointer to the end of valid input in buf[]
  read_infop->counter= 0;
  read_infop->jumppos= 0;
    // store start of file as default jump destination

  // set modul-global variables which are associated with this file
  read_bufp= read_infop->bufp;
  read_bufe= read_infop->bufe;

  // open the file
  PLOGv(2,"Read-opening: %s",read_infop->filename)
  if(read_infop->isstdin) {  // stdin shall be used
    #if read_GZ==0
      read_infop->fd= 0;
    #elif read_GZ==1
      read_infop->fi= stdin;
    #else
      read_infop->fi= gzdopen(0,"rb");
      #if read_GZ==3 && ZLIB_VERNUM>=0x1235
        gzbuffer(read_infop->fi,128*1024);
      #endif
    #endif
    }
  else if(filename!=NULL) {  // a real file shall be opened
    #if read_GZ==0
      read_infop->fd= open(filename,O_RDONLY|O_BINARY);
    #elif read_GZ==1
      read_infop->fi= fopen(filename,"rb");
    #else
      read_infop->fi= gzopen(filename,"rb");
      #if read_GZ==3 && ZLIB_VERNUM>=0x1235
        PLOG(2,"Read-opening: increasing gzbuffer.")
        gzbuffer(read_infop->fi,128*1024);
      #endif
    #endif
    #if read_GZ==0
    if(read_infop->fd<0) {
    #else
    if(read_infop->fi==NULL) {
    #endif
      PERRv("could not open input file: %.80s",
        read_infop->filename)
      free(read_infop); read_infop= NULL;
      read_bufp= read_bufe= NULL;
return 1;
      }
    }  // end   a real file shall be opened
return 0;
  }  // end   read_open()

static void read_close() {
  // close an opened file;
  // read_infop: handle of the file which is to close;
  if(read_infop==NULL)  // handle not valid;
return;
  PLOGv(2,"Read-closing: %s",read_infop->filename)
  #if read_GZ==0
    if(read_infop->fd>0)  // not standard input
      close(read_infop->fd);
  #elif read_GZ==1
    if(!read_infop->isstdin)  // not standard input
      fclose(read_infop->fi);
  #else
    gzclose(read_infop->fi);
  #endif
  free(read_infop); read_infop= NULL;
  read_bufp= read_bufe= NULL;
  }  // end   read_close()

static inline bool read_input() {
  // read data from standard input file, use an internal buffer;
  // make data available at read_bufp;
  // read_open() must have been called before calling this procedure;
  // return: there are no (more) bytes to read;
  // read_bufp: start of next bytes available;
  //            may be incremented by the caller, up to read_bufe;
  // read_bufe: end of bytes in buffer;
  //            must not be changed by the caller;
  // after having called this procedure, the caller may rely on
  // having available at least read_PREFETCH bytes at address
  // read_bufp - with one exception: if there are not enough bytes
  // left to read from standard input, every byte after the end of
  // the remaining part of the file in the buffer will be set to
  // 0x00 - up to read_bufp+read_PREFETCH;
  int l,r;

  if(read_bufp+read_PREFETCH>=read_bufe) {  // read buffer is too low
    if(!read_infop->eof) {  // still bytes in the file
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
        l= (read__buf+read__bufM)-read_bufe-4;
          // number of bytes to read
        #if read_GZ==0
          r= read(read_infop->fd,read_bufe,l);
        #elif read_GZ==1
          r= read(fileno(read_infop->fi),read_bufe,l);
        #else
          r= gzread(read_infop->fi,read_bufe,l);
        #endif
        if(r<=0) {  // no more bytes in the file
          read_infop->eof= true;
            // memorize that there we are at end of file
          l= (read__buf+read__bufM)-read_bufe;
            // remaining space in buffer
          if(l>read_PREFETCH) l= read_PREFETCH;
          memset(read_bufe,0,l);  // 2011-12-24
            // set remaining space up to prefetch bytes in buffer to 0
      break;
          }
        read_infop->counter+= r;
        read_bufe+= r;  // set new mark for end of data
        read_bufe[0]= 0; read_bufe[1]= 0;  // set 4 null-terminators
        read_bufe[2]= 0; read_bufe[3]= 0;
        } while(r<l);  // end   while buffer has not been filled
      }  // end   still bytes to read
    }  // end   read buffer is too low
  return read_infop->eof && read_bufp>=read_bufe;
  }  // end   read__input()

static void read_switch(read_info_t* filehandle) {
  // switch to another already opened file;
  // filehandle: handle of the file which shall be switched to;

  // first, save status of presently processed input file
  if(read_infop!=NULL) {
    read_infop->bufp= read_bufp;
    read_infop->bufe= read_bufe;
    }
  // switch to new file information
  read_infop= filehandle;
  read_bufp= read_infop->bufp;
  read_bufe= read_infop->bufe;
  read_input();
  }  // end   read_switch()

static inline int read_rewind() {
  // rewind the file, i.e., the file pointer is set
  // to the first byte in the file;
  // read_infop: handle of the file which is to rewind;
  // return: ==0: ok; !=0: rewind error;
  bool err;

  #if read_GZ==0
    err= lseek(read_infop->fd,0,SEEK_SET)<0;
  #elif read_GZ==1
    err= fseek(read_infop->fi,0,SEEK_SET)<0;
  #else
    err= gzseek(read_infop->fi,0,SEEK_SET)<0;
  #endif
  if(err) {
    PERRv("could not rewind file: %-80s",read_infop->filename)
return 1;
    }
  read_infop->counter= 0;
  read_bufp= read_bufe;  // force refetch
  read_infop->eof= false;  // force retest for end of file
  read_input();  // ensure prefetch
return 0;
  }  // end   read_rewind()

static inline bool read_setjump() {
  // store the current position in the file as a destination
  // for a jump which will follow later;
  // if global_complexways is false, the call will be ignored;
  // the position is not stored anew if it has been locked
  // with read_infop->lockpos;
  // return: jump position has been stored;
  // if(!global_complexways) return false;
  if(read__jumplock)
return false;
  read_infop->jumppos= read_infop->counter-(read_bufe-read_bufp);
  return true;
  }  // end   read_setjump()

static inline void read_lockjump() {
  // prevent a previously stored jump position from being overwritten;
  read__jumplock= true;
  }  // end   read_lockjump()

static int read_jump() {
  // jump to a previously stored location it;
  // return: 0: jump ok;
  //         1: did not actually jump because we already were
  //            at the desired position;
  //         <0: error;
  #if read_GZ<2
    off_t pos;  // present position in the file;
  #else
    #if __WIN32__
      z_off64_t pos;  // position to jump to; -1: invalid
    #else
      z_off_t pos;  // position to jump to; -1: invalid
    #endif
  #endif
  bool err;

  pos= read_infop->counter-(read_bufe-read_bufp);
  if(read_infop->jumppos==-1) {
    PERRv("no jump destination in file: %.80s",read_infop->filename)
return -1;
    }
  #if read_GZ==0
    err= lseek(read_infop->fd,read_infop->jumppos,SEEK_SET)<0;
  #elif read_GZ==1
    err= fseek(read_infop->fi,read_infop->jumppos,SEEK_SET)<0;
  #else
    err= gzseek(read_infop->fi,read_infop->jumppos,SEEK_SET)<0;
  #endif
  if(err) {
    PERRv("could not jump in file: %.80s",read_infop->filename)
return -2;
    }
  if(read_infop->jumppos!=pos) {  // this was a real jump
    read_infop->counter= read_infop->jumppos;
    read_bufp= read_bufe;  // force refetch
    read_infop->eof= false;  // force retest for end of file
    read_input();  // ensure prefetch
return 0;
    }
  // here: did not actually jump because we already were
  // at the desired position
return 1;
  }  // end   read_jump()

//------------------------------------------------------------
// end Module read_   OSM file read module
//------------------------------------------------------------



//------------------------------------------------------------
// Module wrf_   write files module
//------------------------------------------------------------

// this module provides procedures to write into several files
// which are opened at the same time and later closed at the
// same time;
// as usual, all identifiers of a module have the same prefix,
// in this case 'wrf'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define wrf__BUFSIZE 1600000
typedef struct {
  char* buf;  // write buffer
  char* bufe;  // (const) water mark for buffer filled 100%
  char* bufp;  // write pointer in buffer
  int fd;  // file descriptor
  char name[32];  // file name (no path name)
  } wrf__file_t;
static wrf__file_t wrf__file[]= {
  {NULL,NULL,NULL,0,"osmstrings_nodecount"},
  {NULL,NULL,NULL,0,"osmstrings_region"},
  {NULL,NULL,NULL,0,"osmstrings_subregion"},
  {NULL,NULL,NULL,0,"osmstrings_city"},
  {NULL,NULL,NULL,0,"osmstrings_street"},
  {NULL,NULL,NULL,0,"osmstrings_housenumber"} };
#define wrf__M (sizeof(wrf__file)/sizeof(wrf__file[0]))
  // number of files administrated by this module
wrf__file_t* wrf__filee= wrf__file+wrf__M;

static void wrf__end() {
  // terminate the services of this module;
  wrf__file_t* f;

  f= wrf__file;
  do {  // for all files
    // flush
    if(f->bufp>f->buf) {  // at least one character in buffer
      if(write(f->fd,f->buf,f->bufp-f->buf)<0)
        PERRv("flush error at file: %.80s",f->name)
      f->bufp= f->buf;  // reset buffer pointer;
      }  // at least one character in buffer
    // close
    close(f->fd);
    f->fd= 1;
    // free buffer
    FREE(f->buf)
    } while(++f<wrf__filee);  // for all files
  }  // wrf__end()

//------------------------------------------------------------

static bool wrf_open() {
  // open all files administrated by this module;
  // return: success;
  wrf__file_t* f;

  atexit(wrf__end);
  f= wrf__file;
  do {  // for all files
    f->buf= (char*)malloc(wrf__BUFSIZE);
    if(f->buf==NULL) {
      PERR("not enough memory for preparing string files.")
return false;
      }
    f->bufe= f->buf+wrf__BUFSIZE;
    f->bufp= f->buf;
    f->fd= open(f->name,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,00600);
    if(f->fd<=0) {
      PERRv("could not open output file: %.80s",f->name)
      f->fd= 1;
return false;
      }
    } while(++f<wrf__filee);  // for all files
  return true;
  }  // wrf_open()

static inline void wrf_char(int file,int c) {
  // write one byte to a file, use a buffer;
  // file: number of the file (see wrf__file[] definition);
  //       there is not range check;
  // c: character to write;
  wrf__file_t* f;

  f= wrf__file+file;
  if(f->bufp>=f->bufe) {  // write buffer is full
    // flush
    if(write(f->fd,f->buf,f->bufp-f->buf)<0)
      PERRv("write error at file: %.80s",f->name)
    f->bufp= f->buf;  // reset buffer pointer;
    }  // write buffer is full
  *f->bufp++= (char)c;
  }  // wrf_char();

static inline void wrf_mem(int file,const void* bp,int l) {
  // write a memory area to a file, use a buffer;
  // file: number of the file (see wrf__file[] definition);
  //       there is not range check;
  // bp: start of the memory area to write;
  // l: length of the memory area (in bytes);
  wrf__file_t* f;

  f= wrf__file+file;
  while(--l>=0) {  // for all bytes of the memory area
    if(f->bufp>=f->bufe) {  // write buffer is full
      // flush
      if(write(f->fd,f->buf,f->bufp-f->buf)<0)
        PERRv("write error at file: %.80s",f->name)
      f->bufp= f->buf;  // reset buffer pointer;
      }  // write buffer is full
    *f->bufp++= *(char*)bp;
    (char*)bp++;
    }  // for all bytes of the memory area
  }  // wrf_mem();

static inline void wrf_string(int file,const char* s) {
  // write a string to a file, use a buffer;
  // file: number of the file (see wrf__file[] definition);
  //       there is not range check;
  // s: zero-terminated string to write;
  // the terminating zero is also written to the file;
  wrf__file_t* f;

  f= wrf__file+file;
  for(;;) {  // for all bytes of the string
    if(f->bufp>=f->bufe) {  // write buffer is full
      // flush
      if(write(f->fd,f->buf,f->bufp-f->buf)<0)
        PERRv("write error at file: %.80s",f->name)
      f->bufp= f->buf;  // reset buffer pointer;
      }  // write buffer is full
    *f->bufp++= (char)(*s);
    if(*s==0)
  break;
    s++;
    }  // for all bytes of the string
  }  // wrf_string();

//------------------------------------------------------------
// end   Module wrf_   write files module
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

static const char* write__filename= NULL;
  // last name of the file; ==NULL: standard output;
static const char* write__filename_standard= NULL;
  // name of standard output file; ==NULL: standard output;
static const char* write__filename_temp= NULL;
  // name of the tempfile; ==NULL: no tempfile;
static char write__buf[UINT64_C(16000000)];
static char* write__bufe= write__buf+sizeof(write__buf);
  // (const) water mark for buffer filled 100%
static char* write__bufp= write__buf;
static int write__fd= 1;  // (initially standard output)
static int write__fd_standard= 1;  // (initially standard output)
static inline void write_flush();

static void write__close() {
  // close the last opened file;
  PLOGv(2,"Write-closing FD: %i",write__fd)
  write_flush();
  if(write__fd>1) {  // not standard output
    close(write__fd);
    write__fd= 1;
    }
  }  // end   write__close()

static void write__end() {
  // terminate the services of this module;
  if(write__fd>1)
    write__close();
  if(write__fd_standard>1) {
    write__fd= write__fd_standard;
    write__close();
    write__fd_standard= 0;
    }
  if(loglevel<2)
    if(write__filename_temp!=NULL) unlink(write__filename_temp);
  }  // end   write__end()

//------------------------------------------------------------

static bool write_testmode= false;  // no standard output
static bool write_error= false;  // an error has occurred

static inline void write_flush() {
  if(write__bufp>write__buf && !write_testmode)
      // at least one byte in buffer AND not test mode
    write_error|=
      write(write__fd,write__buf,write__bufp-write__buf)<0;
  write__bufp= write__buf;
  }  // end   write_flush();

static int write_open(const char* filename) {
  // open standard output file;
  // filename: name of the output file;
  //           this string must be accessible until program end;
  //           ==NULL: standard output;
  // this procedure must be called before any output is done;
  // return: 0: OK; !=0: error;
  static bool firstrun= true;

  PLOGv(2,"Write-opening: %s",
    filename==NULL? "stdout": filename)
  if(filename!=NULL) {  // not standard output
    write__fd= open(filename,
      O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,00600);
    if(write__fd<1) {
      PERRv("could not open output file: %.80s",filename)
      write__fd= 1;
return 1;
      }
    write__fd_standard= write__fd;
    write__filename_standard= filename;
    }
  if(firstrun) {
    firstrun= false;
    atexit(write__end);
    }
  return 0;
  }  // end   write_open()

static int write_newfile(const char* filename) {
  // change to another (temporary) output file;
  // filename: new name of the output file;
  //           this string must be accessible until program end
  //           because the name will be needed to delete the file;
  //           ==NULL: change back to standard output file;
  // the previous output file is closed by this procedure, unless
  // it is standard output;
  // return: 0: OK; !=0: error;
  PLOGv(2,"Write-opening: %s",
      filename==NULL? "stdout": filename)
  if(filename==NULL) {  // we are to change back to standard output file
    PLOGv(2,"Write-reopening: %s",
        write__filename_standard==NULL? "stdout":
        write__filename_standard)
    write__close();  // close temporary file
    write__filename= write__filename_standard;
    write__fd= write__fd_standard;
    }
  else {  // new temporary file shall be opened
    PLOGv(2,"Write-opening: %s",filename)
    write__filename= filename;
    unlink(filename);
    write__fd= open(filename,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,00600);
    if(write__fd<1) {
      PERRv("could not open output file: %.80s",filename)
      write__fd= 1;
return 2;
      }
    write__filename_temp= filename;
    }
  return 0;
  }  // end   write_newfile()

static inline void write_char(int c) {
  // write one byte to stdout, use a buffer;
  if(write__bufp>=write__bufe) {  // the write buffer is full
    if(!write_testmode)
      write_error|=
        write(write__fd,write__buf,write__bufp-write__buf)<0;
    write__bufp= write__buf;
    }
  *write__bufp++= (char)c;
  }  // end   write_char();

static inline void write_mem(const byte* b,int l) {
  // write a memory area to stdout, use a buffer;
  while(--l>=0) {
    if(write__bufp>=write__bufe) {  // the write buffer is full
      if(!write_testmode)
        write_error|=
          write(write__fd,write__buf,write__bufp-write__buf)<0;
      write__bufp= write__buf;
      }
    *write__bufp++= (char)(*b++);
    }
  }  // end   write_mem();

static inline void write_str(const char* s) {
  // write a string to stdout, use a buffer;
  while(*s!=0) {
    if(write__bufp>=write__bufe) {  // the write buffer is full
      if(!write_testmode)
        write_error|=
          write(write__fd,write__buf,write__bufp-write__buf)<0;
      write__bufp= write__buf;
      }
    *write__bufp++= (char)(*s++);
    }
  }  // end   write_str();

static inline void write_xmlstr(const char* s) {
  // write an XML string to stdout, use a buffer;
  // every character which is not allowed within an XML string
  // will be replaced by the appropriate decimal sequence;
  static byte allowedchar[]= {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,  // \"&'
    0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,  // <>
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,  // {}DEL
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    #if 1
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    #else
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0};
    #endif
  byte b0,b1,b2,b3;
  int i;
  uint32_t u;
  #define write__char_D(c) { \
    if(write__bufp>=write__bufe) { \
      if(!write_testmode) \
        write_error|= \
          write(write__fd,write__buf,write__bufp-write__buf)<0; \
      write__bufp= write__buf; \
      } \
    *write__bufp++= (char)(c); }

  for(;;) {
    b0= *s++;
    if(b0==0)
  break;
    i= allowedchar[b0];
    if(i==0)  // this character may be written as is
      write__char_D(b0)
    else {  // use numeric encoding
      if(--i<=0)  // one byte
        u= b0;
      else {
        b1= *s++;
        if(--i<=0 && b1>=128)  // two bytes
          u= ((b0&0x1f)<<6)+(b1&0x3f);
        else {
          b2= *s++;
          if(--i<=0 && b1>=128 && b2>=128)  // three bytes
            u= ((b0&0x0f)<<12)+((b1&0x3f)<<6)+(b2&0x3f);
          else {
            b3= *s++;
            if(--i<=0 && b1>=128 && b2>=128 && b3>=128)  // four bytes
              u= ((b0&0x07)<<18)+((b1&0x3f)<<12)+
                ((b1&0x3f)<<6)+(b2&0x3f);
            else
              u= (byte)'?';
            }
          }
        }
      write__char_D('&') write__char_D('#')
      if(u<100) {
        if(u>=10)
          write__char_D(u/10+'0')
        write__char_D(u%10+'0')
        }
      else if(u<1000) {
        write__char_D(u/100+'0')
        write__char_D((u/10)%10+'0')
        write__char_D(u%10+'0')
        }
      else {
        char st[30];

        uint32toa(u,st);
        write_str(st);
        }
      write__char_D(';')
      }  // use numeric encoding
    }
  #undef write__char_D
  }  // end   write_xmlstr();

static inline void write_xmlmnstr(const char* s) {
  // write an XML string to stdout, use a buffer;
  // every character which is not allowed within an XML string
  // will be replaced by the appropriate mnemonic or decimal sequence;
  static const byte allowedchar[]= {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,9,0,0,0,9,9,0,0,0,0,0,0,0,0,  // \"&'
    0,0,0,0,0,0,0,0,0,0,0,0,9,0,9,0,  // <>
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,  // {}DEL
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    #if 1
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    #else
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0};
    #endif
  byte b0,b1,b2,b3;
  int i;
  uint32_t u;
  #define write__char_D(c) { \
    if(write__bufp>=write__bufe) { \
      if(!write_testmode) \
        write_error|= \
          write(write__fd,write__buf,write__bufp-write__buf)<0; \
      write__bufp= write__buf; \
      } \
    *write__bufp++= (char)(c); }
  #define D(i) ((byte)(s[i]))
  #define DD ((byte)c)

  for(;;) {
    b0= *s++;
    if(b0==0)
  break;
    i= allowedchar[b0];
    if(i==0)  // this character may be written as is
      write__char_D(b0)
    else if(i==9) {  // there is a mnemonic for this character
      write__char_D('&')
      switch(b0) {
      case '\"':
        write__char_D('q') write__char_D('u') write__char_D('o')
        write__char_D('t')
        break;
      case '&':
        write__char_D('a') write__char_D('m') write__char_D('p')
        break;
      case '\'':
        write__char_D('a') write__char_D('p') write__char_D('o')
        write__char_D('s')
        break;
      case '<':
        write__char_D('l') write__char_D('t')
        break;
      case '>':
        write__char_D('g') write__char_D('t')
        break;
      default:
        write__char_D('?')  // (should never reach here)
        }
      write__char_D(';')
      }  // there is a mnemonic for this character
    else {  // use numeric encoding
      if(--i<=0)  // one byte
        u= b0;
      else {
        b1= *s++;
        if(--i<=0 && b1>=128)  // two bytes
          u= ((b0&0x1f)<<6)+(b1&0x3f);
        else {
          b2= *s++;
          if(--i<=0 && b1>=128 && b2>=128)  // three bytes
            u= ((b0&0x0f)<<12)+((b1&0x3f)<<6)+(b2&0x3f);
          else {
            b3= *s++;
            if(--i<=0 && b1>=128 && b2>=128 && b3>=128)  // four bytes
              u= ((b0&0x07)<<18)+((b1&0x3f)<<12)+
                ((b1&0x3f)<<6)+(b2&0x3f);
            else
              u= (byte)'?';
            }
          }
        }
      write__char_D('&') write__char_D('#')
      if(u<100) {
        if(u>=10)
          write__char_D(u/10+'0')
        write__char_D(u%10+'0')
        }
      else if(u<1000) {
        write__char_D(u/100+'0')
        write__char_D((u/10)%10+'0')
        write__char_D(u%10+'0')
        }
      else {
        char st[30];

        uint32toa(u,st);
        write_str(st);
        }
      write__char_D(';')
      }  // use numeric encoding
    }
  #undef DD
  #undef D
  #undef write__char_D
  }  // end   write_xmlmnstr();

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

#if 0  // currently unused
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
#endif

static inline void write_uint64(uint64_t v) {
  // write an unsigned 64 bit integer number to standard output;
  char s[30],*s1,*s2,c;

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
  }  // end write_uint64()

static inline void write_createsint64(int64_t v,char* sp) {
  // create a signed 64 bit integer number;
  // return:
  // sp[30]: value v as decimal integer string;
  static char *s1,*s2,c;

  s1= sp;
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
  }  // end write_sint64()

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

static inline char* write_createsfix7o(int32_t v,char* s) {
  // convert a signed 7 decimals fixpoint value into a string;
  // keep trailing zeros;
  // v: fixpoint value
  // return: pointer to string terminator;
  // s[12]: destination string;
  char* s1,*s2,*sterm,c;
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
  sterm= s2;
  *s2--= 0;
  while(s2>s1)
    { c= *s1; *s1= *s2; *s2= c; s1++; s2--; }
  return sterm;
  }  // end write_createsfix7o()

static inline void write_sfix7(int32_t v) {
  // write a signed 7 decimals fixpoint value to standard output;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 7;
  while((v%10)==0 && i>1)  // trailing zeros
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
  // keep trailing zeros;
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

static inline void write_sfix6o(int32_t v) {
  // write a signed 6 decimals fixpoint value to standard output;
  // keep trailing zeros;
  char s[20],*s1,*s2,c;
  int i;

  s1= s;
  if(v<0)
    { *s1++= '-'; v= -v; }
  s2= s1;
  i= 6;
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
  }  // end write_sfix6o()

#if 0  // currently unused
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
#endif

static inline void write_createtimestamp(uint64_t v,char* sp) {
  // write a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z",
  // into a string;
  // v: value of the timestamp;
  // sp[21]: destination string;
  time_t vtime;
  struct tm tm;
  int i;

  vtime= v;
  #if __WIN32__
  memcpy(&tm,gmtime(&vtime),sizeof(tm));
  #else
  gmtime_r(&vtime,&tm);
  #endif
  i= tm.tm_year+1900;
  sp+= 3; *sp--= i%10+'0';
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
  }  // end   write_createtimestamp()

static inline void write_timestamp(uint64_t v) {
  // write a timestamp in OSM format, e.g.: "2010-09-30T19:23:30Z"
  char s[30];

  write_createtimestamp(v,s);
  write_str(s);
  }  // end   write_timestamp()

//------------------------------------------------------------
// end   Module write_   write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module o5_   o5m conversion module
//------------------------------------------------------------

// this module provides procedures which convert data to
// o5m format;
// as usual, all identifiers of a module have the same prefix,
// in this case 'o5'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static inline void stw_reset();

#define o5__bufM UINT64_C(5000000)
static byte* o5__buf= NULL;  // buffer for one object in .o5m format
static byte* o5__bufe= NULL;
  // (const) water mark for buffer filled nearly 100%
static byte* o5__bufp= NULL;
static byte* o5__bufr0= NULL,*o5__bufr1= NULL;
  // start end end mark of a reference area in o5__buf[];
  // ==NULL: no mark set;

// basis for delta coding
static int64_t o5_id;
static uint32_t o5_lat,o5_lon;
static int64_t o5_cset;
static int64_t o5_time;
static int64_t o5_ref[3];  // for node, way, relation

static inline void o5__resetvars() {
  // reset all delta coding counters;
  o5__bufp= o5__buf;
  o5__bufr0= o5__bufr1= o5__buf;
  o5_id= 0;
  o5_lat= o5_lon= 0;
  o5_cset= 0;
  o5_time= 0;
  o5_ref[0]= o5_ref[1]= o5_ref[2]= 0;
  stw_reset();
  }  // end   o5__resetvars()

static void o5__end() {
  // clean-up for o5 module;
  // will be called at program's end;
  if(o5__buf!=NULL) {
    free(o5__buf); o5__buf= NULL; }
  }  // end   o5__end()

//------------------------------------------------------------

static inline void o5_reset() {
  // perform and write an o5m Reset;
  o5__resetvars();
  write_char(0xff);  // write .o5m Reset
  }  // end   o5_reset()

static int o5_ini() {
  // initialize this module;
  // must be called before any other procedure is called;
  // return: 0: everything went ok;
  //         !=0: an error occurred;
  static bool firstrun= true;

  if(firstrun) {
    firstrun= false;
    o5__buf= (byte*)malloc(o5__bufM);
    if(o5__buf==NULL)
return 1;
    atexit(o5__end);
    o5__bufe= o5__buf+o5__bufM-400000;
    }
  o5__resetvars();
  return 0;
  }  // end   o5_ini()

static inline void o5_byte(byte b) {
  // write a single byte;
  // writing starts at position o5__bufp;
  // o5__bufp: incremented by 1;
  *o5__bufp++= b;
  }  // end   o5_byte()

static inline int o5_str(const char* s) {
  // write a zero-terminated string;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  byte c;

  p0= o5__bufp;
  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
    PERR(".o5m memory overflow.")
return 0;
      }
    }
  do *o5__bufp++= c= *s++;
    while(c!=0);
return o5__bufp-p0;
  }  // end   o5_str()

static inline int o5_uvar32buf(byte* p,uint32_t v) {
  // write an unsigned 32 bit integer as Varint into a buffer;
  // writing starts at position p;
  // return: bytes written;
  byte* p0;
  uint32_t frac;

  p0= p;
  frac= v&0x7f;
  if(frac==v) {  // just one byte
    *p++= frac;
return 1;
    }
  do {
    *p++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    } while(frac!=v);
  *p++= frac;
return p-p0;
  }  // end   o5_uvar32buf()

static inline int o5_uvar32(uint32_t v) {
  // write an unsigned 32 bit integer as Varint;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  uint32_t frac;

  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
      PERR(".o5m memory overflow.")
return 0;
      }
    }
  p0= o5__bufp;
  frac= v&0x7f;
  if(frac==v) {  // just one byte
    *o5__bufp++= frac;
return 1;
    }
  do {
    *o5__bufp++= frac|0x80;
    v>>= 7;
    frac= v&0x7f;
    } while(frac!=v);
  *o5__bufp++= frac;
return o5__bufp-p0;
  }  // end   o5_uvar32()

static inline int o5_svar32(int32_t v) {
  // write a signed 32 bit integer as signed Varint;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  uint32_t u;
  uint32_t frac;

  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
      PERR(".o5m memory overflow.");
return 0;
      }
    }
  p0= o5__bufp;
  if(v<0) {
    u= -v;
    u= (u<<1)-1;
    }
  else
    u= v<<1;
  frac= u&0x7f;
  if(frac==u) {  // just one byte
    *o5__bufp++= frac;
return 1;
    }
  do {
    *o5__bufp++= frac|0x80;
    u>>= 7;
    frac= u&0x7f;
    } while(frac!=u);
  *o5__bufp++= frac;
return o5__bufp-p0;
  }  // end   o5_svar32()

static inline int o5_svar64(int64_t v) {
  // write a signed 64 bit integer as signed Varint;
  // writing starts at position o5__bufp;
  // return: bytes written;
  // o5__bufp: increased by the number of written bytes;
  byte* p0;
  uint64_t u;
  uint32_t frac;

  if(o5__bufp>=o5__bufe) {
    static int msgn= 1;
    if(--msgn>=0) {
      PERR(".o5m memory overflow.")
return 0;
      }
    }
  p0= o5__bufp;
  if(v<0) {
    u= -v;
    u= (u<<1)-1;
    }
  else
    u= v<<1;
  frac= u&0x7f;
  if(frac==u) {  // just one byte
    *o5__bufp++= frac;
return 1;
    }
  do {
    *o5__bufp++= frac|0x80;
    u>>= 7;
    frac= u&0x7f;
    } while(frac!=u);
  *o5__bufp++= frac;
return o5__bufp-p0;
  }  // end   o5_svar64()

static inline void o5_markref(int pos) {
  // mark reference area;
  // pos: ==0: start; ==1: end;
  //      0 is accepted only once per dataset; only the first
  //      request is valid;
  //      1 may be repeated, the last one counts;
  if(pos==0) {
    if(o5__bufr0==o5__buf) o5__bufr0= o5__bufp;
    }
  else
    o5__bufr1= o5__bufp;
  }  // end   o5_markref()

static inline void o5_type(int type) {
  // mark object type we are going to process now;
  // should be called every time a new object is started to be
  // written into o5_buf[];
  // type: object type; 0: node; 1: way; 2: relation;
  //       if object type has changed, a 0xff byte ("reset")
  //       will be written;
  static int oldtype= -1;

  // process changes of object type
  if(type!=oldtype) {  // object type has changed
    oldtype= type;
    o5_reset();
    }
  oldtype= type;
  }  // end   o5_type()

static void o5_write() {
  // write o5__buf[] contents to standard output;
  // include object length information after byte 0 and include
  // ref area length information right before o5__bufr0 (if !=NULL);
  // if buffer is empty, this procedure does nothing;
  byte lens[30],reflens[30];  // lengths as pbf numbers
  int len;  // object length
  int reflen;  // reference area length
  int reflenslen;  // length of pbf number of reflen

  // get some length information
  len= o5__bufp-o5__buf;
  if(len<=0) goto o5_write_end;
  reflen= 0;  // (default)
  if(o5__bufr1<o5__bufr0) o5__bufr1= o5__bufr0;
  if(o5__bufr0>o5__buf) {
      // reference area contains at least 1 byte
    reflen= o5__bufr1-o5__bufr0;
    reflenslen= o5_uvar32buf(reflens,reflen);
    len+= reflenslen;
    }  // end   reference area contains at least 1 byte

  // write header
  if(--len>=0) {
    write_char(o5__buf[0]);
    write_mem(lens,o5_uvar32buf(lens,len));
    }

  // write body
  if(o5__bufr0<=o5__buf)  // no reference area
    write_mem(o5__buf+1,o5__bufp-(o5__buf+1));
  else {  // valid reference area
    write_mem(o5__buf+1,o5__bufr0-(o5__buf+1));
    write_mem(reflens,reflenslen);
    write_mem(o5__bufr0,o5__bufp-o5__bufr0);
    }  // end   valid reference area

  // reset buffer pointer
  o5_write_end:
  o5__bufp= o5__buf;  // set original buffer pointer to buffer start
  o5__bufr0= o5__bufr1= o5__buf;  // clear reference area marks
  }  // end   o5_write()

//------------------------------------------------------------
// end   Module o5_   o5m conversion module
//------------------------------------------------------------



//------------------------------------------------------------
// Module stw_   string write module
//------------------------------------------------------------

// this module provides procedures for conversions from
// c formatted strings into referenced string data stream objects
// - and writing it to buffered standard output;
// as usual, all identifiers of a module have the same prefix,
// in this case 'stw'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define stw__tabM 15000
#define stw__tabstrM 250  // must be < row size of stw__rab[]
#define stw__hashtabM 150001  // (preferably a prime number)
static char stw__tab[stw__tabM][256];
  // string table; see o5m documentation;
  // row length must be at least stw__tabstrM+2;
  // each row contains a double string; each of the two strings
  // is terminated by a zero byte, the lengths must not exceed
  // stw__tabstrM bytes in total;
static int stw__tabi= 0;
  // index of last entered element in string table
static int stw__hashtab[stw__hashtabM];
  // has table; elements point to matching strings in stw__tab[];
  // -1: no matching element;
static int stw__tabprev[stw__tabM],stw__tabnext[stw__tabM];
  // for to chaining of string table rows which match
  // the same hash value; matching rows are chained in a loop;
  // if there is only one row matching, it will point to itself;
static int stw__tabhash[stw__tabM];
  // has value of this element as a link back to the hash table;
  // a -1 element indicates that the string table entry is not used;

static inline int stw__hash(const char* s1,const char* s2) {
  // get hash value of a string pair;
  // s2: ==NULL: single string; this is treated as s2=="";
  // return: hash value in the range 0..(stw__hashtabM-1);
  // -1: the strings are longer than stw__tabstrM characters in total;
  uint32_t h;
  uint32_t c;
  int len;

  len= stw__tabstrM;
  h= 0;
  for(;;) {
    if((c= *s1++)==0 || --len<0) break; h+= c; 
    if((c= *s1++)==0 || --len<0) break; h+= c<<8;
    if((c= *s1++)==0 || --len<0) break; h+= c<<16;
    if((c= *s1++)==0 || --len<0) break; h+= c<<24;
    }
  if(s2!=NULL) for(;;) {
    if((c= *s2++)==0 || --len<0) break; h+= c; 
    if((c= *s2++)==0 || --len<0) break; h+= c<<8;
    if((c= *s2++)==0 || --len<0) break; h+= c<<16;
    if((c= *s2++)==0 || --len<0) break; h+= c<<24;
    }
  if(len<0)
return -1;
  h%= stw__hashtabM;
  return h;
  }  // end   stw__hash()

static inline int stw__getref(int stri,const char* s1,const char* s2) {
  // get the string reference of a string pair;
  // the strings must not have more than 250 characters in total
  // (252 including terminators), there is no check in this procedure;
  // stri: presumed index in string table (we got it from hash table);
  //       must be >=0 and <stw__tabM, there is no boundary check;
  // s2: ==NULL: it's not a string pair but a single string;
  // stw__hashnext[stri]: chain to further occurrences;
  // return: reference of the string;
  //         ==-1: this string is not stored yet
  int strie;  // index of last occurrence
  const char* sp,*tp;
  int ref;

  if(s2==NULL) s2="";
  strie= stri;
  do {
    // compare the string (pair) with the tab entry
    tp= stw__tab[stri];
    sp= s1;
    while(*tp==*sp && *tp!=0) { tp++; sp++; }
    if(*tp==0 && *sp==0) {
        // first string identical to first string in table
      tp++;  // jump over first string terminator
      sp= s2;
      while(*tp==*sp && *tp!=0) { tp++; sp++; }
      if(*tp==0 && *sp==0) {
          // second string identical to second string in table
        ref= stw__tabi-stri;
        if(ref<=0) ref+= stw__tabM;
return ref;
        }
      }  // end   first string identical to first string in table
    stri= stw__tabnext[stri];
    } while(stri!=strie);
  return -1;
  }  // end   stw__getref()

//------------------------------------------------------------

static inline void stw_reset() {
  // clear string table and string hash table;
  // must be called before any other procedure of this module
  // and may be called every time the string processing shall
  // be restarted;
  int i;

  stw__tabi= 0;
  i= stw__tabM;
  while(--i>=0) stw__tabhash[i]= -1;
  i= stw__hashtabM;
  while(--i>=0) stw__hashtab[i]= -1;
  }  // end   stw_reset()

static void stw_write(const char* s1,const char* s2) {
  // write a string (pair), e.g. key/val, to o5m buffer;
  // if available, write a string reference instead of writing the
  // string pair directly;
  // no reference is used if the strings are longer than
  // 250 characters in total (252 including terminators);
  // s2: ==NULL: it's not a string pair but a single string;
  int h;  // hash value
  int ref;

  /* try to find a matching string (pair) in string table */ {
    int i;  // index in stw__tab[]

    ref= -1;  // ref invalid (default)
    h= stw__hash(s1,s2);
    if(h>=0) {  // string (pair) short enough for the string table
      i= stw__hashtab[h];
      if(i>=0)  // string (pair) presumably stored already
        ref= stw__getref(i,s1,s2);
      }  // end   string (pair) short enough for the string table
    if(ref>=0) {  // we found the string (pair) in the table
      o5_uvar32(ref);  // write just the reference
return;
      }  // end   we found the string (pair) in the table
    else {  // we did not find the string (pair) in the table
      // write string data
      o5_byte(0); o5_str(s1);
      if(s2!=NULL) o5_str(s2);  // string pair, not a single string
      if(h<0)  // string (pair) too long,
          // cannot be stored in string table
return;
      }  // end   we did not find the string (pair) in the table
    }  // end   try to find a matching string (pair) in string table
  // here: there is no matching string (pair) in the table

  /* free new element - if still being used */ {
    int h0;  // hash value of old element

    h0= stw__tabhash[stw__tabi];
    if(h0>=0) {  // new element in string table is still being used
      // delete old element
      if(stw__tabnext[stw__tabi]==stw__tabi)
          // self-chain, i.e., only this element
        stw__hashtab[h0]= -1;  // invalidate link in hash table
      else {  // one or more other elements in chain
        stw__hashtab[h0]= stw__tabnext[stw__tabi];  // just to ensure
          // that hash entry does not point to deleted element
        // now unchain deleted element
        stw__tabprev[stw__tabnext[stw__tabi]]= stw__tabprev[stw__tabi];
        stw__tabnext[stw__tabprev[stw__tabi]]= stw__tabnext[stw__tabi];
        }  // end   one or more other elements in chain
      }  // end   next element in string table is still being used
    }  // end   free new element - if still being used

  /* enter new string table element data */ {
    char* sp;
    int i;

    sp= stpcpy0(stw__tab[stw__tabi],s1)+1;
      // write first string into string table
    if(s2==NULL)  // single string
      *sp= 0;  // second string must be written as empty string
        // into string table
    else
      stpcpy0(sp,s2);  // write second string into string table
    i= stw__hashtab[h];
    if(i<0)  // no reference in hash table until now
      stw__tabprev[stw__tabi]= stw__tabnext[stw__tabi]= stw__tabi;
        // self-link the new element;
    else {  // there is already a reference in hash table
      // in-chain the new element
      stw__tabnext[stw__tabi]= i;
      stw__tabprev[stw__tabi]= stw__tabprev[i];
      stw__tabnext[stw__tabprev[stw__tabi]]= stw__tabi;
      stw__tabprev[i]= stw__tabi;
      }
    stw__hashtab[h]= stw__tabi;  // link the new element to hash table
    stw__tabhash[stw__tabi]= h;  // backlink to hash table element
    // new element now in use; set index to oldest element
    if(++stw__tabi>=stw__tabM) {  // index overflow
      stw__tabi= 0;  // restart index
      if(loglevel>=2) {
        static int rs= 0;
        PINFOv("String table index restart %i",++rs)
        }
      }  // end   index overflow
    }  // end   enter new string table element data
  }  // end   stw_write()

//------------------------------------------------------------
// end   Module stw_   string write module
//------------------------------------------------------------



//------------------------------------------------------------
// Module str_   string read module
//------------------------------------------------------------

// this module provides procedures for conversions from
// strings which have been stored in data stream objects to
// c-formatted strings;
// as usual, all identifiers of a module have the same prefix,
// in this case 'str'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define str__tabM (15000+4000)
  // +4000 because it might happen that an object has a lot of
  // key/val pairs or refroles which are not stored already;
#define str__tabstrM 250  // must be < row size of str__rab[]
typedef struct str__info_struct {
  // members of this structure must not be accessed
  // from outside this module;
  char tab[str__tabM][256];
    // string table; see o5m documentation;
    // row length must be at least str__tabstrM+2;
    // each row contains a double string; each of the two strings
    // is terminated by a zero byte, the logical lengths must not
    // exceed str__tabstrM bytes in total;
    // the first str__tabM lines of this array are used as
    // input buffer for strings;
  int tabi;  // index of last entered element in string table;
  int tabn;  // number of valid strings in string table;
  struct str__info_struct* prev;  // address of previous unit;
  } str_info_t;
str_info_t* str__infop= NULL;

static void str__end() {
  // clean-up this module;
  str_info_t* p;

  while(str__infop!=NULL) {
    p= str__infop->prev;
    free(str__infop);
    str__infop= p;
    }
  }  // end str__end()

//------------------------------------------------------------

static str_info_t* str_open() {
  // open an new string client unit;
  // this will allow us to process multiple o5m input files;
  // return: handle of the new unit;
  //         ==NULL: error;
  // you do not need to care about closing the unit(s);
  static bool firstrun= true;
  str_info_t* prev;

  prev= str__infop;
  str__infop= (str_info_t*)malloc(sizeof(str_info_t));
  if(str__infop==NULL) {
    PERR("could not get memory for string buffer.")
return NULL;
    }
  str__infop->tabi= 0;
  str__infop->tabn= 0;
  str__infop->prev= prev;
  if(firstrun) {
    firstrun= false;
    atexit(str__end);
    }
  return str__infop;
  }  // end   str_open()

static inline void str_switch(str_info_t* sh) {
  // switch to another string unit
  // sh: string unit handle;
  str__infop= sh;
  }  // end str_switch()

static inline void str_reset() {
  // clear string table;
  // must be called before any other procedure of this module
  // and may be called every time the string processing shall
  // be restarted;
  if(str__infop!=NULL)
    str__infop->tabi= str__infop->tabn= 0;
  }  // end   str_reset()

static void str_read(byte** pp,char** s1p,char** s2p) {
  // read an o5m formatted string (pair), e.g. key/val, from
  // standard input buffer;
  // if got a string reference, resolve it, using an internal
  // string table;
  // no reference is used if the strings are longer than
  // 250 characters in total (252 including terminators);
  // pp: address of a buffer pointer;
  //     this pointer will be incremented by the number of bytes
  //     the converted protobuf element consumes;
  // s2p: ==NULL: read not a string pair but a single string;
  // return:
  // *s1p,*s2p: pointers to the strings which have been read;
  char* p;
  int len1,len2;
  int ref;
  bool donotstore;  // string has 'do not store flag'  2012-10-01

  p= (char*)*pp;
  if(*p==0) {  // string (pair) given directly
    p++;
    donotstore= false;
    *s1p= p;
    len1= strlen(p);
    p+= len1+1;
    if(s2p==NULL) {  // single string
      if(!donotstore && len1<=str__tabstrM) {
          // single string short enough for string table
        stpcpy0(str__infop->tab[str__infop->tabi],*s1p)[1]= 0;
          // add a second terminator, just in case someone will try
          // to read this single string as a string pair later;
        if(++str__infop->tabi>=str__tabM) str__infop->tabi= 0;
        if(str__infop->tabn<str__tabM) str__infop->tabn++;
        }  // end   single string short enough for string table
      }  // end   single string
    else {  // string pair
      *s2p= p;
      len2= strlen(p);
      p+= len2+1;
      if(!donotstore && len1+len2<=str__tabstrM) {
          // string pair short enough for string table
        memcpy(str__infop->tab[str__infop->tabi],*s1p,len1+len2+2);
        if(++str__infop->tabi>=str__tabM) str__infop->tabi= 0;
        if(str__infop->tabn<str__tabM) str__infop->tabn++;
        }  // end   string pair short enough for string table
      }  // end   string pair
    *pp= (byte*)p;
    }  // end   string (pair) given directly
  else {  // string (pair) given by reference
    ref= pbf_uint32(pp);
    if(ref>str__infop->tabn) {  // string reference invalid
      PWARNv("invalid .o5m string reference: %i->%i",
        str__infop->tabn,ref)
      *s1p= "(invalid)";
      if(s2p!=NULL)  // caller wants a string pair
        *s2p= "(invalid)";
      }  // end   string reference invalid
    else {  // string reference valid
      ref= str__infop->tabi-ref;
      if(ref<0) ref+= str__tabM;
      *s1p= str__infop->tab[ref];
      if(s2p!=NULL)  // caller wants a string pair
        *s2p= strchr(str__infop->tab[ref],0)+1;
      }  // end   string reference valid
    }  // end   string (pair) given by reference
  }  // end   str_read()

//------------------------------------------------------------
// end   Module str_   string read module
//------------------------------------------------------------



//------------------------------------------------------------
// Module wo5_   write o5m module
//------------------------------------------------------------

// this module provides procedures which write osm objects;
// it uses procedures from module o5_;
// as usual, all identifiers of a module have the same prefix,
// in this case 'wo5'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static inline void wo5__author(int32_t hisver,int64_t histime,
    int64_t hiscset,uint32_t hisuid,const char* hisuser) {
  // write osm object author;
  // must not be called if writing PBF format;
  // hisver: version; 0: no author is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  // global_fakeauthor: the author contents will be faked that way
  //                     that the author data will be as short as
  //                     possible;
  // global fakeversion: same as global_fakeauthor, but for .osm
  //                     format: just the version will be written;
  // note that when writing o5m format, this procedure needs to be
  // called even if there is no author information to be written;
  // PBF and csv: this procedure is not called;
  if(global_fakeauthor|global_fakeversion) {
    hisver= 1; histime= 1; hiscset= 1; hisuid= 0; hisuser= "";
    }
  if(hisver==0 || global_dropversion)  // no version number
    o5_byte(0x00);
  else {  // version number available
    o5_uvar32(hisver);
    if(global_dropauthor) histime= 0;
    o5_svar64(histime-o5_time); o5_time= histime;
    if(histime!=0) {
        // author information available
      o5_svar64(hiscset-o5_cset); o5_cset= hiscset;
      if(hisuid==0 || hisuser==NULL || hisuser[0]==0)
          // user identification not available
        stw_write("","");
      else {  // user identification available
        byte uidbuf[30];

        uidbuf[o5_uvar32buf(uidbuf,hisuid)]= 0;
        stw_write((const char*)uidbuf,hisuser);
        }  // end   user identification available
      }  // end   author information available
    }  // end   version number available
  }  // end   wo5__author()

//------------------------------------------------------------

static void wo5_start(int format,bool bboxvalid,
    int32_t x1,int32_t y1,int32_t x2,int32_t y2,int64_t timestamp) {
  // start writing .o5m objects;
  // format: must be 0; parameter just for compatibility reasons;
  // bboxvalid: the following bbox coordinates are valid;
  // x1,y1,x2,y2: bbox coordinates (base 10^-7);
  // timestamp: file timestamp; ==0: no timestamp given;
  static const byte o5mfileheader[]= {0xff,0xe0,0x04,'o','5','m','2'};
  static const byte o5cfileheader[]= {0xff,0xe0,0x04,'o','5','c','2'};

  if(format!=0) {
    PERRv("wrong output file format: %d",format);
    format= 0;
    }

  if(global_outo5c)
    write_mem(o5cfileheader,sizeof(o5cfileheader));
  else
    write_mem(o5mfileheader,sizeof(o5mfileheader));
  if(timestamp!=0) {  // timestamp has been supplied
    o5_byte(0xdc);  // timestamp
    o5_svar64(timestamp);
    o5_write();  // write this object
    }
  if(bboxvalid) {
      // borders are to be applied OR bbox has been supplied
    o5_byte(0xdb);  // border box
    o5_svar32(x1); o5_svar32(y1);
    o5_svar32(x2); o5_svar32(y2);
    o5_write();  // write this object
    }
  }  // end   wo5_start()

static void wo5_end() {
  // end writing .o5m objects;
  o5_write();  // write last object - if any
  write_char(0xfe);  // write o5m eof indicator
  }  // end   wo5_end()

static inline void wo5_flush() {
  // write temporarily stored object information;
  o5_write();  // write last object - if any
  write_flush();
  }  // end   wo5_flush()

static inline void wo5_reset() {
  // write an .o5m-format Reset (see format definition in OSM Wiki);
  // note that this is done automatically at every change of
  // object type; this procedure offers to write additional Resets
  // at every time you want;
  o5_reset();  // (just a wrapper)
  }  // end   wo5_reset()

static inline void wo5_node(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser,int32_t lon,int32_t lat) {
  // write .o5m node body;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  // lon: latitude in 100 nanodegree;
  // lat: latitude in 100 nanodegree;
  o5_write();  // write last object - if any
  o5_type(0);
  o5_byte(0x10);  // data set id for node
  o5_svar64(id-o5_id); o5_id= id;
  wo5__author(hisver,histime,hiscset,hisuid,hisuser);
  o5_svar32(lon-o5_lon); o5_lon= lon;
  o5_svar32(lat-o5_lat); o5_lat= lat;
  }  // end   wo5_node()

static void wo5_node_close() {
  // complete writing an .o5m node;
  // does actually nothing at present; might change in future;
  }  // end   wo5_node_close()

static inline void wo5_way(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write .o5m way body;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  o5_write();  // write last object - if any
  o5_type(1);
  o5_byte(0x11);  // data set id for way
  o5_svar64(id-o5_id); o5_id= id;
  wo5__author(hisver,histime,hiscset,hisuid,hisuser);
  o5_markref(0);
  }  // end   wo5_way()

static inline void wo5_way_close() {
  // complete writing an OSM way;
  // does actually nothing at present; might change in future;
  }  // end   wo5_way_close()

static inline void wo5_relation(int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write .o5m relation body;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  o5_write();  // write last object - if any
  o5_type(2);
  o5_byte(0x12);  // data set id for relation
  o5_svar64(id-o5_id); o5_id= id;
  wo5__author(hisver,histime,hiscset,hisuid,hisuser);
  o5_markref(0);
  }  // end   wo5_relation()

static inline void wo5_relation_close() {
  // complete writing an OSM relation;
  // does actually nothing at present; might change in future;
  }  // end   wo5_relation_close()

static void wo5_delete(int otype,int64_t id,
    int32_t hisver,int64_t histime,int64_t hiscset,
    uint32_t hisuid,const char* hisuser) {
  // write .o5m delete request;
  // this is possible for o5m format only;
  // for any other output format, this procedure does nothing;
  // otype: 0: node; 1: way; 2: relation;
  // id: id of this object;
  // hisver: version; 0: no author information is to be written
  //                     (necessary if o5m format);
  // histime: time (seconds since 1970)
  // hiscset: changeset
  // hisuid: uid
  // hisuser: user name
  if(otype<0 || otype>2)
return;
  o5_write();  // write last object - if any
  o5_type(otype);
  o5_byte(0x10+otype);  // data set id
  o5_svar64(id-o5_id); o5_id= id;
  wo5__author(hisver,histime,hiscset,hisuid,hisuser);
  }  // end   wo5_delete()

static inline void wo5_noderef(int64_t noderef) {
  // write .o5m object node reference;
  o5_svar64(noderef-o5_ref[0]); o5_ref[0]= noderef;
  o5_markref(1);
  }  // end   wo5_noderef()

static inline void wo5_ref(int64_t refid,int reftype,
    const char* refrole) {
  // write .o5m object reference;
  char o5typerole[4000];

  o5_svar64(refid-o5_ref[reftype]); o5_ref[reftype]= refid;
  o5typerole[0]= reftype+'0';
  strmcpy(o5typerole+1,refrole,sizeof(o5typerole)-1);
  stw_write(o5typerole,NULL);
  o5_markref(1);
  }  // end   wo5_ref()

static inline void wo5_node_keyval(const char* key,const char* val) {
  // write an .o5m node object's keyval;
  stw_write(key,val);  // (just a wrapper)
  }  // end   wo5_node_keyval()

static inline void wo5_wayrel_keyval(const char* key,const char* val) {
  // write an .o5m way or relation object's keyval;
  stw_write(key,val);  // (just a wrapper)
  }  // end   wo5_wayrel_keyval()

//------------------------------------------------------------
// end   Module wo5_   write o5m module
//------------------------------------------------------------



//------------------------------------------------------------
// Module oo_   osm processing module
//------------------------------------------------------------

// this module provides procedures which read osm objects,
// process them and write them as polygon objects;
// as usual, all identifiers of a module have the same prefix,
// in this case 'oo'; one underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static int64_t oo__strtosint64(const char* s) {
  // read a number and convert it to a signed 64-bit integer;
  // return: number;
  int sign;
  int64_t i;
  byte b;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  i= 0;
  for(;;) {
    b= (byte)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i*sign;
  }  // end   oo__strtosint64()

typedef struct {
  read_info_t* ri;  // file handles for input files
  int format;  // input file format;
    // ==-9: unknown; ==0: o5m; ==10: xml; ==-1: pbf;
  str_info_t* str;  // string unit handle (if o5m format)
  const char* filename;
  bool endoffile;
  int deleteobject;  // replacement for .osc <delete> tag
    // 0: not to delete; 1: delete this object; 2: delete from now on;
  int64_t o5id;  // for o5m delta coding
  int32_t o5lon,o5lat;  // for o5m delta coding
  int64_t o5histime;  // for o5m delta coding
  int64_t o5hiscset;  // for o5m delta coding
  int64_t o5rid[3];  // for o5m delta coding
  } oo__if_t;
static oo__if_t oo__if[global_fileM];
static oo__if_t* oo__ifp= oo__if;  // currently used element in oo__if[]
#define oo__ifI (oo__ifp-oo__if)  // index
static oo__if_t* oo__ife= oo__if;  // logical end of elements in oo__if[]
static oo__if_t* oo__ifee= oo__if+global_fileM;
  // physical end of oo_if[]
static int oo_ifn= 0;  // number of currently open files

static int oo__getformat() {
  // determine the formats of all opened files of unknown format
  // and store these determined formats;
  // do some intitialization for the format, of necessary;
  // oo__if[].format: !=-9: do nothing for this file;
  // return: 0: ok; !=0: error;
  //         5: .pbf format (cannot be processed by this program);
  //         6: .osm format (cannot be processed by this program);
  // oo__if[].format: input file format; ==0: o5m; ==10: xml; ==-1: pbf;
  oo__if_t* ifptemp;
  byte* bufp;
  #define bufsp ((char*)bufp)  // for signed char

  ifptemp= oo__ifp;
  oo__ifp= oo__if;
  while(oo__ifp<oo__ife) {  // for all input files
    if(oo__ifp->ri!=NULL && oo__ifp->format==-9) {
        // format not yet determined
      read_switch(oo__ifp->ri);
      if(read_bufp>=read_bufe) {  // file empty
        PERRv("file empty: %.80s",oo__ifp->filename)
return 2;
        }
      bufp= read_bufp;
      if(bufp[0]==0 && bufp[1]==0 && bufp[2]==0 &&
          bufp[3]>8 && bufp[3]<20) {  // presumably .pbf format
        PERR("cannot process .pbf format.")
return 5;
        }
      else if(strzcmp(bufsp,"<?xml")==0 ||
          strzcmp(bufsp,"<osm")==0) {  // presumably .osm format
        oo__ifp->format= 10;
        PERR("cannot process .osm format.")
return 6;
        }
      else if(bufp[0]==0xff && bufp[1]==0xe0 && (
          strzcmp(bufsp+2,"\x04""o5m2")==0 ||
          strzcmp(bufsp+2,"\x04""o5c2")==0 )) {
          // presumably .o5m format
        oo__ifp->format= 0;
        oo__ifp->str= str_open();
          // call some initialization of string read module
        }
      else if((bufp[0]==0xff && bufp[1]>=0x10 && bufp[1]<=0x12) ||
          (bufp[0]==0xff && bufp[1]==0xff &&
          bufp[2]>=0x10 && bufp[2]<=0x12) ||
          (bufp[0]==0xff && read_bufe==read_bufp+1)) {
          // presumably shortened .o5m format
        if(loglevel>=2)
          PINFOv("Not a standard .o5m file header %.80s\n",
            oo__ifp->filename)
        oo__ifp->format= 0;
        oo__ifp->str= str_open();
          // call some initialization of string read module
        }
      else {  // unknown file format
        PERRv("unknown file format: %.80s",oo__ifp->filename)
return 3;
        }
      }  // format not yet determined
    oo__ifp++;
    }  // for all input files
  oo__ifp= ifptemp;
  return 0;
  #undef bufsp
  }  // end oo__getformat()

static void oo__reset() {
  // reset counters for writing o5m files;
  if(oo__ifp->format==0) {  // o5m
    oo__ifp->o5id= 0;
    oo__ifp->o5lat= oo__ifp->o5lon= 0;
    oo__ifp->o5hiscset= 0;
    oo__ifp->o5histime= 0;
    oo__ifp->o5rid[0]= oo__ifp->o5rid[1]= oo__ifp->o5rid[2]= 0;
    str_reset();
    }  // o5m
  }  // oo__reset()

static void oo__close() {
  // close an input file;
  // oo__ifp: handle of currently active input file;
  // if this file has already been closed, nothing happens;
  // after calling this procedure, the handle of active input file
  // will be invalid;
  if(oo__ifp!=NULL && oo__ifp->ri!=NULL) {
    if(!oo__ifp->endoffile  && oo_ifn>0)  // missing logical end of file
      PWARNv("unexpected end of input file: %.80s",oo__ifp->filename)
    read_close(oo__ifp->ri);
    oo__ifp->ri= NULL;
    oo_ifn--;
    }
  oo__ifp= NULL;
  }  // end oo__close()

static void oo__end() {
  // clean-up this module;
  oo_ifn= 0;  // mark end of program;
    // this is used to supress warning messages in oo__close()
  while(oo__ife>oo__if) {
    oo__ifp= --oo__ife;
    oo__close();
    }
  oo_ifn= 0;
  }  // end oo__end()

//------------------------------------------------------------

static bool oo_open(const char* filename) {
  // open an input file;
  // filename[]: path and name of input file;
  //             ==NULL: standard input;
  // return: 0: ok; 1: no appropriate input file;
  //         2: maximum number of input files exceeded;
  // the handle for the current input file oo__ifp is set
  // to the opened file;
  // after having opened all input files, call oo__getformat();
  // you do not need to care about closing the file;
  static bool firstrun= true;

  if(oo__ife>=oo__ifee) {
    PERR("too many input files.")
return 2;
    }
  if(read_open(filename)!=0)
return 1;
  oo__ife->ri= read_infop;
  oo__ife->str= NULL;
  oo__ife->format= -9;  // 'not yet determined'
  oo__ife->filename= filename;
  oo__ife->endoffile= false;
  oo__ife->deleteobject= 0;
  oo__ifp= oo__ife++;
  oo_ifn++;
  if(firstrun) {
    firstrun= false;
    atexit(oo__end);
    }
  return 0;
  }  // end   oo_open()

static int oo_sequencetype= -1;
  // type of last object which has been processed;
  // -1: no object yet; 0: node; 1: way; 2: relation;
static int64_t oo_sequenceid= INT64_C(-0x7fffffffffffffff);
  // id of last object which has been processed;

static int oo_main() {
  // start reading osm objects;
  // return: ==0: ok; !=0: error;
  // this procedure must only be called once;
  // before calling this procedure you must open an input file
  // using oo_open();
  bool writeheader;  // header must be written
  int otype;  // type of currently processed object;
    // 0: node; 1: way; 2: relation;
  int64_t id;
  int32_t lon,lat;
  uint32_t hisver;
  int64_t histime;
  int64_t hiscset;
  uint32_t hisuid;
  char* hisuser;  // flag mask 128
  bool inside;  // the node lies inside a border polygon
  bool keep;  // the node must be kept
  const char* region;  // polygon the node lies inside;
    // NULL: node lies outside of all polygons
  const char* subregion;  // subpolygon the node lies inside;
    // NULL: there is no second polygon with lower admin_level
  byte* bufp;  // pointer in read buffer
  byte* bufe;  // pointer in read buffer, end of object
  byte b;  // latest byte which has been read
  int l;
  byte* bp;
  char* sp;
  int64_t nodecount;  // number of nodes written

  // procedure initialization
  atexit(oo__end);
  writeheader= true;
  nodecount= 0;
  // get input file format and care about tempfile name
  if(oo__getformat())
return 5;

  // process the file
  for(;;) {  // read input file

    // get next object
    read_input();

    // care about recursive processing
    if(read_bufp>=read_bufe) {  // at end of input file;
  break;
      }  // end   at end of input file
    if(oo__ifp->endoffile) {  // after logical end of file
      PWARN("unexpected contents after logical end of file.")
  break;
      }
    bufp= read_bufp;
    b= *bufp;

    // care about header and unknown objects
    /* parse .o5m */ {
      if(b<0x10 || b>0x12) {  // not a regular dataset id
        if(b>=0xf0) {  // single byte dataset
          if(b==0xff)  // file start, resp. o5m reset
            oo__reset();
          else if(b==0xfe)
            oo__ifp->endoffile= true;
          else if(write_testmode)
            PWARNv("unknown .o5m short dataset id: 0x%02x",b)
          read_bufp++;
  continue;
          }  // end   single byte dataset
        else {  // unknown multibyte dataset
          if(write_testmode && b!=0xe0 && b!=0xdb && b!=0xdc)
                PWARNv("unknown .o5m dataset id: 0x%02x",b)
          read_bufp++;
          l= pbf_uint32(&read_bufp);  // jump over this dataset
          read_bufp+= l;  // jump over this dataset
  continue;
          }  // end   unknown multibyte dataset
        }  // end   not a regular dataset id
      otype= b&3;
      }  // end   o5m

    // write header
    if(writeheader) {
      writeheader= false;
      if(oo__ifp->filename==NULL) PLOG(2,"input file: stdin")
      else PLOGv(2,"input file: %s",oo__ifp->filename);
      if(!global_outnone)
        wo5_start(0,false,0,0,0,0,0);
      }

    // object initialization
    hisver= 0;
    histime= 0;
    hiscset= 0;
    hisuid= 0;
    hisuser= "";
    if(oo__ifp->deleteobject==1) oo__ifp->deleteobject= 0;

    /* read and process one osm object */ {
      // read object id
      bufp++;
      l= pbf_uint32(&bufp);
      read_bufp= bufe= bufp+l;
      id= oo__ifp->o5id+= pbf_sint64(&bufp);
      // read author
      hisver= pbf_uint32(&bufp);
      if(hisver!=0) {  // author information available
        histime= oo__ifp->o5histime+= pbf_sint64(&bufp);
        if(histime!=0) {
          hiscset= oo__ifp->o5hiscset+= pbf_sint32(&bufp);
          str_read(&bufp,&sp,&hisuser);
          hisuid= pbf_uint64((byte**)&sp);
          }
        }  // end   author information available
      if(bufp>=bufe)
          // just the id and author, i.e. this is a delete request
        oo__ifp->deleteobject= 1;
      else {  // not a delete request
        oo__ifp->deleteobject= 0;
        // read coordinates (for nodes only)
        if(otype==0) {  // node
          // read node body
          lon= oo__ifp->o5lon+= pbf_sint32(&bufp);
          lat= oo__ifp->o5lat+= pbf_sint32(&bufp);
          inside= poly_querypolygon(lon,lat,&region,&subregion);
          keep= inside || global_keepnoregion!=NULL;
          }  // end   node
        // read noderefs (for ways only)
        if(otype==1) {  // way
          PWARN("way objects cannot be processed.")
          l= pbf_uint32(&bufp);
          bp= bufp+l;
          if(bp>bufe) bp= bufe;  // (format error)
          while(bufp<bp)  // dispose of references
            oo__ifp->o5rid[0]+= pbf_sint64(&bufp);
          }  // end   way
        // read refs (for relations only)
        else if(otype==2) {  // relation
          int64_t ri;  // temporary, refid
          int rt;  // temporary, reftype
          char* rr;  // temporary, refrole

          PWARN("relation objects cannot be processed.")
          l= pbf_uint32(&bufp);
          bp= bufp+l;
          if(bp>bufe) bp= bufe;  // (format error)
          while(bufp<bp) {  // each reference
            ri= pbf_sint64(&bufp);
            str_read(&bufp,&rr,NULL);
            rt= *rr++ -'0';
            oo__ifp->o5rid[rt]+= ri;  // (delta-coded ref id)
            }  // each reference
          }  // end   relation
        // read key/val pairs
        if(otype==0) {  // node
          char* k,*v;  // key and val tag
          const char* name,*weight,*wikipedia,*admin_level,*ags;
          const char* street,*place;
          bool kboundary;  // we have found a "boundary" key

          name= weight= wikipedia= admin_level= ags=
            street= place= NULL;
          kboundary= false;

          if(global_outnone) {
            char* k,*v;  // key and val tag

            while(bufp<bufe)  // each k-v pair
              str_read(&bufp,&k,&v);  // read and dispose of it
            }
          else if(!keep) {  // not keeping the node
            if(global_keepboundarynodes) {
              while(bufp<bufe) {  // each k-v pair
                str_read(&bufp,&k,&v);  // read key-val pair
                if(strcmp(k,global_regionkey)!=0 &&
                    strcmp(k,global_subregionkey)!=0) {
                    // only if the key name does not conflict with
                    // regions' keys we might want to add later
                  if(strcmp(k,"boundary")==0) {
                    if(strcmp(v,"administrative")==0)
                      kboundary= true;
                    }
                  else if(strcmp(k,"name")==0) name= v;
                  else if(strcmp(k,"bBoxWidthWeight")==0) weight= v;
                  else if(strcmp(k,"wikipedia")==0) wikipedia= v;
                  else if(strcmp(k,"admin_level")==0) admin_level= v;
                  else if(strcmp(k,
                    "de:amtlicher_gemeindeschluessel")==0) ags= v;
                  }  // no conflict with regions' keys
                }  // each k-v pair
              if(kboundary) {
                  // this is the node-representation of a polygon
                wo5_node(id,
                  hisver,histime,hiscset,hisuid,hisuser,lon,lat);
                nodecount++;
                if(name!=NULL) {  // name tag-key exists
                  int j;
                  char s[100];

                  if(wikipedia!=NULL && (
                      (j= strzlcmp(wikipedia,"de:Berlin-"))>0 ||
                      (j= strzlcmp(wikipedia,"de:Köln-"))>0 ) ) {
                      // subregion of Berlin or Köln

                    strmcpy(s,wikipedia+3,j-3);  // extract region's name
                    wo5_node_keyval(global_regionkey,s);
                    if(global_exportstrings) {
                      wrf_string(1,s);
                      wrf_string(3,s);  // add as city name as well
                      }
                    s[j-3-1]= '-';  // append '-'
                    strmcpy(s+j-3,name,100-j);
                    wo5_node_keyval(global_subregionkey,s);
                    if(global_exportstrings) {
                      wrf_string(2,s);
                      wrf_string(3,s);  // add as city name as well
                      }
                    }  // subregion of Berlin
                  else {  // regular region
                    wo5_node_keyval(global_regionkey,name);
                    if(global_exportstrings) {
                      wrf_string(1,name);
                      wrf_string(3,name);  // add as city name as well
                      }
                    }  // regular region
                  }  // name tag-key exists
                if(weight!=NULL)
                  wo5_node_keyval("bBoxWidthWeight",weight);
                if(admin_level!=NULL)
                  wo5_node_keyval("admin_level",admin_level);
                wo5_node_keyval("boundary","administrative");
                if(ags!=NULL) wo5_node_keyval(
                  "de:amtlicher_gemeindeschluessel",ags);
                if(name!=NULL)
                  wo5_node_keyval("name",name);
                if(wikipedia!=NULL)
                  wo5_node_keyval("wikipedia",wikipedia);
                wo5_node_close();
                }  // this is the node-representation of a polygon
              }
            else {
              char* k,*v;  // key and val tag

              while(bufp<bufe)  // each k-v pair
                str_read(&bufp,&k,&v);  // read and dispose of it
              }
            }  // not keeping the node
          else {  // output allowed AND keeping the node
            wo5_node(id,
              hisver,histime,hiscset,hisuid,hisuser,lon,lat);
            nodecount++;
            while(bufp<bufe) {  // each k-v pair
              str_read(&bufp,&k,&v);  // read key-val pair
              if(strcmp(k,"name")==0) name= v;
              else if(strcmp(k,"wikipedia")==0) wikipedia= v;
              if(strcmp(k,global_regionkey)!=0 &&
                  strcmp(k,global_subregionkey)!=0) {
                  // only if the key name does not conflict with
                  // regions' keys we might want to add later
                wo5_node_keyval(k,v);
                if(strzcmp(k,"addr:")==0) {
                  if(global_exportstrings) {
                    if(strcmp(k+5,"city")==0)
                      wrf_string(3,v);
                    else if(strcmp(k+5,"street")==0) {
                      street= v;
                      wrf_string(4,v);
                      }
                    else if(strcmp(k+5,"place")==0) {
                      place= v;
                      wrf_string(4,place);
                        // add place to street strings
                      }
                    else if(strcmp(k+5,"housenumber")==0) {
                        // housenumber
                      if(global_separatehousenumbers &&
                          strchr(v,';')!=NULL) {
                          // option "separate housenumbers" AND
                          // there is at least one semicolon ,,,
                        char* v0,*vp;
                        char vc[256];
                        bool subsequent;

                        subsequent= false;
                        strMcpy(vc,v);
                        v0= vc;
                        for(;;) {  // for all substrings
                          while(*v0==' ') v0++;
                            // ignore leading blanks
                          vp= strchr(v0,';');  // search for semicolon
                          if(vp!=NULL) {  // (further) semicolon(s)
                            if(vp>v0 && vp[-1]==' ') vp[-1]= 0;
                                // remove trailing blank from substring
                            if(vp>v0+1 && vp[-2]==' ') vp[-2]= 0;
                                // remove second trailing blank
                            *vp++= 0;  // terminate substring at ';'
                            }  // (further) semicolon(s)
                          if(*v0!=0) {
                              // substring has at least one character
                            if(subsequent)
                              nodecount++;
                            subsequent= true;
                            wrf_string(5,v0);  // write substring
                            }
                          if(vp==NULL)  // no further substrings
                        break;
                          v0= vp;  // continue with next substring
                          }  // for all substrings
                        }  // option "separate housenumbers" AND
                           // there is at least one semicolon
                      else  // keep housenumber combinations as they are
                        wrf_string(5,v);
                      }  // housenumber
                    }
                  }
                else if(strcmp(k,"name")==0)
                  name= v;
                else if(strcmp(k,"boundary")==0) {
                  if(strcmp(v,"administrative")==0)
                    kboundary= true;
                  }
                }  // no conflict with regions' keys
              }  // each k-v pair
            if(!kboundary) {  // not a polygon's center node
              if(name!=NULL) {  // street name
                wo5_node_keyval("addr:street",name);
                  // add name as street address
                if(global_exportstrings)
                    wrf_string(4,name);  // add name to street strings
                }  // street name
              }  // not a polygon's center node
            if(kboundary && global_keepboundarynodes) {
                // this is the node-representation of a polygon AND
                // it shall be left as it is

              if(name!=NULL) {  // name tag-key exists
                int j;
                char s[100];

                if(wikipedia!=NULL && (
                    (j= strzlcmp(wikipedia,"de:Berlin-"))>0 ||
                    (j= strzlcmp(wikipedia,"de:Köln-"))>0 ) ) {
                    // subregion of Berlin or Köln

                  strmcpy(s,wikipedia+3,j-3);  // extract region's name
                  wo5_node_keyval(global_regionkey,s);
                  if(global_exportstrings) {
                    wrf_string(1,s);
                    wrf_string(3,s);  // add as city name as well
                    }
                  s[j-3-1]= '-';  // append '-'
                  strmcpy(s+j-3,name,100-j);
                  wo5_node_keyval(global_subregionkey,s);
                  if(global_exportstrings) {
                    wrf_string(2,s);
                    wrf_string(3,s);  // add as city name as well
                    }
                  }  // subregion of Berlin
                else {  // regular region
                  wo5_node_keyval(global_regionkey,name);
                  if(global_exportstrings) {
                    wrf_string(1,name);
                    wrf_string(3,name);  // add as city name as well
                    }
                  }  // regular region
                }  // name tag-key exists
              }  // this is the node-representation of a polygon ...
            else {
              if(inside) {  // node lies inside at least one polygon
                if(region!=NULL) {
                  wo5_node_keyval(global_regionkey,region);
                  if(global_exportstrings) {
                    wrf_string(1,region);
                    wrf_string(3,region);  // add as city name as well
                    }
                  }
                if(subregion!=NULL) {
                  wo5_node_keyval(global_subregionkey,subregion);
                  if(global_exportstrings) {
                    wrf_string(2,subregion);
                    wrf_string(3,subregion);// add as city name as well
                    }
                  }
                }  // node lies inside at least one polygon
              else if(global_keepnoregion[0]!=0) {
                  // node lies outside of all polygons AND
                  // node shall be tagged with a "noregion" value
                wo5_node_keyval(global_regionkey,global_keepnoregion);
                if(global_exportstrings) wrf_string(1,region);
                }
              }
            wo5_node_close();
            }  // output allowed AND keeping the node
          }  // node
        else {  // not a node
          char* k,*v;  // key and val tag

          while(bufp<bufe)  // each k-v pair
            str_read(&bufp,&k,&v);  // read and dispose of it
          }  // not a node
        }  // end   not a delete request
      }  // read and process one osm object

    /* check sequence */ {
      if(otype<=oo_sequencetype &&
          (otype<oo_sequencetype || id<oo_sequenceid ||
          (oo_ifn>1 && id<=oo_sequenceid)))
        PWARNv("wrong sequence at %s %"PRIi64,ONAME(otype),id)
      oo_sequencetype= otype; oo_sequenceid= id;
      }

    }  // end   read all input files
  if(!writeheader && !global_outnone) {
    if(global_exportstrings) {
      // write number of node objects to file
      char s[30];

      int64toa(nodecount,s);
      wrf_mem(0,s,strlen(s)); wrf_char(0,'\n');
      }
    wo5_flush();
    wo5_end();
    }
  return 0;
  }  // end   oo_main()

//------------------------------------------------------------
// end   Module oo_   osm processing module
//------------------------------------------------------------



#if !__WIN32__
void sigcatcher(int sig) {
  PINFO("Output has been terminated.")
  exit(1);
  }  // end   sigcatcher()
#endif

int main(int argc,char** argv) {
  // main program;
  // for the meaning of the calling line parameters please look at the
  // contents of helptext[];
  bool usesstdin;
  static char outputfilename[400]= "";  // standard output file name
    // =="": standard output 'stdout'
  int r,l;
  const char* a;  // command line argument
  static FILE* parafile= NULL;
  static char* aa= NULL;  // buffer for parameter file line
  char* ap;  // pointer in aa[]
  int aamax;  // maximum length of string to read
  #define main__aaM 1000000

  #if !__WIN32__
  /* care about signal handler */ {
    static struct sigaction siga;

    siga.sa_handler= sigcatcher;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags= 0;
    sigaction(SIGPIPE,&siga,NULL);
    }
  #endif

  // initializations
  usesstdin= false;
  #if __WIN32__
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
  #endif

  // read command line parameters
  if(argc<=1) {  // no command line parameters given
    fprintf(stderr,"\n" PROGRAM " " VERSION "\n"
      "(Please use  -h  to display a short guide.)\n\n");
return 0;
    }
  while(parafile!=NULL || argc>0) {
      // for every parameter in command line
    if(parafile!=NULL) do {
        // there are parameters waiting in a parameter file
      ap= aa;
      for(;;) {
        aamax= main__aaM-1-(ap-aa);
        if(fgets(ap,aamax,parafile)==NULL) {
          if(ap>aa) {
            if(ap>aa && ap[-1]==' ')
              *--ap= 0;  // cut one trailing space
      break;
            }
          goto parafileend;
          }
        if(strzcmp(ap,"// ")==0)
      continue;
        if(ap>aa && (*ap=='\r' || *ap=='\n' || *ap==0)) {
            // end of this parameter
          while(ap>aa && (ap[-1]=='\r' || ap[-1]=='\n')) *--ap= 0;
            // eliminate trailing NL
          if(ap>aa && ap[-1]==' ')
            *--ap= 0;  // cut one trailing space
      break;
          }
        ap= strchr(ap,0);  // find end of string
        while(ap>aa && (ap[-1]=='\r' || ap[-1]=='\n'))
          *--ap= 0;  // cut newline chars
        *ap++= ' '; *ap= 0;  // add a space
        }
      a= aa;
      while(*a!=0 && strchr(" \t\r\n",*a)!=NULL) a++;
      if(*a!=0)
    break;
    parafileend:
      fclose(parafile); parafile= NULL;
      free(aa); aa= NULL;
      } while(false);
    if(parafile==NULL) {
      if(--argc<=0)
  break;
      argv++;  // switch to next parameter; as the first one is just
        // the program name, we must do this previous reading the
        // first 'real' parameter;
      a= argv[0];
      }
    if((l= strzlcmp(a,"--parameter-file="))>0 && a[l]!=0) {
        // parameter file
      parafile= fopen(a+l,"r");
      if(parafile==NULL) {
        PERRv("Cannot open parameter file: %.80s",a+l)
        perror(PROGRAM);
return 1;
        }
      aa= (char*)malloc(main__aaM);
      if(aa==NULL) {
        PERR("Cannot get memory for parameter file.")
        fclose(parafile); parafile= NULL;
return 1;
        }
      aa[0]= 0;
  continue;  // take next parameter
      }
    if(loglevel>0)  // verbose mode
      PINFOv("Parameter %.2000s",a)
    if(strcmp(a,"-h")==0) {  // user wants parameter overview
      fprintf(stdout,"%s",shorthelptext);  // print brief help text
        // (took "%s", to prevent oversensitive compiler reactions)
return 0;
      }
    if(strcmp(a,"-help")==0 || strcmp(a,"--help")==0) {
        // user wants help text
      fprintf(stdout,"%s",helptext);  // print help text
        // (took "%s", to prevent oversensitive compiler reactions)
return 0;
      }
    if((l= strzlcmp(a,"-B="))>0 && a[l]!=0) {
        // border consideration by polygon file
      global_polygonfile= a+l;
  continue;  // take next parameter
      }
    if(strzcmp(a,"-b=")==0) {
        // manually specified bounding box
      if(!poly_box(a+3)) {
        PERR("use bounding box format: "
          " -b=\"x1,y1,x2,y2\"\n");
return 3;
        }
      continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--max-polygons="))>0 && a[l]!=0) {
        // define maximum number of polygons
      global_maxpolygons= oo__strtosint64(a+l);
      if(global_maxpolygons<4) global_maxpolygons= 4;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--max-edges="))>0 && a[l]!=0) {
        // define maximum number of edges
      global_maxedges= oo__strtosint64(a+l);
      if(global_maxedges<4) global_maxedges= 4;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--use-admin-levels"))>0) {
        // expect values of admin levels in polygon file
      if(a[l]=='=' && strlen(a+l+1)<=100)
          // a user-defined separator has been chosen AND
          // its length is reasonable (security precaution)
        global_useadminlevel= a+l+1;
      else
        global_useadminlevel= ";";
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--region-key="))>0 && a[l]!=0) {
        // define a key for region tagging
      global_regionkey= a+l;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--subregion-key="))>0 && a[l]!=0) {
        // define a key for subregion tagging
      global_subregionkey= a+l;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--keep-noregion"))>0) {
        // keep node which are not in any region
      if(a[l]=='=' && strlen(a+l+1)<=100)
        // tagging value for "not in any region" AND
          // its length is reasonable (security precaution)
        global_keepnoregion= a+l+1;
      else
        global_keepnoregion= "";
  continue;  // take next parameter
      }
    if(strcmp(a,"--keep-boundary-nodes")==0) {
        // user wants boundary nodes to be kept
      global_keepboundarynodes= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--export-strings")==0) {
        // user wants string export into files
      global_exportstrings= true;
      wrf_open();
  continue;  // take next parameter
      }
    if(strcmp(a,"--separate-housenumbers")==0) {
        // user wants separated housnumbers
      global_separatehousenumbers= true;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-o5m")==0) {
        // user wants output in .o5m format
      // do nothing since this is default; furthermore, the program
      // does not know how to write other formats anyway;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-none")==0) {
        // user does not want any standard output
      global_outnone= true;
  continue;  // take next parameter
      }
    if(strzcmp(a,"-o=")==0 && a[3]!=0) {
        // reroute standard output to a file
      strMcpy(outputfilename,a+3);
  continue;  // take next parameter
      }
    if((strcmp(a,"-v")==0 || strcmp(a,"--verbose")==0 ||
        strzcmp(a,"-v=")==0 || strzcmp(a,"--verbose=")==0) &&
        loglevel==0) {  // test mode - if not given already
      char* sp;

      sp= strchr(a,'=');
      if(sp!=NULL) loglevel= sp[1]-'0'; else loglevel= 1;
      if(loglevel<1) loglevel= 1;
      if(loglevel>MAXLOGLEVEL) loglevel= MAXLOGLEVEL;
      if(a[1]=='-') {  // must be "--verbose" and not "-v"
        if(loglevel==1)
          PINFO("Verbose mode.")
        else
          PINFOv("Verbose mode %i.",loglevel)
        }
  continue;  // take next parameter
      }
    if(strcmp(a,"-t")==0) {
        // test mode
      write_testmode= true;
      PINFO("Entering test mode.")
  continue;  // take next parameter
      }
    if(strcmp(a,"-")==0) {  // use standard input
      usesstdin= true;
      if(oo_open(NULL))  // file cannot be read
return 2;
  continue;  // take next parameter
      }
    if(a[0]=='-') {
      PERRv("unrecognized option: %.80s",a)
return 1;
      }
    // here: parameter must be a file name
    if(strcmp(a,"/dev/stdin")==0)
      usesstdin= true;
    if(oo_open(a))  // file cannot be read
return 2;
    }  // end   for every parameter in command line

  // process parameters
  if(oo_ifn==0) {  // no input files given
    PERR("use \"-\" to read from standard input or try:  "
      PROGRAM " -h")
return 0;  // end the program, because without having input files
      // we do not know what to do;
    }
  if(usesstdin)
    PLOG(1,"using standard input.")
  if(write_open(outputfilename[0]!=0? outputfilename: NULL)!=0)
return 3;
  if(global_separatehousenumbers && !global_exportstrings)
    global_separatehousenumbers= false;

  // initialization
  if(global_polygonfile!=NULL && !poly_file(global_polygonfile)) {
    PERRv("no polygon file or too large: %s",global_polygonfile)
return 4;
    }
  if(o5_ini()!=0) {
    PERR("not enough memory for .o5m buffer.")
return 5;
    }
  if(!poly_prepare()) {
    PERR("polygon-matrix initialization error.")
return 6;
    }

  // do the work
  r= oo_main();
  if(loglevel>=2) {  // verbose
    if(read_bufp!=NULL && read_bufp<read_bufe)
      PINFOv("Next bytes to parse:\n"
        "  %.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X",
        read_bufp[0],read_bufp[1],read_bufp[2],read_bufp[3],
        read_bufp[4],read_bufp[5],read_bufp[6],read_bufp[7])
    }  // verbose
  write_flush();
  if(write_error) {
    r= 92;
    PERR("write error.")
    }
  if(loglevel>0) {  // verbose mode
    if(oo_sequenceid!=INT64_C(-0x7fffffffffffffff))
      PINFOv("Last processed: %s %"PRIu64".",
        ONAME(oo_sequencetype),oo_sequenceid)
    if(r!=0)
      PINFOv("Exit: %i\n",r)
    }  // verbose mode
  return r;
  }  // end   main()

