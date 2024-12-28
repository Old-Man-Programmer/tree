TEST_ENTRIES = \
	\
	no-arg \
	\
	a-option \
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

clean:
	@for ENTRY in $(TEST_ENTRIES); do rm -rf $${ENTRY}/actual; done
	rm -rf $(TEST_DIRECTORY)
