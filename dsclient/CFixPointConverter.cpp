/*
 * Copyright: (C) 2000 Julius O. Smith
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 *   Julius O. Smith  jos@ccrma.stanford.edu
 *
 */
/* This code was modified by Bruce Forsberg (forsberg@tns.net) to make it
   into a C++ class
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <math.h>

#include "CFixPointConverter.h"
#include "CFixPointConverterLargeFilter.h"
#include "CFixPointConverterSmallFilter.h"


/*
 * The configuration constants below govern
 * the number of bits in the input sample and filter coefficients, the
 * number of bits to the right of the binary-point for fixed-point math, etc.
 */

/* Conversion constants */
#define Nhc       8
#define Na        7
#define Np       (Nhc+Na)
#define Npc      (1<<Nhc)
#define Amask    ((1<<Na)-1)
#define Pmask    ((1<<Np)-1)
#define Nh       16
#define Nb       16
#define Nhxn     14
#define Nhg      (Nh-Nhxn)
#define NLpScl   13
/* Description of constants:
 *
 * Npc - is the number of look-up values available for the lowpass filter
 *    between the beginning of its impulse response and the "cutoff time"
 *    of the filter.  The cutoff time is defined as the reciprocal of the
 *    lowpass-filter cut off frequence in Hz.  For example, if the
 *    lowpass filter were a sinc function, Npc would be the index of the
 *    impulse-response lookup-table corresponding to the first zero-
 *    crossing of the sinc function.  (The inverse first zero-crossing
 *    time of a sinc function equals its nominal cutoff frequency in Hz.)
 *    Npc must be a power of 2 due to the details of the current
 *    implementation. The default value of 512 is sufficiently high that
 *    using linear interpolation to fill in between the table entries
 *    gives approximately 16-bit accuracy in filter coefficients.
 *
 * Nhc - is log base 2 of Npc.
 *
 * Na - is the number of bits devoted to linear interpolation of the
 *    filter coefficients.
 *
 * Np - is Na + Nhc, the number of bits to the right of the binary point
 *    in the integer "time" variable. To the left of the point, it indexes
 *    the input array (X), and to the right, it is interpreted as a number
 *    between 0 and 1 sample of the input X.  Np must be less than 16 in
 *    this implementation.
 *
 * Nh - is the number of bits in the filter coefficients. The sum of Nh and
 *    the number of bits in the input data (typically 16) cannot exceed 32.
 *    Thus Nh should be 16.  The largest filter coefficient should nearly
 *    fill 16 bits (32767).
 *
 * Nb - is the number of bits in the input data. The sum of Nb and Nh cannot
 *    exceed 32.
 *
 * Nhxn - is the number of bits to right shift after multiplying each input
 *    sample times a filter coefficient. It can be as great as Nh and as
 *    small as 0. Nhxn = Nh-2 gives 2 guard bits in the multiply-add
 *    accumulation.  If Nhxn=0, the accumulation will soon overflow 32 bits.
 *
 * Nhg - is the number of guard bits in mpy-add accumulation (equal to Nh-Nhxn)
 *
 * NLpScl - is the number of bits allocated to the unity-gain normalization
 *    factor.  The output of the lowpass filter is multiplied by LpScl and
 *    then right-shifted NLpScl bits. To avoid overflow, we must have
 *    Nb+Nhg+NLpScl < 32.
 */


using namespace std;

/** \brief Standard constructor
    \param high_quality true for high quality, false for low quality
    \param linear_interpolation
    \param filter_interpolation  */

CFixPointConverter::CFixPointConverter(bool high_quality, bool linear_interpolation, bool filter_interpolation) {

   interpFilt = filter_interpolation;
   largeFilter = high_quality;
   linearInterp = linear_interpolation;

   _X = NULL;
   _Y = NULL;
   m_vol = 1.0;
}

/*! Destructor */
CFixPointConverter::~CFixPointConverter() {
   deleteMemory();
}


void CFixPointConverter::deleteMemory() {
   int i;

   // Delete memory for the input and output arrays
   if (_X != NULL) {
      for (i = 0; i < m_nChans; i++) {
         delete [] _X[i];
         _X[i] = NULL;
         delete [] _Y[i];
         _Y[i] = NULL;
      }
      delete [] _X;
      _X = NULL;
      delete [] _Y;
      _Y = NULL;
   }
}

/*! \brief initialize the resampling class 
    
    This function will allow one to stream data. When a new data stream is to
    be input then this function should be called. Even if the factor and number
    of channels don't change. Otherwise each new data block sent to resample
    will be considered part of the previous data block. This function also allows
    one to specified a multiplication factor to adjust the final output. This
    applies to the small and large filter. 

    \param factor = Sndout/Sndin 
    \param channels number of sound channels 
    \param volume = 1.0 factor to multiply amplitude 

*/

void CFixPointConverter::initialize(double fac, int channels, double volume) {

   int i;

   // Delete all previous allocated input and output buffer memory
   deleteMemory();

   m_factor = fac;
   m_nChans = channels;
   m_initial = true;
   m_vol = volume;

   // Allocate all new memory
   _X = new short * [m_nChans];
   _Y = new short * [m_nChans];

   for (i = 0; i < m_nChans; i++) {
      // Add extra to allow of offset of input data (Xoff in main routine)
      _X[i] = new short[IBUFFSIZE + 256];
      _Y[i] = new short[(int)(((double)IBUFFSIZE)*m_factor)];
      memset(_X[i], 0, sizeof(short) * (IBUFFSIZE + 256));
   }
}



/** \brief do the actual resampling

    Resampling function. Inside one of three 
    functions is called depending on the desired quality.
    
    \param inCount number of input samples in inArray to convert
    \param outCount number of output samples to compute
    \param inArray[] input data (length inCount * nChans)
    \param outArray[] output data (length outCount * nChans)
    \param factor      resampling factor

    \return number of output samples returned    
*/

int CFixPointConverter::resample(       /*  */
    int& inCount,               /* number of input samples to convert */
    int outCount,               /* number of output samples to compute */
    short inArray[],            /* input data */
    short outArray[],           /* output data */
	double factor)              /* factor */
{
  int Ycount;
   
  if (linearInterp == true)
    // Use fast method with no filtering. Poor quality
    Ycount = resampleFast(inCount,outCount,inArray,outArray);
   
  else if (largeFilter == false) 
    // Use small filtering. Good quality
    Ycount = resampleWithFilter( inCount, outCount, inArray, outArray,
                                 SMALL_FILTER_IMP, SMALL_FILTER_IMPD, 
	                               (unsigned short)(SMALL_FILTER_SCALE * m_vol),
                                 SMALL_FILTER_NMULT, SMALL_FILTER_NWING);
   
  else 
    // Use large filtering Great quality
    Ycount = resampleWithFilter( inCount, outCount, inArray, outArray,
                                 LARGE_FILTER_IMP, LARGE_FILTER_IMPD, 
	                               (unsigned short)(LARGE_FILTER_SCALE * m_vol),
                                 LARGE_FILTER_NMULT, LARGE_FILTER_NWING);                               

  m_initial = false;

  return (Ycount);
}



int CFixPointConverter::err_ret(char *s) {
    cerr << "resample: " << s << endl; /* Display error message  */
    return -1;
}

int CFixPointConverter::readData(int   inCount,       /* _total_ number of frames in input file */
                                 short inArray[],     /* input data */
                                 short *outPtr[],     /* array receiving chan samps */
                                 int   dataArraySize, /* size of these arrays */
                                 int   Xoff,          /* read into input array starting at this index */
                                 bool  init_count) 
{
   int    i, Nsamps, c;
   static unsigned int framecount;  /* frames previously read */
   short *ptr;

   if (init_count == true)
      framecount = 0;       /* init this too */

   Nsamps = dataArraySize - Xoff;   /* Calculate number of samples to get */

   // Don't overrun input buffers
   if (Nsamps > (inCount - (int)framecount))
   {
      Nsamps = inCount - framecount;
   }

   for (c = 0; c < m_nChans; c++)
   {
      ptr = outPtr[c];
      ptr += Xoff;        /* Start at designated sample number */

      for (i = 0; i < Nsamps; i++)
         *ptr++ = (short) inArray[c * inCount + i + framecount];
   }

   framecount += Nsamps;

   if ((int)framecount >= inCount)            /* return index of last samp */
      return (((Nsamps - (framecount - inCount)) - 1) + Xoff);
   else
      return 0;
}



int CFixPointConverter::SrcLinear(short X[], 
                                  short Y[], 
                                  double factor, 
                                  unsigned int *Time, 
                                  unsigned short& Nx, 
                                  unsigned short Nout) 
{

	short iconst;
	short *Xp, *Ystart;
	int v,x1,x2;

	double dt;                  /* Step through input signal */ 
	unsigned int dtb;           /* Fixed-point version of Dt */
//	unsigned int endTime;       /* When Time reaches EndTime, return to user */
	unsigned int start_sample, end_sample;

	dt = 1.0/factor;            /* Output sampling period */
	dtb = (unsigned int)(dt*(1<<Np) + 0.5); /* Fixed-point representation */

	start_sample = (*Time)>>Np;
	Ystart = Y;
//	endTime = *Time + (1<<Np)*(int)Nx;
	/* 
	* TODO
	* DAS: not sure why this was changed from *Time < endTime
	* update: *Time < endTime causes seg fault.  Also adds a clicking sound.
	*/
	while (Y - Ystart != Nout)
//	while (*Time < endTime)
	{
		iconst = (*Time) & Pmask;
		Xp = &X[(*Time)>>Np];      /* Ptr to current input sample */
		x1 = *Xp++;
		x2 = *Xp;
		x1 *= ((1<<Np)-iconst);
		x2 *= iconst;
		v = x1 + x2;
		*Y++ = WordToHword(v,Np);   /* Deposit output */
		*Time += dtb;		    /* Move to next sample by time increment */
	}
	end_sample = (*Time)>>Np;
	Nx = end_sample - start_sample;
	return (Y - Ystart);            /* Return number of output samples */
}


int CFixPointConverter::SrcUp(short X[],
                              short Y[],
                              double factor,
                              unsigned int *Time,
                              unsigned short& Nx,
                              unsigned short Nout,
                              unsigned short Nwing,
                              unsigned short LpScl,
                              short Imp[],
                              short ImpD[],
                              bool Interp)
{
	
  short *Xp, *Ystart;
	int v;

	double dt;                  /* Step through input signal */ 
	unsigned int dtb;           /* Fixed-point version of Dt */
//	unsigned int endTime;       /* When Time reaches EndTime, return to user */
	unsigned int start_sample, end_sample;

	dt = 1.0/factor;            /* Output sampling period */
	dtb = (unsigned int)(dt*(1<<Np) + 0.5); /* Fixed-point representation */

	start_sample = (*Time)>>Np;
	Ystart = Y;
//	endTime = *Time + (1<<Np)*(int)Nx;
	/* 
	* TODO
	* DAS: not sure why this was changed from *Time < endTime
	* update: *Time < endTime causes seg fault.  Also adds a clicking sound.
	*/
	while (Y - Ystart != Nout)
//	while (*Time < endTime)
	{
		Xp = &X[*Time>>Np];      /* Ptr to current input sample */
		/* Perform left-wing inner product */
		v = FilterUp(Imp, ImpD, Nwing, Interp, Xp, (short)(*Time&Pmask),-1);
		/* Perform right-wing inner product */
		v += FilterUp(Imp, ImpD, Nwing, Interp, Xp+1, 
                       (short)((((*Time)^Pmask)+1)&Pmask), 1);
		v >>= Nhg;		/* Make guard bits */
		v *= LpScl;		/* Normalize for unity filter gain */
		*Y++ = WordToHword(v,NLpScl);   /* strip guard bits, deposit output */
		*Time += dtb;		/* Move to next sample by time increment */
	}
	end_sample = (*Time)>>Np;
	Nx = end_sample - start_sample;
	return (Y - Ystart);        /* Return the number of output samples */
}



int CFixPointConverter::SrcUD(short X[],
                              short Y[],
                              double factor,
                              unsigned int *Time,
                              unsigned short& Nx,
                              unsigned short Nout,
                              unsigned short Nwing,
                              unsigned short LpScl,
                              short Imp[],
                              short ImpD[],
                              bool Interp)
{
	short *Xp, *Ystart;
	int v;

	double dh;                  /* Step through filter impulse response */
	double dt;                  /* Step through input signal */
//	unsigned int endTime;       /* When Time reaches EndTime, return to user */
	unsigned int dhb, dtb;      /* Fixed-point versions of Dh,Dt */
	unsigned int start_sample, end_sample;

	dt = 1.0/factor;            /* Output sampling period */
	dtb = (unsigned int)(dt*(1<<Np) + 0.5); /* Fixed-point representation */

	dh = MIN(Npc, factor*Npc);  /* Filter sampling period */
	dhb = (unsigned int)(dh*(1<<Na) + 0.5); /* Fixed-point representation */

	start_sample = (*Time)>>Np;
	Ystart = Y;
//	endTime = *Time + (1<<Np)*(int)Nx;
	/* 
	* TODO
	* DAS: not sure why this was changed from *Time < endTime
	* update: *Time < endTime causes seg fault.  Also adds a clicking sound.
	*/
	while (Y - Ystart != Nout)
//	while (*Time < endTime)
	{
		Xp = &X[*Time>>Np];	/* Ptr to current input sample */
		v = FilterUD(Imp, ImpD, Nwing, Interp, Xp, (short)(*Time&Pmask),
				  -1, dhb);	/* Perform left-wing inner product */
		v += FilterUD(Imp, ImpD, Nwing, Interp, Xp+1, 
                       (short)((((*Time)^Pmask)+1)&Pmask), 1, dhb);	/* Perform right-wing inner product */
		v >>= Nhg;		/* Make guard bits */
		v *= LpScl;		/* Normalize for unity filter gain */
		*Y++ = WordToHword(v,NLpScl);   /* strip guard bits, deposit output */
		*Time += dtb;		/* Move to next sample by time increment */
	}
	
	end_sample = (*Time)>>Np;
	Nx = end_sample - start_sample;
	return (Y - Ystart);        /* Return the number of output samples */
}

/** \brief fast version of resampling routine

    This function implements the resampling routine without applying a filter.
    It is very fast, but quality is poor.


    \param inCount    number of input samples to convert
    \param outCount   number of output samples to compute
    \param inArray[]  input data (length inCount * nChans)
    \param outArray[] output data (length outCount * nChans)

    \return number of output samples
*/
int CFixPointConverter::resampleFast( int& inCount, 
                                      int outCount,
                                      short inArray[], 
                                      short outArray[]) 
{
    unsigned int Time2;		/* Current time/pos in input sample */
#if 0
    unsigned short Ncreep;
#endif
    unsigned short Xp, Xoff, Xread;
    int OBUFFSIZE = (int)(((double)IBUFFSIZE)*m_factor);
    unsigned short Nout = 0, Nx, orig_Nx;
    unsigned short maxOutput;
    int total_inCount = 0;
    int c, i, Ycount, last;
    bool first_pass = true;


    Xoff = 10;

    Nx = IBUFFSIZE - 2*Xoff;     /* # of samples to process each iteration */
    last = 0;			/* Have not read last input sample yet */
    Ycount = 0;			/* Current sample and length of output file */

    Xp = Xoff;			/* Current "now"-sample pointer for input */
    Xread = Xoff;		/* Position in input array to read into */

    if (m_initial == true)
       _Time = (Xoff<<Np);	/* Current-time pointer for converter */

    do {
		if (!last)		/* If haven't read last sample yet */
		{
	   	 last = readData(inCount, inArray, _X, 
					 IBUFFSIZE, (int)Xread,first_pass);
          first_pass = false;
	    	 if (last && (last-Xoff<Nx)) { /* If last sample has been read... */
				Nx = last-Xoff;	/* ...calc last sample affected by filter */
			 	if (Nx <= 0)
		  			break;
	    	 }
		}

      if ((outCount-Ycount) > (OBUFFSIZE - (2*Xoff*m_factor)) )
      	maxOutput = OBUFFSIZE - (unsigned short)(2*Xoff*m_factor);
      else
      	maxOutput = outCount-Ycount;

      for (c = 0; c < m_nChans; c++)
      {
			orig_Nx = Nx;
	   	Time2 = _Time;
	   /* Resample stuff in input buffer */
	   	Nout=SrcLinear(_X[c],_Y[c],m_factor,&Time2,orig_Nx,maxOutput);
      }
		Nx = orig_Nx;
      _Time = Time2;

		_Time -= (Nx<<Np);	/* Move converter Nx samples back in time */
		Xp += Nx;		/* Advance by number of samples processed */
#if 0
	Ncreep = (Time>>Np) - Xoff; /* Calc time accumulation in Time */
	if (Ncreep) {
	    Time -= (Ncreep<<Np);    /* Remove time accumulation */
	    Xp += Ncreep;            /* and add it to read pointer */
	}
#endif
      for (c = 0; c < m_nChans; c++)
      {
	   	for (i=0; i<IBUFFSIZE-Xp+Xoff; i++) { /* Copy part of input signal */
	       	_X[c][i] = _X[c][i+Xp-Xoff]; /* that must be re-used */
	   	}
      }
		if (last) {		/* If near end of sample... */
	    	last -= Xp;		/* ...keep track were it ends */
	    	if (!last)		/* Lengthen input by 1 sample if... */
	      	last++;		/* ...needed to keep flag true */
		}
		Xread = IBUFFSIZE - Nx;	/* Pos in input buff to read new data into */
		Xp = Xoff;
	
		Ycount += Nout;
		if (Ycount>outCount) {
	    	Nout -= (Ycount-outCount);
	    	Ycount = outCount;
		}

		if (Nout > OBUFFSIZE) /* Check to see if output buff overflowed */
	 		return err_ret("Output array overflow");

      for (c = 0; c < m_nChans; c++)
	   	for (i = 0; i < Nout; i++)
            outArray[c * outCount + i + Ycount - Nout] = _Y[c][i];

      total_inCount += Nx;

    } while (Ycount < outCount); /* Continue until done */

    inCount = total_inCount;

    return(Ycount);		/* Return # of samples in output file */
}

/** \brief High quality version of resampling routine

    This function implements the resampling routine with a filter applied.
    Quality depends on the filter used. At the moment, there are two different 
    filters available.
    \see resample

    \param inCount    number of input samples to convert
    \param outCount   number of output samples to compute
    \param inArray[]  input data (length inCount * nChans)
    \param outArray[] output data (length outCount * nChans)

    \return number of output samples 
*/
int CFixPointConverter::resampleWithFilter(int& inCount,		/* number of input samples to convert */
                                           int outCount,		/* number of output samples to compute */
                                           short inArray[],            /* input data */
                                           short outArray[],           /* output data */
                                           short Imp[], short ImpD[],
                                           unsigned short LpScl, 
                                           unsigned short Nmult, 
                                           unsigned short Nwing)
{
    unsigned int Time2;		/* Current time/pos in input sample */
#if 0
    unsigned short Ncreep;
#endif
    unsigned short Xp, Xoff, Xread;
    int OBUFFSIZE = (int)(((double)IBUFFSIZE)*m_factor);
    unsigned short Nout = 0, Nx, orig_Nx;
    unsigned short maxOutput;
    int total_inCount = 0;
    int c, i, Ycount, last;
    bool first_pass = true;


    /* Account for increased filter gain when using factors less than 1 */
    if (m_factor < 1)
      LpScl = (unsigned short)(LpScl*m_factor + 0.5);

    /* Calc reach of LP filter wing & give some creeping room */
    Xoff = (unsigned short)(((Nmult+1)/2.0) * MAX(1.0,1.0/m_factor) + 10);

    if (IBUFFSIZE < 2*Xoff)      /* Check input buffer size */
      return err_ret("IBUFFSIZE (or factor) is too small");

    Nx = IBUFFSIZE - 2*Xoff;     /* # of samples to process each iteration */
    
    last = 0;			/* Have not read last input sample yet */
    Ycount = 0;			/* Current sample and length of output file */
    Xp = Xoff;			/* Current "now"-sample pointer for input */
    Xread = Xoff;		/* Position in input array to read into */

    if (m_initial == true)
       _Time = (Xoff<<Np);	/* Current-time pointer for converter */
    
    do {
		if (!last)		/* If haven't read last sample yet */
		{
	    	last = readData(inCount, inArray, _X, IBUFFSIZE, (int)Xread,first_pass);
        first_pass = false;
	    	if (last && (last-Xoff<Nx)) { /* If last sample has been read... */
				Nx = last-Xoff;	/* ...calc last sample affected by filter */
				if (Nx <= 0)
		  			break;
	    	}
		}

      if ( (outCount-Ycount) > (OBUFFSIZE - (2*Xoff*m_factor)) )
      	maxOutput = OBUFFSIZE  - (unsigned short)(2*Xoff*m_factor);
      else
      	maxOutput = outCount-Ycount;

      for (c = 0; c < m_nChans; c++)
      {
			orig_Nx = Nx;
	   	Time2 = _Time;
           /* Resample stuff in input buffer */
	   	if (m_factor >= 1) {	/* SrcUp() is faster if we can use it */
	       	Nout=SrcUp(_X[c],_Y[c],m_factor, &Time2,Nx,maxOutput,Nwing,LpScl,Imp,ImpD,interpFilt);
	   	}
	   	else {
	       	Nout=SrcUD(_X[c],_Y[c],m_factor, &Time2,Nx,maxOutput,Nwing,LpScl,Imp,ImpD,interpFilt);
	   	}
      }
      _Time = Time2;

		_Time -= (Nx<<Np);	/* Move converter Nx samples back in time */
		Xp += Nx;		/* Advance by number of samples processed */
#if 0
	Ncreep = (Time>>Np) - Xoff; /* Calc time accumulation in Time */
	if (Ncreep) {
	    Time -= (Ncreep<<Np);    /* Remove time accumulation */
	    Xp += Ncreep;            /* and add it to read pointer */
	}
#endif
		if (last) {		/* If near end of sample... */
			 last -= Xp;		/* ...keep track were it ends */
			 if (!last)		/* Lengthen input by 1 sample if... */
				last++;		/* ...needed to keep flag true */
		}
		
		Ycount += Nout;
		if (Ycount > outCount) {
			 Nout -= (Ycount - outCount);
			 Ycount = outCount;
		}

		if (Nout > OBUFFSIZE) /* Check to see if output buff overflowed */
		  return err_ret("Output array overflow");
		
	    for (c = 0; c < m_nChans; c++)
		{
			for (i = 0; i < Nout; i++)
			{
				outArray[c * outCount + i + Ycount - Nout] = _Y[c][i];
			}
		}

		int act_incount = (int)Nx;

		for (c = 0; c < m_nChans; c++)
		{
			for (i=0; i<IBUFFSIZE-act_incount+Xoff; i++) { /* Copy part of input signal */
				 _X[c][i] = _X[c][i+act_incount]; /* that must be re-used */
			}
		}
		Xread = IBUFFSIZE - Nx; /* Pos in input buff to read new data into */
		Xp = Xoff;

		total_inCount += Nx;

    } while (Ycount < outCount); /* Continue until done */

    inCount = total_inCount;

    return(Ycount);		/* Return # of samples in output file */
}

int CFixPointConverter::FilterUp(short Imp[], 
	                               short ImpD[], 
	                               unsigned short Nwing, 
	                               bool Interp,
	                               short *Xp, 
	                               short Ph, 
	                               short Inc)
{
	short *Hp, *Hdp = NULL, *End;
	short a = 0;
	int v, t;

	v=0;
	Hp = &Imp[Ph>>Na];
	End = &Imp[Nwing];
	
	if (Interp) 
	{
		Hdp = &ImpD[Ph>>Na];
		a = Ph & Amask;
	}
	
	if (Inc == 1)		/* If doing right wing...              */
	{				/* ...drop extra coeff, so when Ph is  */
		End--;			/*    0.5, we don't do too many mult's */
		if (Ph == 0)		/* If the phase is zero...           */
		{			/* ...then we've already skipped the */
			 Hp += Npc;		/*    first sample, so we must also  */
			 Hdp += Npc;		/*    skip ahead in Imp[] and ImpD[] */
		}
	}
	
	if (Interp)
	{
		while (Hp < End) 
		{
			t = *Hp;		/* Get filter coeff */
			t += (((int)*Hdp)*a)>>Na; /* t is now interp'd filter coeff */
			Hdp += Npc;		/* Filter coeff differences step */
			t *= *Xp;		/* Mult coeff by input sample */
			if (t & (1<<(Nhxn-1)))  /* Round, if needed */
				t += (1<<(Nhxn-1));
			t >>= Nhxn;		/* Leave some guard bits, but come back some */
			v += t;			/* The filter output */
			Hp += Npc;		/* Filter coeff step */
			Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
		}
	}	
	else 
	{
		while (Hp < End) 
		{
			t = *Hp;		/* Get filter coeff */
			t *= *Xp;		/* Mult coeff by input sample */
			if (t & (1<<(Nhxn-1)))  /* Round, if needed */
				t += (1<<(Nhxn-1));
			t >>= Nhxn;		/* Leave some guard bits, but come back some */
			v += t;			/* The filter output */
			Hp += Npc;		/* Filter coeff step */
			Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
		}
	}
	return(v);
}


int CFixPointConverter::FilterUD(short Imp[], 
	                               short ImpD[],
	                               unsigned short Nwing, 
	                               bool Interp,
	                               short *Xp, 
	                               short Ph, 
	                               short Inc, 
	                               unsigned short dhb)
{
	short a;
	short *Hp, *Hdp, *End;
	int v, t;
	unsigned int Ho;

	v=0;
	Ho = (Ph*(unsigned int)dhb)>>Np;
	End = &Imp[Nwing];
	if (Inc == 1)		/* If doing right wing...              */
	{				/* ...drop extra coeff, so when Ph is  */
		End--;			/*    0.5, we don't do too many mult's */
		if (Ph == 0)		/* If the phase is zero...           */
			Ho += dhb;		/* ...then we've already skipped the */
	}				/*    first sample, so we must also  */
			/*    skip ahead in Imp[] and ImpD[] */
	
	if (Interp)
	{
		while ((Hp = &Imp[Ho>>Na]) < End) 
		{
			t = *Hp;		/* Get IR sample */
			Hdp = &ImpD[Ho>>Na];  /* get interp (lower Na) bits from diff table*/
			a = Ho & Amask;	/* a is logically between 0 and 1 */
			t += (((int)*Hdp)*a)>>Na; /* t is now interp'd filter coeff */
			t *= *Xp;		/* Mult coeff by input sample */
			if (t & 1<<(Nhxn-1))	/* Round, if needed */
				t += 1<<(Nhxn-1);
			t >>= Nhxn;		/* Leave some guard bits, but come back some */
			v += t;			/* The filter output */
			Ho += dhb;		/* IR step */
			Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
		}
	}
	else 
	{
		while ((Hp = &Imp[Ho>>Na]) < End) 
		{
			t = *Hp;		/* Get IR sample */
			t *= *Xp;		/* Mult coeff by input sample */
			if (t & 1<<(Nhxn-1))	/* Round, if needed */
				t += 1<<(Nhxn-1);
			t >>= Nhxn;		/* Leave some guard bits, but come back some */
			v += t;			/* The filter output */
			Ho += dhb;		/* IR step */
			Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
		}
	}
	return(v);
}
