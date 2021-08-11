sortbootorder: main.c ui_screen.c boot_data.c ui_options.c utils.c \
               ui_records.c cbfs.c
	$(CC) -o $@ -Werror -g $^ -lcurses -lreadline
