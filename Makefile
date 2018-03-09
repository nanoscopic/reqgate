CC = g++
CFLAGS = -Wall -O2

# LIB = -lpthread
LIB = -lnanomsg -Lnanomsg
XMLBARE = xmlbare/red_black_tree.c xmlbare/string_tree.c xmlbare/parser.c xmlbare/stack.c xmlbare/misc.c 
HFILES = reggate.h lumith.h            thread_data.h str_buffer.h base64.h                                 static.h server.h request.h urlpath.h response.h multipart-parser-c/multipart_parser.h postdata.h cookies.h
CFILES = reqgate.c lumith.c $(XMLBARE) thread_data.c str_buffer.c base64.c picohttpparser/picohttpparser.c static.c server.c request.c urlpath.c response.c multipart-parser-c/multipart_parser.c postdata.c cookies.c

all: reqgate

tiny: $(CFILES) $(HFILES)
	$(CC) -O3 $(CFLAGS) -o tiny $(CFILES) -Inanomsg/include $(LIB)

clean:
	rm -f *.o tiny *~
