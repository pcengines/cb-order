sortbootorder: main.c list_menu.c boot_order.c options.c utils.c records.c \
			   cbfs.c
	$(CC) -o $@ -Werror -g $^ -lcurses -lreadline
