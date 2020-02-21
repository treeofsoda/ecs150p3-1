# Target library
lib := libuthread.a
objs:= sem.o tps.o
statobjs:= queue.o thread.o


CC:= gcc
CFLAGS:= -Wall -Wextra -Werror
CFlags += -g
ifneq ($(V),1)
Q = @
endif
all: $(lib)
deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)
$(lib): $(objs) $(statobjs)
	ar rcs $@ $^
%.o: %.c
	@echo "CC $@"
	$(Q) $(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)
clean:    
	@echo "clean"
	$(Q) rm -f $(lib) $(targets) $(objs) $(deps)

## TODO: Phase 1 and Phase 2
