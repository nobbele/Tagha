#include <time.h>
#include <string.h>
#include "tagha_libc.h"

/* clock_t clock(void); */
static void native_clock(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)params;
	const clock_t cl = clock();
	memcpy(retval, &cl, sizeof cl);
}

/* time_t time(time_t *timer); */
static void native_time(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	const time_t t = time(params[0].Ptr);
	memcpy(retval, &t, sizeof t);
}

/* double difftime(time_t end, time_t beginning); */
static void native_difftime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	time_t end, begin;
	memcpy(&end, &params[0], sizeof end);
	memcpy(&begin, &params[1], sizeof begin);
	retval->Double = difftime(end, begin);
}

/* time_t mktime(struct tm *timeptr); */
static void native_mktime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	const time_t t = mktime(params[0].Ptr);
	memcpy(retval, &t, sizeof t);
}

/* char *asctime(const struct tm *timeptr); */
static void native_asctime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = asctime(params[0].Ptr);
}

/* char *ctime(const time_t *timer); */
static void native_ctime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = ctime(params[0].Ptr);
}

/* struct tm *gmtime(const time_t *timer); */
static void native_gmtime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = gmtime(params[0].Ptr);
}

/* struct tm *localtime(const time_t *timer); */
static void native_localtime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = localtime(params[0].Ptr);
}

/* size_t strftime(char *ptr, size_t maxsize, const char *format, const struct tm *timeptr); */
static void native_strftime(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = strftime(params[0].Ptr, params[1].UInt64, params[2].Ptr, params[3].Ptr);
}


bool tagha_module_load_time_natives(struct TaghaModule *const module)
{
	const struct TaghaNative libc_time_natives[] = {
		{"clock", native_clock},
		{"time", native_time},
		{"difftime", native_difftime},
		{"mktime", native_mktime},
		{"asctime", native_asctime},
		{"ctime", native_ctime},
		{"gmtime", native_gmtime},
		{"localtime", native_localtime},
		{"strftime", native_strftime},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, libc_time_natives) : false;
}

