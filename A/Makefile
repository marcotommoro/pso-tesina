NAME := es
UTILS_FILE := "../utils/utils.c"

all: clean build

build:
	gcc $(NAME).c $(UTILS_FILE) -o $(NAME) -lpthread -w

clean:
	rm -f es