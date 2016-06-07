LIB_SOURCES=lib/compress.c
LIB_OBJECTS=$(LIB_SOURCES:.c=.o)

TEST_LIST= test_buffer
CFLAGS += -g -O0

%.o:%.c 
	$(CC) $(CFLAGS)  -I. -c -o $@ $^

liblz17.a: $(LIB_OBJECTS)
	$(AR) rcs $@ $^

clean: $(LIB_OBJECTS) liblz17.a
	rm -f $^ $(TEST_LIST)

define gen_test_rule
$(1): tests/$(1).o liblz17.a
	$(CC) $(CFLAGS)  tests/$(1).o -o $(1)  liblz17.a

test: $(1)
	./$(1)
endef

$(foreach test_name,$(TEST_LIST),$(eval $(call gen_test_rule,$(test_name))))

