sortbootorder: main.c list_menu.c boot_order.c options.c
	$(CC) -o $@ $^ -lcurses
