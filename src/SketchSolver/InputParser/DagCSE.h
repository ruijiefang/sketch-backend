#ifndef DAGCSE_H_
#define DAGCSE_H_

#include "BooleanDAG.h"

class DagCSE : public NodeVisitor
{
	map<string, bool_node*> cse_map;
	BooleanDAG& dag;	
	string ccode;
public:
	DagCSE(BooleanDAG& p_dag);
	virtual ~DagCSE();	
	
	void eliminateCSE();
	
	virtual void visit( AND_node& node );
	virtual void visit( OR_node& node );
	virtual void visit( XOR_node& node );
	virtual void visit( SRC_node& node );
	virtual void visit( DST_node& node );
	virtual void visit( PT_node& node );
	virtual void visit( CTRL_node& node );
	virtual void visit( PLUS_node& node );
	virtual void visit( TIMES_node& node );
	virtual void visit( ARRACC_node& node );
	virtual void visit( DIV_node& node );
	virtual void visit( MOD_node& node );
	virtual void visit( GT_node& node );
	virtual void visit( GE_node& node );
	virtual void visit( LT_node& node );
	virtual void visit( LE_node& node );
	virtual void visit( EQ_node& node );
	virtual void visit( ARRASS_node& node );
	virtual void visit( ACTRL_node& node );
	virtual void visit( ASSERT_node &node);	
};

#endif /*DAGCSE_H_*/