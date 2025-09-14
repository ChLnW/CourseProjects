CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
CFLAGS += -pipe -MP -MMD

SRC = retbleed-assignment.c
OBJ = $(SRC:.c=.o)
EXECUTABLE = retbleed

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

verify_secret: verify_secret.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

submission: verify_secret
	@echo "Generating submission"
	./grade.sh

clean:
	rm -f $(OBJ) $(EXECUTABLE)
	rm -f verify_secret
	rm -f submission.txt
