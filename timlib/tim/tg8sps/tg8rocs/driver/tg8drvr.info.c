/* Tg8 info file creation program.                                         */
/* Alastair.Bland@cern.ch 19th March 2004                                  */

#include <stdio.h>
#include <tg8drvrP.h>

static Tg8DrvrInfoTable info_table = {
   0xDEC00000,   /* Base address (0xDEC00000 usually) */
   0x10000,      /* Address increment (0x10000 usually) */
   0xB8,         /* The first interrupt vector number (0xB8 usually) */
   2,            /* Interrupt level (2 usually) */
   0x20          /* The default SSC Header code (0x20:SL, 0x34:PS) */
};

int main(int argc,char *argv[]) {
   write(1, info_table, sizeof(info_table));
   return(0);
}
