#include<stdio.h>

/* mini sandbox with embedded policy */
typedef action_t* (*rule_t)(const sandbox_t*, const event_t*, action_t*);
typedef struct
{
   sandbox_t sbox;
   policy_t default_policy;
   rule_t sc_table[INT16_MAX + 1];
} minisbox_t;

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

