
# Stream image viewer


## Required libraries

	sudo apt install libgtk-3-dev

Note that GTK+ version 3.22 or newer is required. To check GTK+ version:

	pkg-config --modversion gtk+-3.0




## Compilation

	mkdir _build
	cd _build
	cmake ..
	make
	sudo make install


## Running

Create test image:

	./mktestshmim

View image:

	./shmimview ims1

