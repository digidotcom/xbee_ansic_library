#include <stdio.h>
#include "xbee/platform.h"
#include "../unittest.h"
#include "xbee/cbuf.h"

#define RXBUFSIZE 31

static xbee_cbuf_t *rxBuffer;
static uint8_t intRxBuffer[RXBUFSIZE + XBEE_CBUF_OVERHEAD];
uint8_t source_data[31];
uint8_t read_buffer[31];

void test_get_and_put( void)// This test shows various get and put calls
{
	rxBuffer = (xbee_cbuf_t *) intRxBuffer;
	xbee_cbuf_init(rxBuffer, RXBUFSIZE);
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 31),31, NULL, "put 1 failed"); // test 1 (fill completely)
	test_compare(xbee_cbuf_free(rxBuffer), 0, NULL, "check free 1 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 31, NULL, "check used 1 failed");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 1), 0, NULL, "put into full failed"); // test 6 (try to add another even though full, stores less than required)
	test_compare(xbee_cbuf_free(rxBuffer), 0, NULL, "check free 2 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 31, NULL, "check used 2 failed");
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 31), 31, NULL, "get from full failed"); // test 7 (get and empty whole buffer)
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 3 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 3 failed");
	test_bool(memcmp(read_buffer, source_data, 31) == 0, "Contents from the buffer are corrupted after 3");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 31), 31, NULL, "put 4 failed"); // same as 1 test 10
	test_compare(xbee_cbuf_free(rxBuffer), 0, NULL, "check free 4 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 31, NULL, "check used 4 failed"); // test 12
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 22), 22, NULL, "get from full 5 failed"); // test 13 (get an arbitrary amount)
	test_compare(xbee_cbuf_free(rxBuffer), 22, NULL, "check free 5 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 9, NULL, "check used 5 failed");
	test_bool( memcmp(read_buffer, source_data, 22) == 0, "Contents from the buffer are corrupted after 5");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 10), 10, NULL, "put 6 failed"); // test 16
	test_compare(xbee_cbuf_free(rxBuffer), 12, NULL, "check free 6 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 19, NULL, "check used 6 failed");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 31), 12, NULL, "put from full 7 failed"); // test 19 (stores less than required)
	test_compare(xbee_cbuf_free(rxBuffer), 0, NULL, "check free 7 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 31, NULL, "check used 7 failed");
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 31), 31, NULL, "get from full 8 failed"); // test 22
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 8 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 8 failed");
	test_bool((memcmp(read_buffer, source_data + 22, 9) == 0) 
		   && (memcmp(read_buffer + 9, source_data, 10) == 0) 
		   && (memcmp(read_buffer + 19, source_data, 12) == 0)
		   , "Contents from the buffer are corrupted after 8");
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 3), 0, NULL, "get from full 9 failed"); // test 25 (gets less than required)
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 9 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 9 failed");
}

void test_wrap_around(void) // This test show that put and get successfully wraps around
{
	rxBuffer = (xbee_cbuf_t *) intRxBuffer;
	xbee_cbuf_init(rxBuffer, RXBUFSIZE);	
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 1 failed"); // test 1 (see if initialized to 0)
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 1 failed");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 15), 15, NULL, "put into empty failed"); // test 3 
	test_compare(xbee_cbuf_free(rxBuffer), 16, NULL, "check free 2 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 15, NULL, "check used 2 failed");
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 3), 3, NULL, "get from full 3 failed"); // test 6 
	test_compare(xbee_cbuf_free(rxBuffer), 19, NULL, "check free 3 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 12, NULL, "check used 3 failed");
	test_bool( memcmp(read_buffer, source_data, 3) == 0, "Contents from the buffer are corrupted after 3");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 19), 19, NULL, "put 4 failed"); // test 9 test put wrap around
	test_compare(xbee_cbuf_free(rxBuffer), 0, NULL, "check free 4 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 31, NULL, "check used 4 failed");
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 31), 31, NULL, "get from full 4 failed"); // test 12 test get wrap around
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 4 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 4 failed");
	test_bool( (memcmp(read_buffer, source_data + 3, 12) == 0)
			&& (memcmp(read_buffer + 12, source_data, 17) == 0)
			&& (memcmp(read_buffer + 29, source_data + 17, 2) == 0)
			, "Contents from the buffer are corrupted after 4");
}

void test_zero_length(void) // test if 0 length is passed
{
	rxBuffer = (xbee_cbuf_t *) intRxBuffer;
	xbee_cbuf_init(rxBuffer, RXBUFSIZE);	
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 1 failed"); // test 1 (see if initialized to 0)
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 1 failed");
	
	test_compare(xbee_cbuf_put(rxBuffer, source_data, 0), 0, NULL, "put into empty failed"); // test 3 
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 2 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 2 failed");
	
	test_compare(xbee_cbuf_get(rxBuffer, read_buffer, 0), 0, NULL, "get from full 3 failed"); // test 6 
	test_compare(xbee_cbuf_free(rxBuffer), 31, NULL, "check free 3 failed");
	test_compare(xbee_cbuf_used(rxBuffer), 0, NULL, "check used 3 failed");
}

int main( int argc, char *argv[])
{
	int failures = 0;
	int i;
	for( i = 0; i < 31; i++)
		source_data[i] = i;
	failures += DO_TEST(test_get_and_put);
	failures += DO_TEST(test_wrap_around);
	failures += DO_TEST(test_zero_length);

	return test_exit( failures);
}
