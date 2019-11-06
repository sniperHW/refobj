#include "refobj.h"
#include <stdio.h>
#include <assert.h>


void destructor(void *obj) {
	printf("destructor\n");
}

int main() {

	setup_sigsegv();

	refobj *obj = malloc(sizeof(*obj));
	refobj_init(obj,destructor);
	ident ident = make_ident(obj);

	assert(NULL != cast2refobj(ident));

	printf("%u\n",refobj_dec(obj));

	printf("%u\n",refobj_dec(obj));

	assert(NULL == cast2refobj(ident));

	return 0;
}