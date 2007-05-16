/**CFile****************************************************************

  FileName    [abcCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Consistency checking procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id$]

***********************************************************************/

#include "abc.h"
#include "main.h"
//#include "seq.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static bool Abc_NtkCheckNames( Abc_Ntk_t * pNtk );
static bool Abc_NtkCheckPis( Abc_Ntk_t * pNtk );
static bool Abc_NtkCheckPos( Abc_Ntk_t * pNtk );
//static bool Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
static bool Abc_NtkCheckNet( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet );
static bool Abc_NtkCheckNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode );
static bool Abc_NtkCheckLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pLatch );

static bool Abc_NtkComparePis( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb );
static bool Abc_NtkComparePos( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb );
static bool Abc_NtkCompareLatches( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb );

static int  Abc_NtkIsAcyclicHierarchy( Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the integrity of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheck( Abc_Ntk_t * pNtk )
{ 
   return !Abc_FrameIsFlagEnabled( "check" ) || Abc_NtkDoCheck( pNtk );
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of the network after reading.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckRead( Abc_Ntk_t * pNtk )
{
   return !Abc_FrameIsFlagEnabled( "checkread" ) || Abc_NtkDoCheck( pNtk );
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkDoCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pNet, * pNode;
    int i;

    // check network types
    if ( !Abc_NtkIsNetlist(pNtk) && !Abc_NtkIsLogic(pNtk) && !Abc_NtkIsStrash(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Unknown network type.\n" );
        return 0;
    }
    if ( !Abc_NtkHasSop(pNtk) && !Abc_NtkHasBdd(pNtk) && !Abc_NtkHasAig(pNtk) && !Abc_NtkHasMapping(pNtk) && !Abc_NtkHasBlackbox(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Unknown functionality type.\n" );
        return 0;
    }
    if ( Abc_NtkHasMapping(pNtk) )
    {
        if ( pNtk->pManFunc != Abc_FrameReadLibGen() )
        {
            fprintf( stdout, "NetworkCheck: The library of the mapped network is not the global library.\n" );
            return 0;
        }
    }

    // check CI/CO numbers
    if ( Abc_NtkPiNum(pNtk) + Abc_NtkLatchNum(pNtk) != Abc_NtkCiNum(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Number of CIs does not match number of PIs and latches.\n" );
        fprintf( stdout, "One possible reason is that latches are added twice:\n" );
        fprintf( stdout, "in procedure Abc_NtkCreateObj() and in the user's code.\n" );
        return 0;
    }
    if ( Abc_NtkPoNum(pNtk) + Abc_NtkAssertNum(pNtk) + Abc_NtkLatchNum(pNtk) != Abc_NtkCoNum(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Number of COs does not match number of POs, asserts, and latches.\n" );
        fprintf( stdout, "One possible reason is that latches are added twice:\n" );
        fprintf( stdout, "in procedure Abc_NtkCreateObj() and in the user's code.\n" );
        return 0;
    }

    // check the names
    if ( !Abc_NtkCheckNames( pNtk ) )
        return 0;

    // check PIs and POs
    Abc_NtkCleanCopy( pNtk );
    if ( !Abc_NtkCheckPis( pNtk ) )
        return 0;
    if ( !Abc_NtkCheckPos( pNtk ) )
        return 0;

    // check the connectivity of objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NtkCheckObj( pNtk, pObj ) )
            return 0;

    // if it is a netlist change nets and latches
    if ( Abc_NtkIsNetlist(pNtk) )
    {
        if ( Abc_NtkNetNum(pNtk) == 0 )
        {
            fprintf( stdout, "NetworkCheck: Netlist has no nets.\n" );
            return 0;
        }
        // check the nets
        Abc_NtkForEachNet( pNtk, pNet, i )
            if ( !Abc_NtkCheckNet( pNtk, pNet ) )
                return 0;
    }
    else
    {
        if ( Abc_NtkNetNum(pNtk) != 0 )
        {
            fprintf( stdout, "NetworkCheck: A network that is not a netlist has nets.\n" );
            return 0;
        }
    }

    // check the nodes
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigCheck( pNtk->pManFunc );
    else
    {
        Abc_NtkForEachNode( pNtk, pNode, i )
            if ( !Abc_NtkCheckNode( pNtk, pNode ) )
                return 0;
    }

    // check the latches
    Abc_NtkForEachLatch( pNtk, pNode, i )
        if ( !Abc_NtkCheckLatch( pNtk, pNode ) )
            return 0;

    // finally, check for combinational loops
//  clk = clock();
    if ( !Abc_NtkIsAcyclic( pNtk ) )
    {
        fprintf( stdout, "NetworkCheck: Network contains a combinational loop.\n" );
        return 0;
    }
//  PRT( "Acyclic  ", clock() - clk );

    // check the EXDC network if present
    if ( pNtk->pExdc )
        Abc_NtkCheck( pNtk->pExdc );

    // check the hierarchy
    if ( Abc_NtkIsNetlist(pNtk) && pNtk->tName2Model )
    {
        stmm_generator * gen;
        Abc_Ntk_t * pNtkTemp;
        char * pName;
        // check other networks
        stmm_foreach_item( pNtk->tName2Model, gen, &pName, (char **)&pNtkTemp )
        {
            pNtkTemp->fHiePath = pNtkTemp->fHieVisited = 0;
            if ( !Abc_NtkCheck( pNtkTemp ) )
                return 0;
        }
        // check acyclic dependency of the models
        if ( !Abc_NtkIsAcyclicHierarchy( pNtk ) )
        {
            fprintf( stdout, "NetworkCheck: Network hierarchical dependences contains a cycle.\n" );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    Vec_Int_t * vNameIds;
    char * pName;
    int i, NameId;
/*
    if ( Abc_NtkIsNetlist(pNtk) )
    {

        // check that each net has a name
        Abc_NtkForEachNet( pNtk, pObj, i )
        {
            if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) )
            {
                fprintf( stdout, "NetworkCheck: Net \"%s\" has different name in the name table and at the data pointer.\n", pObj->pData );
                return 0;
            }
        }
    }
    else
*/
    {
        // check that each CI/CO has a name
        Abc_NtkForEachCi( pNtk, pObj, i )
        {
            pObj = Abc_ObjFanout0Ntk(pObj);
            if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) == NULL )
            {
                fprintf( stdout, "NetworkCheck: CI with ID %d is in the network but not in the name table.\n", pObj->Id );
                return 0;
            }
        }
        Abc_NtkForEachCo( pNtk, pObj, i )
        {
            pObj = Abc_ObjFanin0Ntk(pObj);
            if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) == NULL )
            {
                fprintf( stdout, "NetworkCheck: CO with ID %d is in the network but not in the name table.\n", pObj->Id );
                return 0;
            }
        }
    }

    // return the array of all IDs, which have names
    vNameIds = Nm_ManReturnNameIds( pNtk->pManName );
    // make sure that these IDs correspond to live objects
    Vec_IntForEachEntry( vNameIds, NameId, i )
    {
        if ( Vec_PtrEntry( pNtk->vObjs, NameId ) == NULL )
        {
            Vec_IntFree( vNameIds );
            pName = Nm_ManFindNameById(pObj->pNtk->pManName, NameId);
            fprintf( stdout, "NetworkCheck: Object with ID %d is deleted but its name \"%s\" remains in the name table.\n", NameId, pName );
            return 0;
        }
    }
    Vec_IntFree( vNameIds );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Checks the PIs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckPis( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // check that PIs are indeed PIs
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsPi(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Object \"%s\" (id=%d) is in the PI list but is not a PI.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        if ( pObj->pData )
        {
            fprintf( stdout, "NetworkCheck: A PI \"%s\" has a logic function.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFaninNum(pObj) > 0 )
        {
            fprintf( stdout, "NetworkCheck: A PI \"%s\" has fanins.\n", Abc_ObjName(pObj) );
            return 0;
        }
        pObj->pCopy = (Abc_Obj_t *)1;
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy == NULL && Abc_ObjIsPi(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Object \"%s\" (id=%d) is a PI but is not in the PI list.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        pObj->pCopy = NULL;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the POs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckPos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // check that POs are indeed POs
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsPo(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Net \"%s\" (id=%d) is in the PO list but is not a PO.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        if ( pObj->pData )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" has a logic function.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFaninNum(pObj) != 1 )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" does not have one fanin.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" has fanouts.\n", Abc_ObjName(pObj) );
            return 0;
        }
        pObj->pCopy = (Abc_Obj_t *)1;
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy == NULL && Abc_ObjIsPo(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Net \"%s\" (id=%d) is in a PO but is not in the PO list.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        pObj->pCopy = NULL;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Checks the connectivity of the object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin, * pFanout;
    int i, Value = 1;
    int k;

    // check the network
    if ( pObj->pNtk != pNtk )
    {
        fprintf( stdout, "NetworkCheck: Object \"%s\" does not belong to the network.\n", Abc_ObjName(pObj) );
        return 0;
    }
    // check the object ID
    if ( pObj->Id < 0 || (int)pObj->Id >= Abc_NtkObjNumMax(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Object \"%s\" has incorrect ID.\n", Abc_ObjName(pObj) );
        return 0;
    }

    if ( !Abc_FrameIsFlagEnabled("checkfio") )
        return Value;

    // go through the fanins of the object and make sure fanins have this object as a fanout
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        if ( Vec_IntFind( &pFanin->vFanouts, pObj->Id ) == -1 )
        {
            fprintf( stdout, "NodeCheck: Object \"%s\" has fanin ", Abc_ObjName(pObj) );
            fprintf( stdout, "\"%s\" but the fanin does not have it as a fanout.\n", Abc_ObjName(pFanin) );
            Value = 0;
        }
    }
    // go through the fanouts of the object and make sure fanouts have this object as a fanin
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( Vec_IntFind( &pFanout->vFanins, pObj->Id ) == -1 )
        {
            fprintf( stdout, "NodeCheck: Object \"%s\" has fanout ", Abc_ObjName(pObj) );
            fprintf( stdout, "\"%s\" but the fanout does not have it as a fanin.\n", Abc_ObjName(pFanout) );
            Value = 0;
        }
    }

    // make sure fanins are not duplicated
    for ( i = 0; i < pObj->vFanins.nSize; i++ )
        for ( k = i + 1; k < pObj->vFanins.nSize; k++ )
            if ( pObj->vFanins.pArray[k] == pObj->vFanins.pArray[i] )
            {
                printf( "Warning: Node %s has", Abc_ObjName(pObj) );
                printf( " duplicated fanin %s.\n", Abc_ObjName(Abc_ObjFanin(pObj,k)) );
            }

    // save time: do not check large fanout lists
    if ( pObj->vFanouts.nSize > 100 )
        return Value;

    // make sure fanouts are not duplicated
    for ( i = 0; i < pObj->vFanouts.nSize; i++ )
        for ( k = i + 1; k < pObj->vFanouts.nSize; k++ )
            if ( pObj->vFanouts.pArray[k] == pObj->vFanouts.pArray[i] )
            {
                printf( "Warning: Node %s has", Abc_ObjName(pObj) );
                printf( " duplicated fanout %s.\n", Abc_ObjName(Abc_ObjFanout(pObj,k)) );
            }

    return Value;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckNet( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet )
{
    if ( Abc_ObjFaninNum(pNet) == 0 )
    {
        fprintf( stdout, "NetworkCheck: Net \"%s\" is not driven.\n", Abc_ObjName(pNet) );
        return 0;
    }
    if ( Abc_ObjFaninNum(pNet) > 1 )
    {
        fprintf( stdout, "NetworkCheck: Net \"%s\" has more than one driver.\n", Abc_ObjName(pNet) );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode )
{
    // detect internal nodes that do not have nets
    if ( Abc_NtkIsNetlist(pNtk) && Abc_ObjFanoutNum(pNode) == 0 )
    {
        fprintf( stdout, "Node (id = %d) has no net to drive.\n", pNode->Id );
        return 0;
    }
    // the node should have a function assigned unless it is an AIG
    if ( pNode->pData == NULL )
    {
        fprintf( stdout, "NodeCheck: An internal node \"%s\" does not have a logic function.\n", Abc_ObjName(pNode) );
        return 0;
    }
    // the netlist and SOP logic network should have SOPs
    if ( Abc_NtkHasSop(pNtk) )
    {
        if ( !Abc_SopCheck( pNode->pData, Abc_ObjFaninNum(pNode) ) )
        {
            fprintf( stdout, "NodeCheck: SOP check for node \"%s\" has failed.\n", Abc_ObjName(pNode) );
            return 0;
        }
    }
    else if ( Abc_NtkHasBdd(pNtk) )
    {
        int nSuppSize = Cudd_SupportSize(pNtk->pManFunc, pNode->pData);
        if ( nSuppSize > Abc_ObjFaninNum(pNode) )
        {
            fprintf( stdout, "NodeCheck: BDD of the node \"%s\" has incorrect support size.\n", Abc_ObjName(pNode) );
            return 0;
        }
    }
    else if ( !Abc_NtkHasMapping(pNtk) && !Abc_NtkHasAig(pNtk) )
    {
        assert( 0 );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCheckLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pLatch )
{
    int Value = 1;
    // check whether the object is a latch
    if ( !Abc_ObjIsLatch(pLatch) )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" is in a latch list but is not a latch.\n", Abc_ObjName(pLatch) );
        Value = 0;
    }
    // make sure the latch has a reasonable return value
    if ( (int)pLatch->pData < ABC_INIT_ZERO || (int)pLatch->pData > ABC_INIT_DC )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" has incorrect reset value (%d).\n", 
            Abc_ObjName(pLatch), (int)pLatch->pData );
        Value = 0;
    }
    // make sure the latch has only one fanin
    if ( Abc_ObjFaninNum(pLatch) != 1 )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" has wrong number (%d) of fanins.\n", Abc_ObjName(pLatch), Abc_ObjFaninNum(pLatch) );
        Value = 0;
    }
    // make sure the latch has only one fanout
    if ( Abc_ObjFanoutNum(pLatch) != 1 )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" has wrong number (%d) of fanouts.\n", Abc_ObjName(pLatch), Abc_ObjFanoutNum(pLatch) );
        Value = 0;
    }
    // make sure the latch input has only one fanin
    if ( Abc_ObjFaninNum(Abc_ObjFanin0(pLatch)) != 1 )
    {
        fprintf( stdout, "NodeCheck: Input of latch \"%s\" has wrong number (%d) of fanins.\n", 
            Abc_ObjName(Abc_ObjFanin0(pLatch)), Abc_ObjFaninNum(Abc_ObjFanin0(pLatch)) );
        Value = 0;
    }
    // make sure the latch input has only one fanout
    if ( Abc_ObjFanoutNum(Abc_ObjFanin0(pLatch)) != 1 )
    {
        fprintf( stdout, "NodeCheck: Input of latch \"%s\" has wrong number (%d) of fanouts.\n", 
            Abc_ObjName(Abc_ObjFanin0(pLatch)), Abc_ObjFanoutNum(Abc_ObjFanin0(pLatch)) );
        Value = 0;
    }
    // make sure the latch output has only one fanin
    if ( Abc_ObjFaninNum(Abc_ObjFanout0(pLatch)) != 1 )
    {
        fprintf( stdout, "NodeCheck: Output of latch \"%s\" has wrong number (%d) of fanins.\n", 
            Abc_ObjName(Abc_ObjFanout0(pLatch)), Abc_ObjFaninNum(Abc_ObjFanout0(pLatch)) );
        Value = 0;
    }
    return Value;
}




/**Function*************************************************************

  Synopsis    [Compares the PIs of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkComparePis( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    if ( Abc_NtkPiNum(pNtk1) != Abc_NtkPiNum(pNtk2) )
    {
        printf( "Networks have different number of primary inputs.\n" );
        return 0;
    }
    // for each PI of pNet1 find corresponding PI of pNet2 and reorder them
    Abc_NtkForEachPi( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPi(pNtk2,i)) ) != 0 )
        {
            printf( "Primary input #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPi(pNtk2,i)) );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the POs of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkComparePos( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    if ( Abc_NtkPoNum(pNtk1) != Abc_NtkPoNum(pNtk2) )
    {
        printf( "Networks have different number of primary outputs.\n" );
        return 0;
    }
    // for each PO of pNet1 find corresponding PO of pNet2 and reorder them
    Abc_NtkForEachPo( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPo(pNtk2,i)) ) != 0 )
        {
            printf( "Primary output #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPo(pNtk2,i)) );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the latches of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCompareBoxes( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk1) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk2) );
    if ( !fComb )
        return 1;
    if ( Abc_NtkBoxNum(pNtk1) != Abc_NtkBoxNum(pNtk2) )
    {
        printf( "Networks have different number of latches.\n" );
        return 0;
    }
    // for each PI of pNet1 find corresponding PI of pNet2 and reorder them
    Abc_NtkForEachBox( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(Abc_ObjFanout0(pObj1)), Abc_ObjName(Abc_ObjFanout0(Abc_NtkBox(pNtk2,i))) ) != 0 )
        {
            printf( "Box #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(Abc_ObjFanout0(pObj1)), Abc_ObjName(Abc_ObjFanout0(Abc_NtkBox(pNtk2,i))) );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the signals of the networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkCompareSignals( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fOnlyPis, int fComb )
{
    Abc_NtkOrderObjsByName( pNtk1, fComb );
    Abc_NtkOrderObjsByName( pNtk2, fComb );
    if ( !Abc_NtkComparePis( pNtk1, pNtk2, fComb ) )
        return 0;
    if ( !fOnlyPis )
    {
        if ( !Abc_NtkCompareBoxes( pNtk1, pNtk2, fComb ) )
            return 0;
        if ( !Abc_NtkComparePos( pNtk1, pNtk2, fComb ) )
            return 0;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns 0 if the network hierachy contains a cycle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkIsAcyclicHierarchy_rec( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNext;
    Abc_Obj_t * pObj;
    int i;
    // return if visited
    if ( pNtk->fHieVisited )
        return 1;
    pNtk->fHieVisited = 1;
    // return if black box
    if ( Abc_NtkHasBlackbox(pNtk) )
        return 1;
    assert( Abc_NtkIsNetlist(pNtk) );
    // go through all the children networks
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        pNtkNext = pObj->pData;
        assert( pNtkNext != NULL );
        if ( pNtkNext->fHiePath )
            return 0;
        pNtk->fHiePath = 1;
        if ( !Abc_NtkIsAcyclicHierarchy_rec( pNtkNext ) )
            return 0;
        pNtk->fHiePath = 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if the network hierachy contains a cycle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkIsAcyclicHierarchy( Abc_Ntk_t * pNtk )
{
    assert( Abc_NtkIsNetlist(pNtk) && pNtk->tName2Model );
    pNtk->fHiePath = 1;
    return Abc_NtkIsAcyclicHierarchy_rec( pNtk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

