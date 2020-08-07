LIBS += -lpthread -lrt

all: test userspace test_app

test: 
	gcc test.c -o test

userspace:
	gcc userspace.c -o userspace $(LIBS)

test_app:
	gcc -o test_app test_app.c


clean:
	rm -f test userspace test_app
