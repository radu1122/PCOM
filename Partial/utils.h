#define MAX_LEN 1600
#define STDIN 0
#define TOPIC_SIZE 50
#define MAX_CLIENTS 30

#define DIE(condition, message) \
	do { \
		if ((condition)) { \
			fprintf(stderr, "[%d]: %s\n", __LINE__, (message)); \
			perror(""); \
			exit(1); \
		} \
	} while (0)

