#include<stdio.h>
#include "sandbox.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>



#ifndef INT16_MAX
#define INT16_MAX (32767)
#endif /* INT16_MAX */


#ifndef PROG_NAME
#define PROG_NAME "sample2"
#endif /* PROG_NAME */

/* result code translation table */
const char* result_name[] =
{
    "PD", "OK", "RF", "ML", "OL", "TL", "RT", "AT", "IE", "BP", NULL,
};

/* mini sandbox with embedded policy */
typedef action_t* (*rule_t)(const sandbox_t*, const event_t*, action_t*);
typedef struct
{
   sandbox_t sbox;
   policy_t default_policy;
   rule_t sc_table[INT16_MAX + 1];
} minisbox_t;

/* initialize and apply local policy rules */
void policy_setup(minisbox_t*);

typedef enum
{
    P_ELAPSED = 0, P_CPU = 1, P_MEMORY = 2,
} probe_t;

res_t probe(const sandbox_t*, probe_t);


int main(int argc, const char* argv[])
{
  if (argc < 2)
    {
      fprintf(stderr, "synopsis: " PROG_NAME " foo/bar.exe\n");
      return EX_USAGE;
    }

  minisbox_t msb;

  
  return 0;
}



