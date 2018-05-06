#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

// Agent
struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;

  uthread_cond_t  smoker_match;
  uthread_cond_t  smoker_paper;
  uthread_cond_t  smoker_tobacco;
  uthread_cond_t  ready;
  
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);

  agent->smoker_match = uthread_cond_create(agent->mutex);
  agent->smoker_paper = uthread_cond_create(agent->mutex);
  agent->smoker_tobacco = uthread_cond_create(agent->mutex);
  agent->ready = uthread_cond_create(agent->mutex);
  return agent;
}

//Smoker
struct Smoker {
	int type;
	struct Agent* agent;
};

struct Smoker* createSmoker(int av, struct Agent* agent){
	struct Smoker* smoker = malloc(sizeof(struct Smoker));
	smoker->type = av;
	smoker->agent = agent;
	return smoker;
}

//Signaler
struct Signaler{
	int ready_count;
	struct Agent* agent;
};

struct Signaler* createSignaler(struct Agent* agent){
	struct Signaler* signaler = malloc(sizeof(struct Signaler));
	signaler->ready_count     = 0;
	signaler->agent           = agent;
	return signaler;
}



//
// TODO
// You will probably need to add some procedures and struct etc.
//


void* smoker_match(void* av){      // Requires paper & tobacco (6)
	struct Smoker* sm = av;
	uthread_mutex_lock(sm->agent->mutex);
	while(1){
		printf("%s\n","Smoker_match is about to WAIT!");
		uthread_cond_wait(sm->agent->smoker_match);
		printf("%s\n","Smoker_match is SIGNALLED!");
		uthread_cond_signal(sm->agent->smoke);
		smoke_count[MATCH]++;
		printf("%s\n","Smoker_match SMOKED!!!!!!!!!");
	}
	
	uthread_mutex_unlock(sm->agent->mutex);
	return NULL;
}

void* smoker_paper(void* av){      // Requires match & tobacco (5)
	struct Smoker* sm = av;
	uthread_mutex_lock(sm->agent->mutex);
	while(1){
		printf("%s\n","Smoker_paper is about to WAIT!");
		uthread_cond_wait(sm->agent->smoker_paper);
		printf("%s\n","Smoker_paper is SIGNALLED!");
		uthread_cond_signal(sm->agent->smoke);
		smoke_count[PAPER]++;
		printf("%s\n","Smoker_paper SMOKED!!!!!!!!!");
	}
	
	uthread_mutex_unlock(sm->agent->mutex);

	return NULL;
}

void* smoker_tobacco(void* av){    // Requires match & paper (3)
	struct Smoker* sm = av;
	uthread_mutex_lock(sm->agent->mutex);
	while(1){
		printf("%s\n","Smoker_tobacco is about to WAIT!");
		uthread_cond_wait(sm->agent->smoker_tobacco);
		printf("%s\n","Smoker_tobacco is SIGNALLED!");
		uthread_cond_signal(sm->agent->smoke);
		smoke_count[TOBACCO]++;
		printf("%s\n","Smoker_tobacco SMOKED!!!!!!!!!");
	}
	
	uthread_mutex_unlock(sm->agent->mutex);
	return NULL;
}


// Signaler
void* signaler_match(void* sig){
	struct Signaler* si = sig;
	uthread_mutex_lock(si->agent->mutex);
	while(1){
		printf("%s\n","signaler_match is waiting!");
		uthread_cond_wait(si->agent->match);
		printf("signaler_match Before ADD SUM: %d\n", si->ready_count);
		si->ready_count+=1;
		printf("signaler_match After ADD SUM: %d\n", si->ready_count);
		if(si->ready_count==3){
			printf("%s\n","Signalling smoker_tobacco!");
			uthread_cond_signal(si->agent->smoker_tobacco);
			si->ready_count=0;
		}
		if(si->ready_count==5){
			printf("%s\n","Signalling smoker_paper!");
			uthread_cond_signal(si->agent->smoker_paper);
			si->ready_count=0;
		}
		

	}
	uthread_mutex_unlock(si->agent->mutex);
}

void* signaler_paper(void* sig){
	struct Signaler* si = sig;
	uthread_mutex_lock(si->agent->mutex);
	while(1){
		printf("%s\n","signaler_paper is waiting!");
		uthread_cond_wait(si->agent->paper);
		printf("signaler_paper Before ADD SUM: %d\n", si->ready_count);
		si->ready_count+=2;
		printf("signaler_paper After ADD SUM: %d\n", si->ready_count);
		if(si->ready_count==3){
			printf("%s\n","Signalling smoker_tobacco!");
			uthread_cond_signal(si->agent->smoker_tobacco);
			si->ready_count=0;
		}
		if(si->ready_count==6){
			printf("%s\n","Signalling smoker_match!");
			uthread_cond_signal(si->agent->smoker_match);
			si->ready_count=0;
		}
	}
	uthread_mutex_unlock(si->agent->mutex);
}

void* signaler_tobacco(void* sig){
	struct Signaler* si = sig;
	uthread_mutex_lock(si->agent->mutex);
	while(1){
		printf("%s\n","signaler_tobacco is waiting!");
		uthread_cond_wait(si->agent->tobacco);
		printf("signaler_tobacco Before ADD SUM: %d\n", si->ready_count);
		si->ready_count+=4;
		printf("signaler_tobacco After ADD SUM: %d\n", si->ready_count);
		if(si->ready_count==5){
			printf("%s\n","Signalling smoker_paper!");
			uthread_cond_signal(si->agent->smoker_paper);
			si->ready_count=0;
		}
		if(si->ready_count==6){
			printf("%s\n","Signalling smoker_match!");
			uthread_cond_signal(si->agent->smoker_match);
			si->ready_count=0;
		}
	}
	uthread_mutex_unlock(si->agent->mutex);
}


/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
		printf("%s\n","Match distributed!");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
		printf("%s\n","Paper distributed!");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
		printf("%s\n","Tobacco distributed!");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke); // Smoker smokes

    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[7];
  uthread_init (7);

  struct Agent*  a = createAgent();
  struct Signaler* s = createSignaler(a);
  struct Smoker* sm_m = createSmoker(1, a);
  struct Smoker* sm_p = createSmoker(2, a);
  struct Smoker* sm_to = createSmoker(4, a);


  t[0] = uthread_create(smoker_match, sm_p);
  t[1] = uthread_create(smoker_paper, sm_to);
  t[2] = uthread_create(smoker_tobacco, sm_m);


  t[3] = uthread_create(signaler_match, s);
  t[4] = uthread_create(signaler_paper, s);
  t[5] = uthread_create(signaler_tobacco, s);


  uthread_join (uthread_create (agent, a), 0);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}