CFLAGS := -I /usr/local/include -I . -Wall -Wextra -MMD -MP -O3
LDFLAGS := -L /usr/local/lib -lcurses

PRG := cb-order

THIRD_PARTY := cbfs_image.c common.c fmap.c partitioned_file.c xdr.c
THIRD_PARTY := $(addprefix third-party/,$(THIRD_PARTY))

SRC := cbfs.c boot_data.c main.c utils.c ui_screen.c ui_options.c ui_main.c \
       ui_records.c
SRC := $(addprefix src/,$(SRC))

ALL_SRC := $(THIRD_PARTY) $(SRC)

OBJ := $(ALL_SRC:.c=.o)
DEP := $(ALL_SRC:.c=.d)

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
