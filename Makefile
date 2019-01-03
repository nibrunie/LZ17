LIB_SOURCES=lib/compress.c
LIB_OBJECTS=$(LIB_SOURCES:.c=.o)

ARITH_CODING_DIR ?= ../ArithmeticCoding

LIB_ARITH_CODING = $(ARITH_CODING_DIR)/libarithcoding.a
ARITH_CODING_PATH = $(ARITH_CODING_DIR)/lib/

DEBUG_BUILD_CFLAGS = -Werror -g
RELEASE_BUILD_CFLAGS =

TEST_LIST= test_buffer

ifeq ($(BUILD_MODE),DEBUG)
CFLAGS += -Wall $(DEBUG_BUILD_CFLAGS) --pedantic -g -O0
else
CFLAGS += -Wall $(RELEASE_BUILD_CFLAGS) --pedantic -O0
endif

%.o:%.c
	$(CC) $(CFLAGS)  -I. -I$(ARITH_CODING_PATH) -c -o $@ $^

liblz17.a: $(LIB_OBJECTS)
	$(AR) rcs $@ $^

clean:
	rm -f $(LIB_OBJECTS) liblz17.a $(TEST_LIST)

lz17utils: liblz17.a utils/comp_util.c
	$(CC) $(CFLAGS) $(LFLAGS) -I$(ARITH_CODING_PATH) -Ilib utils/comp_util.c -o lz17utils liblz17.a $(LIB_ARITH_CODING)

define gen_test_rule
$(1): tests/$(1).o liblz17.a
	$(CC) $(CFLAGS) tests/$(1).o -o $(1) liblz17.a $(LIB_ARITH_CODING)

test: $(1)
	./$(1)
endef

$(foreach test_name,$(TEST_LIST),$(eval $(call gen_test_rule,$(test_name))))

