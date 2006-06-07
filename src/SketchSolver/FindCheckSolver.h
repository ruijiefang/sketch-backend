#ifndef FINDCHECKSOLVER_H_
#define FINDCHECKSOLVER_H_

#include "BooleanToCNF.h"

class SolverException{
	public:
	int code;
	string msg;
	SolverException(int code_p, const string& msg_p){ msg = msg_p; code = code_p; };
};


class FindCheckSolver{
	private:
	int controlSize;
	SATSolver& mngFind;
	varDir dirFind;	
	
	SATSolver& mngCheck;
	varDir dirCheck;
	int randseed;
	int iterlimit;
	
	map<string, int> controlVars;
	vector<bitVector> inputs;
	int * ctrl;	
	protected:
	int nseeds;
	//Reserved variable names.
	const string IN;
	const string OUT;
	const string SOUT;
	int Nin;
	int Nout;
	
	
	typedef int const* ctrl_iterator;
	
	virtual ctrl_iterator begin()const{
		return ctrl;	
	}
	
	virtual ctrl_iterator end()const{
		return ctrl + controlSize;
	}
	
	virtual void defineSketch(SATSolver& mng, varDir& dir)=0;
	virtual void defineSpec(SATSolver& mng, varDir& dir)=0;
	
	virtual void addEqualsClauses(SATSolver& mng, varDir& dir);
	virtual void addDiffersClauses(SATSolver& mng, varDir& dir);

	virtual void buildChecker();
	virtual void setupCheck();
	virtual void setNewControls(int controls[], int ctrlsize);
	virtual bool check(int controls[], int ctrlsize, int input[]);
		
	virtual void setupFind();
	virtual void addInputsToTestSet(int input[], int insize);
	virtual bool find(int input[], int insize, int controls[]);
	
	virtual void printDiagnostics(SATSolver& mng, char c);
	virtual void printDiagnostics();
	public:
	FindCheckSolver(SATSolver& finder, SATSolver& checker);
	void setIterLimit(int p_iterlimit);
	virtual void declareControl(const string& ctrl, int size);	
	
	virtual int getInSize();
	virtual int getCtrlSize();
	virtual bool solve();
	virtual void setup();
	void set_randseed(int seed){ randseed = seed; };
};

#endif /*FINDCHECKSOLVER_H_*/
