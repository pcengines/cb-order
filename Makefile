CFLAGS := -I /usr/local/include -Wall -Wextra -MMD -MP -O3
LDFLAGS := -L /usr/local/lib -lcurses -lreadline

PRG := sortbootorder

SRC := cbfs.c boot_data.c main.c utils.c ui_screen.c ui_options.c ui_main.c \
       ui_records.c
SRC := $(addprefix src/,$(SRC))

OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)

.PHONY: all debug clean

all: $(PRG)

debug: CFLAGS += -O0 -g
debug: LDFLAGS += -g
debug: all

clean:
	-$(RM) $(OBJ) $(DEP)

$(PRG): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(DEP)
