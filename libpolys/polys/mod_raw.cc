/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/*
 * ABSTRACT: machine depend code for dynamic modules
 *
 * Provides: dynl_check_opened()
 *           dynl_open()
 *           dynl_sym()
 *           dynl_error()
 *           dynl_close()
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>


#ifdef HAVE_CONFIG_H
#include "libpolysconfig.h"
#endif /* HAVE_CONFIG_H */
#include <misc/auxiliary.h>

#include <omalloc/omalloc.h>

#include <reporter/reporter.h>

#include <resources/feResource.h>

#include "mod_raw.h"

#ifdef HAVE_STATIC
#undef HAVE_DL
#endif
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if defined(HAVE_DL)

/*****************************************************************************
 *
 * General section
 * These are just wrappers around the repsective dynl_* calls
 * which look for the binary in the bin_dir of Singular and ommit warnings if
 * somethings goes wrong
 *
 *****************************************************************************/
static BOOLEAN warn_handle = FALSE;
static BOOLEAN warn_proc = FALSE;
#ifndef DL_TAIL
#ifdef NDEBUG
#define DL_TAIL ".so"
#else
#define DL_TAIL ".so"
//#define DL_TAIL "_g.so"
#endif
#endif

void* dynl_open_binary_warn(const char* binary_name, const char* msg)
{
  void* handle = NULL;
  char* binary_name_so=NULL;

  // try %b/MOD first
  {
    const char* bin_dir = feGetResource('b');
    const int binary_name_so_length = 6 + strlen(DL_TAIL)
               + strlen(binary_name)
               +strlen(DIR_SEPP)*2
               +strlen(bin_dir);
    binary_name_so = (char *)omAlloc0( binary_name_so_length * sizeof(char) );
    snprintf(binary_name_so, binary_name_so_length, "%s%s%s%s%s%s", bin_dir, DIR_SEPP,"MOD",DIR_SEPP,binary_name, DL_TAIL);
    handle = dynl_open(binary_name_so);

  }

  // try P_PROCS_DIR (%P)
  if (handle == NULL)
  {
    const char* proc_dir = feGetResource('P');
    if (proc_dir != NULL)
    {
      if (binary_name_so!=NULL) omFree(binary_name_so);
      const int binary_name_so_length = 3 + strlen(DL_TAIL) + strlen(binary_name) + strlen(DIR_SEPP) + strlen(proc_dir);
      binary_name_so = (char *)omAlloc0( binary_name_so_length * sizeof(char) );
      snprintf(binary_name_so, binary_name_so_length, "%s%s%s%s", proc_dir, DIR_SEPP, binary_name, DL_TAIL);
      handle = dynl_open(binary_name_so);
    }
  }

  if (handle == NULL && ! warn_handle)
  {
      Warn("Could not find dynamic library: %s%s (tried %s)",
              binary_name, DL_TAIL,binary_name_so);
      Warn("Error message from system: %s", dynl_error());
      if (msg != NULL) Warn("%s", msg);
      Warn("See the INSTALL section in the Singular manual for details.");
      warn_handle = TRUE;
  }
  omfree((ADDRESS)binary_name_so );

  return  handle;
}

void* dynl_sym_warn(void* handle, const char* proc, const char* msg)
{
  void *proc_ptr = NULL;
  if (handle != NULL)
  {
    proc_ptr = dynl_sym(handle, proc);
    if (proc_ptr == NULL && ! warn_proc)
    {
      Warn("Could load a procedure from a dynamic library");
      Warn("Error message from system: %s", dynl_error());
      if (msg != NULL) Warn("%s", msg);
      Warn("See the INSTALL section in the Singular manual for details.");
      warn_proc = TRUE;
    }
  }
  return proc_ptr;
}

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * SECTION generic ELF: ix86-linux / alpha-linux / IA64-linux /x86_64_Linux  *
 *                      SunOS-5 / IRIX-6 / ppcMac-Darwin / FreeeBSD          *
 *****************************************************************************/
// relying on gcc to define __ELF__, check with cpp -dM /dev/null
// Mac OsX is an ELF system, but does not define __ELF__
// Solaris is an ELF system, but does not define __ELF__
#if defined(__ELF__)
#define HAVE_ELF_SYSTEM
#endif

#if defined(ppcMac_darwin)
#define HAVE_ELF_SYSTEM
#endif

#if defined(ix86Mac_darwin)
#define HAVE_ELF_SYSTEM
#endif

#if defined(x86_64Mac_darwin)
#define HAVE_ELF_SYSTEM
#endif

#if (defined(__APPLE__) && defined(__MACH__)) && (!defined(HAVE_ELF_SYSTEM))
#define HAVE_ELF_SYSTEM
#endif

#if defined(SunOS_5)
#define HAVE_ELF_SYSTEM
#endif

#if defined(HAVE_ELF_SYSTEM)
#include <dlfcn.h>
#define DL_IMPLEMENTED

static void* kernel_handle = NULL;
int dynl_check_opened(
  char *filename    /* I: filename to check */
  )
{
  return dlopen(filename,RTLD_NOW|RTLD_NOLOAD) != NULL;
}

void *dynl_open(
  char *filename    /* I: filename to load */
  )
{
// glibc 2.2:
  if ((filename==NULL) || (dlopen(filename,RTLD_NOW|RTLD_NOLOAD)==NULL))
    return(dlopen(filename, RTLD_NOW|RTLD_GLOBAL));
  else
    Werror("module %s already loaded",filename);
  return NULL;
// alternative
//    return(dlopen(filename, RTLD_NOW|RTLD_GLOBAL));
}

void *dynl_sym(void *handle, const char *symbol)
{
  if (handle == DYNL_KERNEL_HANDLE)
  {
    if (kernel_handle == NULL)
      kernel_handle = dynl_open(NULL);
    handle = kernel_handle;
  }
  return(dlsym(handle, symbol));
}

int dynl_close (void *handle)
{
  return(dlclose (handle));
}

const char *dynl_error()
{
  return(dlerror());
}
#endif /* ELF_SYSTEM */

/*****************************************************************************
 * SECTION HPUX-9/10                                                         *
 *****************************************************************************/
#if defined(HPUX_9) || defined(HPUX_10)
#define DL_IMPLEMENTED
#include <dl.h>

typedef char *((*func_ptr) ());

int dynl_check_opened(    /* NOTE: untested */
  char *filename    /* I: filename to check */
  )
{
  struct shl_descriptor *desc;
  for (int idx = 0; shl_get(idx, &desc) != -1; ++idx)
  {
    if (strcmp(filename, desc->filename) == 0) return TRUE;
  }
  return FALSE;
}

void *dynl_open(char *filename)
{
  shl_t           handle = shl_load(filename, BIND_DEFERRED, 0);

  return ((void *) handle);
}

void *dynl_sym(void *handle, const char *symbol)
{
  func_ptr        f;

  if (handle == DYNL_KERNEL_HANDLE)
    handle = PROG_HANDLE;

  if (shl_findsym((shl_t *) & handle, symbol, TYPE_PROCEDURE, &f) == -1)
  {
    if (shl_findsym((shl_t *) & handle, symbol, TYPE_UNDEFINED, &f) == -1)
    {
      f = (func_ptr) NULL;
    }
  }
  return ((void *)f);
}

int dynl_close (void *handle)
{
  shl_unload((shl_t) handle);
  return(0);
}

const char *dynl_error()
{
  static char errmsg[] = "shl_load failed";

  return errmsg;
}
#endif /* HPUX_9  or HPUX_10 */

/*****************************************************************************
 * SECTION generic: dynamic madules not available
 *****************************************************************************/
#ifndef DL_IMPLEMENTED

int dynl_check_opened(char *filename)
{
  return FALSE;
}

void *dynl_open(char *filename)
{
  return(NULL);
}

void *dynl_sym(void *handle, const char *symbol)
{
  return(NULL);
}

int dynl_close (void *handle)
{
  return(0);
}

const char *dynl_error()
{
  static char errmsg[] = "support for dynamic loading not implemented";
  return errmsg;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* HAVE_DL */