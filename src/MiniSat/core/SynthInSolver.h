#pragma once

#include "SolverTypes.h"
#include "FloatSupport.h"
#include "Vec.h"
#include "Tvalue.h"

class DagOptim;
class bool_node;

class SolverHelper;

namespace MSsolverNS {

	const int EMPTY=INT32_MIN;

	class mstate {
	public:
		int id;
		int level;
		mstate(int _id, int _level) :id(_id), level(_level) {}
	};


	/**
	
	This is a table that keeps track of the values of inputs and outputs.
	*/
	class InputMatrix {
		int ninputs;
		int nfuns;
		vector<Tvalue> tvs;
		vec<int> grid;
		vec<mstate> stack;

	public:

		void print() {
			for (int i = 0; i < ninputs; ++i) {
				for (int j = 0; j < nfuns; ++j) {
					if (grid[valueid(j, i)] == EMPTY) {
						cout << "E|\t";
					} else {
						cout << grid[valueid(j, i)]<<"|\t";
					}
				}
				cout << endl;
			}
		}

		InputMatrix(int inputs, int outputs) 
			:ninputs(inputs + outputs),
			nfuns(0){

		}
		int valueid(int instance, int inputid) {
			return instance*ninputs + inputid;
		}
		void pushInput(int instance, int inputid, int val, int level) {
			int id = valueid(instance, inputid);
			grid[id] = val;
			stack.push(mstate(id, level));
		}
		void backtrack(int level) {
			int i;
			int j = 0;
			for (i = stack.size() - 1; i >= 0; --i) {
				mstate& ms = stack[i];				
				if (ms.level > level) {
					grid[ms.id] = EMPTY;
					++j;
				} else {
					break;
				}
			}
			stack.shrink(j);			
		}		
		int newInstance(vector<Tvalue>& inputs, vector<Tvalue>& outputs) {
			int sz = inputs.size() + outputs.size();
			tvs.insert(tvs.end(), inputs.begin(), inputs.end());
			tvs.insert(tvs.end(), outputs.begin(), outputs.end());
			grid.growTo(grid.size() + sz, EMPTY);			
			return nfuns++;
		}
		Tvalue& getTval(int id) {
			return tvs[id];
		}
		Tvalue& getTval(int nfun, int input) {
			return tvs[valueid(nfun, input)];
		}
		int getVal(int id) {
			return grid[id];
		}
		int getVal(int nfun, int input) {
			return grid[valueid(nfun, input)];
		}
		int getNumInstances() {
			return nfuns;
		}
	};

	class Synthesizer {
	protected:
		/// This matrix records the values of inputs and outputs to all instances of the function you are trying to synthesize.
		InputMatrix* inout;

		/// FlotatManager is only relevant if you are using floating point values.
		FloatManager& fm;
		
	public:
		/**
		The conflict contains the ids of all the inputs/outputs for all function instances 
		that were involved in making the problem unsatisfiable.
		*/
		vec<int> conflict;
        
        Lit softConflictLit;
        bool softConflict = false;

		Synthesizer(FloatManager& _fm) :fm(_fm) {

		}
        
		/*
		Return true if synthesis succeeds. If false, the conflict tells you what went wrong.
		*/
		virtual bool synthesis(vec<Lit>& suggestions)=0;

		/*
		This is here just in case you need to do something when a new instance gets created.
		*/
		virtual void newInstance() = 0;


		/*
		Finished solving, get ready to generate.
		*/
		virtual void finalize() = 0;

		virtual void backtrack(int level) = 0;
		/*
		params are the inputs to the generator, and the function should use the DagOptim to add 
		the necessary nodes to the dag.
		*/
		virtual bool_node* getExpression(DagOptim* dopt, const vector<bool_node*>& params)=0;

		/**
		This outputs the expression for the frontend.
		*/
		virtual void print(ostream& out) = 0;
    
        virtual void getControls(map<string, string>& values) = 0;
				
		Lit getLit(int inputid, int val) {
			Tvalue& tv = inout->getTval(inputid);
			return tv.litForValue(val);
		}
        
        // Methods for handling input output matrix
        // (override these methods only if you have a better interface than InputMatrix)
        
        virtual void set_inout(int ninputs, int noutputs) {
            inout = new InputMatrix(ninputs, noutputs);
        }
        
        virtual int newInputOutputInstance(vector<Tvalue>& inputs, vector<Tvalue>& outputs) {
            return inout->newInstance(inputs, outputs);
        }
        
        virtual void backtrackInputs(int level) {
            inout->backtrack(level);
        }
        
        virtual void pushInput(int instance, int inputid, int val, int dlevel, vec<Lit>& conf) {
            int idInGrid = inout->valueid(instance,inputid);
            if (inout->getVal(idInGrid) != EMPTY){
                //writing over non EMPTY values
                Tvalue& tv = inout->getTval(idInGrid);
                Lit l1 = tv.litForValue(inout->getVal(idInGrid));
                conf.push(~l1);
                Lit l2 = tv.litForValue(val);
                conf.push(~l2);
            } else {
                inout->pushInput(instance, inputid, val, dlevel);
            }
        }
        
        virtual void getConflictLits(vec<Lit>& conf) {
            for (int i = 0; i < conflict.size(); ++i) {
                int id = conflict[i];
                Tvalue& tv = inout->getTval(id);
                Lit l = tv.litForValue(inout->getVal(id));
                conf.push(~l);
            }
            if (softConflict) {
                conf.push(~softConflictLit);
            }
        }
        
        
	};

	extern int ID;

	class SynthInSolver {
		vec<int> tmpbuf;
		Synthesizer* s;
		int maxlevel;
		int id;
	public:
		int solverIdx; // Stores the index of this object in the sins vectors in the miniSAT solver.
		SynthInSolver(Synthesizer* syn, int inputs, int outputs, int idx) :s(syn), solverIdx(idx) {
			syn->set_inout(inputs, outputs);
			maxlevel = 0;
			id = ++ID;
		}
		~SynthInSolver() {
			delete s;
		}

		bool_node* getExpression(DagOptim* dopt, const vector<bool_node*>& params) {
			return s->getExpression(dopt, params);

		}

		void print(ostream& out) {
			s->print(out);
		}
    
        void getControls(map<string, string>& values) {
            s->getControls(values);
        }

		int newInstance(vector<Tvalue>& inputs, vector<Tvalue>& outputs) {
			s->newInstance();
			return s->newInputOutputInstance(inputs, outputs);
		}
		
		void finalize() {
			s->finalize();
		}

		void backtrack(int level, vec<Lit>& suggestions) {
			if (maxlevel <= level) { return;  }
			s->backtrackInputs(level);
			s->backtrack(level);
			suggestions.clear();
		}

		/* Returns the stack level of the input.
		*/
		Clause* pushInput(int instance, int inputid, int val, int dlevel, vec<Lit>& suggestions) {
			//id = valueid(instance,inputid)
			//write only over EMPTY values
			//cout << "ID=" << id << endl;
            
            vec<Lit> conf;
            s->pushInput(instance, inputid, val, dlevel, conf);
            if (conf.size() > 0) { // Conflict detected
                int sz = sizeof(Clause) + sizeof(uint32_t)*(conf.size());
                tmpbuf.growTo((sz / sizeof(int)) + 1);
                void* mem = (void*)&tmpbuf[0];
                return new (mem) Clause(conf, false);
            }
            // No conflict detected
            maxlevel = dlevel;
            if (!s->synthesis(suggestions)) {
                vec<Lit> conf;
                s->getConflictLits(conf);
                int sz = sizeof(Clause) + sizeof(uint32_t)*(conf.size());
                tmpbuf.growTo((sz / sizeof(int)) + 1);
                void* mem = (void*)&tmpbuf[0];
                return new (mem) Clause(conf, false);
            }
            
            return NULL;
		}

	};

}
