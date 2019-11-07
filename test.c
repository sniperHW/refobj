#include "refobj.h"
#include <stdio.h>
#include <assert.h>
#include "time.h"


void destructor(void *obj) {
	printf("destructor\n");
	free(obj);
}

int main() {

	setup_sigsegv();

	refobj *obj = malloc(sizeof(*obj));
	refobj_init(obj,destructor);
	ident ident = make_ident(obj);

	assert(NULL != cast2refobj(ident));

	printf("%u\n",refobj_dec(obj));


	uint64_t begin = chk_accurate_tick64();
	for(int i = 0;i < 1000000;i++) {
		cast2refobj(ident);
		refobj_dec(obj);
	}
	uint64_t end = chk_accurate_tick64();

	printf("%llums\n",end-begin);

	printf("%u\n",refobj_dec(obj));

	assert(NULL == cast2refobj(ident));



	return 0;
}