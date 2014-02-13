TARGETS = dwgrep

DIRS = .

ALLSOURCES = $(foreach dir,$(DIRS),					   \
		$(wildcard $(dir)/*.cc $(dir)/*.hh $(dir)/*.c $(dir)/*.h)) \
	     Makefile

CCSOURCES = $(filter %.cc,$(ALLSOURCES))
DEPFILES = $(patsubst %.cc,%.cc-dep,$(CCSOURCES))

CXXFLAGS = -g -Wall
CFLAGS = -g -Wall

all: $(TARGETS)

%.cc-dep $(TARGETS): override CXXFLAGS += -std=c++11 -I /usr/include/elfutils/
$(TARGETS): override LDFLAGS += -ldw -lelf

dwgrep: dwgrep.o value.o patx.o wset.o

-include $(DEPFILES)

%.cc-dep: %.cc
	$(CXX) $(CXXFLAGS) -MM -MT '$(<:%.cc=%.o) $@' $< > $@

$(TARGETS):
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(foreach dir,$(DIRS),$(dir)/*.o $(dir)/*.*-dep) $(TARGETS)

.PHONY: all clean
