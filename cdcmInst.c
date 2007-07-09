/* $Id: cdcmInst.c,v 1.1 2007/07/09 09:26:29 ygeorgie Exp $ */
/*
; Module Name:	cdcmInst.c
; Module Descr:	Intended for Linux/Lynx driver installation. Helps to unify
;		driver install procedure between two systems.
;		All standart options are parsed here and CDCM group description
;		is build-up. Module-specific options are parsed using defined
;		module-specific vectors.
; Date:         June, 2007.
; Author:       Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmInst.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 2.0   ygeorgie   09/07/07   Production release, CVS controlled.
; 1.0	ygeorgie   28/06/07   Initial version.
*/
#ifdef __linux__
#define _GNU_SOURCE /* asprintf rocks */
#include <sys/sysmacros.h>	/* for major, minor, makedev */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <common/include/generalDrvrHdr.h>
#include <common/include/list.h>
#include <cdcm/cdcmInfoT.h>
#include <cdcm/cdcmInst.h>
#include <cdcm/cdcmNamingConv.h>

#ifdef __linux__

#define streq(a,b) (strcmp((a),(b)) == 0)
static char *linux_do_cdcm_info_header_file(void);
static void *linux_grab_file(const char*, unsigned long*);
static const char *linux_moderror(int);
/* sys_init_module system call */
extern long init_module(void *, unsigned long, const char *);

#else  /* __Lynx__ */

extern char *optarg; /* for getopt() calls */
extern int   getopt _AP((int, char * const *, const char *));

#endif

static void init_lists(void);
static char *get_optstring(void);
static void educate_user(void);
static void check_mod_compulsory_params(cdcm_md_t *chk);
static void assert_all_mod_descr(void);
static void assert_compulsory_vectors(void);


/* module description */
cdcm_md_t g_mdesc[MAX_VME_SLOT] = {
  [0 ... MAX_VME_SLOT-1] {
    .md_gid      = -1,
    .md_lun      = -1,
    .md_mpd      =  1,
    .md_ivec     = -1,
    .md_ilev     = -1,
    .md_vme1addr = -1,
    .md_vme2addr = -1,
    .md_pciVenId = -1,
    .md_pciDevId = -1
  }
};

/* Claimed groups list */
LIST_HEAD(glob_grp_list);

/* group description */
cdcm_grp_t g_gdesc[MAX_VME_SLOT] = {
  [0 ... MAX_VME_SLOT-1] {
    .mod_am =  0
  }
};


/*-----------------------------------------------------------------------------
 * FUNCTION:    init_lists
 * DESCRIPTION: List initialization.
 * RETURNS:	void
 *-----------------------------------------------------------------------------
 */
static void
init_lists()
{
  int i;

  for (i = 0; i < MAX_VME_SLOT; i++) {
    /* module list */
    INIT_LIST_HEAD(&g_mdesc[i].md_list);

    /* group lists */
    INIT_LIST_HEAD(&g_gdesc[i].grp_list);
    INIT_LIST_HEAD(&g_gdesc[i].mod_list);
  }
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    get_optstring
 * DESCRIPTION: 
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static char*
get_optstring()
{
  char usr_optstr[32] = { 0 };
  static char cdcm_optstr[64] = {"-G:U:O:M:L:V:C:I:T:h,"};
  char *ptr = cdcm_optstr;

  if (cdcm_inst_ops.mso_get_optstr) {
    (*cdcm_inst_ops.mso_get_optstr)(usr_optstr);
    ptr += strlen(cdcm_optstr);
    strcpy(ptr, usr_optstr);
  }
  return(cdcm_optstr);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    educate_user
 * DESCRIPTION: 
 * RETURNS:	void
 *-----------------------------------------------------------------------------
 */
static void  
educate_user()
{

}


/*-----------------------------------------------------------------------------
 * FUNCTION:    check_mod_compulsory_params
 * DESCRIPTION: cheks if all compulsory parameters are provided for the module.
 *		If error detected - programm will terminate. 
 * RETURNS:	void
 *-----------------------------------------------------------------------------
 */
static void
check_mod_compulsory_params(
			    cdcm_md_t *chk) /* let's checkit */
{
  if (chk->md_lun == -1) { /* LUN not provided */
    printf("LUN is NOT provided!\n");
    goto do_barf;
  }

  if (chk->md_vme1addr == -1) { /* base address is not here. too baaad... */
    printf("Base address is not provided!\n");
    goto do_barf;
  }

  return;			/* we cool */

 do_barf:
  printf("Wrong params detected!\nAborting...\n");
  exit(EXIT_FAILURE);		/* 1 */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    __module_am
 * DESCRIPTION: how many declared modules
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static inline int
__module_am()
{
  int mc = 0;

  while (g_mdesc[mc].md_lun != -1)
    mc++;

  return(mc);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    assert_all_mod_descr
 * DESCRIPTION: Check already prepared module descriptions for any errors and
 *		declaration collisions.
 * RETURNS:	void
 *-----------------------------------------------------------------------------
 */
static void
assert_all_mod_descr()
{
  int mc = 0, cntr1, cntr2;

  mc = __module_am();		/* module amount */

  for (cntr1 = 0; cntr1 < mc; cntr1++) {
    for (cntr2 = cntr1+1; cntr2 < mc; cntr2++) {

      /* LUN checking */
      if (g_mdesc[cntr1].md_lun == g_mdesc[cntr2].md_lun) {
	printf("Identical LUNs (%d) detected!\nAborting...\n", g_mdesc[cntr1].md_lun);
	exit(EXIT_FAILURE);
      }

      /* first base address checking */
      if (g_mdesc[cntr1].md_vme1addr == g_mdesc[cntr2].md_vme1addr) {
	printf("Identical Base VME addresses (0x%x) detected!\nAborting...\n", g_mdesc[cntr1].md_vme1addr);
	exit(EXIT_FAILURE);
      }
    }
  }
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    assert_compulsory_vectors
 * DESCRIPTION: check if all compulsory vectors are provided in
 *		'cdcm_inst_ops' vector table.
 * RETURNS:	void
 *-----------------------------------------------------------------------------
 */
static void
assert_compulsory_vectors()
{
  if (!cdcm_inst_ops.mso_get_mod_name) {
    printf("Compulsory user vector 'get_mod_name' is not provided. Can't get module name!\n");
    goto no_compulsory_vectors;
  }
  
  return;	/* we cool */
  
 no_compulsory_vectors:
  printf("Aborting...\n");
  exit(EXIT_FAILURE);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_vme_arg_parser
 * DESCRIPTION: This function should be called by the module-specific
 *		installation procedure command line args
 * RETURNS:	group list
 *-----------------------------------------------------------------------------
 */
struct list_head
cdcm_vme_arg_parser(
		    int   argc,
		    char *argv[],
		    char *envp[])
{
  int cur_lun, cur_grp, cmdOpt;
  int grp_cntr = 0, comma_cntr = 0;  /* for grouping management */

  /* default + module-specific command line options */
  const char *optstring = get_optstring();

  assert_compulsory_vectors();	/* will barf if something wrong */
  init_lists();			/* init crap */

 

  /* Scan params of the command line */
  while ( (cmdOpt = getopt(argc, argv, optstring)) != EOF ) {
    switch (cmdOpt) {
    case P_T_HELP:	/* Help information */
      educate_user();
      if (cdcm_inst_ops.mso_educate)
	(*cdcm_inst_ops.mso_educate)();
      exit(EXIT_SUCCESS);	/* 0 */
      break;
    case P_T_LUN:	/* Logical Unit Number */
      cur_lun = abs(atoi(optarg));
      if (!WITHIN_RANGE(0, cur_lun, 20)) {
	printf("Wrong LUN (U param). Allowed absolute range is [0, 20]\nAborting...\n");
	exit(EXIT_FAILURE);
      }
      g_mdesc[comma_cntr].md_lun = cur_lun;
      break;
    case P_T_GRP:	/* logical group coupling */
      if (comma_cntr && !grp_cntr) {
	/* should exist from the very beginning i.e. before any separator */
	printf("Group numbering error (G param). Should be set starting from the very beginning\nAborting...\n");
	exit(EXIT_FAILURE);
      }

      cur_grp = atoi(optarg);

      if (!WITHIN_RANGE(1, cur_grp, MAX_VME_SLOT)) {
	printf("Wrong group number (G param). Allowed range is [1, 21]\nAborting...\n");
	exit(EXIT_FAILURE);	/* 1 */
      }
      
      g_mdesc[comma_cntr].md_gid = cur_grp;
      if (!g_gdesc[cur_grp-1].mod_am)
	/* still not in the list of claimed groups */
	list_add(&g_gdesc[cur_grp-1].grp_list, &glob_grp_list);

      /* handle group module */
      list_add(&g_mdesc[comma_cntr].md_list, &g_gdesc[cur_grp-1].mod_list);
      g_gdesc[cur_grp-1].mod_am++;
      grp_cntr++;
      break;
    case P_T_ADDR:	/* First base address */
      g_mdesc[comma_cntr].md_vme1addr = strtol(optarg, NULL, 16);
      break;
    case P_T_N_ADDR:	/* Second base address */
      g_mdesc[comma_cntr].md_vme2addr = strtol(optarg, NULL, 16);
      break;
    case P_T_IVECT:	/* Interrupt vector */
      g_mdesc[comma_cntr].md_ivec = atoi(optarg);
      break;
    case P_T_ILEV:	/* Interrupt level */
      g_mdesc[comma_cntr].md_ilev = atoi(optarg);
      break;
    case P_T_TRANSP:	/* Transparent driver parameter */
      break;
    case P_T_FLG:	/* Debug information printing */
      break;
    case P_T_CHAN: /* Channel amount (amount of minor devices to create) */
      g_mdesc[comma_cntr].md_mpd = atoi(optarg);
      break;
    case P_T_SEPAR:	/* End of module device definition */
      if (comma_cntr == MAX_VME_SLOT) {
	printf("Too many modules declared (max is %d)\nAborting...\n", MAX_VME_SLOT);
	exit(EXIT_FAILURE);
      }
      check_mod_compulsory_params(&g_mdesc[comma_cntr]); /* will barf if error */

      /* set groups and their module lists if no explicit grouping (G param) */
      if (g_mdesc[comma_cntr].md_gid == -1) {
	list_add(&g_gdesc[comma_cntr].grp_list, &glob_grp_list);
	list_add(&g_mdesc[comma_cntr].md_list, &g_gdesc[comma_cntr].mod_list);

	g_gdesc[comma_cntr].mod_am++; /* increase module amount in the group */
	g_mdesc[comma_cntr].md_gid = comma_cntr+1; /* set module group id */
      }

      comma_cntr++;
      break;
    case '?':	/* error */
      printf("Wrong/unknown argument!\n");
      if (cdcm_inst_ops.mso_educate)
	(*cdcm_inst_ops.mso_educate)();
      exit(EXIT_FAILURE);	/* 1 */
      break;
      /* all the rest is not interpreted by the CDCM arg parser */
#ifdef __linux__
    case 1: /* this is non-option args */
#else /* __Lynx__ */
    case 0:
#endif
    default:
      if (cdcm_inst_ops.mso_parse_opt)
	(*cdcm_inst_ops.mso_parse_opt)(cmdOpt, optarg);
      else {
	printf("Wrong/Unknown option %c!\nAborting...\n", cmdOpt);
	exit(EXIT_FAILURE);
      }
      break;
    }
  }

  assert_all_mod_descr();
  return(glob_grp_list);
}


#ifdef __linux__

/*-----------------------------------------------------------------------------
 * FUNCTION:    __linux_dev_name
 * DESCRIPTION: CDCM device name container. It looks like 'cdcm_<mod_name>'
 *		where <mod_name> is the name, provided by the module-specific
 *		installation part.
 * RETURNS:	device name
 *-----------------------------------------------------------------------------
 */
static inline char*
__linux_dev_name()
{
  static char cdcm_dev_nm[32] = { 0 };

  if (cdcm_dev_nm[0])	/* already defined */
    return(cdcm_dev_nm);
  
  snprintf(cdcm_dev_nm, sizeof(cdcm_dev_nm), CDCM_DEV_NM_FMT, (*cdcm_inst_ops.mso_get_mod_name)());
  
  return(cdcm_dev_nm);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    __linux_srv_node
 * DESCRIPTION: CDCM service node name container.
 *		It looks like '/dev/cdcm_<mod_name>
 * RETURNS:	service node name
 *-----------------------------------------------------------------------------
 */
static inline char*
__linux_srv_node()
{
  static char cdcm_s_n_nm[64] = { 0 };

  if (cdcm_s_n_nm[0])		/* already defined */
    return(cdcm_s_n_nm);
  
  snprintf(cdcm_s_n_nm, sizeof(cdcm_s_n_nm), "/dev/%s", __linux_dev_name());
  
  return(cdcm_s_n_nm);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    dr_install
 * DESCRIPTION: Lynx call wrapper. Will install the driver in the system.
 *		Supposed to called _only_ once during installation procedure.
 *		If more - then it will terminate installation.
 * RETURNS:	driver id
 *-----------------------------------------------------------------------------
 */
int
dr_install(
	   char *drvr_fn, /* driver path */
	   int   type)    /* for now _only_ char devices */
{
  static int c_cntr = 0; /* how many times i was already called */
  void *drvrfile = NULL;
  FILE *procfd   = NULL;
  unsigned long len;
  char *options;
  long int ret;
  char buff[32], *bufPtr; /* for string parsing */
  int major_num = 0;
  int devn; /* major/minor device number */
  char *itf;	/* info table file */

  if (c_cntr > 1) {	/* oooops... */
    printf("%s() was called more then once. Should NOT happen!\nAborting...\n", __FUNCTION__);
    exit(EXIT_FAILURE);
  }

  /* put .ko in the local buffer */
  if (!(drvrfile = linux_grab_file(drvr_fn, &len))) {
    fprintf(stderr, "insmod: can't read '%s': %s\n", drvr_fn, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* create info file */
  if ( (itf = linux_do_cdcm_info_header_file()) == NULL) {
    free(drvrfile);
    exit(EXIT_FAILURE);
  }
    
  asprintf(&options, "ipath=%s",  itf);	/* set parameter */
  
  /* insert module in the kernel */
  if ( (ret = init_module(drvrfile, len, options)) != 0 ) {
    fprintf(stderr, "insmod: error inserting '%s': %li %s\n", drvr_fn, ret, linux_moderror(errno));
    free(drvrfile);
    free(options);
    exit(EXIT_FAILURE);
  }

  free(drvrfile);
  free(options);

  /* check if he made it */
  if ( (procfd = fopen("/proc/devices", "r")) == NULL ) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  
  /* search for the device driver entry */
  bufPtr = buff;
  while (fgets(buff, sizeof(buff), procfd))
    if (strstr(buff, __linux_dev_name())) { /* bingo! */
      major_num = atoi(strsep(&bufPtr, " "));
      break;
    }
  
  /* check if we cool */
  if (!major_num) {
    fprintf(stderr, "'%s' device NOT found in proc fs.\n", __linux_dev_name());
    exit(EXIT_FAILURE);
  } else 
    printf("%s device driver installed. Major number %d.\n", __linux_dev_name(), major_num);

  /* create service entry point */
  unlink(__linux_srv_node()); /* if already exist delete it */
  devn = makedev(major_num, 0);
  if (mknod(__linux_srv_node(), S_IFCHR | 0666, devn)) {
    perror("mknod");
    exit(EXIT_FAILURE);
  }

  c_cntr++;
  return(c_cntr);  
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdv_install
 * DESCRIPTION: Lynx call wrapper. Can be called several times from the driver
 *		installation procedure. Depends on how many major devices user
 *		wants to install. If installation programm is written
 *		correctly, then it should be called as many times as groups
 *		declared. i.e. each group represents exactly one character
 *		device.
 *		By Lynx driver strategy - each time cdv_install()
 *		(or bdv_install) is called by the user-space installation
 *		programm, driver install vector from 'dldd' structure is
 *		activated. It returns statics table, that in turn is used in
 *		every entry point of 'dldd' vector table.
 * RETURNS:	major device ID
 *-----------------------------------------------------------------------------
 */
int
cdv_install(
	    char *path,		/* info file */
	    int   driver_id,	/*  */
	    int   extra)	/*  */
{
  static int c_cntr = 0; /* how many times i was already called */
  
  c_cntr++;

#if 0
  if (c_cntr > group_am()) {	/* oooops... */
    printf("%s() was called more times, then groups declared. Should NOT happen!\nAborting...\n", __FUNCTION__);
    exit(EXIT_FAILURE);
  }
#endif

  return(1);
}

      
/*-----------------------------------------------------------------------------
 * FUNCTION:    linux_do_cdcm_info_header_file
 * DESCRIPTION: Setup CDCM header and create info file that will passed to
 *		the driver druring installation.
 * RETURNS:	info file name - if we are cool
 *		NULL           - if we are not
 *-----------------------------------------------------------------------------
 */
static char*
linux_do_cdcm_info_header_file()
{
  static char cdcm_if_nm[64] = { 0 };
  int ifd; /* info file descriptor */

  /* buidup CDCM header */
  cdcm_hdr_t cdcm_hdr = {
    .cih_signature = CDCM_MAGIC_IDENT,
    .cih_grp_am    = list_capacity(&glob_grp_list)
    //strncpy(cdcm_hdr.cih_dnm, (*cdcm_inst_ops.mso_get_mod_name)(), sizeof(cdcm_hdr.cih_dnm))
    //.cih_dnm       = ((*cdcm_inst_ops.mso_get_mod_name)())
  };
  
  strncpy(cdcm_hdr.cih_dnm, (*cdcm_inst_ops.mso_get_mod_name)(), sizeof(cdcm_hdr.cih_dnm));
  snprintf(cdcm_if_nm, sizeof(cdcm_if_nm), "/tmp/%s.info", __linux_dev_name());

  if ( (ifd = open(cdcm_if_nm, O_CREAT | O_RDWR, 0664)) == -1) {
    perror(__FUNCTION__);
    return(NULL);
  }
  /* write CDCM header */
  if( write(ifd, &cdcm_hdr, sizeof(cdcm_hdr)) == -1) {
    perror(__FUNCTION__);
    close(ifd);
    return(NULL);
  }

  close(ifd);
  return(cdcm_if_nm);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    linux_grab_file
 * DESCRIPTION: 
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static void*
linux_grab_file(
		const char *filename, /* .ko file name */
		unsigned long *size)  /* its size will be placed here */
{
  unsigned int max = 16384;
  int ret, fd;
  void *buffer = malloc(max);
  if (!buffer)
    return NULL;
  
  if (streq(filename, "-"))
    fd = dup(STDIN_FILENO);
  else
    fd = open(filename, O_RDONLY, 0);
  
  if (fd < 0)
    return NULL;
  
  *size = 0;
  while ((ret = read(fd, buffer + *size, max - *size)) > 0) {
    *size += ret;
    if (*size == max)
      buffer = realloc(buffer, max *= 2);
  }
  if (ret < 0) {
    free(buffer);
    buffer = NULL;
  }
  close(fd);
  return(buffer);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    linux_moderror
 * DESCRIPTION: We use error numbers in a loose translation...
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static const char*
linux_moderror(
	       int err)		/*  */
{
  switch (err) {
  case ENOEXEC:
    return "Invalid module format";
  case ENOENT:
    return "Unknown symbol in module";
  case ESRCH:
    return "Module has wrong symbol version";
  case EINVAL:
    return "Invalid parameters";
  default:
    return strerror(err);
  }
}
#endif /* __linux__ */
