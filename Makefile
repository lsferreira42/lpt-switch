ll:
	@g++ -ggdb -o lptdriver main.c -L/usr/X11R6/lib -lX11 -Llib/parapin -lparapin -Ilib/parapin
