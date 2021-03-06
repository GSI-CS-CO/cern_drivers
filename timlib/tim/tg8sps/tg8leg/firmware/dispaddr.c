#include <stdio.h>
#include <sys/file.h>
#include <a.out.h>
main(argc,argv)
int argc; char **argv;
{ int startaddr, finaddr;
  if(argc<2) { printf("Filename is not specified\n"); exit(1);}
  if(Tg8ReadFirmware_aout(argv[1],&startaddr,&finaddr) == 0) {
    printf("Problems reading %s\n",argv[1]); exit(1);}
  else
    printf("Code start address is %x fin address is %x\n",
	   startaddr, finaddr);
  exit(0);
}
int Tg8ReadFirmware_aout(file,astrt,afin)
/* This routine reads the executable a.out-type file with the Tg8 firmware  */
/* code and fills the FirmwareObject structure. Returns failure in the      */
/* following cases:                                                         */
/* 1) Problems in opening or reading file                                   */
/* 2) The file is not of a.out executable type                              */
/* the #include <a.out.h> and #include <file.h> should be present           */
char *file; int *astrt, *afin;
{ int f,l; struct exec hdr;
  f = open(file,0,O_RDONLY);
  if(f<0) return(0);
  if(read(f,&hdr,sizeof(hdr)) != sizeof(hdr)) return(0);
  *astrt = hdr.a_entry;
  l = (hdr.a_text + hdr.a_data - sizeof(hdr));
  *afin = *astrt+l;
  return(1);
}
