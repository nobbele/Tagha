#include <string.h>
#include "tagha_libc.h"

/* void *memcpy(void *dest, const void *src, size_t num); */
static void native_memcpy(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = memcpy(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* void *memmove(void *dest, const void *src, size_t num); */
static void native_memmove(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = memmove(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* char *strcpy(char *dest, const char *src); */
static void native_strcpy(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strcpy(params[0].Ptr, params[1].Ptr);
}

/* char *strncpy(char *dest, const char *src, size_t num); */
static void native_strncpy(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strncpy(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* char *strcat(char *dest, const char *src); */
static void native_strcat(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strcat(params[0].Ptr, params[1].Ptr);
}

/* char *strncat(char *dest, const char *src, size_t num); */
static void native_strncat(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strncat(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* int memcmp(const void *ptr1, const void *ptr2, size_t num); */
static void native_memcmp(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = memcmp(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* int strcmp(const char *str1, const char *str2); */
static void native_strcmp(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = strcmp(params[0].Ptr, params[1].Ptr);
}

/* int strcoll(const char *str1, const char *str2); */
static void native_strcoll(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = strcoll(params[0].Ptr, params[1].Ptr);
}

/* int strncmp(const char *str1, const char *str2, size_t num); */
static void native_strncmp(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = strncmp(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* size_t strxfrm(char *dest, const char *src, size_t num); */
static void native_strxfrm(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = strxfrm(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* void *memchr(const void *ptr, int value, size_t num); */
static void native_memchr(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = memchr(params[0].Ptr, params[1].Int32, params[2].UInt64);
}

/* char *strchr(const char *str, int character); */
static void native_strchr(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strchr(params[0].Ptr, params[1].Int32);
}

/* size_t strcspn(const char *str1, const char *str2); */
static void native_strcspn(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = strcspn(params[0].Ptr, params[1].Ptr);
}

/* char *strpbrk(const char *str1, const char *str2); */
static void native_strpbrk(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strpbrk(params[0].Ptr, params[1].Ptr);
}

/* char *strrchr(const char *str, int character); */
static void native_strrchr(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strrchr(params[0].Ptr, params[1].Int32);
}

/* size_t strspn(const char *str1, const char *str2); */
static void native_strspn(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = strspn(params[0].Ptr, params[1].Ptr);
}

/* char *strstr(const char *str1, const char *str2); */
static void native_strstr(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strstr(params[0].Ptr, params[1].Ptr);
}

/* char *strtok(char *str, const char *delimiters); */
static void native_strtok(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strtok(params[0].Ptr, params[1].Ptr);
}

/* void *memset(void *ptr, int value, size_t num); */
static void native_memset(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = memset(params[0].Ptr, params[1].Int32, params[1].UInt64);
}

/* char *strerror(int errnum); */
static void native_strerror(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = strerror(params[0].Int32);
}

/* size_t strlen(const char * str); */
static void native_strlen(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = strlen(params[0].Ptr);
}


bool tagha_module_load_string_natives(struct TaghaModule *const module)
{
	const struct TaghaNative libc_string_natives[] = {
		{"memcpy", native_memcpy},
		{"memmove", native_memmove},
		{"strcpy", native_strcpy},
		{"strncpy", native_strncpy},
		{"strcat", native_strcat},
		{"strncat", native_strncat},
		{"memcmp", native_memcmp},
		{"strcmp", native_strcmp},
		{"strcoll", native_strcoll},
		{"strncmp", native_strncmp},
		{"strxfrm", native_strxfrm},
		{"memchr", native_memchr},
		{"strchr", native_strchr},
		{"strcspn", native_strcspn},
		{"strpbrk", native_strpbrk},
		{"strrchr", native_strrchr},
		{"strspn", native_strspn},
		{"strstr", native_strstr},
		{"strtok", native_strtok},
		{"memset", native_memset},
		{"strerror", native_strerror},
		{"strlen", native_strlen},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, libc_string_natives) : false;
}
