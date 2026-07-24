TEST_ENTRIES = \
	\
	no-arg \
	\
	a-option \
	charset-Shift_JIS-option \
	d-option \
	filelimit-option-exceeded \
	filelimit-option-not-exceeded \
	f-option \
	gitignore-option \
	ignore-case-option \
	info-option \
	infofile-option \
	I-option \
	L1-option \
	L1-R-options \
	L2-option \
	L2-R-options \
	l-option \
	matchdirs-option \
	noreport-option \
	o-option \
	P-option \
	prune-option \
	x-option \
	\
	$(NULL)

TEST_DIRECTORY := /tmp/tree-$(shell id -un)

all:
	@echo "***** tests are launched ******"
	if [ -d $(TEST_DIRECTORY) ]; then rm -rf $(TEST_DIRECTORY); fi
	mkdir -p $(TEST_DIRECTORY)
	@FAILED=0; \
	for ENTRY in $(TEST_ENTRIES); do if ! bash $${ENTRY}/test.bash; then \
		FAILED=1; \
	fi; done; \
	echo "***** tests were finished: $${FAILED} ******"; exit "$${FAILED}"

# Run update in order to update expected directories
update:
	@for ENTRY in $(TEST_ENTRIES); do \
	    rm -rf $${ENTRY}/expected; \
	    mv $${ENTRY}/actual $${ENTRY}/expected; \
	done
	mkdir L1-R-options/expected/b
	cp $(TEST_DIRECTORY)/L1-R-options/b/00Tree.html L1-R-options/expected/b/
	mkdir -p L1-R-options/expected/h/j
	cp $(TEST_DIRECTORY)/L1-R-options/h/00Tree.html L1-R-options/expected/h/
	cp $(TEST_DIRECTORY)/L1-R-options/h/j/00Tree.html L1-R-options/expected/h/j/

clean:
	@for ENTRY in $(TEST_ENTRIES); do rm -rf $${ENTRY}/actual; done
	rm -rf $(TEST_DIRECTORY)
