lib_LTLIBRARIES = libthpool.la
libthpool_la_SOURCES = thpool.c
libthpool_la_LDFLAGS = -lpthread
include_HEADERS = thpool.h

libthpool_la_CFLAGS = -Wextra -pedantic -ansi -Werror -Wdouble-promotion \
                   -Wformat=2 -Wswitch-default -Wswitch-enum -Wuninitialized \
                   -Wstrict-aliasing=1 -Wstrict-overflow=5 \
                   -Wsuggest-attribute=pure -Wsuggest-attribute=const \
                   -Wsuggest-attribute=noreturn -Wsuggest-attribute=format \
                   -Warray-bounds=2 -Wtrampolines -Wfloat-equal \
                   -Wtraditional-conversion -Wdeclaration-after-statement \
                   -Wundef -Wshadow -Wunsafe-loop-optimizations \
                   -Wbad-function-cast -Wcast-qual -Wcast-align \
                   -Wwrite-strings -Wconversion -Wjump-misses-init \
                   -Wlogical-op -Waggregate-return \
                   -Wno-aggressive-loop-optimizations -Wstrict-prototypes \
                   -Wmissing-prototypes -Wmissing-declarations -Wpacked \
                   -Wredundant-decls -Wnested-externs -Winline \
                   -Wvector-operation-performance -Wdisabled-optimization \
                   -Wstack-protector -Wunsuffixed-float-constants \
                   -Winit-self
