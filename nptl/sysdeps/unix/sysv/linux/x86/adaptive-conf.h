#ifndef _ADAPTIVE_CONF_H
#define _ADAPTIVE_CONF_H 1

/* Should make sure there is no false sharing on this */

struct hle_adaptive_config 
{
  int retry_lock_busy;
  int retry_lock_internal_abort;
  int retry_try_xbegin;
  int retry_trylock_internal_abort;
};

extern struct hle_adaptive_config __hle_aconf attribute_hidden;
extern void hle_aconf_setup(const char *s) attribute_hidden;

typedef void (*tsx_abort_hook_t) (unsigned);
extern tsx_abort_hook_t __tsx_abort_hook;

#endif
