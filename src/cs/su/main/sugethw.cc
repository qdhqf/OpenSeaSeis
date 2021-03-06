/* Copyright (c) Colorado School of Mines, 2011.*/
/* All rights reserved.                       */

/* SUGETHW: $Revision: 1.18 $ ; $Date: 2011/11/16 22:10:29 $	*/

#include "csException.h"
#include "csSUTraceManager.h"
#include "csSUArguments.h"
#include "csSUGetPars.h"
#include "su_complex_declarations.h"
#include "cseis_sulib.h"
#include <string>

extern "C" {
  #include <pthread.h>
}
#include "su.h"
#include "segy.h"
#include "header.h"

/*********************** self documentation **********************/
std::string sdoc_sugethw =
" 									"
" SUGETHW - sugethw writes the values of the selected key words		"
" 									"
"   sugethw key=key1,... [output=] <infile [>outfile]			"
" 									"
" Required parameters:							"
" key=key1,...		At least one key word.				"
" 									"
" Optional parameters:							"
" output=ascii		output written as ascii for display		"
" 			=binary for output as binary floats		"
" 			=geom   ascii output for geometry setting	"
" verbose=0 		quiet						"
" 			=1 chatty					"
" 									"
" Output is written in the order of the keys on the command		"
" line for each trace in the data set.					"
" 									"
" Example:								"
" 	sugethw <stdin key=sx,gx					"
" writes sx, gx values as ascii trace by trace to the terminal.		"
" 									"
" Comment: 								"
" Users wishing to edit one or more header field (as in geometry setting)"
" may do this via the following sequence:				"
"     sugethw < sudata output=geom key=key1,key2,... > hdrfile 		"
" Now edit the ASCII file hdrfile with any editor, setting the fields	"
" appropriately. Convert hdrfile to a binary format via:		"
"     a2b < hdrfile n1=nfields > binary_file				"
" Then set the header fields via:					"
"     sushw < sudata infile=binary_file key=key1,key2,... > sudata.edited"
" 									"
;

/*  Sun Feb 24 13:30:07 2013
  Automatically modified for usage in SeaSeis  */
namespace sugethw {


/* Credits:
 *
 *	SEP: Shuki Ronen
 *	CWP: Jack K. Cohen
 *      CWP: John Stockwell, added geom stuff, and getparstringarray
 */
/**************** end self doc ***********************************/

#define ASCII 0
#define BINARY 1
#define GEOM 2

segy tr;

void* main_sugethw( void* args )
{
	cwp_String key[SU_NKEYS];	/* array of keywords		*/
	int nkeys;		/* number of keywords to be gotten 	*/
	int iarg;		/* arguments in argv loop		*/
	int countkey=0;		/* counter of keywords in argc loop	*/
	cwp_String output;	/* string representing output format	*/
	int ioutput=ASCII;	/* integer representing output format	*/
	int verbose;		/* verbose?				*/

	/* Initialize */
	cseis_su::csSUArguments* suArgs = (cseis_su::csSUArguments*)args;
	cseis_su::csSUTraceManager* cs2su = suArgs->cs2su;
	cseis_su::csSUTraceManager* su2cs = suArgs->su2cs;
	int argc = suArgs->argc;
	char **argv = suArgs->argv;
	cseis_su::csSUGetPars parObj;

	void* retPtr = NULL;  /*  Dummy pointer for return statement  */
	su2cs->setSUDoc( sdoc_sugethw );
	if( su2cs->isDocRequestOnly() ) return retPtr;
	parObj.initargs(argc, argv);

	try {  /* Try-catch block encompassing the main function body */


	/* Get key values */
	if (!parObj.getparint("verbose",&verbose))	verbose=0;
	if ((nkeys=parObj.countparval("key"))!=0) {
		parObj.getparstringarray("key",key);
	} else {
		/* support old fashioned method for inputting key fields */
		/* as single arguments:  sugethw key1 key2 ... 		*/
		if (argc==1) throw cseis_geolib::csException("must set at least one key value!");

		for (iarg = 1; iarg < argc; ++iarg) {
			cwp_String keyword;	     /* keyword */

			keyword = argv[iarg];

			if (verbose) warn("argv=%s",argv[iarg]);
			/* get array of types and indexes to be set */
			if ((strncmp(keyword,"output=",7)!=0)) {
				key[countkey] = keyword;
				++countkey;
			}

			if (countkey==0)
				throw cseis_geolib::csException("must set at least one key value!");
		}
		nkeys=countkey;
	}

	/* Set and check output format; recall ioutput initialized to ASCII */
	if (!parObj.getparstring("output", &output))   output = "ascii";

        parObj.checkpars();

	if      (STREQ(output, "binary"))	ioutput = BINARY;
	else if (STREQ(output, "geom"))		ioutput = GEOM;
	else if (!STREQ(output, "ascii"))
		throw cseis_geolib::csException("unknown format output=\"%s\", see self-doc", output);

	/* Loop over traces writing selected header field values */
	while (cs2su->getTrace(&tr)) {
		register int ikey;

		for (ikey = 0; ikey < nkeys; ++ikey) {
			Value val;
			float fval;

			gethdval(&tr, key[ikey], &val);

			switch(ioutput) {
			case ASCII:
				printf("%6s=", key[ikey]);
				printfval(hdtype(key[ikey]), val);
				putchar('\t');
			break;
			case BINARY:
				fval = vtof(hdtype(key[ikey]), val);
				efwrite((char *) &fval, FSIZE, 1, stdout);
			break;
			case GEOM:
				printfval(hdtype(key[ikey]), val);
				putchar(' ');
			break;
			}
		}

		switch(ioutput) {
		case GEOM:
			printf("\n");
		break;
		case ASCII:
			printf("\n\n");
		break;
		}
	}

	su2cs->setEOF();
	pthread_exit(NULL);
	return retPtr;
}
catch( cseis_geolib::csException& exc ) {
  su2cs->setError("%s",exc.getMessage());
  pthread_exit(NULL);
  return retPtr;
}
}

} // END namespace
