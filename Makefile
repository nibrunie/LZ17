LIB_SOURCES=lib/compress.c
LIB_OBJECTS=$(LIB_SOURCES:.c=.o)

LIB_ARITH_CODING = ../ArithmeticCoding/libarithcoding.a
ARITH_CODING_PATH = ../ArithmeticCoding/lib/

TEST_LIST= test_buffer
CFLAGS += -g -O0

%.o:%.c 
	$(CC) $(CFLAGS)  -I. -I$(ARITH_CODING_PATH) -c -o $@ $^

liblz17.a: $(LIB_OBJECTS)
	$(AR) rcs $@ $^

clean: $(LIB_OBJECTS) liblz17.a
	rm -f $^ $(TEST_LIST)

lz17utils: liblz17.a utils/comp_util.c
	$(CC) $(CFLAGS) $(LFLAGS) -I$(ARITH_CODING_PATH) -Ilib utils/comp_util.c -o lz17utils liblz17.a $(LIB_ARITH_CODING)

define gen_test_rule
$(1): tests/$(1).o liblz17.a
	$(CC) $(CFLAGS) tests/$(1).o -o $(1) liblz17.a $(LIB_ARITH_CODING)

test: $(1)
	./$(1)
endef

$(foreach test_name,$(TEST_LIST),$(eval $(call gen_test_rule,$(test_name))))

