#define PROGRAM "osmrelpoly"
#define VERSION "0.0.0"
// 2016-03-15 17:00
//
// compile this file:
//   gcc osmrelpoly.c -O3 -o osmrelpoly
// there may be compiler warnings informing you about unused procedures
// and variables; both have been left in the source file intentionally,
// they are for future expansions;
//
// tests:
// gcc osmrelpoly.c -g -o osmrelpoly
// ./osmconvert o.osm --out-o5m | ./osmrelpoly -
// |csplit -f rel -b %05d.poly - /^$/
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
// The input file (.o5m-formatted) is read sequentially in 3 steps.
//
// Step 1:
// For each node object, both its ID and its position are stored.
//
// Step 2:
// For each way object, its ID is stored. All the way's referenced
// nodes are also stored, however by position and not by ID.
//
// Step 3:
// For each relation object, a borderpolygon is generated.
// Referenced way objects are treated according to their first and
// last nodes as follows.
//   a) First node and last node identical ("closed way"):
//      The way object is treated as independent. In the output
//      stream there will be an own section for this object.
//   b) First node and last node are different:
//      The program will concatenate all those way objects before
//      writing them to output. Provided that:
//        aa) they have the same role,
//        bb) the resulting polygons are closed.
//      That means that this concatenation may result in more than
//      one polygon. Or it may result in no polygon if the given
//      data is incomplete.
// The referenced way objects' roles determine if the .poly section
// is to be marked as positive or as negative (preceding "!" sign).
//   a) Role "outer" (or no role given):
//      The resulting .poly section is positive.
//   b) Role "inner":
//      The resulting .poly section is negative.
// Note that this outer/inner information is redundant. The
// program adds it to the output stream just because the format
// description of .poly files demands it.
// Referenced node objects are ignored as they do not convey any
// information on borders.

// poly file example
//
// australia_v
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



#define MAXLOGLEVEL 5
const char* shorthelptext=
"\n" PROGRAM " " VERSION "  Parameter Overview\n"
"(Please use  --help  to get more information.)\n"
"\n"
"<FILE>                    input file name\n"
"-                         read from standard input\n"
"--max-nodes=<n>           maximum number of nodes (default: 25 Mio.)\n"
"--max-ways=<n>            maximum number of ways (default: 5 Mio.)\n"
"--simplify=<n>            grade of polygon simplification\n"
"--add-admin-levels        add admin_level values to polygons\' names\n"
"-h                        display this parameter overview\n"
"--help                    display a more detailed help\n"
"--out-poly                write output in poly format (default)\n"
"--out-none                no standard output (for testing purposes)\n"
"-o=<outfile>              reroute standard output to a file\n"
"--parameter-file=<file>   param. in file, separated by empty lines\n"
"--verbose                 activate verbose mode\n";
const char* helptext=
"\n" PROGRAM " " VERSION "\n"
"\n"
"This program reads OSM raw data, given in .o5m format, and converts\n"
"all included relations to border polygons in .poly file format.\n"
"\n"
"-h\n"
"        Display a short parameter overview.\n"
"\n"
"--help\n"
"        Display this help.\n"
"\n"
"--max-nodes=<n>\n"
"        Defines for how much node objects to allocate memory space.\n"
"        16 bytes of main memory are required per node object.\n"
"        Default: 25 Million nodes.\n"
"\n"
"--max-ways=<n>\n"
"        Defines for how much way objects to allocate memory space.\n"
"        32 bytes of main memory are required per way object.\n"
"        Default: 5 Million ways.\n"
"\n"
"--simplify=<n>\n"
"        Here you can specify the grade of polygon simplification.\n"
"        The higher this value, the simpler the created polygon will\n"
"        be. The unit of this value is meter, it determines the\n"
"        maximum deviation permitted. Default: 0 (no simplification).\n"
"        Mathematical background:\n"
"        This value is squared and then compared with the spanning-\n"
"        triangle of every three directly linked points. If the\n"
"        triangle's area is smaller, the point in the middle will be\n"
"        removed.\n"
"\n"
"--add-admin-levels\n"
"--add-admin-levels=<separator>\n"
"        If a relation has an admin_level tag, its value will be\n"
"        appended to the relation\'s name. This format will be used:\n"
"            Name of the region;admin_level\n"
"        To get a different separator, name it like this:\n"
"            --add-admin-level=\";\"\n"
"        or\n"
"            --add-admin-level=\", level: \"\n"
"\n"
"--out-poly\n"
"        Data will be written in .poly format. This is the default\n"
"        output format.\n"
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
"./" PROGRAM " germany.o5m -o=germany.poly\n"
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

static bool global_outnone= false;  // no standard output at all
static int64_t global_maxnodes= 25000000;
static int64_t global_maxways= 5000000;
static int64_t global_simplify= 0;  // grade of polygon simplification;
  // the higher this value, the simpler the created polygon;
  // unit: square meters of the smallest allowed spanning triangle;
  // 0: no simplification;
static const char* global_addadminlevel= NULL;
  // separator for admin_level;
  // NULL: do not add admin_level to region's name
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
#define PLOG(ll,f,...) \
  { if(loglevel>=ll) PINFO(f) }
#define PLOGv(ll,f,...) \
  { if(loglevel>=ll) PINFOv(f,__VA_ARGS__) }
#define ONAME(i) \
  (i==0? "node": i==1? "way": i==2? "relation": "unknown object")
#define global_fileM 1002  // maximum number of input files

//------------------------------------------------------------
// end   Module Global   global variables for this program
//------------------------------------------------------------



#define COORDINATE_NIL 2000000000L  // nil value for geocoordinates
  // which are stored as 10^-7 fixpoint numbers in int32_t

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
  // s[]: digit string;
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
  // jump over a protobuf-formatted element - no matter
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
  if(loglevel>=2)
    fprintf(stderr,"Read-opening: %s\n",read_infop->filename);
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
        if(loglevel>=2)
          fprintf(stderr,"Read-opening: increasing gzbuffer.\n");
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
  if(loglevel>=2)
    fprintf(stderr,"Read-closing: %s\n",read_infop->filename);
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
  // the position is not stored anew if it has been locked
  // with read_infop->lockpos;
  // return: jump position has been stored;
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
  if(loglevel>=2)
    fprintf(stderr,"Write-closing FD: %i\n",write__fd);
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

  if(loglevel>=2)
    fprintf(stderr,"Write-opening: %s\n",
      filename==NULL? "stdout": filename);
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
  if(loglevel>=2)
    fprintf(stderr,"Write-opening: %s\n",
      filename==NULL? "stdout": filename);
  if(filename==NULL) {  // we are to change back to standard output file
    if(loglevel>=2)
      fprintf(stderr,"Write-reopening: %s\n",
        write__filename_standard==NULL? "stdout":
        write__filename_standard);
    write__close();  // close temporary file
    write__filename= write__filename_standard;
    write__fd= write__fd_standard;
    }
  else {  // new temporary file shall be opened
    if(loglevel>=2)
      fprintf(stderr,"Write-opening: %s\n",filename);
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
// Module posn_   OSM node position module
//------------------------------------------------------------

// this module provides a geocoordinate array for to store
// the coordinates of all OSM node objects;
// the procedures posn_set() and posn_get() allow access to
// this array;
// as usual, all identifiers of a module have the same prefix,
// in this case 'posn'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

struct posn__mem_struct {  // element of array
  int64_t id;
  int32_t x,y;
  } __attribute__((__packed__));
  // (do not change this structure; the search algorithm expects
  // the size of this structure to be exactly 16 bytes)
typedef struct posn__mem_struct posn__mem_t;
static posn__mem_t* posn__mem= NULL;  // start address of array
static posn__mem_t* posn__meme= NULL;  // logical end address
static posn__mem_t* posn__memee= NULL;  // physical end address

static void posn__end() {
  // clean-up for posn module;
  // will be called at program's end;
  if(posn__mem==NULL)
    PERR("not enough memory. Reduce --max-nodes=")
  else {  // was initialized
    if(posn__meme>=posn__memee)  // not enough space in array
      PERR("not enough space. Increase --max-nodes=")
    else {
      int64_t siz;

      siz= (char*)posn__memee-(char*)posn__mem;
      siz= siz/4*3;
      if((char*)posn__meme-(char*)posn__mem>siz)
          // low space in array
        PWARN("low space. Try to increase --max-nodes=")
      }
    free(posn__mem);
    posn__mem= NULL;
    }
  }  // end   posn__end()

#define posn__mem_size 16  // size of structure
#define posn__mem_mask ~0x0f
  // bitmask to start at base of structure (for size 16)

//------------------------------------------------------------

static int posn_ini() {
  // initialize the posn module;
  // return: 0: OK; 1: not enough memory;
  int64_t siz;

  if(posn__mem!=NULL)  // already initialized
return 0;
  atexit(posn__end);  // chain-in the clean-up procedure
  // allocate memory for the array
  siz= posn__mem_size*global_maxnodes;
  posn__mem= (posn__mem_t*)malloc(siz+posn__mem_size*2);
  if(posn__mem==NULL)  // not enough memory
return 1;
  posn__meme= posn__mem;
  posn__memee= (posn__mem_t*)((char*)posn__mem+siz);
  return 0;
  }  // end   posn_ini()

static inline void posn_set(int64_t id,int32_t x,int32_t y) {
  // set geoposition for a specific object ID;
  // id: id of the object;
  // x,y: geocoordinates in 10^-7 degrees;
  if(posn__meme>=posn__memee)  // not enough space in array
    exit(70001);
  posn__meme->id= id;
  posn__meme->x= x;
  posn__meme->y= y;
  posn__meme++;
  }  // end   posn_set()

static int32_t* posn_xy= NULL;  // position of latest read coordinates;
  // posn_xy[0]: x; posn_xy[1]: y;
  // posn_xy==NULL: no geoposition available for this id;

static inline void posn_get(int64_t id) {
  // get the previously stored geopositions of an object;
  // id: id of the object;
  // return: posn_xy[0]: x; posn_xy[1]: y;
  //         the caller may change the values for x and y;
  //         posn_xy==NULL: no geoposition available for this id;
  char* min,*max,*middle;
  int64_t middle_id;

  min= (char*)posn__mem;
  max= (char*)posn__meme;
  while(max>min) {  // binary search
    middle= (((max-min-posn__mem_size)/2)&(posn__mem_mask))+min;
    middle_id= *(int64_t*)middle;
    if(middle_id==id) {  // we have found the right object
      posn_xy= (int32_t*)(middle+8);
return;
      }
    if(middle_id>id)
      max= middle;
    else
      min= middle+posn__mem_size;
    }  // binary search
  // here: did not find the geoposition of the object in question
  posn_xy= NULL;
  }  // end   posn_get();

//------------------------------------------------------------
// end   Module posn_   OSM node position module
//------------------------------------------------------------



//------------------------------------------------------------
// Module posw_   OSM way position module
//------------------------------------------------------------

// this module provides an array for to store information of
// OSM way objects;
// the procedures posw_set...() and posw_get() allow access to
// this array;
// this module uses the procedure posn_get() of the module 'posn';
// as usual, all identifiers of a module have the same prefix,
// in this case 'posw'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

// Storage
// there are two storage containers in this module:
//   A) array for storing the ways' meta information;
//      (see array of structures posw__mem)
//   B) list for storing the coordinates of the ways;
//      (see array of int32_t posw__coord)

struct posw__mem_struct {  // element of array
  int64_t id;  // id of this way object
  int64_t firstnode;  // id of first referenced node
  int64_t lastnode;  // id of last referenced node
  int32_t* xy;  // start address of polygon's coordinates array;
    // coordinates are stored in pairs;
    // the last valid coordinate pair must be followed by a pair
    // of dummy coordinates: posw_nil,0;
    // way objects which do not have at least two referenced nodes
    // are considered as invalid by this module and therefore not
    // stored;
  } __attribute__((__packed__));
  // (do not change this structure; the search algorithm expects
  // the size of this structure to be not more than 32 bytes;
  // i.e., this module may be comiled for 32 bit and 64 machines,
  // it is not suited for 128 bit machines;)
typedef struct posw__mem_struct posw__mem_t;
static posw__mem_t* posw__mem= NULL;  // start address of array
static posw__mem_t* posw__memi= NULL;  // currently entered element
static posw__mem_t* posw__meme= NULL;  // logical end address
static posw__mem_t* posw__memee= NULL;  // physical end address
#define posw__mem_size 32  // size of structure
#define posw__mem_mask ~0x1f
  // bitmask to start at base of structure (for size 32)

static int32_t* posw__coord= NULL;  // start addr. of coordinates array
    // coordinates are stored in pairs; the unit is 10^-7 degrees;
    // the first valid coordinate pair of each list in this container
    // is preceded by a pair of dummy coordinates: posw_nil,cpn, where
    // cpn stands for the number of regular coordinate in the list;
    // each last valid coordinate pair must be followed by a pair
    // of dummy coordinates too: posw_nil,0;
    // to allow processing this array in backward direction, the first
    // coordinate pair at start of this container, i.e., right before
    // the first list also is a dummy coordinate pair;
static int32_t* posw__coorde= NULL;  // logical end address
static int32_t* posw__coordee= NULL;  // physical end address

static void posw__end() {
  // clean-up for posw module;
  // will be called at program's end;
  if(posw__mem==NULL)
    PERR("not enough memory. Reduce --max-ways=")
  else {  // was initialized
    if(posw__meme>=posw__memee)  // not enough space in array
      PERR("not enough space. Increase --max-ways=")
    else {
      int64_t siz;

      siz= (char*)posw__memee-(char*)posw__mem;
      siz= siz/4*3;
      if((char*)posw__meme-(char*)posw__mem>siz)
          // low space in array
        PWARN("low space. Try to increase --max-ways=")
      }
    free(posw__mem);
    posw__mem= NULL;
    }
  if(posw__coord==NULL)
    PERR("not enough polygon memory. Reduce --max-nodes=")
  else {  // was initialized
    if(posw__coorde>=posw__coordee)  // not enough space in array
      PERR("not enough polygon space. Increase --max-nodes=")
    else {
      int64_t siz;

      siz= (char*)posw__coordee-(char*)posw__coord;
      siz= siz/4*3;
      if((char*)posw__coorde-(char*)posw__coord>siz)
          // low space in coordinares list
        PWARN("low polygon space. Try to increase --max-nodes=")
      }
    free(posw__coord);
    posw__coord= NULL;
    }
  }  // end   posw__end()

//------------------------------------------------------------

static int posw_ini() {
  // initialize the posw module;
  // return: 0: OK; 1: not enough memory;
  int64_t siz;

  if(posw__mem!=NULL)  // already initialized
return 0;
  atexit(posw__end);  // chain-in the clean-up procedure
  // allocate memory for the array
  siz= posw__mem_size*global_maxways;
  posw__mem= (posw__mem_t*)malloc(siz+posw__mem_size*2);
  if(posw__mem==NULL)  // not enough memory
return 1;
  posw__meme= posw__memi= posw__mem;
  posw__memee= (posw__mem_t*)((char*)posw__mem+siz);
  // allocate memory for the coordinates list
  siz= sizeof(int32_t)*2*(global_maxnodes+global_maxways*2);
  siz+= siz/8;  // because some way objects share the same nodes
  posw__coord= (int32_t*)malloc(siz+sizeof(int32_t)*6);
  if(posw__coord==NULL)  // not enough memory
return 1;
  posw__coorde= posw__coord;
  posw__coordee= (int32_t*)((char*)posw__coord+siz);
  return 0;
  }  // end   posw_ini()

// results for searches using posw_get():
// (the first two are also used by posw_set...() procedures to
// communicate with each other;)
static int64_t posw_firstnode= 0;  // id of first referenced node
static int64_t posw_lastnode= 0;  // id of last referenced node
static int32_t* posw_xy= NULL;
  // start of latest read coordinates list;
  // posw_xy[0],posw_xy[1]: coordinates of first node;
  // posw_xy[2],posw_xy[3]: coordinates of second node;
  // posw_xy[...]==posw_nil: end of list;
  // posw_xy==NULL: no coordinates list available for this id;
static const int32_t posw_nil= COORDINATE_NIL;  // this value for
  // an x-coordinate marks the end of each coordinates list;

static inline void posw_setstart(int64_t id) {
  // start setting data of one way object;
  // id: id of this way object;
  // sequence:
  // to store a way object's information call at first this procedure;
  // after that, call posw_setnode() for each referenced node object
  // of this way object;
  // after the last referenced node object, call posw_setend();
  // if posw_setnode() is not called at least two times for one way
  // object, the module will not store any information about the way object;
  if(posw__meme>=posw__memee)  // not enough space in array
    exit(70011);
  posw__memi= posw__meme;
  posw__meme= (posw__mem_t*)((char*)posw__meme+posw__mem_size);
    // we cannot do 'posw__memee++' here because on 32-bit machines
    // the structure might have the size 28 instead of 32,
    // and the search algorithm in posw_get() depends on the size 32;
  posw__memi->id= id;
  posw_firstnode= 0;  // meaning that this id is currently unknown
  posw_lastnode= 0;  // meaning that this id is currently unknown
  posw__memi->xy= posw__coorde;  // start of coordinates list
  }  // end   posw_setstart()

static inline void posw__storenode(int64_t id,int32_t x,int32_t y) {
  // -- module-internal procedure --
  // write the coordinates of one node into coordinates array;
  // id: id of this node object;
  // x,y: coordinates of this node object; the unit is 10^-7 degrees;
  if(posw__coorde>=posw__coordee)
      // not enough space in coordinates array
    exit(70012);
  if(posw_firstnode==0) {  // id of first node object is unknown yet
    posw_firstnode= id;  // remember this first node object id
    // to allow backward processing, put a terminator at list start
    *posw__coorde++= posw_nil;
    *posw__coorde++= 0;
      // the number of coordinate pairs will later be entered here
    }
  else
    posw_lastnode= id;  // as of now, this node object is the last one
  *posw__coorde++= x;
  *posw__coorde++= y;
  }  // end   posw__storenode()

static inline void posw__simplify(int64_t id,int32_t x,int32_t y) {
  // simplify a way's coordinates and stores them into
  // coordinates array by calling posw__storenode();
  // all coordinates of the way must be entered by subsequent
  // calls of this procedure;
  // id: id of one referenced node object;
  //     ==0: there is no more coordinate pair for this way;
  //          the procedure will flush the buffer and store all
  //          containing coordinates into the array;
  // x,y: the node's coordinates;
  // the coordinates are not stored directly, the newest
  // two coordinate pairs are always buffered (fifo);
  // they will actually be written as soon as necessary because of
  // the small buffer; the caller can force writing the buffer
  // contents by calling this procedure with x==COORDINATE_NIL;
  static int64_t idll,idl; // buffer for the latest two node ids;
  static union {
    struct {int32_t xll,yll,xl,yl;} __attribute__((__packed__));
    struct {uint64_t cll,cl;} __attribute__((__packed__));
      // cll,cl just for faster access when moving coordinate pairs;
    } buf;  // buffer for the latest two coordinate pairs;
  static int bufn= 0;  // number of coordinate pairs in buffer;
  int64_t area;
    // area size of the triangle spanned by the last three points
  static int64_t areasum= 0;
    // sum of the areas of subsequently shrunk spanning triangles;
    // this is to prevent malicious shape manipulations

  switch(bufn) {  // in dependence of buffer level
  default:
  case 0:  // buffer is empty
    if(id==0)  // buffer shall be written completely
      return;  // nothing to do because buffer is empty
    idll= id;  // copy new id to id buffer
    buf.xll= x; buf.yll= y;  // copy new coordinates to buffer
    bufn++;  // first entry is used now
    return;
  case 1:
    if(id==0) {  // buffer shall be written completely
      posw__storenode(idll,buf.xll,buf.yll); bufn= 0; return; }
    idl= id;  // copy new id to id buffer
    buf.xl= x; buf.yl= y;  // copy new coordinates to buffer
    bufn++;  // second entry is used now
    return;
  case 2:
    if(id==0) {  // buffer shall be written completely
      posw__storenode(idll,buf.xll,buf.yll);
      posw__storenode(idl,buf.xl,buf.yl); bufn= 0;
      areasum= 0;
      return;
      }
    // determine if the point in the middle can be removed without
    // doing too much harm;
    // decision criterion:
    // the larger the area of the triangle which is spanned by the
    // three points, the more important is the point in the middle;
    // this means, we have to calculate the triangles area and compare
    // the result with a user-defined parameter; this parameter
    // represents the desired grade of polygon simplification;
    // how to calculate the area of a random triangle:
    // take the area of the circumscribed right-angled triangle which
    // is spanned by first and last point,
    // subtract the areas of the circumscribed right-angled triangles
    // which are spanned by first and middle as well as middle and
    // last point, then subtract the remaining square;
    if(global_simplify>0) {  // user wants polygon simplification
      area= (int64_t)(x-buf.xll)*(int64_t)(y-buf.yll);
        // outer triangle
      area-= (int64_t)(buf.xl-buf.xll)*(int64_t)(buf.yl-buf.yll);
        // first triangle
      area-= (int64_t)(x-buf.xl)*(int64_t)(y-buf.yl);
        // second triangle
      area/= 2;  // oops, we calculated rectangles, now it are triangles
      area-= (int64_t)(buf.xl-buf.xll)*(int64_t)(y-buf.yl);
        // square
      if(area<0) area= -area;  // absolute value
      area/= cosrk(buf.yl);
        // convert to square meters, take latitude into account
      }  // user wants polygon simplification
    else
      area= 1;
    if(area<=global_simplify && areasum<global_simplify) {
        // point in the middle is nearly redundant
      // overwrite point in the middle with coordinates from
      // newest point
      if(loglevel>=5) {
        char sx[15],sy[15];

        write_createsfix7o(buf.xl,sx);
        write_createsfix7o(buf.yl,sy);
        PINFOv("  simplifying %"PRIi64" (%s,%s)",id,sx,sy)
        }
      idl= id;
      buf.xl= x; buf.yl= y;
      areasum+= area;  // cumulate area
      }  // point in the middle is nearly redundant
    else {  // point in the middle is important
      posw__storenode(idll,buf.xll,buf.yll);
        // write oldest point to output
      idll= idl;
      buf.cll= buf.cl;  // move point in the middle to oldest point      
      idl= id;
      buf.xl= x; buf.yl= y;  // store new point as point in the middle
      areasum= 0;  // reset cumulated area
      }  // point in the middle is important
    return;
    }  // in dependence of buffer level
  #undef D
  }  // posw__simplify()

static inline void posw_setnode(int64_t id) {
  // store the coordinates of the latest specified way;
  // id: id of this node object;
  // (see remarks at posw_setstart();)
  if(posw__coorde>=posw__coordee)  // not enough space coordinates list
    exit(70012);
  posn_get(id);
  if(posn_xy==NULL)  // the geoposition of this node is not available
return;
  posw__simplify(id,posn_xy[0],posn_xy[1]);
  }  // end   posw_setnode()

static inline void posw_setend() {
  // end setting data of one way object;
  // (see remarks at posw_setstart();)
  posw__simplify(0,0,0);
    // terminate writing coordinates for this way;
  posw__memi->firstnode= posw_firstnode;
  posw__memi->lastnode= posw_lastnode;

  if(posw_lastnode==0) {  // there have not been stored
      // at least two nodes for this way object
    // remove this way object's information
    posw__meme = posw__memi;
    if(posw_firstnode!=0) {
        // id of first node has already been stored;
        // i.e. the first node's coordinates have been stored as well,
        // as same as the preceding dummy coordinate pair;
      // remove these coordinates
      posw__coorde-= 4;
      // remember that coordinates have been removed
      posw_firstnode= 0;
      }
return;
    }
  // here: a valid way object information has been stored;
  // enter the number of coordinate pairs of this list into the header
  posw__memi->xy[1]= (posw__coorde-posw__memi->xy)/2-1;
  // now add a dummy geoposition to the coordinates list;
  // this is done to mark this section of the list as complete;
  // there is no need to increment posw__coorde pointer because a possibly
  // added next coordinates list will start with posw_nil as well;
  posw__coorde[0]= posw_nil;
  posw__coorde[1]= 0;
  // prepare for the next way object information
  posw_firstnode= posw_lastnode= 0;
  posw__memi= posw__meme;
  }  // end   posw_setend()

static inline void posw_get(int64_t id) {
  // get the previously stored information of a way object;
  // id: id of the way object;
  // return:
  // posw_firstnode: id of the first referenced node object;
  //                 0: there is no information about this way object;
  // posw_lastnode:  id of the last referenced node object;
  //                 0: there is no information about this way object;
  // posw_xy: start of latest read coordinates list;
  //   posw_xy[0],posw_xy[1]: coordinates of first node;
  //   posw_xy[2],posw_xy[3]: coordinates of second node;
  //   posw_xy[...]==posw_nil: end of list;
  //   posw_xy==NULL: no coordinates list available for this id;
  //   the caller may change the list contents;
  // this procedure must not be called while still way object
  // information is being stored (posw_set...());)
  char* min,*max,*middle;
  int64_t middle_id;

  min= (char*)posw__mem;
  max= (char*)posw__meme;
  while(max>min) {  // binary search
    middle= (((max-min-posw__mem_size)/2)&(posw__mem_mask))+min;
    middle_id= *(int64_t*)middle;
    if(middle_id==id) {  // we have found the right object
      posw_firstnode= ((posw__mem_t*)middle)->firstnode;
      posw_lastnode= ((posw__mem_t*)middle)->lastnode;
      posw_xy= ((posw__mem_t*)middle)->xy;
return;
      }
    if(middle_id>id)
      max= middle;
    else
      min= middle+posw__mem_size;
    }  // binary search
  // here: did not find the object in question
  posw_firstnode= posw_lastnode= 0;
  posw_xy= NULL;
  }  // end   posw_get();

//------------------------------------------------------------
// end   Module posw_   OSM way position module
//------------------------------------------------------------



//------------------------------------------------------------
// Module posr_   OSM relation processing module
//------------------------------------------------------------

// this module provides a container for storing information of
// OSM multipolygon objects;
// the procedures posr_set...() and posr_get() allow access to
// this array;
// this module uses the procedure posw_get() of the module 'posw';
// as usual, all identifiers of a module have the same prefix,
// in this case 'posr'; an underline will follow for a global
// accessible identifier, two underlines if the identifier
// is not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

struct posr__mem_struct {  // element of container
  int64_t firstnode;  // id of first referenced node
  int64_t lastnode;  // id of last referenced node
  int32_t* xy;  // start address of polygon's coordinates array;
    // (see structure posw__mem_struct in module 'posw';
  int32_t stamp;  // to mark this element as been processed
  };
typedef struct posr__mem_struct posr__mem_t;

// separate containers for outer and inner polygons
static posr__mem_t* posr__outer= NULL;  // start address of array
static posr__mem_t* posr__outere= NULL;  // logical end address
static posr__mem_t* posr__outeree= NULL;  // physical end address
static posr__mem_t* posr__inner= NULL;  // start address of array
static posr__mem_t* posr__innere= NULL;  // logical end address
static posr__mem_t* posr__inneree= NULL;  // physical end address
#define posr__max 50000  // maximum number of elements per container

static int posr__subn= 0;  // number of currently written subpolygon;
  // this is to recognize when the next subpolygon is written;
  // used by posr__writepolygonheader() and posr__writepolygon();
static const char* posr__name= NULL;
  // name of the currently processed polygon;
  // used by posr_setend() and posr__writepolygon();

static void posr__writepolygonheader(const char* name) {
  // write a new polygon headline into output file;
  // name[]: name of the new polygon;
  //         ==NULL: end of all polygons in the file;
  // note that the header is just stored and not written immediately,
  // it is just stored and written with the first subpolygon;
  if(name==NULL) {  // footer is to be written to polygon file
    if(posr__subn!=0) {  // subpolygon footer needs to be written
      write_str("END" NL);
      posr__subn= 0;
      }
    write_str("END" NL NL);
    }
  else  // regular polygon headline shall be written
    posr__name= name;  // store this name to write it later;
  }  // end   posr__writepolygonheader()

static void posr__writepolygon(int subn,int32_t* xy,bool reverse) {
  // write a subpolygon to output file;
  // subn: number of subpolygon; will be written into file only if
  //       it has changed since the last call of this procedure;
  //       <0: write it as a positive number with preceding '!';
  //       this is used to mark inner subpolygons;
  // xy[]: coordinate list;
  // starts and ends with a pair of dummy coordinates: posw_nil,n,
  // where n is the number of coordinate pairs in the list if it is
  // the dummy pair at the start; n is 0 in the dummy at the end;
  // reverse: write the polygon's coordinates in reverse order;
  static int32_t xlast= COORDINATE_NIL,ylast= COORDINATE_NIL;
    // when concatenating two ways, the coordinates we have chosen
    // to concatenate the ways at, belong to both ways; these
    // coordinates should appear in the resulting polygon file only
    // once; the variables 'xlast' and 'ylast' help to achieve this;
  int32_t x,y;  // coordinates
  #define D(x,y) { write_char(' '); write_sfix7o(x); \
    write_char(' '); write_sfix7o(y); write_str(NL); }

  if(posr__name!=NULL) {  // a new header needs to be written
    write_str(posr__name); write_str(NL);
    posr__name= NULL;
    posr__subn= 0;
    xlast= ylast= COORDINATE_NIL;  // reset compare values
    }
  if(subn!=posr__subn) {  // number of subpolygon has changed
    if(posr__subn!=0) {
        // subpolygon footer needs to be written
      write_str("END" NL);
      posr__subn= subn;
      xlast= ylast= COORDINATE_NIL;  // reset compare values
      }  // subpolygon footer needs to be written
    posr__subn= subn;  // store the new number
    if(subn>=0) {  // outer subpolygon: no '!'
      write_uint32(subn); write_str(NL); }
    else {  // inner subpolygon: write preceding '!'
      write_char('!'); write_uint32(-subn); write_str(NL); }
    }  // number of subpolygon has changed
  if(xy==NULL || xy[0]!=posw_nil)  // address not valid
return;
  if(reverse) {  // writing in reverse order is required
    xy+= xy[1]*2+2;  // jump to end of list
    for(;;) {  // for each pair of coordinates
      y= *--xy;
      x= *--xy;
      if(x==posw_nil)  // start of coordinates list
    break;
      if(x==xlast && y==ylast)  // coordinate douplicates
    continue;  // ignore these coordinates
      xlast= x; ylast= y;
      D(x,y);
      }  // for each pair of coordinates
    }  // writing in reverse order is required
  else {  // writing in regular order is required
    xy+= 2;  // jump over header dummy-coordinate pair
    for(;;) {  // for each pair of coordinates
      x= *xy++;
      if(x==posw_nil)  // end of coordinates list
    break;
      y= *xy++;
      if(x==xlast && y==ylast)  // coordinate douplicates
    continue;  // ignore these coordinates
      xlast= x; ylast= y;
      D(x,y);
      }  // for each pair of coordinates
    }  // writing in regular order is required
  #undef D
  }  // end   posr__writepolygon()

static void posr__end() {
  // clean-up for posr module;
  // will be called at program's end;
  if(posr__outer==NULL)
    PERR("not enough memory for outer polygon objects.")
  else {  // was initialized
    if(posr__outere>=posr__outeree)  // not enough space in container
      PERR("not enough space in inner polygon array.")
    free(posr__outer);
    posr__outer= NULL;
    }
  if(posr__inner==NULL)
    PERR("not enough memory for inner polygon objects.")
  else {  // was initialized
    if(posr__innere>=posr__inneree)  // not enough space in container
      PERR("not enough space in inner polygon array.")
    free(posr__inner);
    posr__inner= NULL;
    }
  }  // end   posr__end()

//------------------------------------------------------------

static int posr_ini() {
  // initialize the posr module;
  // return: 0: OK; 1: not enough memory;
  int64_t siz;

  if(posr__outer!=NULL || posr__inner!=NULL)  // already initialized
return 0;
  atexit(posr__end);  // chain-in the clean-up procedure
  // allocate memory for the two containers
  siz= sizeof(posr__mem_t)*posr__max;
  // allocate memory for outer-polygon container
  posr__outer= (posr__mem_t*)malloc(siz+sizeof(posr__mem_t)*2);
  if(posr__outer==NULL)  // not enough memory
return 1;
  posr__outere= posr__outer;
  posr__outeree= (posr__mem_t*)((char*)posr__outer+siz);
  // allocate memory for inner-polygon container
  posr__inner= (posr__mem_t*)malloc(siz+sizeof(posr__mem_t)*2);
  if(posr__inner==NULL)  // not enough memory
return 1;
  posr__innere= posr__inner;
  posr__inneree= (posr__mem_t*)((char*)posr__inner+siz);
  return 0;
  }  // end   posr_ini()

static inline void posr_setstart() {
  // start setting data of one relation object;
  // sequence:
  // to process a relation object's information call at first this
  // procedure; after that, call posr_setway() for each referenced
  // way object of this way object;
  // after the last referenced way object, call posr_setend();
  posr__outere= posr__outer;  // clear container
  posr__innere= posr__inner;  // clear container
  }  // end   posr_setstart()

static inline void posr_setouter(int64_t id) {
  // store information about a referenced outer way;
  // id: id of this way object;
  // (see remarks at posr_setstart();)
  if(posr__outere>=posr__outeree)  // not enough space in array
    exit(70021);
  posw_get(id);
  if(posw_firstnode!=0) {  // information about this way is available
    posr__outere->firstnode= posw_firstnode;
      // id of first referenced node
    posr__outere->lastnode= posw_lastnode;
      // id of last referenced node
    posr__outere->xy= posw_xy;
      // start address of polygon's coordinates array;
    posr__outere->stamp= 0;  // initialize process counter
    posr__outere++;
    }
  }  // end   posr_setouter()

static inline void posr_setinner(int64_t id) {
  // store information about a referenced inner way;
  // id: id of this way object;
  // (see remarks at posr_setstart();)
  if(posr__innere>=posr__inneree)  // not enough space in array
    exit(70022);
  posw_get(id);
  if(posw_firstnode!=0) {  // information about this way is available
    posr__innere->firstnode= posw_firstnode;
      // id of first referenced node
    posr__innere->lastnode= posw_lastnode;
      // id of last referenced node
    posr__innere->xy= posw_xy;
      // start address of polygon's coordinates array;
    posr__innere->stamp= 0;  // initialize process counter
    posr__innere++;
    }
  }  // end   posr_setinner()

static inline void posr_setend(int64_t id,const char* name) {
  // terminate setting data of one relation object,
  // and process this data;
  // id: id of this relation;
  // name[]: name of the currently processed polygon;
  //         ==NULL: end of all polygons in the file;
  // processing the data means:
  // start with first node id of first referenced outer way object;
  // search for first/last node with same id of another way object;
  // take the other end-node from that way object and repeat the
  // search until the cycle is complete or the search is unsuccessful;
  // then start with the latest processed node and walk the way back,
  // but this time write the polygon into the output file and
  // virtually remove each processed way object from the container;
  // after having processed all outer way object, do the same with
  // the inner ones;
  // however: if there have been two or more unclosed outer polygon,
  // do not process the inner way objects at all;
  // (also see remarks at posr_setstart();)
  // elements in posr__outer resp. posr__inner:
  posr__mem_t* elea;  // first valid element;
  posr__mem_t* ele;  // latest processed valid element;
  posr__mem_t* e;  // currently examined element;
  posr__mem_t* elee;  // logical end of storage container;
  // other variables:
  int64_t node,n;  // currently examined node
  int64_t closingnode;  // id of first examined node; the subpolygon
    // is considered 'closed' if last examined node has the same id;
  int subpolyn;  // number of subpolygon
  int32_t stamp;
  int unclosedn;  // number of unclosed subpolygons

  posr__writepolygonheader(name);
  if(posr__outere==posr__outer) {  // no valid way object references
    PLOGv(2,"no outer ways in relation %"PRIi64" %s",id,name)
goto posr__setend_end;
    }
  subpolyn= 0;
  unclosedn= 0;
  PLOGv(3,"processing relation %"PRIi64" %s",id,name)

  // process closed outer way objects
  elee= posr__outere;
  ele= posr__outer;
  while(ele<elee) {
    node= ele->firstnode;
    if(node!=0 && node==ele->lastnode) {
        // way object has not been removed AND it is a closed way
      // write this subpolygon
      subpolyn++;
      PLOGv(4,"  closed outer: %"PRIi64,(int64_t)(ele-posr__outer))
      posr__writepolygon(subpolyn,ele->xy,false);
      ele->firstnode= 0;  // mark this node as processed
      }
    ele++;  // take next way object
    }

  // process remaining, unclosed outer way objects
  elea= posr__outer;
  stamp= 0;
  for(;;) {  // for all unclosed outer way objects
    stamp++;

    // search for next valid node
    while((node= elea->firstnode)==0) {
        // this way object has been removed
      elea++;  // take next way object
      if(elea>=elee)  // all outer way objects have been removed
        goto posr__setend_outer_end;
      }
    PLOGv(5,"  starting at outer %"PRIi64,(int64_t)(elea-posr__outer))
    node= elea->firstnode;
    closingnode= elea->lastnode;
    elea->stamp= stamp;  // mark this element to avoid cycles

    // follow the chain
    ele= elea;
    e= ele+1;
    while(e<elee) {
      n= e->firstnode;
      if(n!=0 && e->stamp!=stamp) {
          // way has not been removed and has not been processed lately
        if(n==node) {
          ele= e;
          node= ele->lastnode;
          ele->stamp= stamp;  // mark this element to avoid cycles
          PLOGv(5,"  chain: %"PRIi64,(int64_t)(ele-posr__outer))
          e= elea;
          }
        else if(e->lastnode==node) {
          ele= e;
          node= n;
          ele->stamp= stamp;  // mark this element to avoid cycles
          PLOGv(5,"  chain: %"PRIi64,(int64_t)(ele-posr__outer))
          e= elea;
          }
        }
      e++;
      }

    // determine if this subpolygon is closed
    if(node!=closingnode) {  // subpolygon is not closed
      PLOGv(5,"  unclosed: outer %"PRIi64,(int64_t)(ele-posr__outer))
      unclosedn++;  // increment number of unclosed subpolygons
      }

    // follow the chain in opposite direction while writing the polygon
    PLOG(4,"  concatenating outer ways:")
    subpolyn++;
    for(;;) {  // for each subpolygon in chain
      // write this subpolygon
      if(node==ele->firstnode) {
        PLOGv(4,"  outer %"PRIi64,(int64_t)(ele-posr__outer))
        posr__writepolygon(subpolyn,ele->xy,false);
        node= ele->lastnode;
        ele->firstnode= 0;  // remove this way object
        }
      else {
        PLOGv(4,"  outer %"PRIi64,(int64_t)(ele-posr__outer))
        posr__writepolygon(subpolyn,ele->xy,true);
        node= ele->firstnode;
        ele->firstnode= 0;  // remove this way object
        }
      // look for the next subpolygon
      e= elea;
      do {
        n= e->firstnode;
        if(n!=0 && (n==node || e->lastnode==node))
            // way has not been removed AND we have found the linked way
      break;
        e++;
        } while(e<elee);
      if(e>=elee)  // no more subpolygons in this chain
    break;
      ele= e;
      }
    }  // for all unclosed outer way objects
  posr__setend_outer_end:

  // do not process inner subpolygons if there are two or more
  // unclosed outer subpolygons, or if there is no outer subpolygon
  if(subpolyn==0 || unclosedn>=2) {
    PLOG(4,"  ignoring inner ways because of unclosed outer ways.")
goto posr__setend_end;
    }  

  // process closed inner way objects
  elee= posr__innere;
  ele= posr__inner;
  while(ele<elee) {
    node= ele->firstnode;
    if(node!=0 && node==ele->lastnode) {
        // way object has not been removed AND it is a closed way
      // write this subpolygon
      subpolyn++;
      PLOGv(4,"  closed inner: %"PRIi64,(int64_t)(ele-posr__inner))
      posr__writepolygon(-subpolyn,ele->xy,false);
      ele->firstnode= 0;  // mark this node as processed
      }
    ele++;  // take next way object
    }

  // process remaining, unclosed inner way objects
  elea= posr__inner;
  stamp= 0;
  for(;;) {  // for all unclosed inner way objects
    stamp++;

    // search for next valid node
    while((node= elea->firstnode)==0) {
        // this way object has been removed
      elea++;  // take next way object
      if(elea>=elee)  // all inner way objects have been removed
        goto posr__setend_inner_end;
      }
    PLOGv(5,"  starting at inner %"PRIi64,(int64_t)(elea-posr__inner))
    node= elea->firstnode;
    closingnode= elea->lastnode;
    elea->stamp= stamp;  // mark this element to avoid cycles

    // follow the chain
    ele= elea;
    e= ele+1;
    while(e<elee) {
      n= e->firstnode;
      if(n!=0 && e->stamp!=stamp) {
          // way has not been removed and has not been processed lately
        if(n==node) {
          ele= e;
          node= ele->lastnode;
          ele->stamp= stamp;  // mark this element to avoid cycles
          PLOGv(5,"  chain: %"PRIi64,(int64_t)(ele-posr__inner))
          e= elea;
          }
        else if(e->lastnode==node) {
          ele= e;
          node= n;
          ele->stamp= stamp;  // mark this element to avoid cycles
          PLOGv(5,"  chain: %"PRIi64,(int64_t)(ele-posr__inner))
          e= elea;
          }
        }
      e++;
      }

    // determine if this subpolygon is closed
    if(node!=closingnode) {  // subpolygon is not closed
      PLOGv(5,"  unclosed: inner %"PRIi64,(int64_t)(ele-posr__inner))
      unclosedn++;  // increment number of unclosed subpolygons
      }

    // follow the chain in opposite direction while writing the polygon
    PLOG(4,"  concatenating inner ways:")
    subpolyn++;
    for(;;) {  // for each subpolygon in chain
      // write this subpolygon
      if(node==ele->firstnode) {
        PLOGv(4,"  inner %"PRIi64,(int64_t)(ele-posr__inner))
        posr__writepolygon(-subpolyn,ele->xy,false);
        node= ele->lastnode;
        ele->firstnode= 0;  // remove this way object
        }
      else {
        PLOGv(4,"  inner %"PRIi64,(int64_t)(ele-posr__inner))
        posr__writepolygon(-subpolyn,ele->xy,true);
        node= ele->firstnode;
        ele->firstnode= 0;  // remove this way object
        }
      // look for the next subpolygon
      e= elea;
      do {
        n= e->firstnode;
        if(n!=0 && (n==node || e->lastnode==node))
            // way has not been removed AND we have found the linked way
      break;
        e++;
        } while(e<elee);
      if(e>=elee)  // no more subpolygons in this chain
    break;
      ele= e;
      }
    }  // for all unclosed inner way objects
  posr__setend_inner_end:

posr__setend_end:
  posr__writepolygonheader(NULL);
  posr__outere= posr__outer;  // clear container
  posr__innere= posr__inner;  // clear container
  }  // end   posr_setend()

//------------------------------------------------------------
// end   Module posr_   OSM relation processing module
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
// Module oo_   osm to poly module
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
  // do some initialization for the format, of necessary;
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
  byte* bufp;  // pointer in read buffer
  #define bufsp ((char*)bufp)  // for signed char
  byte* bufe;  // pointer in read buffer, end of object
  byte b;  // latest byte which has been read
  int l;
  byte* bp;
  char* sp;

  // procedure initialization
  atexit(oo__end);
  writeheader= true;
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
      }

    // object initialization
    hisver= 0;
    histime= 0;
    hiscset= 0;
    hisuid= 0;
    hisuser= "";
    if(oo__ifp->deleteobject==1) oo__ifp->deleteobject= 0;

    // read one osm object
    /***/ {  // o5m
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
          posn_set(id,lon,lat);
          }  // end   node
        // read noderefs (for ways only)
        if(otype==1) {  // way
          l= pbf_uint32(&bufp);
          bp= bufp+l;
          if(bp>bufe) bp= bufe;  // (format error)
          posw_setstart(id);
          while(bufp<bp)
            posw_setnode(oo__ifp->o5rid[0]+= pbf_sint64(&bufp));
          posw_setend();
          }  // end   way
        // read refs (for relations only)
        else if(otype==2) {  // relation
          int64_t ri,rid;  // temporary, refid
          int rt;  // temporary, reftype
          char* rr;  // temporary, refrole

          posr_setstart();
          l= pbf_uint32(&bufp);
          bp= bufp+l;
          if(bp>bufe) bp= bufe;  // (format error)
          while(bufp<bp) {  // each reference
            ri= pbf_sint64(&bufp);
            str_read(&bufp,&rr,NULL);
            rt= *rr++ -'0';
            rid= oo__ifp->o5rid[rt]+= ri;  // (delta-coded)
            if(rr[0]=='o' || rr[0]==0)  // outer
              posr_setouter(rid);
            else if(rr[0]=='i')  // inner
              posr_setinner(rid);
            }  // each reference
          }  // end   relation
        // read node key/val pairs
        if(otype!=2) {  // not a relation
          char* k,*v;  // key and val tag

          while(bufp<bufe)  // each k-v pair
            str_read(&bufp,&k,&v);  // read and dispose of it
          }  // not a relation
        else {  // relation
          // check for necessary tags
          char* k,*v;  // key and val tag
          char name[404],*namep;  // some space to concatenate
            // name, wikipedia and admin_level
          char* vname;  // name tag
          char* vnameprefix;  // name prefix tag
          char* vwikipedia;  // wikipedia tag
          char* vadminlevel;  // admin_level tag

          vname= vnameprefix= vwikipedia= vadminlevel= NULL;
          while(bufp<bufe) {  // each k-v pair
            str_read(&bufp,&k,&v);
            if(k[0]=='n' && k[1]=='a' && k[2]=='m' && k[3]=='e') {
                // tag key starts with "name"
              if(k[4]==0)  // tag key _is_ "name"
                vname= v;
              else if(k[4]==':' && k[5]=='p' && k[6]=='r' &&
                  k[7]=='e' && k[8]=='f' && k[9]=='i' && k[10]=='x' &&
                  k[11]==0) {
                // this is a special extension for
                // German urban districts:
                if(strcmp(v,"Ortsteil")==0)
                  vnameprefix= v;
                }
              }  // tag key starts with "name"
            else if(k[0]=='w' && k[1]=='i' && k[2]=='k' && k[3]=='i' &&
                k[4]=='p' && k[5]=='e' && k[6]=='d' && k[7]=='i' &&
                k[8]=='a' && k[9]==0)
              vwikipedia= v;
            else if(k[0]=='a' && k[1]=='d' && k[2]=='m' && k[3]=='i' &&
                k[4]=='n' && k[5]=='_' && k[6]=='l' && k[7]=='e' &&
                k[8]=='v' && k[9]=='e' && k[10]=='l' && k[11]==0)
              vadminlevel= v;
            }  // each k-v pair
          if(vname!=NULL) {  // there is a name
            namep= stpmcpy(name,vname,101);
            if(vnameprefix!=NULL && vwikipedia!=NULL) {
                // there are prefix and wikipedia tags
              char* wp0,*wp1;  // pointers within vwikipedia[]
              int len;

              wp0= strchr(vwikipedia,':');
              wp1= strchr(vwikipedia,'-');
              len= wp1-wp0;
              if(wp0!=NULL && wp1!=NULL && len>0) {
                if(len>100) len= 101;
                memcpy(name,wp0+1,len);
                namep= stpmcpy(name+len,vname,101);
                }
              }  // there are prefix and wikipedia tags
            if(global_addadminlevel!=NULL && vadminlevel!=NULL) {
              // add value of admin_level tag to relation's name
              namep= stpmcpy(namep,global_addadminlevel,101);
                // append separator
              stpmcpy(namep,vadminlevel,101);
                // append value of admin_level tag key
              }  // add value of admin_level tag
            posr_setend(id,name);
            }  // there is a name
          }  // relation
        }  // end   not a delete request
      }  // end   o5m

    // check sequence
    /***/ {
      if(otype<=oo_sequencetype &&
          (otype<oo_sequencetype || id<oo_sequenceid ||
          (oo_ifn>1 && id<=oo_sequenceid)))
        PWARNv("wrong sequence at %s %"PRIi64,ONAME(otype),id)
      oo_sequencetype= otype; oo_sequenceid= id;
      }

    }  // end   read all input files
  if(!writeheader)
    ;
  return 0;
  }  // end   oo_main()

//------------------------------------------------------------
// end   Module oo_   osm to poly module
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
    if(strcmp(a,"--out-poly")==0) {
        // user wants output in .poly format
      // do nothing since this is default; furthermore, the program
      // does not know how to write other formats anyway;
  continue;  // take next parameter
      }
    if(strcmp(a,"--out-none")==0) {
        // user does not want any standard output
      global_outnone= true;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--max-nodes="))>0 && a[l]!=0) {
        // define maximum number of nodes
      global_maxnodes= oo__strtosint64(a+l);
      if(global_maxnodes<4) global_maxnodes= 4;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--max-ways="))>0 && a[l]!=0) {
        // define maximum number of ways
      global_maxways= oo__strtosint64(a+l);
      if(global_maxways<4) global_maxways= 4;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--simplify="))>0 && a[l]!=0) {
        // define grade of simplification
      global_simplify= oo__strtosint64(a+l);
      if(global_simplify<0) global_simplify= 0;
      global_simplify*= global_simplify;
  continue;  // take next parameter
      }
    if((l= strzlcmp(a,"--add-admin-levels"))>0) {
        // append values of admin levels
      if(a[l]=='=' && strlen(a+l+1)<=100)
          // a user-defined separator has been chosen AND
          // its length is reasonable (security precaution)
        global_addadminlevel= a+l+1;
      else
        global_addadminlevel= ";";
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
  if(write_open(outputfilename[0]!=0? outputfilename: NULL)!=0)
return 3;

  // initialization
  posn_ini();
  posw_ini();
  posr_ini();

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

