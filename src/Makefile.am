if ENABLE_OSMCONVERT
bin_PROGRAMS = osmconvert osmfilter osmupdate
else
bin_PROGRAMS = osmfilter osmupdate
endif

if ENABLE_OSMCONVERT
osmconvert_SOURCES = osmconvert.c
osmconvert_LDFLAGS = -lz
endif

osmfilter_SOURCES = osmfilter.c

osmupdate_SOURCES = osmupdate.c
