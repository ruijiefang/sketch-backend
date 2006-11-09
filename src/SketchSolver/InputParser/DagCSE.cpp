#include "DagCSE.h"
#include "SATSolver.h"
#include <sstream>


DagCSE::DagCSE(BooleanDAG& p_dag): dag(p_dag)
{
}

DagCSE::~DagCSE()
{
	
	
}



void DagCSE::eliminateCSE(){
	int k=0;
	for(int i=0; i<dag.size(); ){
		// Get the code for this node. 
		dag[i]->accept(*this);
		// look it up in the cse map.		
		Dout(cout<<dag[i]->id<<"  "<<dag[i]->get_name()<<": "<<ccode<<endl) ;
		if( cse_map.find(ccode) != cse_map.end() ){
			// if we do find it, then remove the node and replace it with its cse.
			//replace will remove the current element of the dag, so we don't
			//have to increment either node_it or i.
			bool_node * cse = cse_map[ccode];
			Dout(cout<<"replacing "<<dag[i]->get_name()<<" -> "<<cse->get_name()<<endl );			
			cout<<"replacing "<<dag[i]->get_name()<<" -> "<<cse->get_name()<<endl;
			dag.replace(i, cse); 	
		}else{
			// if we don't find it, just add it.
			cse_map[ccode] = dag[i];
			++i;
		}
	}
	dag.relabel();
	Dout(cout<<" end cse "<<endl);
}



 void DagCSE::visit( AND_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id; 	
 	str<<node.mother_sgn<<"*"<<mid<<"&"<<node.father_sgn<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( OR_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_sgn<<"*"<<mid<<"|"<<node.father_sgn<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( XOR_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_sgn<<"*"<<mid<<"xor"<<node.father_sgn<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( SRC_node& node ){
 	stringstream str;
 	str<<node.id;
 	ccode = str.str();
 }
 void DagCSE::visit( DST_node& node ){
  	stringstream str;
 	str<<node.id;
 	ccode = str.str();
 }
 void DagCSE::visit( PT_node& node ){
 	stringstream str;
 	str<<node.id;
 	ccode = str.str();
 }
 void DagCSE::visit( CTRL_node& node ){
 	stringstream str;
 	str<<node.id;
 	ccode = str.str();
 }
 void DagCSE::visit( PLUS_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"+"<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( TIMES_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"*"<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 
 void DagCSE::visit( DIV_node& node ){
 	 stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"/"<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( MOD_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"%"<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( GT_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<">"<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( GE_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<">="<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( LT_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"<"<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( LE_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"<="<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 void DagCSE::visit( EQ_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	int fid = node.father == NULL? -1: node.father->id;
 	str<<node.mother_quant<<"*"<<mid<<"=="<<node.father_quant<<"*"<<fid;
 	ccode = str.str();
 }
 
 void DagCSE::visit( ARRACC_node& node ){
 	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	str<<node.mother_sgn<<"*"<<mid<<"|";
 	for(int i=0; i<node.multi_mother.size(); ++i){
 		int mmid = node.multi_mother[i] == NULL? -1: node.multi_mother[i]->id;
 		str<<node.multi_mother_sgn[i]<<"*"<<mmid<<",";
 	}
 	ccode = str.str();
 }
 
 void DagCSE::visit( ARRASS_node& node ){
	stringstream str;
 	int mid = node.mother == NULL? -1: node.mother->id;
 	str<<node.mother_sgn<<"*"<<mid<<"="<<node.mother_quant<<"?";
 	for(int i=0; i<node.multi_mother.size(); ++i){
 		int mmid = node.multi_mother[i] == NULL? -1: node.multi_mother[i]->id;
 		str<<node.multi_mother_sgn[i]<<"*"<<mmid<<":";
 	}
 	ccode = str.str();
 }
 void DagCSE::visit( ACTRL_node& node ){
 	stringstream str;
 	for(int i=0; i<node.multi_mother.size(); ++i){
 		int mmid = node.multi_mother[i] == NULL? -1: node.multi_mother[i]->id;
 		str<<node.multi_mother_sgn[i]<<"*"<<mmid<<",";
 	}
 	ccode = str.str();
 }
 
 
 
 
 void DagCSE::visit( ASSERT_node &node){
 	stringstream str;
 	str<<node.id;
 	ccode = str.str();
 }
 
