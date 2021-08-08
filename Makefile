sortbootorder: main.c list_menu.c boot_order.c options.c utils.c
	$(CC) -o $@ $^ -lcurses -lreadline
