/**CFile****************************************************************

  FileName    [satUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [C-language MiniSat solver.]

  Synopsis    [Additional SAT solver procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id$]

***********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "satSolver.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct clause_t
{
    int size_learnt;
    lit lits[0];
};

static inline int  clause_size( clause* c )  { return c->size_learnt >> 1; }
static inline lit* clause_begin( clause* c ) { return c->lits;             }

static void Sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, bool fIncrement );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the clauses in the solver into a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverWriteDimacs( sat_solver * p, char * pFileName, lit* assumptionsBegin, lit* assumptionsEnd, int incrementVars )
{
    FILE * pFile;
    void ** pClauses;
    int nClauses, i;

    // count the number of clauses
    nClauses = p->clauses.size + p->learnts.size;
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != l_Undef )
            nClauses++;

    // start the file
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Sat_SolverWriteDimacs(): Cannot open the ouput file.\n" );
        return;
    }
    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFile, "p cnf %d %d\n", p->size, nClauses );

    // write the original clauses
    nClauses = p->clauses.size;
    pClauses = p->clauses.ptr;
    for ( i = 0; i < nClauses; i++ )
        Sat_SolverClauseWriteDimacs( pFile, pClauses[i], incrementVars );

    // write the learned clauses
    nClauses = p->learnts.size;
    pClauses = p->learnts.ptr;
    for ( i = 0; i < nClauses; i++ )
        Sat_SolverClauseWriteDimacs( pFile, pClauses[i], incrementVars );

    // write zero-level assertions
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != l_Undef )
            fprintf( pFile, "%s%d%s\n",
                     (p->assigns[i] == l_False)? "-": "",
                     i + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");

    // write the assumptions
    if (assumptionsBegin) {
        for (; assumptionsBegin != assumptionsEnd; assumptionsBegin++) {
            fprintf( pFile, "%s%d%s\n",
                     lit_sign(*assumptionsBegin)? "-": "",
                     lit_var(*assumptionsBegin) + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");
        }
    }

    fprintf( pFile, "\n" );
    fclose( pFile );
}   

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, bool fIncrement )
{
    lit * pLits = clause_begin(pC);
    int nLits = clause_size(pC);
    int i;

    for ( i = 0; i < nLits; i++ )
        fprintf( pFile, "%s%d ", (lit_sign(pLits[i])? "-": ""),  lit_var(pLits[i]) + (int)(fIncrement>0) );
    if ( fIncrement )
        fprintf( pFile, "0" );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverPrintStats( FILE * pFile, sat_solver * p )
{
    printf( "starts        : %d\n", (int)p->stats.starts );
    printf( "conflicts     : %d\n", (int)p->stats.conflicts );
    printf( "decisions     : %d\n", (int)p->stats.decisions );
    printf( "propagations  : %d\n", (int)p->stats.propagations );
    printf( "inspects      : %d\n", (int)p->stats.inspects );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


