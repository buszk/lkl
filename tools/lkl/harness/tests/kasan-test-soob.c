#include "generic_test.h"

int main() {
	kasan_test(KASAN_SOOB_TEST_ID);
	return 0;
}
