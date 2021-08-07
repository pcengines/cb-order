sortbootorder: main.c list_menu.c boot_order.c
	$(CC) -o $@ $^ -lcurses
