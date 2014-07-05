TARGETS = dwgrep test-parser

DIRS = .

ALLSOURCES = $(foreach dir,$(DIRS),					   \
		$(wildcard $(dir)/*.cc $(dir)/*.hh $(dir)/*.c $(dir)/*.h   \
			   $(dir)/*.ll $(dir)/*.yy)			   \
		)							   \
	     Makefile

CCSOURCES = $(filter %.cc,$(ALLSOURCES))
YYSOURCES = $(filter %.yy,$(ALLSOURCES))
LLSOURCES = $(filter %.ll,$(ALLSOURCES))
DEPFILES := $(patsubst %.ll,%.cc-dep,$(LLSOURCES))
DEPFILES := $(DEPFILES) $(patsubst %.yy,%.cc-dep,$(YYSOURCES))
DEPFILES := $(DEPFILES) $(patsubst %.cc,%.cc-dep,$(CCSOURCES))
CXXOPTFLAGS = -O2

YACC = bison

all: $(TARGETS)
check: dwgrep test-parser
	./test-parser
	(cd ./tests/; ./tests.sh)

%.cc-dep $(TARGETS): override CXXFLAGS = -g3 $(CXXOPTFLAGS) -Wall -std=c++11 -I /usr/include/elfutils/
dwgrep: override LDFLAGS += -ldw -lelf
dwgrep test-parser: override LDFLAGS += -lgmpxx -lgmp

dwgrep: dwgrep.o parser.o lexer.o tree.o tree_cr.o constant.o op.o	\
	valfile.o value.o build.o cache.o dwcst.o vfcst.o atval.o
test-parser: test-parser.o parser.o lexer.o tree.o tree_cr.o	\
	constant.o dwcst.o vfcst.o

test-parser.o: CXXOPTFLAGS = -O0

parser.cc: lexer.hh

dwcst.o: known-dwarf.h
known-dwarf.h: known-dwarf.awk /usr/include/dwarf.h
	gawk -f $^ > $@.new
	mv -f $@.new $@

%.hh %.cc: %.yy
	$(YACC) -d -o $(<:%.yy=%.cc) $<

%.hh %.cc: %.ll
	$(LEX) $<

-include $(DEPFILES)

%.cc-dep: %.cc
	$(CXX) $(CXXFLAGS) -MM -MT '$(<:%.cc=%.o) $@' $< > $@

$(TARGETS):
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(foreach dir,$(DIRS),$(dir)/*.o $(dir)/*.*-dep) \
		$(patsubst %.yy,%.cc,$(YYSOURCES)) \
		$(patsubst %.yy,%.hh,$(YYSOURCES)) \
		$(patsubst %.ll,%.cc,$(LLSOURCES)) \
		$(patsubst %.ll,%.hh,$(LLSOURCES)) \
		$(TARGETS)

.PHONY: all clean
