TARGETS = dwgrep test-parser test-int

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
check: dwgrep test-parser test-int
	./test-int
	./test-parser
	(cd ./tests/; ./tests.sh)

%.cc-dep $(TARGETS): override CXXFLAGS = -g3 $(CXXOPTFLAGS) -Wall	\
	-std=c++14 -I /usr/include/elfutils/

dwgrep: override LDFLAGS += -ldw -lelf
builtin-dw.o: override CXXFLAGS += -fno-var-tracking-assignments

dwgrep: dwgrep.o parser.o lexer.o stack.o tree.o tree_cr.o op.o		\
	build.o cache.o atval.o builtin.o builtin-shf.o builtin-dw.o	\
	builtin-closure.o builtin-cmp.o builtin-cst.o constant.o	\
	init.o int.o overload.o selector.o value.o value-closure.o	\
	value-cst.o value-dw.o value-seq.o value-str.o dwcst.o

test-parser: test-parser.o parser.o lexer.o stack.o tree.o tree_cr.o	\
	build.o constant.o init.o int.o builtin.o overload.o op.o	\
	selector.o value.o value-closure.o value-cst.o value-str.o	\
	value-seq.o builtin-shf.o builtin-closure.o builtin-cmp.o	\
	builtin-cst.o

test-int: test-int.o int.o

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
