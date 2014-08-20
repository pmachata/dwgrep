void *foo (void *, void *);

void bar (void *ptr) {
	({
		void *_ptr = foo (0, &ptr);
		_ptr != 0;
	});
}
