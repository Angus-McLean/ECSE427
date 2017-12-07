#include <strings.h>


int nameValid(const char* name) {
	int length = strlen(name);
	if (length > MAX_FILE_NAME)
		return -1;
	char copy[length+1];
	memset(copy, '\0', sizeof(copy));
	strcpy(copy, name); // because strtok modifies the string

	char* token;
	const char s[2] = ".";
	token = strtok(copy, s); // token = "([]).[]"
	token = strtok(NULL, s); // token = "[].([])"
	if (strlen(token) > MAX_EXTENSION_NAME)
		return -1;
	return 0;
}
