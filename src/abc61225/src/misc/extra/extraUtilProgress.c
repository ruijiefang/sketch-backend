/**CFile****************************************************************

  FileName    [extraUtilProgress.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Progress bar.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id$]

***********************************************************************/

#include <stdio.h>
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct ProgressBarStruct
{
    int              nItemsNext;   // the number of items for the next update of the progress bar
    int              nItemsTotal;  // the total number of items
    int              posTotal;     // the total number of positions
    int              posCur;       // the current position
    FILE *           pFile;        // the output stream 
};

static void Extra_ProgressBarShow( ProgressBar * p, char * pString );
static void Extra_ProgressBarClean( ProgressBar * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the progress bar.]

  Description [The first parameter is the output stream (pFile), where
  the progress is printed. The current printing position should be the
  first one on the given line. The second parameters is the total
  number of items that correspond to 100% position of the progress bar.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ProgressBar * Extra_ProgressBarStart( FILE * pFile, int nItemsTotal )
{
    ProgressBar * p;
    extern int Abc_FrameShowProgress( void * p );
    extern void * Abc_FrameGetGlobalFrame();

    if ( !Abc_FrameShowProgress(Abc_FrameGetGlobalFrame()) ) return NULL;
    p = ALLOC( ProgressBar, 1 );
    memset( p, 0, sizeof(ProgressBar) );
    p->pFile       = pFile;
    p->nItemsTotal = nItemsTotal;
    p->posTotal    = 78;
    p->posCur      = 1;
    p->nItemsNext  = (int)((float)p->nItemsTotal/p->posTotal)*(p->posCur+5)+2;
    Extra_ProgressBarShow( p, NULL );
    return p;
}

/**Function*************************************************************

  Synopsis    [Updates the progress bar.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarUpdate_int( ProgressBar * p, int nItemsCur, char * pString )
{
    if ( p == NULL ) return;
    if ( nItemsCur < p->nItemsNext )
        return;
    if ( nItemsCur > p->nItemsTotal )
        nItemsCur = p->nItemsTotal;
    p->posCur = (int)((double)nItemsCur * p->posTotal / p->nItemsTotal);
    p->nItemsNext  = (int)((double)p->nItemsTotal/p->posTotal)*(p->posCur+10)+1;
    if ( p->posCur == 0 )
        p->posCur = 1;
    Extra_ProgressBarShow( p, pString );
}


/**Function*************************************************************

  Synopsis    [Stops the progress bar.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarStop( ProgressBar * p )
{
    if ( p == NULL ) return;
    Extra_ProgressBarClean( p );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the progress bar of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarShow( ProgressBar * p, char * pString )
{
    int i;
    if ( p == NULL ) return;
    if ( pString )
        fprintf( p->pFile, "%s ", pString );
    for ( i = (pString? strlen(pString) + 1 : 0); i < p->posCur; i++ )
        fprintf( p->pFile, "-" );
    if ( i == p->posCur )
        fprintf( p->pFile, ">" );
    for ( i++  ; i <= p->posTotal; i++ )
        fprintf( p->pFile, " " );
    fprintf( p->pFile, "\r" );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Cleans the progress bar before quitting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ProgressBarClean( ProgressBar * p )
{
    int i;
    if ( p == NULL ) return;
    for ( i = 0; i <= p->posTotal; i++ )
        fprintf( p->pFile, " " );
    fprintf( p->pFile, "\r" );
    fflush( stdout );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

