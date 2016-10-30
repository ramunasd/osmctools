#define PROGRAM "osmgeobase"
#define VERSION "0.0.0"
// 2016-03-30 05:10
//
// compile this file:
//   gcc osmgeobase.c -O3 -o osmgeobase
// there may be compiler warnings informing you about unused procedures
// and variables; both have been left in the source file intentionally,
// they are for future expansions;
//
// tests:
// gcc osmgeobase.c -g -o osmgeobase
// ./osmgeobase -v adrr.o5m
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
// This program creates and fills the data structures for effective
// geocoding.
// An .o5m file containing address data as node objects is read.
// Tag values must also be provided unique-sorted and zero-terminated
// by separate files:
//    strings_region, strings_subregion, strings_city,
//    strings_street, strings_housenumber.
// The file strings_nodecount must contain the actual number of
// node objects of .o5m input file.

#ifndef I  // from here: main program

#define MAXLOGLEVEL 1
  // keep at 1, unless you want do debug
const char* shorthelptext=
"\n" PROGRAM " " VERSION "  Parameter Overview\n"
"(Please use  --help  to get more information.)\n"
"\n"
"<FILE>                    input file name\n"
"-                         read from standard input\n"
"--max-strrefs=<n>         maximum space for string references\n"
"-h                        display this parameter overview\n"
"--help                    display a more detailed help\n"
"--out-none                no standard output (for testing purposes)\n"
"-o=<outfile>              reroute standard output to a file\n"
"--parameter-file=<file>   param. in file, separated by empty lines\n"
"--verbose                 activate verbose mode\n";
const char* helptext=
"\n" PROGRAM " " VERSION "\n"
"\n"
"This program creates and fills the data structures for effective\n"
"geocoding.\n"
"An .o5m file containing address data as node objects is read.\n"
"Tag values must also be provided unique-sorted and zero-terminated\n"
"by separate files:\n"
"    strings_region, strings_subregion, strings_city,\n"
"    strings_street, strings_housenumber.\n"
"The file strings_nodecount must contain the actual number of\n"
"node objects of .o5m input file. If there are semicolon-separated\n"
"housenumbers, this number must have been increased accordingly.\n"
"\n"
"-h\n"
"        Display a short parameter overview.\n"
"\n"
"--help\n"
"        Display this help.\n"
"\n"
"--max-strrefs=<n>\n"
"        The program needs to allocate a certain amount of memory\n"
"        space for string references. The size of this space can be\n"
"        changed by the user, in an absolute manner and in a relative\n"
"        manner. For example, this will allocate 20 % more space than\n"
"        usual:\n"
"            --max-strrefs=120%\n"
"        You also can allocate a concrete amount of MiB:\n"
"            --max-strrefs=800\n"
"        Use verbose option  -v  to see how much memory space is\n"
"        allocated by default.\n"
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
"./" PROGRAM " adrr.o5m\n"
"\n"
"Limitations\n"
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

static bool global_outnone= false;  // no standard output at all
static int32_t global_maxstrrefs= 0;
  // memory space for string references; 0: automatic;
  // <0: %-value for trimming automatically calculated value;
  // >0: absolute value in MiB;
static int64_t global_nodecount= 0;
  // number of nodes as stated in file "strings_nodecount"
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

static uint32_t strtouint32(const char* s) {
  // read a number and convert it to an unsigned 32-bit integer;
  // return: number;
  int32_t i;
  uint8_t b;

  i= 0;
  for(;;) {
    b= (uint8_t)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i;
  }  // strtouint32()

static inline int32_t strtoint32(const char* s) {
  // read a number and convert it to a signed 32-bit integer;
  // return: number;
  int sign;
  int i;
  uint8_t b;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  i= 0;
  for(;;) {
    b= (uint8_t)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i*sign;
  }  // strtoint32()

static inline int64_t strtosint64(const char* s) {
  // read a number and convert it to a signed 64-bit integer;
  // return: number;
  int sign;
  int64_t i;
  uint8_t b;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  i= 0;
  for(;;) {
    b= (uint8_t)(*s++ -'0');
    if(b>=10)
  break;
    i= i*10+b;
    }
  return i*sign;
  }  // strtosint64()

static int32_t strtoint32percent(const char* s) {
  // read an unsigned number and convert it to a signed 32-bit
  // integer; the result will be a positive number unless the
  // character string is followed by a percent sign;
  // this trailing percent sign will negate the value;
  // return: number;
  int32_t i;
  uint8_t b;

  if(*s=='-') s++;  // ignore leading minus sign
  i= 0;
  for(;;) {
    b= (uint8_t)(*s-'0');
    if(b>=10)
  break;
    i= i*10+b;
    s++;
    }
  if(*s==' ') s++;  // jump over space sign
  if(*s=='%')
    return -i;
  return i;
  }  // strtoint32percent()

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

static int32_t lonadapt(int32_t londiff,int32_t lat) {
  // takes a West-East distance given in longitude difference,
  // and calculates the adjusted distance in degrees,
  // i.e., it takes the latitude into account;
  // all units: 100 nano degrees;
  // londiff: West-East distance between two points;
  // lat: latitude at which the distance is to be calculated;
  // return: West-East distance in Equator degrees;
  //         this adjusted longitude difference is then comparable
  //         to latitude differences;
  static const uint32_t cosrtab[901]= {
    UINT32_C(4294967295),UINT32_C(4294960754),UINT32_C(4294941129),
    UINT32_C(4294908421),UINT32_C(4294862630),UINT32_C(4294803756),
    UINT32_C(4294731800),UINT32_C(4294646761),UINT32_C(4294548639),
    UINT32_C(4294437436),UINT32_C(4294313151),UINT32_C(4294175785),
    UINT32_C(4294025338),UINT32_C(4293861811),UINT32_C(4293685204),
    UINT32_C(4293495517),UINT32_C(4293292752),UINT32_C(4293076909),
    UINT32_C(4292847988),UINT32_C(4292605991),UINT32_C(4292350917),
    UINT32_C(4292082769),UINT32_C(4291801546),UINT32_C(4291507249),
    UINT32_C(4291199879),UINT32_C(4290879438),UINT32_C(4290545926),
    UINT32_C(4290199345),UINT32_C(4289839694),UINT32_C(4289466976),
    UINT32_C(4289081192),UINT32_C(4288682342),UINT32_C(4288270429),
    UINT32_C(4287845452),UINT32_C(4287407414),UINT32_C(4286956316),
    UINT32_C(4286492159),UINT32_C(4286014944),UINT32_C(4285524674),
    UINT32_C(4285021349),UINT32_C(4284504971),UINT32_C(4283975542),
    UINT32_C(4283433063),UINT32_C(4282877536),UINT32_C(4282308963),
    UINT32_C(4281727345),UINT32_C(4281132684),UINT32_C(4280524982),
    UINT32_C(4279904241),UINT32_C(4279270462),UINT32_C(4278623648),
    UINT32_C(4277963801),UINT32_C(4277290922),UINT32_C(4276605014),
    UINT32_C(4275906079),UINT32_C(4275194118),UINT32_C(4274469135),
    UINT32_C(4273731130),UINT32_C(4272980107),UINT32_C(4272216068),
    UINT32_C(4271439015),UINT32_C(4270648951),UINT32_C(4269845877),
    UINT32_C(4269029797),UINT32_C(4268200712),UINT32_C(4267358626),
    UINT32_C(4266503540),UINT32_C(4265635459),UINT32_C(4264754383),
    UINT32_C(4263860316),UINT32_C(4262953261),UINT32_C(4262033219),
    UINT32_C(4261100196),UINT32_C(4260154192),UINT32_C(4259195210),
    UINT32_C(4258223255),UINT32_C(4257238328),UINT32_C(4256240433),
    UINT32_C(4255229573),UINT32_C(4254205750),UINT32_C(4253168969),
    UINT32_C(4252119232),UINT32_C(4251056542),UINT32_C(4249980902),
    UINT32_C(4248892316),UINT32_C(4247790788),UINT32_C(4246676320),
    UINT32_C(4245548916),UINT32_C(4244408579),UINT32_C(4243255313),
    UINT32_C(4242089121),UINT32_C(4240910007),UINT32_C(4239717975),
    UINT32_C(4238513027),UINT32_C(4237295169),UINT32_C(4236064403),
    UINT32_C(4234820733),UINT32_C(4233564163),UINT32_C(4232294697),
    UINT32_C(4231012338),UINT32_C(4229717092),UINT32_C(4228408960),
    UINT32_C(4227087949),UINT32_C(4225754060),UINT32_C(4224407300),
    UINT32_C(4223047671),UINT32_C(4221675178),UINT32_C(4220289825),
    UINT32_C(4218891617),UINT32_C(4217480557),UINT32_C(4216056649),
    UINT32_C(4214619899),UINT32_C(4213170311),UINT32_C(4211707888),
    UINT32_C(4210232636),UINT32_C(4208744558),UINT32_C(4207243661),
    UINT32_C(4205729947),UINT32_C(4204203421),UINT32_C(4202664089),
    UINT32_C(4201111955),UINT32_C(4199547024),UINT32_C(4197969300),
    UINT32_C(4196378788),UINT32_C(4194775494),UINT32_C(4193159421),
    UINT32_C(4191530576),UINT32_C(4189888962),UINT32_C(4188234585),
    UINT32_C(4186567450),UINT32_C(4184887562),UINT32_C(4183194926),
    UINT32_C(4181489547),UINT32_C(4179771431),UINT32_C(4178040583),
    UINT32_C(4176297007),UINT32_C(4174540710),UINT32_C(4172771696),
    UINT32_C(4170989972),UINT32_C(4169195542),UINT32_C(4167388411),
    UINT32_C(4165568586),UINT32_C(4163736073),UINT32_C(4161890875),
    UINT32_C(4160033000),UINT32_C(4158162453),UINT32_C(4156279239),
    UINT32_C(4154383364),UINT32_C(4152474835),UINT32_C(4150553656),
    UINT32_C(4148619834),UINT32_C(4146673374),UINT32_C(4144714283),
    UINT32_C(4142742567),UINT32_C(4140758231),UINT32_C(4138761281),
    UINT32_C(4136751725),UINT32_C(4134729567),UINT32_C(4132694813),
    UINT32_C(4130647471),UINT32_C(4128587546),UINT32_C(4126515045),
    UINT32_C(4124429974),UINT32_C(4122332339),UINT32_C(4120222147),
    UINT32_C(4118099404),UINT32_C(4115964116),UINT32_C(4113816290),
    UINT32_C(4111655933),UINT32_C(4109483051),UINT32_C(4107297651),
    UINT32_C(4105099740),UINT32_C(4102889323),UINT32_C(4100666409),
    UINT32_C(4098431003),UINT32_C(4096183113),UINT32_C(4093922745),
    UINT32_C(4091649906),UINT32_C(4089364603),UINT32_C(4087066843),
    UINT32_C(4084756634),UINT32_C(4082433981),UINT32_C(4080098893),
    UINT32_C(4077751376),UINT32_C(4075391437),UINT32_C(4073019085),
    UINT32_C(4070634325),UINT32_C(4068237165),UINT32_C(4065827612),
    UINT32_C(4063405675),UINT32_C(4060971359),UINT32_C(4058524674),
    UINT32_C(4056065625),UINT32_C(4053594220),UINT32_C(4051110468),
    UINT32_C(4048614376),UINT32_C(4046105950),UINT32_C(4043585200),
    UINT32_C(4041052132),UINT32_C(4038506754),UINT32_C(4035949074),
    UINT32_C(4033379100),UINT32_C(4030796840),UINT32_C(4028202301),
    UINT32_C(4025595491),UINT32_C(4022976419),UINT32_C(4020345093),
    UINT32_C(4017701519),UINT32_C(4015045707),UINT32_C(4012377664),
    UINT32_C(4009697399),UINT32_C(4007004920),UINT32_C(4004300235),
    UINT32_C(4001583352),UINT32_C(3998854279),UINT32_C(3996113026),
    UINT32_C(3993359599),UINT32_C(3990594008),UINT32_C(3987816261),
    UINT32_C(3985026366),UINT32_C(3982224332),UINT32_C(3979410168),
    UINT32_C(3976583882),UINT32_C(3973745482),UINT32_C(3970894978),
    UINT32_C(3968032377),UINT32_C(3965157689),UINT32_C(3962270923),
    UINT32_C(3959372087),UINT32_C(3956461190),UINT32_C(3953538241),
    UINT32_C(3950603249),UINT32_C(3947656222),UINT32_C(3944697170),
    UINT32_C(3941726103),UINT32_C(3938743027),UINT32_C(3935747954),
    UINT32_C(3932740892),UINT32_C(3929721850),UINT32_C(3926690837),
    UINT32_C(3923647863),UINT32_C(3920592937),UINT32_C(3917526068),
    UINT32_C(3914447266),UINT32_C(3911356540),UINT32_C(3908253899),
    UINT32_C(3905139352),UINT32_C(3902012910),UINT32_C(3898874582),
    UINT32_C(3895724377),UINT32_C(3892562305),UINT32_C(3889388376),
    UINT32_C(3886202598),UINT32_C(3883004983),UINT32_C(3879795540),
    UINT32_C(3876574278),UINT32_C(3873341207),UINT32_C(3870096337),
    UINT32_C(3866839679),UINT32_C(3863571241),UINT32_C(3860291034),
    UINT32_C(3856999068),UINT32_C(3853695353),UINT32_C(3850379899),
    UINT32_C(3847052716),UINT32_C(3843713815),UINT32_C(3840363204),
    UINT32_C(3837000896),UINT32_C(3833626899),UINT32_C(3830241224),
    UINT32_C(3826843881),UINT32_C(3823434882),UINT32_C(3820014235),
    UINT32_C(3816581952),UINT32_C(3813138044),UINT32_C(3809682519),
    UINT32_C(3806215390),UINT32_C(3802736666),UINT32_C(3799246359),
    UINT32_C(3795744478),UINT32_C(3792231035),UINT32_C(3788706040),
    UINT32_C(3785169504),UINT32_C(3781621438),UINT32_C(3778061852),
    UINT32_C(3774490758),UINT32_C(3770908165),UINT32_C(3767314086),
    UINT32_C(3763708532),UINT32_C(3760091512),UINT32_C(3756463038),
    UINT32_C(3752823122),UINT32_C(3749171773),UINT32_C(3745509004),
    UINT32_C(3741834826),UINT32_C(3738149249),UINT32_C(3734452286),
    UINT32_C(3730743946),UINT32_C(3727024242),UINT32_C(3723293185),
    UINT32_C(3719550786),UINT32_C(3715797057),UINT32_C(3712032009),
    UINT32_C(3708255653),UINT32_C(3704468001),UINT32_C(3700669065),
    UINT32_C(3696858856),UINT32_C(3693037385),UINT32_C(3689204665),
    UINT32_C(3685360707),UINT32_C(3681505523),UINT32_C(3677639124),
    UINT32_C(3673761523),UINT32_C(3669872731),UINT32_C(3665972759),
    UINT32_C(3662061621),UINT32_C(3658139327),UINT32_C(3654205890),
    UINT32_C(3650261321),UINT32_C(3646305633),UINT32_C(3642338838),
    UINT32_C(3638360948),UINT32_C(3634371974),UINT32_C(3630371930),
    UINT32_C(3626360827),UINT32_C(3622338677),UINT32_C(3618305493),
    UINT32_C(3614261287),UINT32_C(3610206072),UINT32_C(3606139859),
    UINT32_C(3602062661),UINT32_C(3597974491),UINT32_C(3593875360),
    UINT32_C(3589765282),UINT32_C(3585644269),UINT32_C(3581512334),
    UINT32_C(3577369488),UINT32_C(3573215746),UINT32_C(3569051119),
    UINT32_C(3564875619),UINT32_C(3560689261),UINT32_C(3556492056),
    UINT32_C(3552284017),UINT32_C(3548065158),UINT32_C(3543835490),
    UINT32_C(3539595027),UINT32_C(3535343783),UINT32_C(3531081768),
    UINT32_C(3526808998),UINT32_C(3522525484),UINT32_C(3518231240),
    UINT32_C(3513926279),UINT32_C(3509610614),UINT32_C(3505284258),
    UINT32_C(3500947224),UINT32_C(3496599526),UINT32_C(3492241177),
    UINT32_C(3487872189),UINT32_C(3483492577),UINT32_C(3479102354),
    UINT32_C(3474701532),UINT32_C(3470290126),UINT32_C(3465868149),
    UINT32_C(3461435615),UINT32_C(3456992536),UINT32_C(3452538927),
    UINT32_C(3448074800),UINT32_C(3443600170),UINT32_C(3439115051),
    UINT32_C(3434619455),UINT32_C(3430113397),UINT32_C(3425596890),
    UINT32_C(3421069948),UINT32_C(3416532585),UINT32_C(3411984814),
    UINT32_C(3407426650),UINT32_C(3402858107),UINT32_C(3398279197),
    UINT32_C(3393689936),UINT32_C(3389090338),UINT32_C(3384480415),
    UINT32_C(3379860183),UINT32_C(3375229655),UINT32_C(3370588846),
    UINT32_C(3365937769),UINT32_C(3361276439),UINT32_C(3356604870),
    UINT32_C(3351923076),UINT32_C(3347231071),UINT32_C(3342528871),
    UINT32_C(3337816488),UINT32_C(3333093938),UINT32_C(3328361235),
    UINT32_C(3323618393),UINT32_C(3318865426),UINT32_C(3314102350),
    UINT32_C(3309329178),UINT32_C(3304545926),UINT32_C(3299752607),
    UINT32_C(3294949237),UINT32_C(3290135830),UINT32_C(3285312400),
    UINT32_C(3280478963),UINT32_C(3275635533),UINT32_C(3270782125),
    UINT32_C(3265918753),UINT32_C(3261045433),UINT32_C(3256162179),
    UINT32_C(3251269007),UINT32_C(3246365930),UINT32_C(3241452965),
    UINT32_C(3236530125),UINT32_C(3231597426),UINT32_C(3226654884),
    UINT32_C(3221702512),UINT32_C(3216740326),UINT32_C(3211768342),
    UINT32_C(3206786574),UINT32_C(3201795038),UINT32_C(3196793749),
    UINT32_C(3191782721),UINT32_C(3186761971),UINT32_C(3181731513),
    UINT32_C(3176691364),UINT32_C(3171641537),UINT32_C(3166582049),
    UINT32_C(3161512915),UINT32_C(3156434151),UINT32_C(3151345772),
    UINT32_C(3146247793),UINT32_C(3141140230),UINT32_C(3136023098),
    UINT32_C(3130896414),UINT32_C(3125760193),UINT32_C(3120614449),
    UINT32_C(3115459200),UINT32_C(3110294461),UINT32_C(3105120247),
    UINT32_C(3099936575),UINT32_C(3094743459),UINT32_C(3089540917),
    UINT32_C(3084328963),UINT32_C(3079107614),UINT32_C(3073876885),
    UINT32_C(3068636792),UINT32_C(3063387352),UINT32_C(3058128581),
    UINT32_C(3052860494),UINT32_C(3047583107),UINT32_C(3042296437),
    UINT32_C(3037000499),UINT32_C(3031695311),UINT32_C(3026380887),
    UINT32_C(3021057244),UINT32_C(3015724399),UINT32_C(3010382367),
    UINT32_C(3005031165),UINT32_C(2999670809),UINT32_C(2994301316),
    UINT32_C(2988922702),UINT32_C(2983534983),UINT32_C(2978138175),
    UINT32_C(2972732295),UINT32_C(2967317360),UINT32_C(2961893387),
    UINT32_C(2956460390),UINT32_C(2951018388),UINT32_C(2945567396),
    UINT32_C(2940107432),UINT32_C(2934638512),UINT32_C(2929160652),
    UINT32_C(2923673869),UINT32_C(2918178181),UINT32_C(2912673603),
    UINT32_C(2907160153),UINT32_C(2901637847),UINT32_C(2896106702),
    UINT32_C(2890566735),UINT32_C(2885017963),UINT32_C(2879460402),
    UINT32_C(2873894071),UINT32_C(2868318984),UINT32_C(2862735161),
    UINT32_C(2857142617),UINT32_C(2851541370),UINT32_C(2845931436),
    UINT32_C(2840312834),UINT32_C(2834685579),UINT32_C(2829049689),
    UINT32_C(2823405181),UINT32_C(2817752073),UINT32_C(2812090382),
    UINT32_C(2806420124),UINT32_C(2800741318),UINT32_C(2795053980),
    UINT32_C(2789358128),UINT32_C(2783653778),UINT32_C(2777940950),
    UINT32_C(2772219659),UINT32_C(2766489924),UINT32_C(2760751761),
    UINT32_C(2755005189),UINT32_C(2749250225),UINT32_C(2743486885),
    UINT32_C(2737715189),UINT32_C(2731935153),UINT32_C(2726146795),
    UINT32_C(2720350133),UINT32_C(2714545185),UINT32_C(2708731967),
    UINT32_C(2702910498),UINT32_C(2697080795),UINT32_C(2691242877),
    UINT32_C(2685396761),UINT32_C(2679542464),UINT32_C(2673680005),
    UINT32_C(2667809402),UINT32_C(2661930672),UINT32_C(2656043833),
    UINT32_C(2650148904),UINT32_C(2644245901),UINT32_C(2638334844),
    UINT32_C(2632415750),UINT32_C(2626488638),UINT32_C(2620553524),
    UINT32_C(2614610428),UINT32_C(2608659367),UINT32_C(2602700360),
    UINT32_C(2596733425),UINT32_C(2590758580),UINT32_C(2584775842),
    UINT32_C(2578785231),UINT32_C(2572786765),UINT32_C(2566780461),
    UINT32_C(2560766339),UINT32_C(2554744416),UINT32_C(2548714710),
    UINT32_C(2542677241),UINT32_C(2536632027),UINT32_C(2530579085),
    UINT32_C(2524518435),UINT32_C(2518450095),UINT32_C(2512374083),
    UINT32_C(2506290418),UINT32_C(2500199119),UINT32_C(2494100203),
    UINT32_C(2487993690),UINT32_C(2481879598),UINT32_C(2475757946),
    UINT32_C(2469628752),UINT32_C(2463492035),UINT32_C(2457347814),
    UINT32_C(2451196108),UINT32_C(2445036935),UINT32_C(2438870314),
    UINT32_C(2432696263),UINT32_C(2426514803),UINT32_C(2420325950),
    UINT32_C(2414129725),UINT32_C(2407926146),UINT32_C(2401715232),
    UINT32_C(2395497002),UINT32_C(2389271475),UINT32_C(2383038670),
    UINT32_C(2376798605),UINT32_C(2370551301),UINT32_C(2364296775),
    UINT32_C(2358035048),UINT32_C(2351766137),UINT32_C(2345490062),
    UINT32_C(2339206843),UINT32_C(2332916498),UINT32_C(2326619047),
    UINT32_C(2320314508),UINT32_C(2314002901),UINT32_C(2307684246),
    UINT32_C(2301358560),UINT32_C(2295025865),UINT32_C(2288686178),
    UINT32_C(2282339520),UINT32_C(2275985909),UINT32_C(2269625365),
    UINT32_C(2263257908),UINT32_C(2256883556),UINT32_C(2250502329),
    UINT32_C(2244114247),UINT32_C(2237719329),UINT32_C(2231317595),
    UINT32_C(2224909063),UINT32_C(2218493754),UINT32_C(2212071687),
    UINT32_C(2205642882),UINT32_C(2199207358),UINT32_C(2192765135),
    UINT32_C(2186316232),UINT32_C(2179860670),UINT32_C(2173398467),
    UINT32_C(2166929644),UINT32_C(2160454220),UINT32_C(2153972214),
    UINT32_C(2147483647),UINT32_C(2140988539),UINT32_C(2134486909),
    UINT32_C(2127978777),UINT32_C(2121464163),UINT32_C(2114943086),
    UINT32_C(2108415567),UINT32_C(2101881625),UINT32_C(2095341281),
    UINT32_C(2088794553),UINT32_C(2082241463),UINT32_C(2075682030),
    UINT32_C(2069116274),UINT32_C(2062544216),UINT32_C(2055965874),
    UINT32_C(2049381270),UINT32_C(2042790423),UINT32_C(2036193353),
    UINT32_C(2029590080),UINT32_C(2022980625),UINT32_C(2016365008),
    UINT32_C(2009743249),UINT32_C(2003115367),UINT32_C(1996481384),
    UINT32_C(1989841319),UINT32_C(1983195192),UINT32_C(1976543025),
    UINT32_C(1969884836),UINT32_C(1963220647),UINT32_C(1956550478),
    UINT32_C(1949874349),UINT32_C(1943192280),UINT32_C(1936504291),
    UINT32_C(1929810404),UINT32_C(1923110638),UINT32_C(1916405014),
    UINT32_C(1909693553),UINT32_C(1902976274),UINT32_C(1896253198),
    UINT32_C(1889524346),UINT32_C(1882789738),UINT32_C(1876049395),
    UINT32_C(1869303338),UINT32_C(1862551585),UINT32_C(1855794160),
    UINT32_C(1849031081),UINT32_C(1842262370),UINT32_C(1835488046),
    UINT32_C(1828708132),UINT32_C(1821922647),UINT32_C(1815131612),
    UINT32_C(1808335048),UINT32_C(1801532976),UINT32_C(1794725416),
    UINT32_C(1787912388),UINT32_C(1781093915),UINT32_C(1774270015),
    UINT32_C(1767440712),UINT32_C(1760606024),UINT32_C(1753765973),
    UINT32_C(1746920580),UINT32_C(1740069865),UINT32_C(1733213850),
    UINT32_C(1726352555),UINT32_C(1719486001),UINT32_C(1712614210),
    UINT32_C(1705737201),UINT32_C(1698854997),UINT32_C(1691967618),
    UINT32_C(1685075084),UINT32_C(1678177418),UINT32_C(1671274639),
    UINT32_C(1664366770),UINT32_C(1657453831),UINT32_C(1650535842),
    UINT32_C(1643612826),UINT32_C(1636684803),UINT32_C(1629751795),
    UINT32_C(1622813822),UINT32_C(1615870906),UINT32_C(1608923067),
    UINT32_C(1601970327),UINT32_C(1595012708),UINT32_C(1588050230),
    UINT32_C(1581082914),UINT32_C(1574110782),UINT32_C(1567133855),
    UINT32_C(1560152155),UINT32_C(1553165701),UINT32_C(1546174517),
    UINT32_C(1539178623),UINT32_C(1532178040),UINT32_C(1525172790),
    UINT32_C(1518162893),UINT32_C(1511148373),UINT32_C(1504129249),
    UINT32_C(1497105543),UINT32_C(1490077277),UINT32_C(1483044472),
    UINT32_C(1476007149),UINT32_C(1468965330),UINT32_C(1461919036),
    UINT32_C(1454868289),UINT32_C(1447813110),UINT32_C(1440753521),
    UINT32_C(1433689543),UINT32_C(1426621198),UINT32_C(1419548507),
    UINT32_C(1412471492),UINT32_C(1405390174),UINT32_C(1398304576),
    UINT32_C(1391214717),UINT32_C(1384120621),UINT32_C(1377022309),
    UINT32_C(1369919802),UINT32_C(1362813122),UINT32_C(1355702290),
    UINT32_C(1348587329),UINT32_C(1341468260),UINT32_C(1334345104),
    UINT32_C(1327217884),UINT32_C(1320086621),UINT32_C(1312951337),
    UINT32_C(1305812053),UINT32_C(1298668792),UINT32_C(1291521574),
    UINT32_C(1284370422),UINT32_C(1277215358),UINT32_C(1270056404),
    UINT32_C(1262893580),UINT32_C(1255726910),UINT32_C(1248556414),
    UINT32_C(1241382115),UINT32_C(1234204034),UINT32_C(1227022194),
    UINT32_C(1219836617),UINT32_C(1212647323),UINT32_C(1205454335),
    UINT32_C(1198257676),UINT32_C(1191057366),UINT32_C(1183853428),
    UINT32_C(1176645884),UINT32_C(1169434756),UINT32_C(1162220065),
    UINT32_C(1155001834),UINT32_C(1147780085),UINT32_C(1140554839),
    UINT32_C(1133326119),UINT32_C(1126093947),UINT32_C(1118858345),
    UINT32_C(1111619334),UINT32_C(1104376937),UINT32_C(1097131176),
    UINT32_C(1089882073),UINT32_C(1082629649),UINT32_C(1075373928),
    UINT32_C(1068114932),UINT32_C(1060852681),UINT32_C(1053587199),
    UINT32_C(1046318508),UINT32_C(1039046629),UINT32_C(1031771586),
    UINT32_C(1024493399),UINT32_C(1017212091),UINT32_C(1009927685),
    UINT32_C(1002640203),UINT32_C(995349666),UINT32_C(988056097),
    UINT32_C(980759519),UINT32_C(973459953),UINT32_C(966157421),
    UINT32_C(958851947),UINT32_C(951543551),UINT32_C(944232257),
    UINT32_C(936918087),UINT32_C(929601063),UINT32_C(922281207),
    UINT32_C(914958542),UINT32_C(907633089),UINT32_C(900304872),
    UINT32_C(892973912),UINT32_C(885640232),UINT32_C(878303854),
    UINT32_C(870964801),UINT32_C(863623095),UINT32_C(856278758),
    UINT32_C(848931812),UINT32_C(841582281),UINT32_C(834230186),
    UINT32_C(826875549),UINT32_C(819518394),UINT32_C(812158743),
    UINT32_C(804796618),UINT32_C(797432041),UINT32_C(790065034),
    UINT32_C(782695622),UINT32_C(775323825),UINT32_C(767949666),
    UINT32_C(760573168),UINT32_C(753194353),UINT32_C(745813244),
    UINT32_C(738429862),UINT32_C(731044232),UINT32_C(723656374),
    UINT32_C(716266313),UINT32_C(708874069),UINT32_C(701479666),
    UINT32_C(694083126),UINT32_C(686684472),UINT32_C(679283726),
    UINT32_C(671880911),UINT32_C(664476049),UINT32_C(657069163),
    UINT32_C(649660276),UINT32_C(642249409),UINT32_C(634836586),
    UINT32_C(627421830),UINT32_C(620005162),UINT32_C(612586605),
    UINT32_C(605166183),UINT32_C(597743917),UINT32_C(590319830),
    UINT32_C(582893945),UINT32_C(575466284),UINT32_C(568036870),
    UINT32_C(560605726),UINT32_C(553172875),UINT32_C(545738338),
    UINT32_C(538302139),UINT32_C(530864300),UINT32_C(523424844),
    UINT32_C(515983793),UINT32_C(508541171),UINT32_C(501097000),
    UINT32_C(493651302),UINT32_C(486204100),UINT32_C(478755418),
    UINT32_C(471305277),UINT32_C(463853700),UINT32_C(456400711),
    UINT32_C(448946331),UINT32_C(441490583),UINT32_C(434033491),
    UINT32_C(426575076),UINT32_C(419115363),UINT32_C(411654372),
    UINT32_C(404192127),UINT32_C(396728652),UINT32_C(389263967),
    UINT32_C(381798097),UINT32_C(374331064),UINT32_C(366862891),
    UINT32_C(359393600),UINT32_C(351923214),UINT32_C(344451757),
    UINT32_C(336979250),UINT32_C(329505716),UINT32_C(322031179),
    UINT32_C(314555661),UINT32_C(307079185),UINT32_C(299601773),
    UINT32_C(292123449),UINT32_C(284644234),UINT32_C(277164153),
    UINT32_C(269683227),UINT32_C(262201480),UINT32_C(254718934),
    UINT32_C(247235613),UINT32_C(239751538),UINT32_C(232266733),
    UINT32_C(224781220),UINT32_C(217295023),UINT32_C(209808163),
    UINT32_C(202320665),UINT32_C(194832550),UINT32_C(187343842),
    UINT32_C(179854563),UINT32_C(172364736),UINT32_C(164874384),
    UINT32_C(157383530),UINT32_C(149892196),UINT32_C(142400406),
    UINT32_C(134908182),UINT32_C(127415548),UINT32_C(119922525),
    UINT32_C(112429136),UINT32_C(104935406),UINT32_C(97441355),
    UINT32_C(89947008),UINT32_C(82452387),UINT32_C(74957514),
    UINT32_C(67462414),UINT32_C(59967107),UINT32_C(52471619),
    UINT32_C(44975970),UINT32_C(37480184),UINT32_C(29984284),
    UINT32_C(22488293),UINT32_C(14992233),UINT32_C(7496128),
    0 };  // cosr values for 10th degrees from 0 to 90
  lat/= 1000000;
    // transform unit 100 nano degree into unit 10th degree
  if(lat<0) lat= -lat;  // make it positive
  if(lat>900) lat= 900; // set maximum of 90 degree
  return ((uint64_t)cosrtab[lat]*(int64_t)londiff)/INT64_C(0x100000000);
  }  // lonadapt()
// the table in the previous procedure has been generated by this
// program:
#if 0  // file cosr.c, run it with: gcc cosr.c -lm -o cosr && ./cosr
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
int main() {
  int i;
  printf("  static const uint32_t cosrtab[901]= "
    "{\n    UINT32_C(4294967295),");
  for(i= 1;i<900;i++) {
    if(i%3==0) printf("\n    ");
    printf("UINT32_C(%"PRIu32"),",(uint32_t)(
      cos(i/1800.0*3.14159265359) * INT64_C(0x100000000) ));
    }
  printf("\n    0");
  printf(" };  // cosr values for 10th degrees from 0 to 90\n");
  return 0; }
#endif

static int32_t geodistance(int32_t x1,int32_t y1,
    int32_t x2,int32_t y2) {
  // approximates the geodistance between two points;
  // x1,y1: geocoordinates of first point;
  // x2,y2: geocoordinates of second point;
  // return: distance as angle;
  // all units in 100 nanodegrees;
  // how this is done:
  // distances in West-East direction and in South-North direction
  // are compared; the longer shorter distance is divided by 3 and
  // added to the value of the longer distance;
  // => all points on the edges of an octagon around point 1
  //    are interpreted as equidistant;
  // this approximation is close enough for this application;
  int32_t xdist,ydist;

  xdist= x2-x1; if(xdist<0) xdist= -xdist;
  ydist= y2-y1; if(ydist<0) ydist= -ydist;
  xdist= lonadapt(xdist,y1);
  if(xdist<ydist)
    return xdist/4+ydist;
  return ydist/4+xdist;
  }  // geodistance



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

static inline void read_lockjump() {
  // prevent a previously stored jump position from being overwritten;
  read__jumplock= true;
  }  // end   read_lockjump()

//------------------------------------------------------------
// end Module read_   OSM file read module
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
// Module wrs_   write stream module
//------------------------------------------------------------

// this module provides procedures to write into an output
// stream;
// as usual, all identifiers of a module have the same prefix,
// in this case 'wrs'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

#define wrs__BUFSIZE 1600000
static char* wrs__buf= NULL;  // write buffer
static char* wrs__bufe= NULL;  // water mark for buffer filled 100%
static char* wrs__bufp= NULL;  // write pointer in buffer
static int wrs__fd= -1;  // file descriptor
static const char* wrs__name;  // file name

static void wrs__end() {
  // terminate the services of this module;
  // flush
  if(wrs__bufp>wrs__buf) {  // at least one character in buffer
    if(write(wrs__fd,wrs__buf,wrs__bufp-wrs__buf)<0)
      PERRv("flush error at file: %.80s",wrs__name)
    wrs__bufp= wrs__buf;  // reset buffer pointer;
    }  // at least one character in buffer
  // close
  if(wrs__fd!=1) {
    close(wrs__fd);
    wrs__fd= 1;
    }
  // free buffer
  FREE(wrs__buf)
  }  // wrs__end()

//------------------------------------------------------------

static bool wrs_open(const char* name) {
  // open an output stream;
  // name[]: file name of this stream;
  // return: success;
  // there is no need to explicitly close the output stream,
  // this is done automatically at program end;
  atexit(wrs__end);

  if(name==NULL) {  // standard output
    wrs__name= "standard output";
    wrs__fd= 1;
    }  // standard output
  else {  // not standard output
    wrs__name= name;
    wrs__buf= (char*)malloc(wrs__BUFSIZE);
    if(wrs__buf==NULL) {
      PERR("not enough memory for output buffer.")
return false;
      }
    wrs__bufe= wrs__buf+wrs__BUFSIZE;
    wrs__bufp= wrs__buf;
    wrs__fd= open(wrs__name,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,00600);
    if(wrs__fd<=0) {
      PERRv("could not open output file: %.80s",wrs__name)
      wrs__fd= -1;
return false;
      }
    }  // not standard output
  return true;
  }  // wrs_open()

static inline void wrs_char(int c) {
  // write one byte to output stream, use a buffer;
  // c: character to write;
  if(wrs__bufp>=wrs__bufe) {  // write buffer is full
    // flush
    if(write(wrs__fd,wrs__buf,wrs__bufp-wrs__buf)<0)
      PERRv("write error at file: %.80s",wrs__name)
    wrs__bufp= wrs__buf;  // reset buffer pointer;
    }  // write buffer is full
  *wrs__bufp++= (char)c;
  }  // wrs_char();

static inline void wrs_mem(const void* bp,int64_t l) {
  // write a memory area to output stream, use a buffer;
  // bp: start of the memory area to write;
  // l: length of the memory area (in bytes);
  while(--l>=0) {  // for all bytes of the memory area
    if(wrs__bufp>=wrs__bufe) {  // write buffer is full
      // flush
      if(write(wrs__fd,wrs__buf,wrs__bufp-wrs__buf)<0)
        PERRv("write error at file: %.80s",wrs__name)
      wrs__bufp= wrs__buf;  // reset buffer pointer;
      }  // write buffer is full
    *wrs__bufp++= *(char*)bp;
    (char*)bp++;
    }  // for all bytes of the memory area
  }  // wrs_mem();

static inline void wrs_str(const char* s) {
  // write a string to output stream, use a buffer;
  // s: zero-terminated string to write;
  // the terminating zero is NOT written;
  for(;;) {  // for all bytes of the string
    if(*s==0)
  break;
    if(wrs__bufp>=wrs__bufe) {  // write buffer is full
      // flush
      if(write(wrs__fd,wrs__buf,wrs__bufp-wrs__buf)<0)
        PERRv("write error at file: %.80s",wrs__name)
      wrs__bufp= wrs__buf;  // reset buffer pointer;
      }  // write buffer is full
    *wrs__bufp++= (char)(*s);
    s++;
    }  // for all bytes of the string
  }  // wrs_str();

static inline void wrs_strz(const char* s,int n) {
  // write a string to output stream, use a buffer;
  // the terminating zero is also written;
  // s[]: zero-terminated string to write;
  // n: maximum number of valid bytes of the string;
  //    example: wrs_strz(abc,40) will never read more than
  //    40 bytes of the string, however a terminating zero is
  //    added if the string length is exactly 40 bytes;
  for(;;) {  // for all bytes of the string
    if(wrs__bufp>=wrs__bufe) {  // write buffer is full
      // flush
      if(write(wrs__fd,wrs__buf,wrs__bufp-wrs__buf)<0)
        PERRv("write error at file: %.80s",wrs__name)
      wrs__bufp= wrs__buf;  // reset buffer pointer;
      }  // write buffer is full
    if(n==0) {
      *wrs__bufp++= 0;
  break;
      }
    *wrs__bufp++= (char)(*s);
    if(*s==0)
  break;
    s++; n--;
    }  // for all bytes of the string
  }  // wrs_strz();

static inline void wrs_uint32s(uint32_t v) {
  // write an unsigned 32 bit integer number as string
  // to output stream;
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
  wrs_str(s);
  }  // wrs_uint32s()

static inline void wrs_uint64s(uint64_t v) {
  // write an unsigned 64 bit integer number as string
  // to output stream;
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
  wrs_str(s);
  }  // wrs_uint64s()

static inline void wrs_int64s(int64_t v) {
  // write a signed 64 bit integer number as string
  // to output stream;
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
  wrs_str(s);
  }  // end wrs_int64s()

static inline void wrs_uint32(uint32_t v) {
  // write an undsigned 32 bit integer number as it is
  // to output stream; just a wrapper for wrs_mem();
  wrs_mem(&v,sizeof(v));
  }  // end wrs_uint32()

static inline void wrs_int64(int64_t v) {
  // write a signed 64 bit integer number as it is
  // to output stream; just a wrapper for wrs_mem();
  wrs_mem(&v,sizeof(v));
  }  // end wrs_int64()

//------------------------------------------------------------
// end   Module wrs_   write stream module
//------------------------------------------------------------



// "I" represents the name for the instances:
//     region, subregion, city, street, housenumber;
#define I region
#include __FILE__
#undef I
#define I subregion
#include __FILE__
#undef I
#define I city
#include __FILE__
#undef I
#define I street
#include __FILE__
#undef I
#define I housenumber
#include __FILE__
#undef I



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
    // this is used to suppress warning messages in oo__close()
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
  byte* bufe;  // pointer in read buffer, end of object
  uint8_t b;  // latest byte which has been read
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
          else
            PLOGv(1,"unknown .o5m short dataset id: 0x%02x",b)
          read_bufp++;
  continue;
          }  // end   single byte dataset
        else {  // unknown multibyte dataset
          if(b!=0xe0 && b!=0xdb && b!=0xdc)
            PLOGv(1,"unknown .o5m dataset id: 0x%02x",b)
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
          const char* region,*subregion,*city,*street,*place,
            *housenumber,*boundary;
          int8_t weight;

          region= subregion= city= street= place=
            housenumber= boundary= NULL;
          weight= 0;
          while(bufp<bufe) {  // each k-v pair
            str_read(&bufp,&k,&v);  // read key-val pair
            if(strcmp(k,"region")==0) region= v;
            else if(strcmp(k,"subregion")==0) subregion= v;
            else if(strcmp(k,"addr:city")==0) city= v;
            else if(strcmp(k,"addr:street")==0) street= v;
            else if(strcmp(k,"addr:place")==0) place= v;
            else if(strcmp(k,"addr:housenumber")==0) housenumber= v;
            else if(strcmp(k,"boundary")==0) boundary= v;
            else if(strcmp(k,"bBoxWidthWeight")==0)
              weight= strtoint32(v);
            }  // each k-v pair
          if(nodecount==0) {
            // write dummy entry;
            // needed later as identifier for 'invalid address'
            data_addradd(COORDINATE_NIL,COORDINATE_NIL,NULL,NULL,
              NULL,NULL,NULL,0);
            nodecount++;
            }
          if(boundary!=NULL && strcmp(boundary,"administrative")==0)
            weight+= 64;  // higher weight for region polygons
          if(place!=NULL && street==NULL)
              // there is a place tag, but no street tag
            street= place;  // take place name as street name
          if((city==NULL && housenumber==NULL) || street!=NULL) {
              // node contains valid address data
              // (do not accept city or housenumber tags
              // without street tag)
            if(housenumber==NULL)
                // there is no housenumber
              data_addradd(lon,lat,region,subregion,
                city,street,housenumber,weight);
            else if(strchr(housenumber,';')==NULL)
                // there is no semicolon in housenumber
              data_addradd(lon,lat,region,subregion,
                city,street,housenumber,weight);
            else {  // there is at least one semicolon in housenumber
              char* v0,*vp;
              char vc[256];
              bool subsequent;

              subsequent= false;
              strMcpy(vc,housenumber);
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
                  data_addradd(lon,lat,region,subregion,
                    city,street,v0,weight);
                  }
                if(vp==NULL)  // no further substrings
              break;
                v0= vp;  // continue with next substring
                }  // for all substrings
              }  // there is at least one semicolon in housenumber
            }  // node contains valid address data
          nodecount++;
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

  // end of processing
  if(nodecount>global_nodecount) {
    PWARN("wrong nodecount in \"strings_nodecount\".")
    PINFOv("  in file \"strings_nodecount\": %"PRIi64,global_nodecount)
    PINFOv("  in .o5m file: %"PRIi64,nodecount)
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
    if((l= strzlcmp(a,"--max-strrefs="))>0 && a[l]!=0) {
        // define maximum memory space for string references
      global_maxstrrefs= strtoint32percent(a+l);
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

  // initialization
  global_nodecount= data_ini("strings_nodecount");
  if(global_nodecount!=0) {
    data_ini_region("strings_region",1);
    data_ini_subregion("strings_subregion",2);
    data_ini_city("strings_city",3);
    data_ini_street("strings_street",4);
    data_ini_housenumber("strings_housenumber",5);
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

  // complete data and write output file
  if(!data_addrcomplete())
return 2;
  if(!data_write(outputfilename[0]!=0? outputfilename: NULL))
return 3;

  if(loglevel>0) {  // verbose mode
    if(oo_sequenceid!=INT64_C(-0x7fffffffffffffff))
      PINFOv("Last processed: %s %"PRIu64".",
        ONAME(oo_sequencetype),oo_sequenceid)
    if(r!=0)
      PINFOv("Exit: %i\n",r)
    }  // verbose mode
  return r;
  }  // end   main()



//------------------------------------------------------------
// Module data_   osm data module
//------------------------------------------------------------

static int data__addr_qsort(const void* a,const void* b) {
  // address data comparison for qsort();
  // sort ascending by city string as first criteria,
  // and descending by weight as second if weight>=64,
  // by street and subsequent by housenumber and weight otherwise;
  const char* aval,*bval;
  int_least32_t aw,bw;
  int_least32_t cv;

  aval= data__str_city[((data__addr_t*)a)->city].string;
  bval= data__str_city[((data__addr_t*)b)->city].string;
  cv= strncmp(aval,bval,data__strstringM);
  if(cv!=0)
return cv;
  aw= ((data__addr_t*)a)->weight;
  bw= ((data__addr_t*)b)->weight;
  if(aw<64) aw= 0;
  if(bw<64) bw= 0;
  cv= bw-aw;
  if(cv!=0)
return cv;
  aval= data__str_street[((data__addr_t*)a)->street].string;
  bval= data__str_street[((data__addr_t*)b)->street].string;
  cv= strncmp(aval,bval,data__strstringM);
  if(cv!=0)
return cv;
  aw= ((data__addr_t*)a)->housenumeric;
  bw= ((data__addr_t*)b)->housenumeric;
  cv= aw-bw;
  if(cv!=0)
return cv;
  aval= data__str_housenumber[((data__addr_t*)a)->housenumber].string;
  bval= data__str_housenumber[((data__addr_t*)b)->housenumber].string;
  cv= strncmp(aval,bval,data__strstringM);
  if(cv!=0)
return cv;
  aw= ((data__addr_t*)a)->weight;
  bw= ((data__addr_t*)b)->weight;
return bw-aw;
  }  // data__addr_qsort()

#else  // from here: procedure templates
#define TEMPLATEM(f,a) f##_##a
#define T(f,a) TEMPLATEM(f,a)
#undef TEMPLATEONCE
#ifndef TEMPLATEFIRSTRUN
#define TEMPLATEFIRSTRUN
#define TEMPLATEONCE

// container for externally stored used-by references
#define data__strrefusedM 64
struct data__strref_struct {
  uint32_t used[data__strrefusedM];
    // indexes for used-references;
  struct data__strref_struct* next;
  };
typedef struct data__strref_struct data__strref_t;

// container for strings and their used-by references
#define data__strstringM 40
#define data__strusedM 5
struct data__str_struct {
  char string[data__strstringM];  // string (including terminator);
    // if maximum string length, the terminator is omitted;
  uint32_t usedn;  // number of backreferences for this string
  union {
    uint32_t used[data__strusedM];
      // backreferences for this string;
      // these backreferences are stored as 32-bit indexes;
      // unused backreferences need not to be zeroed:
      // array elements are valid in dependence of 'usedn';
      // if there are more than 'data__strusedM' backreferences,
      // they are entered as external references (see below);
    struct { uint32_t filler; data__strref_t* usedref,*usedlast; };
      // if there are more than 'data__strusedM' backreferences,
      // these two pointers refer to elements of data__strref;
      // .usedref: first element in chain;
      // .usedlast: last element of the chain;
      // (this substructure should not exceed 20 Bytes in length
      // to keep the container length at 64 Bytes;
    };
  } __attribute__((__packed__));
typedef struct data__str_struct data__str_t;

// memory area for externally stored used-by references
static data__strref_t* data__strref= NULL;  // string references
static data__strref_t* data__strrefe= NULL;  // logical end
static data__strref_t* data__strrefee= NULL;  // physical end
static uint_fast32_t data__strrefn= NULL;  // number of refs

// container for address data
struct data__addr_struct {  // (length 32 Bytes)
  int32_t x,y;  // geocoordinates (unit 10^-7 degree)
  uint32_t region;  // index of region string
  uint32_t subregion;  // index of subregion string
  uint32_t city;  // index of city string
  uint32_t street;  // index of street string
  uint32_t housenumber;  // index of housenumber string
  uint16_t housenumeric;  // numeric representation of housenumber
  int8_t weight;  // geographical extend:
    // binary logarithm of bounding-box width (meters)
    // as provided by 'osmconvert --add-bboxwidthweight';
  uint8_t work;  // temporary variable, for internal use;
    // 0: default; 1: street without city; 2: street was without city;
  } __attribute__((__packed__));
typedef struct data__addr_struct data__addr_t;
static data__addr_t* data__addr= NULL;  // address memory area
static data__addr_t* data__addre= NULL;  // logical end
static data__addr_t* data__addree= NULL;  // physical end
static uint_fast32_t data__addrn= 0;
  // logical number of address datasets
static uint_fast32_t data__nodecount= 0;
  // physical number of node objects (as read from file)

// indexes for geocoordinates
struct data__coco_struct {
  uint64_t yx;  // rounded geocoordinate combination
  uint32_t addrindex;  // index of address element
  uint32_t filler;
  } __attribute__((__packed__));
typedef struct data__coco_struct data__coco_t;
static data__coco_t* data__coco;  // rounded geocoordinate index;
  // search criterion is a geocoordinates combination:
  // ((lat+900000000)&0xffffc000)*2^16 + (lon+1800000000);
  // 16384 * 100 nanodegrees represents a length of about 182 meters;
  // this is done to accelerate the determination of neighboring
  // points within a certain geographical distance;
static data__coco_t* data__cocoe= NULL;  // logical end

// procedure prototypes
static inline uint32_t data_index_region(const char*);
static inline uint32_t data_index_subregion(const char*);
static inline uint32_t data_index_city(const char*);
static inline uint32_t data_index_street(const char*);
static inline uint32_t data_index_housenumber(const char*);
static void data__addused_region(uint32_t,uint32_t);
static void data__addused_subregion(uint32_t,uint32_t);
static void data__addused_city(uint32_t,uint32_t);
static void data__addused_street(uint32_t,uint32_t);
static void data__addused_housenumber(uint32_t,uint32_t);
static bool data__strclean_region();
static bool data__strclean_subregion();
static bool data__strclean_city();
static bool data__strclean_street();
static bool data__strclean_housenumber();
static bool data__write_region();
static bool data__write_subregion();
static bool data__write_city();
static bool data__write_street();
static bool data__write_housenumber();
static int data__addr_qsort(const void*,const void*);
static inline const char* data_string_region(uint32_t);
static inline const char* data_string_subregion(uint32_t);
static inline const char* data_string_city(uint32_t);
static inline const char* data_string_street(uint32_t);
static inline const char* data_string_housenumber(uint32_t);

static int data__strref_qsort(const void* a,const void* b) {
  // strref comparison for qsort()
  uint32_t aval,bval;

  aval= *(uint32_t*)a;
  bval= *(uint32_t*)b;
  if(aval>bval)
return 1;
  if(aval==bval)
return 0;
  return -1;
  }  // data__strref_qsort()

static int data__coco_qsort(const void* a,const void* b) {
  // coordinate comparison for qsort()
  uint64_t aval,bval;

  aval= ((data__coco_t*)a)->yx;
  bval= ((data__coco_t*)b)->yx;
  if(aval>bval)
return 1;
  if(aval==bval)
return 0;
  return -1;
  }  // data__coco_qsort()

#define data__coco_PORTION 16384
  // latitude grid in degrees; 16384 is as much as 182 Meters;

static inline uint64_t data__coco_calc(int32_t x,int32_t y) {
  // calculate the geocoordinate combination;
  // y coordinate will be rounded and left-shifted and then
  // summed with y coordinate;
  uint64_t ii;

  ii= UINT64_C(900000000);
  ii+= y;
  ii&= UINT64_MAX-data__coco_PORTION+1;  // (0xffffffffffffc000)
  ii<<= 32;
  ii+= UINT64_C(1800000000);
  ii+= x;
  return ii;
  }  // data__coco_calc()

static inline data__coco_t* data__coco_find(uint64_t cc) {
  // searches in data__coco and finds the first occurrence
  // which is not smaller than the given compare value;
  // cc: geocoordinate combination to search for;
  // return: position in data__coco[] which matches (or is higher);
  //         if there is no such position, then the highest
  //         position in data__coco[] is returned;
  int_fast32_t i,i1,i2;  // iteration indexes:
    // middle, lower including, upper excluding;

  i1= 0; i2= data__addrn;
  while(i1<i2) {  // binary search
    i= (i1+i2)/2;
    if(cc<data__coco[i].yx) i2= i;
    else i1= i+1;
    }  // binary search
  while(i>0 && cc==data__coco[i-1].yx) i--;
    // find first of multiple occurrences
  if(i>=data__addrn) i= data__addrn-1;
    // check range (the value searched for could be higher than
    // the highest value in data__coco[])
  return data__coco+i;
  }  // data__coco_find()

// geocoordinate combination;
// for data__coco_center() and data__coco_next();
static int32_t data__coco_x,data__coco_y;  // current center
static int32_t data__coco_radius;  // max. allowed distance
static uint64_t data__coco_s,data__coco_n; // South and North edge;
static uint32_t data__coco_w,data__coco_e; // West and East edge;
  // normalized by +180 degree;
static data__coco_t* data__coco_here= NULL;
  // current position in data__coco[]

static inline void data__coco_center(int32_t x,int32_t y,
    int32_t radius) {
  // set center point for subsequent neighbor calculation;
  // x,y: geocoordinates of the center;
  // radius: radius around x,y; unit: 100 latitude nanodegrees;
  // this procedure sets the borders of a square all searched points
  // must be located in;
  // this square is snapped into the latitude grid laid down
  // by data__coco[];
  // the actual neighbor calculation is done by data__coco_next();
  int32_t lonradius;  // radius in longitude degrees

  // set West and East borders
  data__coco_x= x; data__coco_y= y;
  lonradius= INT32_C(0x10000)/lonadapt(INT32_C(0x10000),y)*radius;
    // using lonadapt() in reverse direction
  data__coco_w= UINT32_C(1800000000)+x-lonradius;
  data__coco_e= UINT64_C(1800000000)+x+lonradius;

  // set South and North borders
  data__coco_s= UINT64_C(900000000)+y-radius;
  data__coco_s&= UINT64_MAX-data__coco_PORTION+1;
  data__coco_s<<= 32;
  data__coco_n= UINT64_C(900000000)+y+radius+(data__coco_PORTION-1);
  data__coco_n&= UINT64_MAX-data__coco_PORTION+1;
  data__coco_n<<= 32;

  // set radius
  data__coco_radius= radius;

  // initialize control variable
  data__coco_here= NULL;  // to tell data__coco_next() that the
    // first address object within the radius is still to search for;
  }  // data__coco_center()

static inline data__addr_t* data__coco_next(int32_t* distancep) {
  // get (next) address dataset within radius;
  // return: address dataset within range;
  //         NULL: no more datasets within range;
  // *distancep: distance to the dataset returned;
  uint64_t cc;  // geocoordinate combination;
  data__addr_t* addrp;
  int32_t dist;  // distance

  do {  // until have found an address within the radius
    if(data__coco_here==NULL)  // first run
      data__coco_here= data__coco_find(data__coco_s+data__coco_w);
    else {  // not first run
      data__coco_here++;
      if(data__coco_here>=data__cocoe) {
        *distancep= 0;
return NULL;
        }
      cc= data__coco_here->yx;
      if((cc&0xffffffff)>data__coco_e) {  // beyond Eastern edge
        cc&= 0xffffffff00000000;  // only latitude
        cc+= ((uint64_t)data__coco_PORTION)<<32;
        if(cc>data__coco_n) {  // no more portions within the square
          *distancep= 0;
return NULL;
          }
        data__coco_here= data__coco_find(cc+data__coco_w);
        }  // beyond Eastern edge
      }  // not first run
    addrp= data__addr+data__coco_here->addrindex;
    dist= geodistance(addrp->x,addrp->y,data__coco_x,data__coco_y);
    } while(dist>data__coco_radius);
        // until have found an address within the radius
  *distancep= dist;
  return addrp;
  }  // data__coco_next()

static data__strref_t* data__strrefget() {
  // provide a new string reference container;
  // if there is no element left in string reference memory,
  // a dummy element will be provided;
  // note that the calling procedure must care about all matters
  // of container initialization, including the next-pointer;
  static data__strref_t dummyref= {{0},0};

  if(data__strrefe>=data__strrefee) {  // no free elements left
    PERR("not enough string reference memory size")
    PINFO("try to increase  --max-strrefs=")
return &dummyref;
    }
  data__strrefn++;
  return data__strrefe++;
  }  // data__strrefget()

static void data__end() {
  // terminate the services of this module;
  FREE(data__addr)
  FREE(data__strref)
  FREE(data__coco)
  }  // data__end()

static bool data__addrclean() {
  // remove address dataset duplicates
  // return: success;
  data__addr_t* readp,*writep;
  data__addr_t *readbeforep;
    // dataset read before (for comparisons)

  // examine all address datasets and remove
  // empty ones and duplicates
  readbeforep= data__addr;
  readp= writep= data__addr+1;
  while(readp<data__addre) {  // for each dataset
    if( (readp->region!=readbeforep->region ||
        readp->subregion!=readbeforep->subregion ||
        readp->city!=readbeforep->city ||
        readp->street!=readbeforep->street ||
        readp->housenumber!=readbeforep->housenumber ) &&
        (readp->region!=0 ||
        readp->subregion!=0 ||
        readp->city!=0 ||
        readp->street!=0 ||
        readp->housenumber!=0) ) {
        // this is not an empty dataset and not a duplicate
      if(writep!=readp)
        *writep= *readp;
      writep++;
      }  // this is not an empty dataset and not a duplicate
    readp++; readbeforep++;
    }  // for each dataset
  data__addre= writep;
  data__addrn= data__addre-data__addr;
  return true;
  }  // data__addrclean()

#endif  // TEMPLATEONCE

static int T(data__strtypeid,I)= 0;  // data type id;
  // 1: region; 2: subregion; 3: city; 4: street; 5: housenumber;

// memory area for strings and their used-by references
static data__str_t* T(data__str,I)= NULL;  // string memory area
static data__str_t* T(data__stre,I)= NULL;  // logical end
static uint_fast32_t T(data__strn,I)= NULL;  // number of strings

static void T(data__addused,I)(uint32_t index,uint32_t addrindex) {
  // add used-by information to a string;
  // index: index of the string;
  // addrindex: index of address dataset which uses this string;
  uint32_t usedn;  // number of backreferences for this string
  uint32_t usedremainder;
  data__str_t* strp;  // this string
  data__strref_t* usedp;

  strp= T(data__str,I)+index;
  usedn= strp->usedn;
  if(usedn<data__strusedM) {  // can be stored internally
    strp->used[usedn]= addrindex;
    strp->usedn= usedn+1;
return;
    }
  if(usedn==data__strusedM) {
      // first element to be stored externally
    usedp= data__strrefget();  // get new external container
    usedp->next= NULL;
    memcpy(usedp->used,strp->used,sizeof(uint32_t)*data__strusedM);
    strp->usedref= strp->usedlast= usedp;
    }
  usedremainder= usedn % data__strrefusedM;
  if(usedremainder==0) {  // need a new strref container
    usedp= data__strrefget();  // get new external container
    usedp->next= NULL;
    strp->usedlast->next= usedp;  // link two newest containers
    strp->usedlast= usedp;  // link new container to string
    }
  else
    usedp= strp->usedlast;
  usedp->used[usedremainder]= addrindex;
  strp->usedn= usedn+1;
  }  // data__addused_I()

static bool T(data__strclean,I)() {
  // remove unused strings
  // return: success;
  data__str_t* readp,*writep;
  uint32_t* ttab;  // translation table: old index -> new index
  uint32_t* ttabp;  // pointer in translation table
  uint32_t ttabi;  // index in translation table
  data__addr_t* addrp;  // pointer in address memory space

  if(T(data__strn,I)<=1)  // strings too few for cleaning
return true;

  // allocate memory for translation table
  ttab= (uint32_t*)malloc(sizeof(uint32_t)*T(data__strn,I)+4);
  if(ttab==NULL) {
    PERRv(
      "not enough memory for string index translation type-%i.",
      T(data__strtypeid,I))
return false;
    }

  // examine all strings of this instance and remove unused of them
  ttabp= ttab;
  ttabi= 0;
  *ttabp++= ttabi++;  // (first string is a dummy)
  readp= writep= T(data__str,I)+1;
  while(readp<T(data__stre,I)) {  // for each string
    if(readp->usedn!=0) {
        // this string is used by at least one address dataset
      if(writep!=readp)
        *writep= *readp;
      *ttabp++= ttabi++;
      writep++;
      }
    else
      *ttabp++= 0;
    readp++;
    }  // for each string

  if(writep!=readp) {  // at least one string has been removed
    // adjust length of string table
    T(data__stre,I)= writep;
    T(data__strn,I)= T(data__stre,I)-T(data__str,I);

    // adjust indexes of address datasets, use translation table for this
    for(addrp= data__addr;addrp<data__addre;addrp++)
        // for each address dataset
      addrp->I= ttab[addrp->I];
    }  // at least one string has been removed

  free(ttab);
  return true;
  }  // data__strclean_I()

static bool T(data__write,I)() {
  // write string data to output stream;
  // return: success;
  int64_t strn;  // number of strings
  int64_t strlensum;  // sum of lengths of all strings
                         // (without terminating zeros)
  int64_t strlenmax;  // length of longest string
    // (without terminating zeros); that's usually 40 because of
    // limitation in data__str_t;
  int64_t usednsum;  // total number of all used-references
  int64_t usednmax;  // maximum number of used-references for a
    // single string
  data__str_t* strp;
  int64_t len,usedn;
  uint32_t* usedref;  // temporary storage area for used-refs

  // write statistics information
  strn= T(data__strn,I);
  strlensum= 0;
  strlenmax= 0;
  usednsum= 0;
  usednmax= 0;
  for(strp= T(data__str,I);strp<T(data__stre,I);strp++) {
      // for all strings
    len= strnlen(strp->string,data__strstringM);
    strlensum+= len;
    if(len>strlenmax) strlenmax= len;
    usedn= strp->usedn;
    usednsum+= usedn;
    if(usedn>usednmax) usednmax= usedn;
    }  // for all strings
  #define DD(s,v) PLOGv(1,s ": %"PRIi64,T(data__strtypeid,I),v)
  #define D(s,v) PLOGv(1,s ": %"PRIi64,v)
  DD("writing type-%i strings",strn)
  D("  strings total length",strlensum)
  D("  strings max. length",strlenmax)
  D("  strrefs",usednsum)
  D("  strrefs max",usednmax)
  #undef DD
  #undef D
  wrs_char(0x50+T(data__strtypeid,I));  // section type id
  wrs_int64(sizeof(int64_t)*5);  // number following Bytes
  wrs_int64(strn);  // number of strings
  wrs_int64(strlensum);  // sum of lengths of all strings
                         // (without terminating zeros)
  wrs_int64(strlenmax);  // length of longest string
    // (without terminating zeros); that's usually 40 because of
    // limitation in data__str_t;
  wrs_int64(usednsum);  // total number of all used-references
  wrs_int64(usednmax);  // maximum number of used-references for a
    // single string

  // write strings
  wrs_char(0x60+T(data__strtypeid,I));  // section type id
  wrs_int64(strlensum+strn);  // number following Bytes
  for(strp= T(data__str,I);strp<T(data__stre,I);strp++)
      // for all strings
    wrs_strz(strp->string,data__strstringM);

  // allocate memory for sorting used-references
  usedref= (uint32_t*)malloc(sizeof(uint32_t)*usednmax+4);
  if(usedref==NULL) {
    PERRv("not enough memory for sorting type-%i strrefs",
      T(data__strtypeid,I))
return false;
    }

  // write used-references header
  wrs_char(0x70+T(data__strtypeid,I));  // section type id
  wrs_int64(sizeof(int32_t)*(strn+usednsum+strn));
    // number following Bytes

  // write used-references body
  for(strp= T(data__str,I);strp<T(data__stre,I);strp++) {
      // for all strings
    uint32_t* usedrefp;  // pointer in usedref[]
    uint32_t usedn;  // number of used-references
    uint32_t un;  // number of used-references (count down)
    uint32_t* up;  // pointer to one used-reference
    data__strref_t* srp;  // pointer to used-reference structure
    int i;

    // copy used-references to temporary storage area usedref[]
    usedrefp= usedref;
    usedn= un= strp->usedn;
    if(un<=data__strusedM) {  // references are string-dataset internal
      up= strp->used;
      while(un-->0)
        *usedrefp++= *up++;
      }
    else {  // references are string-dataset external
      srp= strp->usedref;
      up= srp->used;
      i= data__strrefusedM;
      while(un-->0) {
        *usedrefp++= *up++;
        if(--i==0) {
          i= data__strrefusedM;
          srp= srp->next;
          up= srp->used;
          }
        }
      }  // references are string-dataset external

    // sort used-references
    if(usedn>0)
      qsort(usedref,usedn,sizeof(uint32_t),data__strref_qsort);

    // write used-references for this string
    wrs_uint32(usedn);
    if(usedn>0)
      wrs_mem(usedref,sizeof(uint32_t)*usedn);
    wrs_uint32(0);  // write references' terminator
    }  // for all strings
  free(usedref);
  return true;
  }  // data__write_I()

static void T(data__end,I)() {
  // terminate the services of this instance;
  FREE(T(data__str,I))
  }  // data__end_I()

//------------------------------------------------------------

#ifdef TEMPLATEONCE

static int64_t data_ini(const char* file) {
  // initialization of this module;
  // must be called before any data_str_ini_I() procedures;
  // file: file with number of node objects to read;
  // return: number of nodes;
  //         0: error;
  // reads the number of nodes and initializes the data structures
  // for address data;
  int fd;  // file descriptor

  atexit(data__end);

  // open the nodecount file
  fd= open(file,O_RDONLY|O_BINARY);
  if(fd<0) {
    PERRv("could not open input file: %.80s",file)
return 0;
    }

  /* read the file and get the number */ {
    char buf[102];  // read buffer
    char* bufe;  // logical end in buffer
    char* bufee;  // physical end in buffer
    size_t rb;  // number of bytes read

    bufe= buf;
    bufee= buf+ sizeof(buf)-2;
    buf[0]= 0;
    while(bufe<bufee) {  // buffer not full
      rb= read(fd,bufe,bufee-bufe);
      if(rb<=0)
    break;
      bufe+= rb;
      bufe[0]= 0;  // set terminating zero
      }  // buffer not full
    data__nodecount= strtosint64(buf);
    }  // read the file and get the number

  // close the nodecount file
  close(fd);

  // check plausibility
  if(data__nodecount<=0) {
    PERRv("incorrect node count value in file \"%.80s\": %"
      PRIiFAST32,file,data__nodecount)
return 0;
    }

  // allocate memory space for address data
  data__nodecount++;
    // one dummy entry is needed for 'invalid address'
  data__addr= (data__addr_t*)
    malloc(sizeof(data__addr_t)*data__nodecount+4);
  if(data__addr==NULL) {
    PERR("not enough memory for address data")
return 0;
    }
  data__addre= data__addr;  // logical end
  data__addrn= 0;  // logical number of address datasets
  data__addree= data__addr+data__nodecount;  // physical end

  /* care about string reference memory area */ {
    int64_t sizeref;

    // determine size
    sizeref= data__nodecount*sizeof(data__strref_t);
      // calculate automatically defined size
    PLOGv(1,"string reference size: %"PRIi64,sizeref)
    if(global_maxstrrefs>0)  // manually-defined absolute size
      sizeref= global_maxstrrefs*(INT64_C(1024)*1024);
    else if(global_maxstrrefs<0)  // manually-defined relative size
      sizeref*= global_maxstrrefs*-INT64_C(100);
    if(sizeref<128) sizeref= 128;
    sizeref-= (sizeref+sizeof(data__strref_t)-1)%
      sizeof(data__strref_t);
    if(global_maxstrrefs!=0)
      PLOGv(1,"changed by  --max-strrefs=  to: %"PRIi64,sizeref)

    // allocate memory
    data__strref= (data__strref_t*)malloc(sizeref+4);
    if(data__strref==NULL) {
      PERRv("not enough memory for string references: %"
        PRIi64" MiB",sizeref/(1024*1024))
      PINFO("try to decrease  --max-strrefs=")
return 0;
      }
    data__strrefe= data__strref;  // logical end
    data__strrefee= (data__strref_t*)((int8_t*)data__strref+sizeref);
      // physical end
    data__strrefn= 0;  // number of elements in data__strref[]
    }  // care about string reference memory area

  // allocate memory for geocoordinate combinations
  data__coco= (data__coco_t*)
    malloc(sizeof(data__coco_t)*data__nodecount+4);
  if(data__coco==NULL) {
    PERR("not enough memory for geocoordinate combinations")
return 0;
    }

  return data__nodecount;
  }  // data_ini()

static void data_addradd(int32_t x,int32_t y,
    const char* region,const char* subregion,
    const char* city,const char* street,
    const char* housenumber,int8_t weight) {
  // enter a new dataset for an address;
  // x,y: geocoordinates (unit 10^-7 degree);
  // region,subregion,city,street,housenumber:
  //   parts of this address as strings;
  // weight: geographical extent:
  //         binary logarithm of bounding-box width in meters;
  uint32_t si;  // string index
  uint32_t hnr;  // house number (numeric)

  if(data__addre>=data__addree) {  // no free elements left
    PERR("not enough address memory size")
    PINFO("maybe file \"strings_nodecount\" is faulty.")
return;
    }
  data__addre->x= x;
  data__addre->y= y;
  #define D(z) si= T(data_index,z)(z); \
    if(si==0 && z!=NULL) PERRv("unknown " #z " string: %.80s",z) \
    data__addre->z= si;
  D(region)
  D(subregion)
  D(city)
  D(street)
  D(housenumber)
  #undef D
  if(housenumber==NULL)
    hnr= 0;
  else
    hnr= strtouint32(housenumber);
  if(hnr>65535) hnr= 65535;  // limit range to fit into 16 bit
  data__addre->housenumeric= hnr;
  data__addre->weight= weight;
  if(data__addre->street!=0 && data__addre->housenumber==0 &&
      data__addre->city==0)
      // street data set is without city information
    data__addre->work= 1;
  else
    data__addre->work= 0;
  data__addre++; data__addrn++;
  }  // data_addradd()

static bool data_addrcomplete() {
  // complete address data and sort them;
  // return: success;
  bool r;

  r= true;

  // log some information
  PLOGv(1,"string reference memory used: %"PRIi64,
    (int64_t)(data__strrefe-data__strref)*sizeof(data__strref))

  /* initialize rounded geocoordinate combination index */ {
    uint_fast32_t count;
    data__addr_t* addrp;

    for(count= 0,addrp= data__addr,data__cocoe= data__coco;
        count<data__addrn;count++,addrp++,data__cocoe++) {
        // for each address dataset
      data__cocoe->yx= data__coco_calc(addrp->x,addrp->y);
      data__cocoe->addrindex= count;
      }  // for each address dataset
    // sort index by geocoordinate combination
    qsort(data__coco,data__addrn,sizeof(data__coco_t),
      data__coco_qsort);
    }  // initialize rounded geocoordinate combination index

  /* add city to addresses and streets */ {
    // (streets will be recognized as address data which have
    //  street name but no housenumber);
    uint_fast32_t count;
    data__addr_t* addrp,*nearest_ap,*ap;
    uint32_t city,street;
    int32_t x,y;  // geocoordinates of currently examined street
    int32_t nearest_d,d;  // distance in degrees

    for(count= 0,addrp= data__addr;count<data__addrn;
        count++,addrp++) {  // for each address dataset
      city= addrp->city;
      street= addrp->street;

      if(addrp->city==0) {  // city is unknown
        if(street==0) {  // street is unknown
          if(addrp->housenumber==0 && addrp->region!=0) {
              // house number is unknown, but region is known
              // -> must be a region dataset then
            // copy region's name to city string
            const char* rs,*cs;  // region string, city string
            uint32_t ci;  // region index, city index

            rs= data_string_subregion(addrp->subregion);
            if(*rs==0)
              rs= data_string_region(addrp->region);
            ci= data_index_city(rs);
            if(ci==0)
              PWARNv("missing city string for region %.80s",rs)
            else {
              // take region string for city string;
              addrp->city= ci;
              PLOGv(2,"%.80s -> %.80s\n",cs,rs)
              }
            }  // house number is unknown
    continue;
          }  // street is unknown
        if(addrp->housenumber!=0) {
            // house address, not just a street
          uint32_t ci;  // city string index

          ci= data_index_city(data_string_region(addrp->region));
          if(ci==0)
            PWARNv("could not find city string for region %.80s",
              data_string_region(addrp->region))
          else
            addrp->city= ci;
    continue;
          }  // house address, not just a street

        // here: must be a street AND city is unknown
        x= addrp->x; y= addrp->y;
        nearest_ap= NULL; nearest_d= INT32_MAX;
        data__coco_center(x,y,INT32_C(67500));
          // radius 750 m around x,y
        for(;;) {  // for all address objects within radius
          ap= data__coco_next(&d);  // get next point within radius
          if(ap==NULL)  // no other point in range
        break;
          if(ap->street!= addrp->street || ap->city==0 || ap->work!=0)
              // different street name OR no city information OR
              // that address dataset represents a street too
        continue;
          if(d<nearest_d) {        
            nearest_d= d;
            nearest_ap= ap;
            }
          }  // for all address objects within radius
        if(nearest_ap!=NULL) {
            // have found a nearby object with city information
          city= addrp->city= nearest_ap->city;
          addrp->work= 2;
          }  // have found a nearby object with city information
        else {  // no nearby object with city information
          city= addrp->city=
            data_index_city(data_string_region(addrp->region));
            // take region string as city string
          }
        }  // city is unknown

      if(city!=0) {  // city is known
        // we cannot be sure the city's name is correct
        // hence we need to adjust it to the region's name
        const char* rs,*cs;  // region string, city string
        uint32_t ri,ci;  // region index, city index
        int l,c;

        rs= data_string_region(addrp->region);
        cs= data_string_city(city);
        l= strzlcmp(rs,cs);
        c= rs[l];
        if(l==0 || c!=0) {  // not an exact match
          if(l>=3 && c!=0 && (c==' ' || c=='-' || c=='/' || c=='(')) {
              // region string is longer than city string
              // and fully contains the city string AND
              // city string is at least 3 bytes long
              // -> we assume the city string is incomplete,
              // for example "Frankfurt" instead of "Frankfurt am Main";
            ci= data_index_city(rs);
            if(ci==0)
              PWARNv("missing city string for region %.80s",
                data_string_region(addrp->region))
            else {
              // take region string for city string;
              addrp->city= ci;
              PLOGv(2,"%.80s -> %.80s\n",cs,rs)
              }
            }  // region string is longer than city string ...
          else {  // city string does not partially match region string
            ri= data_index_region(cs);
            if(ri==0) {  // city string does not match any region string
              ci= data_index_city(rs);
              if(ci==0)
                PWARNv("missing city string for region %.80s",
                  data_string_region(addrp->region))
              else {
                // take region string for city string;
                addrp->city= ci;
                PLOGv(2,"%.80s -> %.80s\n",cs,rs)
                }
              }  // city string does not match any region string
            else {  // city string matches another region string
              rs= data_string_region(ri);
              ci= data_index_city(rs);
              if(ci==0)
                PWARNv("missing city string for region %.80s",
                  data_string_region(addrp->region))
              else {
                // take the other region string for city string;
                addrp->city= ci;
                PLOGv(2,"%.80s -> %.80s\n",cs,rs)
                }
              }  // city string matches another region string
            }  // city string does not partially match region string
          }  // not an exact match
        }  // city is known

      }  // for each address dataset
    }  // add city to addresses and streets

  /* add subregion information */ {
    data__addr_t* addrp;
    uint32_t si,ci;  // index of subregion and city
    const char* ss,*cs;  // string of subregion and city
    int l;

    for(addrp= data__addr+1;addrp<data__addre;addrp++) {
      // for each address dataset
      si= addrp->subregion;
      if(si!=0) {  // there is a subregion
        ci= addrp->city;
        if(ci!=0) {  // there is a city
          ss= data_string_subregion(si);
          cs= data_string_city(ci);
          l= strzlcmp(ss,cs);
          if(l>=3) {  // contents of subregion string starts
              // with city string AND
              // city string is at least 3 bytes long
            ci= data_index_city(ss);
            if(ci==0)
              PWARNv("missing city string for subregion %.80s",ss)
            else {
              // take subregion string as city string;
              addrp->city= ci;
              PLOGv(2,"%.80s -> sub %.80s\n",cs,ss)
              }
            }  // contents of subregion string starts ...
          }  // there is a city
        }  // there is a subregion
      }  // for each address dataset
    }

  /* order address data by city name, street name, etc. */ {
    qsort(data__addr,data__addrn,sizeof(data__addr[0]),
      data__addr_qsort);
    }

  // remove address dataset duplicates
  r= r && data__addrclean();

  /* add used-by references */ {
    uint_fast32_t addri;
    data__addr_t* addrp;
    uint32_t si;  // string index

    for(addri= 1,addrp= data__addr+1;addri<data__addrn;
        addri++,addrp++) {  // for each address dataset
      #define D(z) si= addrp->z; \
        if(si!=0) T(data__addused,z)(si,addri);
      D(region)
      D(subregion)
      D(city)
      D(street)
      D(housenumber)
      #undef D
      }  // for each address dataset
    }  // add used-by references

  // remove unused strings
  r= r && data__strclean_region();
  r= r && data__strclean_subregion();
  r= r && data__strclean_city();
  r= r && data__strclean_street();
  r= r && data__strclean_housenumber();

  return r;
  }  // data_addrcomplete()

static bool data_write(const char* file) {
  // write address data and string data to output stream;
  // file: name of the file to write;
  //       NULL: standard output;
  // return: success;
  // uses procedures from module wrs_;
  //
  // data format .ogb osmgeobase
  //
  // (uint8_t) 0xfb - start of file
  //
  // (uint8_t) 0xe0 - file format section
  // (int64_t) 15 - length of section contents
  // (char[14+1]) "osmgeobase0000" - file format name
  //
  // (uint8_t) 0x40 - address data section
  // (int64_t) - length of section contents
  // (data__addr_t) - first address dataset
  //                  (see definition of data__addr)
  //                  first dataset is a dummy for 'invalid address'
  // (data__addr_t) - second address dataset
  // (data__addr_t) - ...
  // (data__addt_t) - last address dataset
  //
  // (uint8_t) 0x51 - "region" statistics section
  // (int64_t) 5*8 - length of section contents
  // (int64_t) - number of strings
  // (int64_t) - sum of lengths of all strings (without term. zeros)
  // (int64_t) - length of longest string (without terminating zeros);
  //             usually 40 because of limitation in data__str_t;
  // (int64_t) - total number of all used-references
  // (int64_t) - maximum number of used-references for a single string
  //
  // (uint8_t) 0x61 - "region" strings section
  // (int64_t) - length of section contents
  // (char[l+1]) - first string (zero-terminated);
  //               usually "" for 'no information';
  // (char[l+1]) - second string (zero-terminated)
  // (char[l+1]) - ...
  // (char[l+1]) - last string (zero-terminated)
  //
  // (uint8_t) 0x71 - "region" used-references section
  // (int64_t) - length of section contents
  // (uint32_t) - number of references for first string
  // (uint32_t[]) - references for first string
  // (uint32_t) 0 - references' terminator for first string
  // (uint32_t) - number of references for second string
  // (uint32_t[]) - references for second string
  // (uint32_t) 0 - references' terminator for second string
  // (uint32_t) - ...
  // (uint32_t[]) - ...
  // (uint32_t) - number of references for last string
  // (uint32_t[]) - references for last string
  // (uint32_t) 0 - references' terminator for last string
  //
  // (uint8_t) 0x52 - "subregion" statistics section
  // ...
  // (uint8_t) 0x62 - "subregion" strings section
  // ...
  // (uint8_t) 0x72 - "subregion" used-references section
  // ...
  //
  // (uint8_t) 0x53 - "city" statistics section
  // ... ...
  //
  // (uint8_t) 0x54 - "street" statistics section
  // ... ...
  //
  // (uint8_t) 0x55 - "housenumber" statistics section
  // ... ...
  //
  // all numbers are stored in little-endian order

  if(!wrs_open(file))
return false;

  // write file header
  wrs_char(0xfb);  // file header byte
  wrs_char(0xe0);  // file format section id
  wrs_int64(15);  // length of this section
  wrs_strz("osmgeobase0000",80);  // section contents

  /* write address data */ {
    int64_t len;

    wrs_char(0x40);  // address data section id
    len= (uint8_t*)data__addre-(uint8_t*)data__addr;
    wrs_int64(len);
      // address data length
    wrs_mem(data__addr,len);
    }  // write address data

  /* write string data */ {
    data__write_region();
    data__write_subregion();
    data__write_city();
    data__write_street();
    data__write_housenumber();
    }  // write address data

  // write eof mark
  wrs_char(0xfe);

  return true;
  }  // data_write()

#endif  // TEMPLATEONCE

static bool T(data_ini,I)(const char* file,int strtypeid) {
  // initialization of this instance;
  // data_ini() must have been called before;
  // file: file with zero-terminated strings;
  // strtypeid: data type identifier;
  //            for region: 1, subregion: 2, city: 3, etc.;
  // return: success;
  // reads the string file and stores it in main memory;
  #define data__ini_bufM 2000000  // size of buffer
  uint_fast32_t strn;  // number of strings
  static data__str_t* stree;  // physical end in data__str_I
  int fd;  // file descriptor

  atexit(T(data__end,I));
  T(data__strtypeid,I)= strtypeid;

  // open the string file
  fd= open(file,O_RDONLY|O_BINARY);
  if(fd<0) {
    PERRv("could not open input file: %.80s",file)
return false;
    }

  /* count all strings of the file */ {
    char* buf;  // read buffer
    char* bufp;  // read pointer in buffer
    char* bufe;  // logical end in buffer
    char* bufee;  // physical end in buffer
    size_t rb;  // number of bytes read

    buf= (char*)malloc(data__ini_bufM+4);
    if(buf==NULL) {
      PERRv("not enough memory for analyzing file %.80s",file)
return false;
      }
    strn= 1;  // start with 1 because first entry is a dummy
      // to enable zero-index entries for "unknown value";
    bufe= buf;
    bufee= buf+ data__ini_bufM;
    for(;;) {  // for all strings in file
      rb= read(fd,buf,bufee-buf);
      if(rb<=0)
    break;
      bufe= buf+rb;
      bufp= buf;
      while(bufp<bufe) if(*bufp++==0) strn++;
      }  // for all strings in file
    FREE(buf)
    }  // count all strings of the file

  // rewind the string file (just close and reopen)
  close(fd);
  fd= open(file,O_RDONLY|O_BINARY);
  if(fd<0) {
    PERRv("could not reopen input file: %.80s",file)
return false;
    }

  // allocate memory for the strings
  T(data__str,I)= (data__str_t*)malloc(sizeof(data__str_t)*strn+4);
  if(T(data__str,I)==NULL) {
    PERRv("not enough memory for strings of %.80s",file)
return false;
    }
  T(data__stre,I)= T(data__str,I);  // logical end
  stree= T(data__str,I)+strn;  // physical end

  /* read all strings of the file */ {
    char* buf;  // read buffer
    char* bufp;  // read pointer in buffer
    char* bufe;  // logical end in buffer
    char* bufee;  // physical end in buffer
    size_t rb;  // number of bytes read
    uint_fast32_t strread;  // number of strings read
    char* sp;  // destination pointer for string copy
    int i;

    buf= (char*)malloc(data__ini_bufM+4);
    if(buf==NULL) {
      PERRv("not enough memory for reading file %.80s",file)
return false;
      }
    strread= 0;
    bufp= bufe= buf;
    bufee= buf+ data__ini_bufM;
    memset(T(data__stre,I)++,0,sizeof(data__str_t));
      // make a dummy entry at zero-index for "unknown value";
    for(;;) {  // for all strings in file
      while(bufe-bufp<1000) {  // less than 1000 bytes in buffer
        // move remaining contents to start of buffer
        memmove(buf,bufp,bufe-bufp);
        bufe-= bufp-buf;
        bufp= buf;
        rb= read(fd,bufe,bufee-bufe);
        if(rb<=0)
      break;
        bufe+= rb;
        bufe[0]= bufe[1]= 0;  // set two terminating zeros
        }  // less than 100 bytes in buffer
      if(rb<=0 && bufp==bufe)  // end of file AND buffer empty
    break;
      if(T(data__stre,I)>=stree) {
        PERRv("string memory overflow: %.80s",file)
        FREE(buf)
        close(fd);
return false;
        }
      i= data__strstringM;  // maximum string length
      sp= T(data__stre,I)->string;
      while(*bufp!=0 && i>0) {
        *sp++= *bufp++;  // copy string byte by byte
        i--;
        }
      while(i>=0) {
        *sp++= 0;  // fill remaining string space with zeros
        i--;
        }
      while(*bufp!=0) bufp++;
        // ignore remaining characters of very long strings
      T(data__stre,I)->usedn= 0;
      strread++;
      T(data__stre,I)++;
      if(T(data__stre,I)>=stree)  // end of string space
    break;
      bufp++;  // jump over string terminator
      if(*bufp==0)  // there is a second terminator => end of file
    break;
      }  // for all strings in file
    FREE(buf)
    if(strread==0 && T(data__strtypeid,I)!=2 || 1+strread!=strn) {
        // (did not read any string AND not instance 'subregion')
        // OR did not read the expected number of strings
      PWARNv("could not completely read file %.80s",file)
      PINFOv("read %"PRIuFAST32" strings of %"PRIuFAST32".",
        strread,strn);
      close(fd);
return false;
      }
    }  // read all strings of the file

  // close the string file
  close(fd);

  T(data__strn,I)= strn;
  return true;
  }  // data_ini_I()

static inline const char* T(data_string,I)(uint32_t index) {
  // retrieve the string to a given index;
  return T(data__str,I)[index].string;
  }  // data_string_I()

static inline uint32_t T(data_index,I)(const char* string) {
  // determine the index to a given string;
  // string[]: string the index is to be determined for;
  //           NULL: will return 0;
  // 0: there is no such string in string memory;
  int_fast32_t i,i1,i2;  // iteration indexes:
    // middle, lower including, upper excluding;
  int cv;  // compare value

  if(string==NULL)
return 0;
  i1= 0; i2= T(data__strn,I);
  while(i1<i2) {  // binary search
    i= (i1+i2)/2;
    cv= strncmp(string,T(data__str,I)[i].string,data__strstringM);
    if(cv==0)
return i;
    if(cv<0) i2= i;
    else i1= i+1;
    }  // binary search
return 0;
  }  // data_index_I()

#endif
//------------------------------------------------------------
// end   Module data_   osm data module
//------------------------------------------------------------

