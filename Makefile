TARGETS = dwgrep parser

DIRS = .

ALLSOURCES = $(foreach dir,$(DIRS),					   \
		$(wildcard $(dir)/*.cc $(dir)/*.hh $(dir)/*.c $(dir)/*.h)  \
			   $(dir)/*.l $(dir)/*.y)			   \
	     Makefile

CCSOURCES = $(filter %.cc,$(ALLSOURCES))
DEPFILES = $(patsubst %.cc,%.cc-dep,$(CCSOURCES))

YACC = bison

all: $(TARGETS)

%.cc-dep $(TARGETS): override CXXFLAGS := -g3 -Wall -std=c++11 -I /usr/include/elfutils/
dwgrep: override LDFLAGS += -ldw -lelf

dwgrep: dwgrep.o value.o patx.o wset.o
parser: parser.o lexer.o tree.o

%.hh %.cc: %.yy
	$(YACC) -d -o $(<:%.yy=%.cc) $<

%.cc: %.ll
	$(LEX) $<

-include $(DEPFILES)

%.cc-dep: %.cc
	$(CXX) $(CXXFLAGS) -MM -MT '$(<:%.cc=%.o) $@' $< > $@

$(TARGETS):
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(foreach dir,$(DIRS),$(dir)/*.o $(dir)/*.*-dep) $(TARGETS)

.PHONY: all clean
