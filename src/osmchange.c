// osmchange 2011-04-27 22:00
#define VERSION "0.7"
// (c) Markus Weber, Nuernberg
//     (main modules, algorithms for hash, border polygon clipping)
// (c) Stephan Knauss, Muenchen
//     (C standards, i/o optimizing for Windows)
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

#define MAXLOGLEVEL 2
const char* helptext=
"\nosmchange " VERSION "\n"
"\n"
"This program updates an .osm file using one or more .osc files.\n"
"For example:\n"
"\n"
"  cat a.osm | ./osmchange changefile.osc >b.osm\n"
"  bzcat a.osm.bz2 | ./osmchange changefile.osc | gzip -1 >b.osm.gz\n"
"  cat day24.osm | ./osmchange c24_25.osc c25_26.osc >day26.osm\n"
"\n"
"If you want to limit the region, you can use a bounding box. To do\n"
"this, enter the southwestern and the northeastern corners of the box.\n"
"For example:\n"
"\n"
"  cat a.osm | ./osmchange -b=-0.5,51,0.5,52 >b.osm\n"
"  cat a.osm | ./osmchange -b=8,-5,20,5 changefile.osc >b.osm\n"
"\n"
"For another option to limit the region, you can use a border polygon\n"
"file:\n"
"\n"
"  cat a.osm | ./osmchange -B=border.poly changefile.osc >b.osm\n"
"  cat a.osm | ./osmchange -B=france.poly >b.osm\n"
"\n"
"The format of a border polygon file can be found in the OSM Wiki:\n"
"http://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format\n"
"You do not need to follow strictly the format description, but you\n"
"must ensure that every line of coordinates starts with blanks.\n"
"\n"
"Tuning\n"
"\n"
"If your operating system provides only low performance pipes for\n"
"standard input, you can use the -i parameter to read an uncompressed\n"
".osm file directly. This may be useful under Windows. For example:\n"
"\n"
"  ./osmchange -i=a.osm changefile.osc >b.osm\n"
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
"  -h130-12-2\n"
"\n"
"But keep in mind that the OSM database is continuously expanding. For\n"
"this reason the program-own default value is higher than shown in the\n"
"example, and it may be appropriate to increase it in the future.\n"
"If you do not want to bother with the details, you can enter the\n"
"amount of memory as a sum, and the program will divide it by itself.\n"
"For example:\n"
"\n"
"  -h1000\n"
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
"Input files must contain the objects strictly ordered by their type:\n"
"first, all nodes, next, all ways, followed by all relations. Within\n"
"each of these sections, the objects must be ordered by their ID tags.\n"
"In each object, the ID tag must be the first tag; i.e., it must\n"
"follow immediately the object type \'<node\', \'<way\' or \'<relation\'.\n"
"For example:\n"
"  <node id=\"12345\" lat=\"60.0\" lon=\"179.9\" />\n"
".osm and .osc files usually adhere to both of these conditions. This\n"
"means that you do not have to worry about these limitations. osmchange\n"
"will display an error message if this sequence is broken. You may also\n"
"test the sequence within a file without doing any other processing:\n"
"\n"
"  cat file_to_test.osm | ./osmchange -t\n"
"\n"
"If a polygon file for borders is supplied, there are three further\n"
"limitations:\n"
"First, the maximum number of polygon points is about 40,000.\n"
"Second, ways without a node inside the border polygon will be filtered\n"
"out as well as relations which refer only to such ways. Relations\n"
"which refer solely to unused relations with a lower ID will also be\n"
"filtered out, whereas every other relation will stay. For this reason,\n"
"there might be a few unwanted relations in the program\'s output; but\n"
"usually, this does not matter because they do not have georeferences.\n"
"Third, extremely large relations (more than 200,000 characters) might\n"
"not be filtered out. In this case, you will get a warning message.\n"
"\n"
"Hint: If you apply change files to one regional .osm file over and\n"
"over, there is a chance a way could stay unrecognized:\n"
"The way was completely outside the limiting borders and has been moved\n"
"into the area without making changes to the data set of the way. But\n"
"this case is extremely rare.\n"
"\n"
"Presently, this program is in an experimental state. Please expect\n"
"errors and do not use the program in productive or commercial systems.\n"
"\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"Please send any bug reports to markus.weber@gmx.com\n\n";

#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

typedef enum {false= 0,true= 1} bool;
#define isdig(x) isdigit((unsigned char)(x))
static int loglevel= 0;  // logging to stderr;
  // 0: no logging; 1: small logging; 2: normal logging;
  // 3: extended logging;

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

static char write__buf[UINT64_C(16)*1024*1024];
static char* write__bufe= write__buf+sizeof(write__buf);
static char* write__bufp= write__buf;

//------------------------------------------------------------

static bool write_testmode= false;  // no standard output

static inline void write_stdout(int c) {
  // write one byte to stdout, use a buffer;
  if(write__bufp>=write__bufe) {  // the write buffer is full
    if(!write_testmode)
      write(1,write__buf,write__bufp-write__buf);
    write__bufp= write__buf;
    }
  *write__bufp++= (char)c;
  }  // end   write_stdout();

static inline void write_stdout_nl() {
  // write a NL character to stdout;
  // in case of Windows, precede NL with a CR; 
  #ifdef _WIN32
  write_stdout('\r');  // add a CR character
  #endif
  write_stdout('\n');  // add a NL character
  }  // end   write_stdout_nl();

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
// Module hash_   OSM hash module
//------------------------------------------------------------

// this module provides three hash tables with default sizes
// of 320, 60 and 20 MB;
// the procedures hash_set() and hash_get() allow bitwise access
// to these tables;
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
static unsigned long hash__max[hash__M]= {0,0,0};
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

#if 0  // currently not used in this program
static void hash_set(int o,const char* id) {
  // set a flag for a specific object type and ID;
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries
  //    are not checked;
  // id: id of the object; the id is given as a string of decimal
  //     digits; a specific string terminator is not necessary,
  //     it is assumed that the id number ends with the first
  //     non-digit character;
  unsigned char* mem;  // address of byte in hash table
  uint64_t idi;  // bit number (0..7)
  unsigned int ido;  // bit offset to idi;

  if(!hash__initialized) return;  // error prevention
  idi= 0;
  if(*id!='-') {  // positive id
    while(isdig(*id)) { idi= idi*10+(*id-'0'); id++; }
    }
  else {  // negative id
    id++;
    while(isdig(*id)) { idi= idi*10+(*id-'0'); id++; }
    idi= hash__max[o]*8-idi;
    }
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  *mem|= (1<<ido);  // set bit
  }  // end   hash_set()
#endif

static void hash_seti(int o,int64_t idi) {
  // set a flag for a specific object type and ID;
  // (same as previous procedure, but id must be given as number);
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

static bool hash_get(int o,const char* id) {
  // get the status of a flag for a specific object type and ID;
  // o: object type; 0: node; 1: way; 2: relation;
  //    caution: due to performance reasons the boundaries are not checked;
  // id: id of the object; the id is given as a string of decimal digits;
  //     a specific string terminator is not necessary, it is assumed
  //     that the id number ends with the first non-digit character;
  unsigned char* mem;
  uint64_t idi;
  unsigned int ido;  // bit offset to idi;
  bool flag;

  if(!hash__initialized) return 0;  // error prevention
  idi= 0;
  if(*id!='-') {  // positive id
    while(isdig(*id)) { idi= idi*10+(*id-'0'); id++; }
    }
  else {  // negative id
    id++;
    while(isdig(*id)) { idi= idi*10+(*id-'0'); id++; }
    idi= hash__max[o]*8-idi;
    }
  ido= idi&0x7;  // extract bit number (0..7)
  idi>>=3;  // calculate byte offset
  idi%= hash__max[o];  // consider length of hash table
  mem= hash__mem[o];  // get start address of hash table
  mem+= idi;  // calculate address of the byte
  flag= (*mem&(1<<ido))!=0;  // get status of the addressed bit
  return flag;
  }  // end   hash_get();

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

bool hash_getparse(int type,int64_t id,
    const char* s,const char* fn) {
  // parse a given section and find out if there is at least one
  // object for which its hash flag has been set;
  // type: type of the object we have to parse; here:
  //       2=='way'; 3=='relation';
  // id: ID of the object we have to parse;
  // s[]: characters to parse; parsing will take place until
  //      a null character or one of these tags has been found:
  //      "</way>", "</relation>";
  // fn[]: file name, for error messages;
  // return: there is at least one object with a set flag OR
  //         there were no references at all;
  // uses: hash_get();
  int l;  // length of matched key identifier
  int o;  // object type; 0: node; 1: way; 2: relation;
  int ret;

  ret= true;  // (default)
  while(*s!=0) {  // still characters to parse
    if(*s=='<') {  // a key identifier starts here
      if(strzcmp(s,"</way>")==0 || strzcmp(s,"</relation>")==0)
return ret;
      o= 0;  // (just to get sure)
      if(type==3 &&
          (l= strzlcmp(s,"<member type=\"relation\" ref=\""))>0) {
          // this object is a relation and refers to another relation
        int64_t refid;  // ID of referenced relation
        const char* sp;

        o= 2;
        // find out if the referenced relation has a lower id;
        // if so, we can assume that that relation must have been
        // flagged if it was needed; if it has a higher id, 
        // we cannot be sure if the referenced relation refers to
        // objects within the border of the defined polygon,
        // because the referenced relation may have not yet been
        // tested for dependencies; so we must keep the object;
        sp= s+l;
        refid= 0;
        if(*sp!='-') {  // positive refid
          while(isdig(*sp)) { refid= refid*10+(*sp-'0'); sp++; }
          }
        else {  // negative refid
          sp++;
          while(isdig(*sp)) { refid= refid*10+(*sp-'0'); sp++; }
          refid= -refid;
          }
        if(refid>=id)
return true;
        if(hash_geti(o,refid))  // there is flag for the
            // related object in hash table
return true;
        ret= false;  // memorize that there was at least one reference
        s+= l-1;  // jump over the search string
        }  // end   this object refers to a relation;
      else if((l= strzlcmp(s,"<nd ref=\""))>0) o= 0;
      else if((l= strzlcmp(s,"<member type=\"node\" ref=\""))>0) o= 0;
      else if((l= strzlcmp(s,"<member type=\"way\" ref=\""))>0) o= 1;
      if(l>0) {  // we found one of the searched key identifiers
        if(hash_get(o,s+l))  // there is flag for the
            // related object in hash table
return true;
        ret= false;  // memorize that there was at least one reference
        s+= l-1;  // jump over the search string
        }
      }  // end   a key identifier starts here
    s++;  // take the next character
    }  // end   still characters to parse
  fprintf(stderr,"osmchange: %s %"PRIi64" too large in %s\n",
    (type==2? "Way": type==3? "Relation": "?"), id, fn);
  hash__error_number= 1;
  return true;
    // return true, because we are not sure if this object has nodes
    // within the borders (we did not get the whole object data);
  }  // end   hash_getparse()
#if 0  // OSM XML examples
  <node id="703066091" version="2" lat="49.777873" lon="9.621692"/>
  <way id="56012785" version="2" changeset="4484048">
    <nd ref="361195677"/>
  </way>
  <relation id="7044" version="4" changeset="4446778">
    <member type="node" ref="251589955" role="via"/>
    <member type="way" ref="23250694" role="from"/>
    <member type="relation" ref="14184" role=""/>
  </relation>
#endif

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

static const long border__nil= 2000000000L;
static long border__bx1= 2000000000L,border__by1,
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
  long x1,y1,x2,y2;  // coordinates of the edge; always: x1<x2;
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

static bool border__ini(void) {
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

static long border__strtodeg(const char* s) {
  // read a number which represents a degree value and
  // convert it to a fixpoint number;
  // s[]: string with the number between -180 and 180,
  //      e.g. "-179.99", "11", ".222";
  // return: number in 10 millionth degrees;
  //         =='border__nil': syntax error;
  static const long di[]= {10000000L,10000000L,1000000L,100000L,
    10000L,1000L,100L,10L,1L};
  static const long* dig= di+1;
  int sign;
  int d;  // position of decimal digit;
  long k;
  char c;

  if(*s=='-') { s++; sign= -1; } else sign= 1;
  if(!isdig(*s) && *s!='.')
return border__nil;
  k= 0;
  d= -1;
  do {  // for every digit
    c= *s++;
    if(c=='.') { d= 0; continue; }  // start fractional part
    else if(!isdig(c) || c==0)
  break;
    k= k*10+c-'0';
    if(d>=0) d++;
    } while(d<7);  // end   for every digit
  k*= dig[d]*sign;
  return k;
  }  // end   border__strtodeg()

static int border__qsort_edge(const void* a,const void* b) {
  // edge comparison for qsort()
  return ((border__edge_t*)a)->x1 > ((border__edge_t*)b)->x1;
  }

//------------------------------------------------------------

static bool border_strtocoord(const char* s,long* xp,long* yp) {
  // find the geographical coordinates in a string and
  // convert them into degree numbers;
  // in fixpoint format (10 millionth degrees);
  // s[]: string with the coordinates; possible formats:
  //        id="1234" user="someone" lat="-70.500" lon= "89"
  //        lon= ".01" lat="-179.56789"
  //      the search ends as soon as > or a null character
  //      has been found;
  // return: conversion successful;
  // x,y: coordinates in 0.0000001 degrees;
  //      x=='border__nil': found no coordinate;
  char c;
  long x,y;

  x= y= border__nil;
  for(;;) {  // for characters in string
    c= *s;
    if(c=='\"') { do c= *++s; while(c!='\"' && c!=0); }
    if(c=='>' || c==0)
  break;
    if(c==' ' && s[1]=='l') {  // may be start of " lat" or " lon"
      if(s[2]=='o' && s[3]=='n' && s[4]=='=' && s[5]=='\"') {
          // " lon=\""
        x= border__strtodeg(s+6);
        if(y!=border__nil)
  break;
        s+= 4;
        }
      else if(s[2]=='a' && s[3]=='t' && s[4]=='=' && s[5]=='\"') {
          // " lat=\""
        y= border__strtodeg(s+6);
        if(x!=border__nil)
  break;
        s+= 4;
        }
      }  // end   may be start of " lat" or " lon"
    s++;
    } // end   for characters in string
  if(y==border__nil || y==border__nil) {
    *xp= border__nil; *yp= border__nil;
return false;
    }
  *xp= x; *yp= y;
return true;
  }  // end   // border_strtocoord()

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
  static long nil;

  if(!border__ini())
return false;
  nil= border__nil;

  /* get border polygon */ {
    border__edge_t* bep;  // growing pointer in border__edge[]
    border__edge_t* bee;  // memory end of border__edge[]
    FILE* fi;
    char s[80],*sp;
    long x0,y0;  // coordinate of the first point in a section;
      // this is used to close an unclosed polygon;
    long x1,y1;  // last coordinates
    long x,y;

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
              fprintf(stderr,"+ %i %li,%li,%li,%li\n",
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
            fprintf(stderr,"c %i %li,%li,%li,%li\n",
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
              fprintf(stderr,"- %i %li,%li,%li,%li\n",
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
    long x1,x2;
    border__chain_t* bcp;  // growing pointer in chain storage
    border__edge_t* bep;  // pointer in border__edge[]
    border__edge_t* bep2;  // referenced edge
    border__chain_t* bcp2;  // chain of referenced edge;

    bep= border__edge;
    bcp= border__chain;
    while(bep->x1!=nil) {  // for each edge in list
      if(loglevel>=1)
        fprintf(stderr,"> %i %li,%li,%li,%li\n",
          (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      x1= bep->x1; x2= bep->x2;
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
      fprintf(stderr,"> %i %li,%li,%li,%li\n",
        (int)(bep-border__edge),bep->x1,bep->y1,bep->x2,bep->y2);
      bcp= bep->chain;
      while(bcp!=NULL) {  // for each chain link in edge
        bep2= bcp->edge;
        fprintf(stderr,"  %i %li,%li,%li,%li\n",
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

static bool border_queryinside(long x,long y) {
  // determine if the given coordinate lies inside or outside the
  // border polygon(s);
  // x,y: coordinates of the given point in 0.0000001 degrees;
  // return: point lies inside the border polygon(s);
  static long nil;

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
          if( (long long)(y-bep->y1)*(long long)(bep->x2-bep->x1) <
              (long long)(x-bep->x1)*(long long)(bep->y2-bep->y1) ) {
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

//------------------------------------------------------------
// end Module border_   OSM border module
//------------------------------------------------------------



//------------------------------------------------------------
// Module read_   OSM read module
//------------------------------------------------------------

// this module provides procedures for reading the input .osm
// and .osc input files;
// as usual, all identifiers of a module have the same prefix,
// in this case 'read'; an underline will follow in case of a
// global accessible object, two underlines in case of objects
// which are not meant to be accessed from outside this module;
// the sections of private and public definitions are separated
// by a horizontal line: ----

static inline int64_t read__id(const char* s) {
  // read the id of an OSM XML object;
  // s[]: string with the ID, e.g.: id="12345" lat="49.0" ...
  //      the string must start with "id=\"";
  // return: id of the object; 0: invalid id;
  int64_t id;

  if(s[0]!='i' || s[1]!='d'  || s[2]!='=' ||
      (s[3]!='\"' && s[3]!='\''))
return 0;
  s+= 4;  // jump over "id=\""
  id= 0;
  if(*s!='-') {  // positive id
    while(isdig(*s)) { id= id*10+(*s-'0'); s++; }
    }
  else {  // negative id
    s++;
    while(isdig(*s)) { id= id*10+(*s-'0'); s++; }
    id= -id;
    }
  return id;
  }  // end   read__id()

#define read__prefetch UINT32_C(1000000)
  // read prefetch because we want to get access to several
  // bytes which are to be read next
typedef struct read__struct {
  char buf[UINT32_C(19000000)+read__prefetch];
  char* bufe;
  char* bufp;
  int fd;  // file descriptor
  bool shortobject;  // the last encountered object may be written
    // the short XML form, e.g.: <node ... />
    // otherwise it hast to be in the long form: <node> ... </node>
  int section;  // section we are inside;
    // 0: no section; 1: delete; 2: create; 3: modify;
  int type;  // last encountered object type;
    // 0: none; 1: node; 2: way; 3: relation;
  int64_t id;  // last encountered object id
  uint64_t typeid;  // last encountered object typ and id;
    // lower bits: id; higher bits: type; mask: 0x7000000000000000LL;
    // 0x1000000000000000LL: node;
    // 0x2000000000000000LL: way;
    // 0x3000000000000000LL: relation;
    // note: to deal with negative ids, we add always:
    //  0x800000000000000LL
  uint64_t typeidold;  // previous encountered
    // object type and id, for plausibility checks;
  bool writeflag;  // every input byte shall be written to output;
  char fn[32];  // file name, for error messages;
  struct read__struct* next;
  } read_info;
static read_info* read_infochain= NULL;
  // pointer to first read info structure;
static int read__error_number= 0;
  // 1: Wrong sequence in one of the files

static void read__closeall(void) {
  // close all previously opened input files;
  // this procedure has no parameters because we want to be able
  // to call it via atexit();
  read_info* ri;

  while(read_infochain!=NULL) {
    ri= read_infochain;
    if(ri->fd!=0)
      close(ri->fd);
    ri->fd= -1;
    read_infochain= ri->next;
    free(ri);
    }
  }  // end   read__closeall()

static bool read__input(read_info* ri) {
  // read data from an input file, use an internal buffer;
  // in dependence of ri->writeflag, every read byte will be
  // written to standard output;
  // the procedure returns if either the end of the input file
  // has been reached or a new object start, e.g. "<node>",
  // has been encountered;
  // *ri: read information structure;
  // return: there are no (more) bytes to read;
  // *ri: information about last encountered object;
  // to get object ID information easier, we use a prefetch of
  // 'read__prefetch' bytes;
  char c;  // last character which has been read;
  bool writeend;  // end writing to stdout after the next '>';
  int l,r;
  bool ret;

  writeend= false;
  for(;;) {  // until break;
    if(ri->bufp+read__prefetch>=ri->bufe) {  // read buffer is too low
      if(ri->fd>=0) {  // still bytes in the file
        if(ri->bufe>ri->bufp) {  // bytes remaining in buffer
          memmove(ri->buf,ri->bufp,ri->bufe-ri->bufp);
            // move remaining bytes to start of buffer
          ri->bufe= ri->buf+(ri->bufe-ri->bufp);
            // protect the remaining bytes at buffer start
          }
        else  // no remaining bytes in buffer
          ri->bufe= ri->buf;  // no bytes remaining to protect
        ri->bufp= ri->buf;
#if 1
        do {  // while buffer has not been filled
          l= (ri->buf+sizeof(ri->buf))-ri->bufe-1;
            // number of bytes to read
          r= read(ri->fd,ri->bufe,l);
//fprintf(stderr,"read bytes: wanted %u, got %u\n",l,r);
          if(r<=0) {  // no more bytes in the file
            ri->fd= -1;  // memorize that there we are at end of file
        break;
            }
          ri->bufe+= r;  // set new mark for end of data
          ri->bufe[0]= 0;  // set null-terminator
          } while(r<l);  // end   while buffer has not been filled
#else
        r= read(ri->fd,ri->bufe,sizeof(ri->buf)-read__prefetch-1);
          // fill the rest of the buffer with bytes from the file
//fprintf(stderr,"read bytes: wanted %u, got %u\n",sizeof(ri->buf)-read__prefetch-1,r);
        if(r<=0)  // no more bytes in the file
          ri->fd= -1;  // memorize that there we are at end of file
        else {
          ri->bufe+= r;  // set new mark for end of data
          ri->bufe[0]= 0;  // set null-terminator
          }
#endif
        }  // end   still bytes to read
      if(ri->fd<0 && ri->bufp>=ri->bufe) {
          // buffer empty and no more bytes in the file
        ri->section= 0;
        ri->type= 0;
        ri->id= 0;
        ri->typeid= 0;
        ri->typeidold= 0;
        ri->writeflag= false;
        ret= true;
  break;
        }
      }  // end   read buffer is too low
    c= *ri->bufp;
    ri->bufp++;
    if(c=='<') {  // new object
      ri->shortobject= false;  // (default)
      if(strzcmp(ri->bufp,"delete>")==0)
        ri->section= 1;
      else if(strzcmp(ri->bufp,"create>")==0)
        ri->section= 2;
      else if(strzcmp(ri->bufp,"modify>")==0)
        ri->section= 3;
      else if(ri->bufp[0]=='/' && (
          strzcmp(ri->bufp+1,"delete>")==0 ||
          strzcmp(ri->bufp+1,"create>")==0 ||
          strzcmp(ri->bufp+1,"modify>")==0))
        ri->section= 0;
      else if((l= strzlcmp(ri->bufp,"node "))>0) {
        ri->type= 1;
        ri->id= read__id(ri->bufp+l);
        ri->typeid= UINT64_C(0x1800000000000000)+ri->id;
        #define read__plausi(t) if(ri->typeid<=ri->typeidold) \
          { fprintf(stderr,"osmchange: Wrong sequence at " \
          #t " %"PRIi64" in %s\n",ri->id,ri->fn); \
          read__error_number= 1; } \
          ri->typeidold= ri->typeid;
        read__plausi(node);
        ri->shortobject= true;  // may be a short object
        ret= false;
  break;
        }
      else if((l= strzlcmp(ri->bufp,"way "))>0) {
        ri->type= 2;
        ri->id= read__id(ri->bufp+l);
        ri->typeid= UINT64_C(0x2800000000000000)+ri->id;
        read__plausi(way);
        ri->shortobject= true;  // may be a short object
        ret= false;
  break;
        }
      else if((l= strzlcmp(ri->bufp,"relation "))>0) {
        ri->type= 3;
        ri->id= read__id(ri->bufp+l);
        ri->typeid= UINT64_C(0x3800000000000000)+ri->id;
        read__plausi(relation);
        ri->shortobject= true;  // may be a short object
        ret= false;
  break;
        }
      else if(strzlcmp(ri->bufp,"osm version=\"") &&
          ri->writeflag) {
        char* p;

        write_stdout('<');
        p= ri->bufp;
        for(;;) { write_stdout(*p); if(*p=='\"') break; p++; }
        p++;
        for(;;) { write_stdout(*p); if(*p=='\"') break; p++; }
        p= " generator=\"osmchange "VERSION"\">";
        do write_stdout(*p++); while(*p!=0);
        write_stdout_nl();
        ri->writeflag= false;
        ri->shortobject= true;  // may be a short object
        ret= false;
  break;
        }
      else if(strzcmp(ri->bufp,"/node>")==0 ||
          strzcmp(ri->bufp,"/way>")==0 ||
          strzcmp(ri->bufp,"/relation>")==0) {
        ri->type= 0;
        ri->id= 0;
        ri->typeid= 0;
        writeend= true;
        }
      }  // end   new object
    else if(c=='>')
      ri->shortobject= false;
    else if(c=='/' && ri->bufp[0]=='>' && ri->shortobject) {
        // end of object using the short XML form
      ri->type= 0;
      ri->id= 0;
      ri->typeid= 0;
      ri->shortobject= false;
      writeend= true;
      }
    if(ri->writeflag) {
      write_stdout(c);  // write the read-in character to stdout
      if(writeend && c=='>') {  // writing is to end
        write_stdout_nl();
        ri->writeflag= false;
        }
      }
    }  // end   until break
  return ret;
  }  // end   read__input()

#define read__queryend(ri) (ri->fd<0 && ri->bufp>=ri->bufe)
  // query if the end of a specific input file has been reached

static uint64_t read__typeid= UINT64_C(0x7fffffffffffffff);
  // presently considered typeid;
  // a typeid is a combination of type and id of an object; example:
  // type 'node', id "12345";
  // as typeid is a 64 bit value, the object type will be coded
  // within the highest bits to allow hierarchical sorts:
  // primary type and secondary id;
  // bits 0x7000000000000000LL: 1==node; 2==way; 3==relation;
  // to consider negative ids, we always add an offset:
  //       0x800000000000000LL;

//------------------------------------------------------------

static read_info* read_open(const char* fn) {
  // open an input file;
  // fn[]: name of the file:
  //       fn==NULL: read from stdin;
  // return: pointer to read info structure;
  //         ==NULL: could not open this file;
  //         the read info structure is always initialized with
  //         default parameters;
  // for the first call of this procedure, the member 'writeflag'
  // will be set to ensure that the first section of the file
  // (resp. standard input) will be copied to standard output;
  static bool firstrun= true;
  static bool firstwriteflag= true;
  read_info* ri;
  int fd;

  if(firstrun) {
    firstrun= false;
    atexit(read__closeall);
    }
  if(fn==NULL)  // to read from stdin
    fd= 0;
  else {  // not to read from stdin
    fd= open(fn,O_RDONLY);
    if(fd<0)
return NULL;
    }  // end   not to read from stdin
  ri= (read_info*)malloc(sizeof(read_info));
  if(ri==NULL)
return NULL;
  // initialize read info structure
  ri->bufe= ri->buf;
  ri->bufp= ri->buf;
  ri->fd= fd;
  ri->shortobject= false;
  ri->section= 0;
  ri->type= 0;
  ri->id= 0;
  ri->typeid= 0;
  ri->typeidold= 0;
  ri->writeflag= firstwriteflag;
  firstwriteflag= false;
  if(fn==NULL) strcpy(ri->fn,"standard input");
  else strMcpy(ri->fn,fn);
  ri->next= read_infochain;
  read_infochain= ri;
  return ri;
  }  // end   read_open()

static inline bool read_allinput(read_info** rif) {
  // read all input files until the next relevant object has been
  // encountered;
  // but: files which have already been read till an object with a
  // typeid greater read__typeid are not read;
  // rif[]: NULL-terminated list with the addresses of the file's
  //        read info structures;
  // return: all files have been read completely;
  // the read info structures will be updated accordingly to the
  // encountered object types and IDs;
  read_info* ri;
  bool ret;
  int i;

  ret= true;  // (default)
  for(;;) {  // for all input files
    ri= *rif;
    if(ri==NULL)
  break;
    if(!read__queryend(ri)) {
      ret= false;
      if(ri->typeid==0) {  // 2011-04-27,,,
        i= 10;
        while(--i>=0 && ri->typeid==0)
          read__input(ri);
        }
      else if(ri->typeid<=read__typeid)
        read__input(ri);
      }
    rif++;
    }  // end  for all input files
  return ret;
  }  // end  read_allinput()

static void read_setwriteflag(read_info** rif) {
  // pick out the most relevant object and set the write flag there;
  // rif[]: NULL-terminated list with the addresses of the file's
  //        read info structures;
  //        the list is expected in the following order:
  //        .osm file, oldest .osc file .. newest .osc file;
  // return:
  // the 'writeflag' elements in the read info structures will
  // be updated;
  read_info** rip,*rim,*ri;

  // delete all write flags
  rip= rif;
  for(;;) {  // for all input files
    ri= *rip;    if(ri==NULL)
  break;
    ri->writeflag= false;
    rip++;
    }  // end  for all input files

  // find the most recent file with the lowest typeid
  read__typeid= UINT64_C(0x7fffffffffffffff);
    // just to ensure that this value will be subceeded;
  rim= NULL;
  while(rip>rif) {  // for all input files, starting with the last
    rip--;
    ri= *rip;
    if(ri->typeid>0 && ri->typeid<read__typeid) {
        // new minimum encountered
      rim= ri;
      read__typeid= ri->typeid;  // memorize this minimum
      }
    }  // end  for all input files, starting with the last

  // set new write flag if applicable
  if(rim!=NULL) {  // found a minimum
    if(rim->section!=1) {  // no .osc delete section
      rim->writeflag= true;  // (default)
      if(border_active) {  // borders are to be considered
        if(rim->type==1) {  // type is "node"
          long x,y;

          if(border_strtocoord(rim->bufp,&x,&y)) {
              // object has a coordinate
            if(!border_queryinside(x,y))
                // coordinate lies outside the region's borders
              rim->writeflag= false;
            }  // end   object has a coordinate
          }  // end   type is "node"
        else if(rim->type>=2) {  // type is 'way' or 'relation'
          // for a way:
          // find out if there are references to nodes which have been
          // marked in the hash table; if so, mark this way too;
          // for a relation:
          // find out if there are references to nodes or ways which
          // have been marked in the hash table;
          // if so, mark this relation too;
          if(!hash_getparse(rim->type,rim->id,rim->bufp,rim->fn))
              rim->writeflag= false;
          }  // end   type is "way"
        if(rim->writeflag && rim->type>0)
            // writing is requested AND regular object type
          hash_seti(rim->type-1,rim->id);
            // set flag as sign that this object is valid
        }  // end   borders are to be considered
      }  // end   no .osc delete section
    if(rim->writeflag) {  // writing is requested
      // make up for '<' because we jumped over it
      #if 0
      write_stdout(' '); write_stdout(' ');
      write_stdout(' '); write_stdout(' ');
      #endif
      write_stdout('<');
      }
    }  // end   found a minimum
  }  // end  read_setwriteflag()

static bool read_queryerror() {
  // determine if an error has occurred;
  return read__error_number;
  }  // end   read_queryerror()

//------------------------------------------------------------
// end Module read_   OSM read module
//------------------------------------------------------------



int main(int argc,const char *argv[]) {
  // main program;
  // for the meaning of the calling line parameters please look at the
  // contents of helptext[];
  int h_n,h_w,h_r;  // user-suggested hash size in MiB, for
    // hash tables of nodes, ways, and relations;
  static read_info* rifiles[4000];
    // pointer to info structure for every input file
  read_info** riend;
    // end of the input structure pointers in rifiles[]

  // initialization
  h_n= h_w= h_r= 0;
  riend= NULL;  // 'no file has been opended yet'

  /* read command line parameters and open input files */ {
    if(argc<=1) {  // no command line parameters given
      fprintf(stderr,"osmchange " VERSION "\n"
        "Applies changes to an .osm file and sets limiting borders.\n"
        "To get detailed help, please enter: ./osmchange -h\n");
return 0;  // end the program, because without having parameters
        // we do not know what to do;
      }
    while(--argc>0) {  // for every parameter in command line
      argv++;  // switch to next parameter; as the first one is just
        // the program name, we must do this previous reading the
        // first 'real' parameter;
      if(strcmp(argv[0],"-h")==0) {
          // user wants help text
        fprintf(stderr,"%s",helptext);  // print help text
          // (strange format string just to suppress
          // that silly compiler warning)
return 0;
        }
      if(strzcmp(argv[0],"-t")==0) {
          // test mode
        write_testmode= true;
        if(argv[0][2]==0)
          fprintf(stderr,"osmchange: Entering test mode.\n");
        else {
          loglevel= argv[0][2]-'0';
          if(loglevel<1) loglevel= 1;
          if(loglevel>MAXLOGLEVEL) loglevel= MAXLOGLEVEL;
          fprintf(stderr,"osmchange: Entering loglevel %i.\n",loglevel);
          }
        continue;  // take next parameter
        }
      if(argv[0][0]=='-' && argv[0][1]=='h' && isdig(argv[0][2])) {
          // "-h...": user wants a specific hash size;
          // note that we accept "-h" only if it is continued by a
          // digit, so that a plain "-h" would not be recognized
          // and therefore print the help text;
        const char* p;

        p= argv[0]+2;  // jump over "-h"
        h_n= h_w= h_r= 0;
        // read the up to three values for hash tables' size;
        // format examples: "-h200-20-10", "-h1200"
        while(isdig(*p)) { h_n= h_n*10+*p-'0'; p++; }
        if(*p!=0) { p++; while(isdig(*p)) { h_w= h_w*10+*p-'0'; p++; } }
        if(*p!=0) { p++; while(isdig(*p)) { h_r= h_r*10+*p-'0'; p++; } }
        continue;  // take next parameter
        }
      if(strzcmp(argv[0],"-b=")==0) {
          // border consideration by a bounding box
        if(!border_box(argv[0]+3)) {
          fprintf(stderr,"osmchange: Use border format: "
            " -b\"x1,y1,x2,y2\"\n");
return 3;
          }  // end   border consideration by a bounding box
        continue;  // take next parameter
        }
      if(strzcmp(argv[0],"-B=")==0) {
          // border consideration by polygon file
        if(!border_file(argv[0]+3)) {
          fprintf(stderr,
            "osmchange: No polygon file or too large: %s\n",
            argv[0]);
return 4;
          }  // end   border consideration by polygon file
        continue;  // take next parameter
        }
      if(strzcmp(argv[0],"-i=")==0 && riend==NULL) {
          // use input file instead of stdin AND
          // no file has been opened yet
        riend= rifiles;
        *riend= read_open(argv[0]+3);
        if(*riend==NULL) {
          fprintf(stderr,"osmchange: Could not open: %s\n",argv[0]+3);
return 1;
          }
        riend++;
        continue;  // take next parameter
        }
      if(riend>=rifiles+(sizeof(rifiles)/sizeof(rifiles[0]))-2) {
        fprintf(stderr,"osmchange: Too many input files.\n");
return 11;
        }
      // here: not a parameter; hence we assume, it's a .osc filename
      if(riend==NULL) {  // no file has been opended yet
        // assume that .osm file shall be read from stdin;
        riend= rifiles;
        *riend= read_open(NULL);
        if(*riend==NULL) {
          fprintf(stderr,"osmchange: Could not open standard input.\n");
return 1;
          }
        riend++;
        }
      *riend= read_open(argv[0]);
      if(*riend==NULL) {
        fprintf(stderr,"osmchange: Could not open: %s\n",argv[0]);
return 2;
        }
      riend++;
      }  // end   for every parameter in command line
    }  // end   read command line parameters
  // open stdin in case it has not been opened yet
  if(riend==NULL) {  // no file has been opended yet
    riend= rifiles;
    *riend= read_open(NULL);
    if(*riend==NULL) {
      fprintf(stderr,"osmchange: Could not open standard input.\n");
return 1;
      }
    riend++;
    }
  *riend= NULL;  // terminate list of pointers to read info structures

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
      fprintf(stderr,"osmchange: Hash size had to be reduced.\n");
    else if(r==2)
      fprintf(stderr,"osmchange: Not enough memory for hash.\n");
    }  // end   user wants borders

  // process input data
  for(;;) {  // until all files have been processed
    bool e;

    // read all files until the next relevant object
    e= read_allinput(rifiles);
    if(e)
  break;
    // set write flag for that file in list which has encountered the
    // object with the lowest type/id; if there are more than one of
    // these files, take the file with the highest index in file list;
    read_setwriteflag(rifiles);
    }  // end   until all files have been processed

  // add bounding tag for the OSM file: "</osm>"
  write_stdout('<'); write_stdout('/');
  write_stdout('o'); write_stdout('s'); write_stdout('m');
  write_stdout('>');
  write_stdout_nl();
  write_flush();  // flush write buffer

  if(read_queryerror()!=0)
return 21;
  if(hash_queryerror()!=0)
return 12;
  return 0;
  }  // end   main()

