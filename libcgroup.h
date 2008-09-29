/*
 * Copyright IBM Corporation. 2007
 *
 * Author:	Balbir Singh <balbir@linux.vnet.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _LIBCG_H
#define _LIBCG_H

#include <features.h>

__BEGIN_DECLS

#include <grp.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <linux/cn_proc.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __USE_GNU
#define __USE_GNU
#endif

/* Maximum number of mount points/controllers */
#define MAX_MNT_ELEMENTS	8
/* Estimated number of groups created */
#define MAX_GROUP_ELEMENTS	128

#ifdef DEBUG
#define dbg(x...) printf(x)
#else
#define dbg(x...) do {} while(0)
#endif

/*
 * NOTE: Wide characters are not supported at the moment. Wide character support
 * would require us to use a scanner/parser that can parse beyond ASCII
 */


/* Maximum length of a line in the daemon config file */
#define CGROUP_RULE_MAXLINE (FILENAME_MAX + LOGIN_NAME_MAX + \
	CG_CONTROLLER_MAX + 3)

/* Definitions for the uid and gid members of a cgroup_rules */
#define CGRULE_INVALID (-1)
#define CGRULE_WILD (-2)

/* Flags for cgroup_change_cgroup_uid_gid() */
enum cgflags {
	CGFLAG_USECACHE = 0x01,
};

enum cgroup_errors {
	ECGROUPNOTCOMPILED=50000,
	ECGROUPNOTMOUNTED,
	ECGROUPNOTEXIST,
	ECGROUPNOTCREATED,
	ECGROUPSUBSYSNOTMOUNTED,
	ECGROUPNOTOWNER,
	ECGROUPMULTIMOUNTED,/* Controllers bound to different mount points */
	ECGROUPNOTALLOWED,  /* This is the stock error. Default error. */
	ECGMAXVALUESEXCEEDED,
	ECGCONTROLLEREXISTS,
	ECGVALUEEXISTS,
	ECGINVAL,
	ECGCONTROLLERCREATEFAILED,
	ECGFAIL,
	ECGROUPNOTINITIALIZED,
	ECGROUPVALUENOTEXIST,
	/* Represents error coming from other libraries like glibc. libcgroup
	 * users need to check errno upon encoutering ECGOTHER.
	 */
	ECGOTHER,
	ECGROUPNOTEQUAL,
	ECGCONTROLLERNOTEQUAL,
	ECGROUPPARSEFAIL, /* Failed to parse rules configuration file. */
	ECGROUPNORULES, /* Rules list does not exist. */
	ECGMOUNTFAIL,
};

#define CG_NV_MAX 100
#define CG_CONTROLLER_MAX 100
#define CG_VALUE_MAX 100
/* Max number of mounted hierarchies. Event if one controller is mounted per
 * hier, it can not exceed CG_CONTROLLER_MAX
 */
#define CG_HIER_MAX  CG_CONTROLLER_MAX

/* Functions and structures that can be used by the application*/
struct cgroup;
struct cgroup_controller;

int cgroup_init(void);
int cgroup_attach_task(struct cgroup *cgroup);
int cgroup_modify_cgroup(struct cgroup *cgroup);
int cgroup_create_cgroup(struct cgroup *cgroup, int ignore_ownership);
int cgroup_delete_cgroup(struct cgroup *cgroup, int ignore_migration);
int cgroup_attach_task_pid(struct cgroup *cgroup, pid_t tid);
struct cgroup *cgroup_get_cgroup(struct cgroup *cgroup);
int cgroup_create_cgroup_from_parent(struct cgroup *cgroup, int ignore_ownership);
int cgroup_copy_cgroup(struct cgroup *dst, struct cgroup *src);

/**
 * Changes the cgroup of a program based on the rules in the config file.  If a
 * rule exists for the given UID or GID, then the given PID is placed into the
 * correct group.  By default, this function parses the configuration file each
 * time it is called.
 * 
 * The flags can alter the behavior of this function:
 *      CGFLAG_USECACHE: Use cached rules instead of parsing the config file
 * 
 * This function may NOT be thread safe.
 * 	@param uid The UID to match
 * 	@param gid The GID to match
 * 	@param pid The PID of the process to move
 * 	@param flags Bit flags to change the behavior, as defined above
 * 	@return 0 on success, > 0 on error
 * TODO: Determine thread-safeness and fix if not safe.
 */
int cgroup_change_cgroup_uid_gid_flags(const uid_t uid, const gid_t gid,
				const pid_t pid, const int flags);

/**
 * Provides backwards-compatibility with older versions of the API.  This
 * function is deprecated, and cgroup_change_cgroup_uid_gid_flags() should be
 * used instead.  In fact, this function simply calls the newer one with flags
 * set to 0 (none).
 * 	@param uid The UID to match
 * 	@param gid The GID to match
 * 	@param pid The PID of the process to move
 * 	@return 0 on success, > 0 on error
 * 
 */
int cgroup_change_cgroup_uid_gid(uid_t uid, gid_t gid, pid_t pid);

/**
 * Changes the cgroup of a program based on the path provided.  In this case,
 * the user must already know into which cgroup the task should be placed and
 * no rules will be parsed.
 *
 *  returns 0 on success.
 */
int cgroup_change_cgroup_path(char *path, pid_t pid, char *controllers[]);

/**
 * Print the cached rules table.  This function should be called only after
 * first calling cgroup_parse_config(), but it will work with an empty rule
 * list.
 * 	@param fp The file stream to print to
 */
void cgroup_print_rules_config(FILE *fp);

/**
 * Reloads the rules list, using the given configuration file.  This function
 * is probably NOT thread safe (calls cgroup_parse_rules_config()).
 * 	@return 0 on success, > 0 on failure
 */
int cgroup_reload_cached_rules(void);

/**
 * Initializes the rules cache.
 * 	@return 0 on success, > 0 on failure
 */
int cgroup_init_rules_cache(void);

/**
 * Get the current cgroup path where the task specified by pid_t pid
 * has been classified
 */
int cgroup_get_current_controller_path(pid_t pid, const char *controller,
					char **current_path);

/* The wrappers for filling libcg structures */

struct cgroup *cgroup_new_cgroup(const char *name);
struct cgroup_controller *cgroup_add_controller(struct cgroup *cgroup,
						const char *name);
void cgroup_free(struct cgroup **cgroup);
void cgroup_free_controllers(struct cgroup *cgroup);
int cgroup_add_value_string(struct cgroup_controller *controller,
				const char *name, const char *value);
int cgroup_add_value_int64(struct cgroup_controller *controller,
				const char *name, int64_t value);
int cgroup_add_value_uint64(struct cgroup_controller *controller,
				const char *name, u_int64_t value);
int cgroup_add_value_bool(struct cgroup_controller *controller,
				const char *name, bool value);
int cgroup_compare_cgroup(struct cgroup *cgroup_a, struct cgroup *cgroup_b);
int cgroup_compare_controllers(struct cgroup_controller *cgca,
					struct cgroup_controller *cgcb);
int cgroup_set_uid_gid(struct cgroup *cgroup, uid_t tasks_uid, gid_t tasks_gid,
					uid_t control_uid, gid_t control_gid);
int cgroup_get_uid_gid(struct cgroup *cgroup, uid_t *tasks_uid,
		gid_t *tasks_gid, uid_t *control_uid, gid_t *control_gid);
int cgroup_get_value_string(struct cgroup_controller *controller,
					const char *name, char **value);
int cgroup_set_value_string(struct cgroup_controller *controller,
					const char *name, const char *value);
int cgroup_get_value_int64(struct cgroup_controller *controller,
					const char *name, int64_t *value);
int cgroup_set_value_int64(struct cgroup_controller *controller,
					const char *name, int64_t value);
int cgroup_get_value_uint64(struct cgroup_controller *controller,
					const char *name, u_int64_t *value);
int cgroup_set_value_uint64(struct cgroup_controller *controller,
					const char *name, u_int64_t value);
int cgroup_get_value_bool(struct cgroup_controller *controller,
						const char *name, bool *value);
int cgroup_set_value_bool(struct cgroup_controller *controller,
						const char *name, bool value);
/*
 * Config related stuff
 */
int cgroup_config_load_config(const char *pathname);

__END_DECLS

#endif /* _LIBCG_H  */
