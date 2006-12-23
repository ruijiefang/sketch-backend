#ifndef SATSOLVER_H
#define SATSOLVER_H

#include <iostream>

using namespace std;


#define Assert( in, msg) if(!(in)){cout<<msg<<endl; exit(1); }
#define Dout( out )       out 
#define dout(out)  /* (cout << "[" << __FUNCTION__ << ":" << __LINE__ << "] " << out << endl); */
#define CheckRepeats( AR, N) /* for(int _i=0; _i<N; ++_i){ for(int _j=_i+1; _j<N; ++_j){ Assert( (AR[_i])/2 != (AR[_j])/2, "REPEAT ENTRY IN CLAUSE "<<_i<<"  "<<_j<<"  "<<AR[_i] ); } } */
#define FileOutput( out ) /* out */


class SATSolver {	

protected:
    string name;
    FileOutput( ofstream output );
public:
    enum SATSolverResult{
	UNDETERMINED,
	UNSATISFIABLE,
	SATISFIABLE,
	TIME_OUT,
	MEM_OUT,
	ABORTED
    };

    SATSolver(const string& name_p):name(name_p){		
	FileOutput( string nm = name; nm += ".circuit"; );
	FileOutput( output.open(nm.c_str()) );		
    }

    virtual void annotate(const string& msg)=0;
    virtual void annotateInput(const string& name, int i, int sz)=0;
    virtual void addChoiceClause(int x, int a, int b, int c)=0;
    virtual void addXorClause(int x, int a, int b)=0;
    virtual void addOrClause(int x, int a, int b)=0;
    virtual void addBigOrClause(int* a, int size)=0;
    virtual void addAndClause(int x, int a, int b)=0;
    virtual void addEqualsClause(int x, int a)=0;
    virtual void addEquateClause(int x, int a)=0;
    virtual void setVarClause(int x)=0;
    virtual void assertVarClause(int x)=0;

    virtual int getVarVal(int id)=0;
    virtual int newVar()=0;

    virtual int newInVar()=0;
    virtual void disableVarBranch(int i)=0;

    virtual bool ignoreOld()=0;

    virtual void deleteClauseGroup(int i)=0;
    virtual int solve()=0;

    virtual void reset()=0;
    virtual void cleanupDatabase()=0;

    virtual void clean()=0;	
    virtual void printDiagnostics(char c)=0;	 
};


#endif
