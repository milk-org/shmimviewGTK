
# Stream image viewer


## Required libraries

	sudo apt install libgtk-3-dev

Note that GTK+ version 3.16 or newer is required. To check GTK+ version:

	pkg-config --modversion gtk+-3.0




## Compilation

	gcc -O2 shmimviewGTK.c `pkg-config --cflags --libs gtk+-3.0` -lm -lImageStreamIO -o shmimviewGTK


## Running

Create test image:

	./mktestshmim

View image:

	./shmimviewGTK ims1

