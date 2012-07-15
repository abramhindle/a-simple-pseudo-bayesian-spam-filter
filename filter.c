#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

/* Screw seperate complication */
#include "lookup3.c"

#define INIT "-init"
#define INIT_LENGTH 5

#define SPAM "-spam"
#define SPAM_LENGTH 5

#define HAM "-ham"
#define HAM_LENGTH 4

#define CLASS "-class"
#define CLASS_LENGTH 6

#define HASH_INIT 54334561

/* define ENTRIES 1000000*/
/* define ENTRIES 100000 */
/* define ENTRIES 1048576 */
#define ENTRIES 131072
/* define ENTRIES 10000 */
#define SIZE_OF_ENTRIES sizeof(int) * ENTRIES

#define HASH_ID 762
#define min(x,y) ((x > y)?y:x)
#define max(x,y) ((x > y)?y:x)


struct hash {
	int id;
	int * ham;
	int * spam;
	int fdham;
	int fdspam;
};


struct hash open_hashes() {
	struct hash h;
	h.fdham = open(".ham.db", O_RDWR | O_CREAT,0600);
        if (h.fdham == -1) {
            perror("fdham");
            exit(1);
        }
	h.fdspam = open(".spam.db", O_RDWR | O_CREAT,0600);
        if (h.fdspam == -1) {
            perror("fdspam");
            exit(1);
        }
	
	h.id = HASH_ID;
	/* ERROR CHECK? */
 	h.ham = mmap((caddr_t)0, SIZE_OF_ENTRIES, PROT_READ | PROT_WRITE , MAP_SHARED, h.fdham, 0);
	if (h.ham == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
 	h.spam = mmap((caddr_t)0, SIZE_OF_ENTRIES, PROT_READ | PROT_WRITE , MAP_SHARED, h.fdspam, 0);
	if (h.spam == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	return h;
}

void close_hash(struct hash h) {
	assert(h.id == HASH_ID);
	munmap(h.ham, SIZE_OF_ENTRIES);
	munmap(h.spam, SIZE_OF_ENTRIES);
	close(h.fdham);
	close(h.fdspam);
}

int create_hash() {
	struct hash data;
	int result;
	data = open_hashes();
	assert(data.id == HASH_ID);
	result = lseek(data.fdspam, SIZE_OF_ENTRIES - 1, SEEK_SET);
 	if (result == -1) {
		close(data.fdspam);
		perror("Error calling lseek() to 'stretch' the file");
		exit(EXIT_FAILURE);
	}
	result = lseek(data.fdham, SIZE_OF_ENTRIES - 1, SEEK_SET);
 	if (result == -1) {
		close(data.fdham);
		perror("Error calling lseek() to 'stretch' the file");
		exit(EXIT_FAILURE);
	}
	result = write(data.fdham, "", 1);
	result = write(data.fdspam, "", 1);
	
	memset(data.ham, 0, sizeof(int) * ENTRIES);
	memset(data.spam, 0, sizeof(int) * ENTRIES);

	close_hash(data);
	return 0;
}

/* PUT A HASH HERE! */
int hash_index(int input) {
	return abs(
		hashlittle(&input, sizeof(int), HASH_INIT)
		% ENTRIES
	);
	/* return abs(input % ENTRIES); */
}

void insert_entry( int * arr, int input) {
	int index = hash_index(input);
	/* printf("Entry [%d] [%d] \n",index,arr[index]); */
	arr[index] = arr[index] + 1;
}
int get_entry( int * arr, int input) {
	int index = hash_index(input);
	/* printf("Entry [%d] [%d] \n",index,arr[index]); */
	return arr[index];
}
void insert_ham( struct hash * h, int input) {
	insert_entry(h->ham,input);
}
void insert_spam( struct hash * h, int input) {
	insert_entry(h->spam,input);
}
int sum(int * v) {
	int i = 0;
	int res = 0;
	for ( i = 0 ; i < ENTRIES ; i++ ) {
		res += v[i];
	}
	return res;
}
void sums(struct hash * h, int * spamcnt, int * hamcnt) {
	*spamcnt = sum(h->spam);
	*hamcnt = sum(h->ham);
}
int learn_from_message(int spam, int ham) {
	struct hash h;
	int input = 0;
	int ch = 0;

	h = open_hashes();
	assert(h.id == HASH_ID);
	
	while ((ch = getchar()) != EOF) {
		input = (ch | (input << 8*(sizeof(char))));
		/* printf("%d\n",input);*/
		if (spam) {
			insert_spam(&h, input);
		}
		if (ham) {
			insert_ham(&h, input);
		}
	}

	close_hash(h);
	
	return 0;
}
int classify_message() {
	int count = 0;
	int spam_count = 0;
	int ham_count = 0;
	int total_spam_count = 0;
	int total_ham_count = 0;
	double spamish = 0.0;
	double hamish = 0.0;
	int spamv  = 0;
	int hamv = 0;

	/* used for product bayes */
	double pprod = 1.0;
	double qprod = 1.0;
	double bayes = 0.0;

	/* used for logarithmic bayes */
	double fhamv = 0.0;
	double fspamv = 0.0;
	double fbayes = 0.0;

	double f_total_spam_count = 1.0;
	double f_total_ham_count = 1.0;
	/* CLONE START */
	struct hash h;
	int input = 0;
	int ch = 0;

	h = open_hashes();
	assert(h.id == HASH_ID);

	sums(&h, &total_spam_count, &total_ham_count);
	f_total_spam_count = (double) total_spam_count;
	f_total_ham_count = (double) total_ham_count;

	fbayes = log((f_total_spam_count / (f_total_spam_count + f_total_ham_count)) / (f_total_ham_count / (f_total_spam_count + f_total_ham_count)));

	while ((ch = getchar()) != EOF) {
		input = (ch | (input << 8*(sizeof(char))));
		/* CLONE END */
		/* printf("C %d\n",input);  */
		count++;
		hamv = get_entry(h.ham, input);
		ham_count += hamv;
		spamv = get_entry(h.spam, input);
		spam_count += spamv;
		
		fspamv = spamv / f_total_spam_count;
		fhamv  = hamv  / f_total_ham_count;

		
		if (fspamv > 0.0 && fhamv > 0.0) {
			fbayes += log(fspamv / fhamv);
			/* printf("NEW FBAYES %e\n",fbayes); */
		}

		if ((hamv + spamv) > 5) {	
			bayes = (max(0.01,
					min(0.99,		
						((min(1 , (spamv / f_total_spam_count))) / 
							((min(1.0, (spamv / f_total_spam_count))) + (min(1.0, (hamv / f_total_ham_count))))))));
			pprod += bayes;
			qprod += (1.0 - bayes);
		} else {
			/* printf("WARNING %d %d\n",spamv, hamv); */
		}
		/* printf("%e %e %e\n",bayes, pprod,qprod); */
		

		/* CLONE START */
	}

	spamish = spam_count/(1.0*total_spam_count);
	hamish  = ham_count/(1.0*total_ham_count);

	bayes = pprod / (pprod + qprod);
	/* bayes  = spamish / (1.0*(hamish + spamish)); */

	close_hash(h);
	printf("Spam: %d Ham: %d  Count: %d\n",spam_count,ham_count,count);
	printf("TOTAL Spam: %d Ham: %d  Count: %d\n", total_spam_count, total_ham_count, count);
	printf("Spam: %f Ham: %f  Count: %d\n", spamish, hamish, count);
	printf("Bayes: %e\n", bayes);
	printf("Log-Bayes: %e\n", fbayes);
	if (fbayes > 0.0) { return 1; } else { return 0; } 
	if (bayes > 0.5 ) { return 1; } else { return 0; }
	/* CLONE END */
}
int read_as_spam() {
	return learn_from_message(1,0);
}
int read_as_ham() {
	return learn_from_message(0,1);
}


int main(int argc, char ** argv) {
	if (argc > 1 && 0 == strncmp(argv[1], INIT, INIT_LENGTH)) {
		puts("INIT!\n");
		create_hash();
		exit(0);
	} else if (argc > 1 && 0 == strncmp(argv[1], SPAM, SPAM_LENGTH)) {
		puts("SPAM!\n");
		read_as_spam();
		exit(0);
	} else if (argc > 1 && 0 == strncmp(argv[1], HAM, HAM_LENGTH)) {
		puts("HAM!\n");
		read_as_ham();
		exit(0);
	} else if (argc==1 || (argc > 1 && 0 == strncmp(argv[1], CLASS, CLASS_LENGTH))) {
		puts("Classifier!\n");
		return classify_message();
	}
	return 0;
}
