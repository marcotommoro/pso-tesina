NAME := es
UTILS_FILE := "../utils/utils.c"

all: build run

build:
	gcc $(NAME).c $(UTILS_FILE)  -o $(NAME) -lpthread -w
run:
	./$(NAME)
