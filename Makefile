sortbootorder: main.c ui_screen.c boot_data.c options.c utils.c records.c cbfs.c
	$(CC) -o $@ -Werror -g $^ -lcurses -lreadline
