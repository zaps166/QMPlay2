/*-
 * Copyright (c) 2005 Boris Mikhaylov
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cmath>
#include <stdlib.h>
#include <memory.h>

#include "bs2b.hpp"

/* Set up bs2b data. */
static void init( t_bs2bdp bs2bdp )
{
    double Fc_lo; /* Lowpass filter cut frequency (Hz) */
    double Fc_hi; /* Highboost filter cut frequency (Hz) */
    double G_lo;  /* Lowpass filter gain (multiplier) */
    double G_hi;  /* Highboost filter gain (multiplier) */
    double GB_lo; /* Lowpass filter gain (dB) */
    double GB_hi; /* Highboost filter gain (dB) ( 0 dB is highs ) */
    double level; /* Feeding level (dB) ( level = GB_lo - GB_hi ) */
    double x;

    if( ( bs2bdp->srate > BS2B_MAXSRATE ) || ( bs2bdp->srate < BS2B_MINSRATE ) )
        bs2bdp->srate = BS2B_DEFAULT_SRATE;

    Fc_lo = bs2bdp->level & 0xffff;
    level = ( bs2bdp->level & 0xffff0000 ) >> 16;

    if( ( Fc_lo > BS2B_MAXFCUT ) || ( Fc_lo < BS2B_MINFCUT ) ||
        ( level > BS2B_MAXFEED ) || ( level < BS2B_MINFEED ) )
    {
        bs2bdp->level = BS2B_DEFAULT_CLEVEL;
        Fc_lo = bs2bdp->level & 0xffff;
        level = ( bs2bdp->level & 0xffff0000 ) >> 16;
    }

    level /= 10.0;

    GB_lo = level * -5.0 / 6.0 - 3.0;
    GB_hi = level / 6.0 - 3.0;

    G_lo  = pow( 10, GB_lo / 20.0 );
    G_hi  = 1.0 - pow( 10, GB_hi / 20.0 );
    Fc_hi = Fc_lo * pow( 2.0, ( GB_lo - 20.0 * log10( G_hi ) ) / 12.0 );

    /* $fc = $Fc / $s;
     * $d  = 1 / 2 / pi / $fc;
     * $x  = exp(-1 / $d);
     */

    x = exp( -2.0 * M_PI * Fc_lo / bs2bdp->srate );
    bs2bdp->b1_lo = x;
    bs2bdp->a0_lo = G_lo * ( 1.0 - x );

    x = exp( -2.0 * M_PI * Fc_hi / bs2bdp->srate );
    bs2bdp->b1_hi = x;
    bs2bdp->a0_hi = 1.0 - G_hi * ( 1.0 - x );
    bs2bdp->a1_hi = -x;

    bs2bdp->gain  = 1.0 / ( 1.0 - G_hi + G_lo );
} /* init() */

/* Single pole IIR filter.
 * O[n] = a0*I[n] + a1*I[n-1] + b1*O[n-1]
 */

/* Lowpass filter */
#define lo_filter( in, out_1 ) \
    ( bs2bdp->a0_lo * in + bs2bdp->b1_lo * out_1 )

/* Highboost filter */
#define hi_filter( in, in_1, out_1 ) \
    ( bs2bdp->a0_hi * in + bs2bdp->a1_hi * in_1 + bs2bdp->b1_hi * out_1 )

static void cross_feed_d( t_bs2bdp bs2bdp, double *sample )
{
    /* Lowpass filter */
    bs2bdp->lfs.lo[ 0 ] = lo_filter( sample[ 0 ], bs2bdp->lfs.lo[ 0 ] );
    bs2bdp->lfs.lo[ 1 ] = lo_filter( sample[ 1 ], bs2bdp->lfs.lo[ 1 ] );

    /* Highboost filter */
    bs2bdp->lfs.hi[ 0 ] =
        hi_filter( sample[ 0 ], bs2bdp->lfs.asis[ 0 ], bs2bdp->lfs.hi[ 0 ] );
    bs2bdp->lfs.hi[ 1 ] =
        hi_filter( sample[ 1 ], bs2bdp->lfs.asis[ 1 ], bs2bdp->lfs.hi[ 1 ] );
    bs2bdp->lfs.asis[ 0 ] = sample[ 0 ];
    bs2bdp->lfs.asis[ 1 ] = sample[ 1 ];

    /* Crossfeed */
    sample[ 0 ] = bs2bdp->lfs.hi[ 0 ] + bs2bdp->lfs.lo[ 1 ];
    sample[ 1 ] = bs2bdp->lfs.hi[ 1 ] + bs2bdp->lfs.lo[ 0 ];

    /* Bass boost cause allpass attenuation */
    sample[ 0 ] *= bs2bdp->gain;
    sample[ 1 ] *= bs2bdp->gain;
} /* cross_feed_d() */

/* Exported functions.
 * See descriptions in "bs2b.h"
 */

t_bs2bdp bs2b_open( void )
{
    t_bs2bdp bs2bdp = NULL;

    if( NULL != ( bs2bdp = ( t_bs2bdp )malloc( sizeof( t_bs2bd ) ) ) )
    {
        memset( bs2bdp, 0, sizeof( t_bs2bd ) );
        bs2b_set_srate( bs2bdp, BS2B_DEFAULT_SRATE );
    }

    return bs2bdp;
} /* bs2b_open() */

void bs2b_close( t_bs2bdp bs2bdp )
{
    free( bs2bdp );
} /* bs2b_close() */

void bs2b_set_level( t_bs2bdp bs2bdp, uint32_t level )
{
    if( NULL == bs2bdp ) return;

    if( level == bs2bdp->level ) return;

    bs2bdp->level = level;
    init( bs2bdp );
} /* bs2b_set_level() */

uint32_t bs2b_get_level( t_bs2bdp bs2bdp )
{
    return bs2bdp->level;
} /* bs2b_get_level() */

void bs2b_set_level_fcut( t_bs2bdp bs2bdp, int fcut )
{
    uint32_t level;

    if( NULL == bs2bdp ) return;

    level = bs2bdp->level;
    level &= 0xffff0000;
    level |= ( uint32_t )fcut;
    bs2b_set_level( bs2bdp, level );
} /* bs2b_set_level_fcut() */

int bs2b_get_level_fcut( t_bs2bdp bs2bdp )
{
    return( ( int )( bs2bdp->level & 0xffff ) );
} /* bs2b_get_level_fcut() */

void bs2b_set_level_feed( t_bs2bdp bs2bdp, int feed )
{
    uint32_t level;

    if( NULL == bs2bdp ) return;

    level = bs2bdp->level;
    level &= ( uint32_t )0xffff;
    level |= ( uint32_t )feed << 16;
    bs2b_set_level( bs2bdp, level );
} /* bs2b_set_level_feed() */

int bs2b_get_level_feed( t_bs2bdp bs2bdp )
{
    return( ( int )( ( bs2bdp->level & 0xffff0000 ) >> 16 ) );
} /* bs2b_get_level_feed() */

int bs2b_get_level_delay( t_bs2bdp bs2bdp )
{
    int fcut;

    fcut = bs2bdp->level & 0xffff; /* get cut frequency */

    if( ( fcut > BS2B_MAXFCUT ) || ( fcut < BS2B_MINFCUT ) )
        return 0;

    return bs2b_level_delay( fcut );
} /* bs2b_get_level_delay() */

void bs2b_set_srate( t_bs2bdp bs2bdp, uint32_t srate )
{
    if( NULL == bs2bdp ) return;

    if( srate == bs2bdp->srate ) return;

    bs2bdp->srate = srate;
    init( bs2bdp );
    bs2b_clear( bs2bdp );
} /* bs2b_set_srate() */

uint32_t bs2b_get_srate( t_bs2bdp bs2bdp )
{
    return bs2bdp->srate;
} /* bs2b_get_srate() */

void bs2b_clear( t_bs2bdp bs2bdp )
{
    if( NULL == bs2bdp ) return;
    memset( &bs2bdp->lfs, 0, sizeof( bs2bdp->lfs ) );
} /* bs2b_clear() */

int bs2b_is_clear( t_bs2bdp bs2bdp )
{
    int loopv = sizeof( bs2bdp->lfs );

    while( loopv )
    {
        if( ( ( char * )&bs2bdp->lfs )[ --loopv ] != 0 )
            return 0;
    }

    return 1;
} /* bs2b_is_clear() */

char const *bs2b_runtime_version( void )
{
    return BS2B_VERSION_STR;
} /* bs2b_runtime_version() */

uint32_t bs2b_runtime_version_int( void )
{
    return BS2B_VERSION_INT;
} /* bs2b_runtime_version_int() */

void bs2b_cross_feed_f( t_bs2bdp bs2bdp, float *sample, int n )
{
    double sample_d[ 2 ];

    if( n > 0 )
    {
        while( n-- )
        {
            sample_d[ 0 ] = ( double )sample[ 0 ];
            sample_d[ 1 ] = ( double )sample[ 1 ];

            cross_feed_d( bs2bdp, sample_d );

            /* Clipping of overloaded samples */
            if( sample_d[ 0 ] >  1.0 ) sample_d[ 0 ] =  1.0;
            if( sample_d[ 0 ] < -1.0 ) sample_d[ 0 ] = -1.0;
            if( sample_d[ 1 ] >  1.0 ) sample_d[ 1 ] =  1.0;
            if( sample_d[ 1 ] < -1.0 ) sample_d[ 1 ] = -1.0;

            sample[ 0 ] = ( float )sample_d[ 0 ];
            sample[ 1 ] = ( float )sample_d[ 1 ];

            sample += 2;
        } /* while */
    } /* if */
} /* bs2b_cross_feed_f() */
