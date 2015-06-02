#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <Sort.h>
using namespace std;

#include "BooleanToCNF.h"
#include "Tvalue.h"

#ifndef SAT_Manager
#define SAT_Manager void *
#endif



void SolverHelper::writeDIMACS(ofstream& dimacs_file){	
	for(map<string, int>::iterator fit = varmap.begin(); fit != varmap.end(); ++fit){
		dimacs_file<<"c hole "<<fit->first<<" "<<(fit->second+1);
		if(arrsize.count(fit->first)>0){
			dimacs_file<<" - "<<(fit->second + 1 + arrsize[fit->first]-1);
		}
		dimacs_file<<endl;
	}
	dimacs_file<<"c YES="<<YES+1<<endl;	
	mng.writeDIMACS(dimacs_file);

}

int
SolverHelper::assertVectorsDiffer (int v1, int v2, int size)
{
    int N = size;
    int lastone = 0;
    for(int i=0; i<N; ++i){		
	int cur = addXorClause(v1+i, v2+i);
	if(lastone != 0){
	    lastone = addOrClause(lastone, cur);
	}else{
	    lastone = cur;
	}		
    }
    return lastone;

}


void SolverHelper::addHelperC(Tvalue& tv){
	if(tv.isSparse() ){
		gvvec& gv = tv.num_ranges;
		int size = gv.size();
		if(size == 1){ return; }
		if(size == 2){
			addHelperC(-gv[0].guard, -gv[1].guard);
			return;
		}
		int* x = new int[size];
		for(int i=0; i<size; ++i){
			x[i] = -gv[i].guard;
		}		
		MSsolverNS::sort(x, size);
		int l = this->setStrBO(x, size, ':', 0);		
		int rv;
		if(!this->memoizer.condAdd(&tmpbuf[0], l, 0, rv)){
			mng.addCountingHelperClause(x, gv.size());
		}
		delete x;

		/*
		gvvec& gv = tv.num_ranges;
		if(gv.size()<7){
			for(int i=0; i<gv.size()-1; ++i){
				for(int j=i+1; j < gv.size(); ++j){
					addHelperC(-gv[i].guard, -gv[j].guard);
				}
			}
		}else{
			addHelperC(-gv[0].guard, -gv[1].guard);
			int t = addOrClause(gv[0].guard, gv[1].guard);
			for(int i=2; i<gv.size()-1; ++i){
				addHelperC(-t, -gv[i].guard);
				t = addOrClause(t, gv[i].guard);
			}
			addHelperC(-t, -gv[gv.size()-1].guard);
		}
		*/
		
	}
	
}





int SolverHelper::setStrTV(Tvalue& tv){
	const gvvec& gv=tv.num_ranges;
	if(gv.size()*20 > tmpbuf.size()){ tmpbuf.resize(gv.size() * 22); }
	char* tch = &tmpbuf[0];
	int p=0;
	for(int i=0; i<gv.size(); ++i){				
		{
			int tt = gv[i].guard;
			tt = tt>0 ? (tt<<1) : ((-tt)<<1 | 0x1);
			writeInt(tch, tt, p);
			tch[p] = '|'; p++;
		}
		{
			int tt = gv[i].value;
			tt = tt>0 ? (tt<<1) : ((-tt)<<1 | 0x1);
			writeInt(tch, tt, p);
			tch[p] = ','; p++;
		}
	}
	tch[p] = 0;
	return p;
}


int SolverHelper::intToBit(int id){
	map<int, int>::iterator it = iToBit.find(id);
	if(it != iToBit.end()){
		return it->second;
	}

	int rv = 0;
	if(mng.iVarHasBitMapping(id, rv)){
		iToBit[id] = rv;
		return rv;
	}

	// first check the cache.

	//Assert that we don't already have a mapping for this int var.
	 
	rv = newAnonymousVar ();
	iToBit[id] = rv;
	Tvalue tv=rv;
	tv.makeSparse(*this);
	mng.addMapping(id, tv);
	regIclause(id, tv);
	return rv;
}

void SolverHelper::regIclause(int id, Tvalue& tv){
	Assert(tv.isSparse(), "noqiue");
	 void* mem = malloc(sizeof(MSsolverNS::Clause) + sizeof(uint32_t)*(tv.getSize()*2+1) );
	 vec<Lit> ps;
	 const gvvec& gv=tv.num_ranges;	 
	 ps.push(toLit(id));
	 for(int i=0; i<gv.size(); ++i){
		 ps.push(lfromInt(gv[i].guard));
		 ps.push(toLit(gv[i].value));
	 }
	 mng.intSpecialClause(ps);
}


int SolverHelper::intClause(Tvalue& tv){
	if(tv.isInt()){ return tv.getId(); }
	
	if(!tv.isSparse()){
		tv.makeSparse(*this);
	}

	if(doMemoization){
		int l = this->setStrTV(tv);
		int rv;
		int tt = mng.nextIntVar();
		if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
			tv.makeSuperInt(rv);
			return rv;
		}
	}
	Assert(tv.getSize()>0, "WTF?!!");

	if(tv.getSize()==1){
		int id = mng.addIntVar();
		const gvvec& gv=tv.num_ranges;
		int val = gv[0].value;
		mng.setIntVal(id, val);
		tv.makeSuperInt(id);
		cout<<" TV="<<tv<<endl;
		return id;
	}

	int id = mng.addIntVar(tv);
	regIclause(id, tv);	
	 tv.makeSuperInt(id);
	 cout<<" TV="<<tv<<endl;
	 return id;	
}



int SolverHelper::plus(int x, int y){
		int vx ;
		bool kx = mng.isIntVarKnown(x, vx);
		int vy ;
		bool ky = mng.isIntVarKnown(y, vy);
		if(kx && ky){			
			Tvalue tv;
			tv.makeIntVal(YES, vx+vy);
			intClause(tv);
			return tv.getId();
		}
		if(kx && vx==0){
			return y;
		}
		if(ky && vy==0){
			return x;
		}

		if(doMemoization){
			int l = this->setStr(min(x,y), '+' ,max(x,y));
			int rv;
			int tt = mng.nextIntVar();
			if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
				return rv;
			}
		}
		return mng.plus(x, y);
	}

int SolverHelper::minus(int x, int y){
	int vy ;
	bool ky = mng.isIntVarKnown(y, vy);
	if(ky && vy==0){
		return x;
	}
	int vx ;	
	if(ky && mng.isIntVarKnown(x, vx)){			
		Tvalue tv;
		tv.makeIntVal(YES, vx-vy);
		intClause(tv);
		return tv.getId();
	}

	if(doMemoization){
		int l = this->setStr(x, '-' ,y);
		int rv;
		int tt = mng.nextIntVar();
		if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
			return rv;
		}
	}
	return mng.minus(x, y);
}

int SolverHelper::times(int x, int y){
		int vx ;
		bool kx = mng.isIntVarKnown(x, vx);
		int vy ;
		bool ky = mng.isIntVarKnown(y, vy);
		if((kx && ky) || (kx &&vx==0) || (ky&&vy==0) ){			
			Tvalue tv;
			tv.makeIntVal(YES, vx*vy);
			intClause(tv);
			return tv.getId();
		}
		if(kx && vx==1){
			return y;
		}
		if(ky && vy==1){
			return x;
		}
		if(doMemoization){
			int l = this->setStr(min(x,y), '*' ,max(x,y));
			int rv;
			int tt = mng.nextIntVar();
			if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
				return rv;
			}
		}
		return mng.times(x, y);
	}

int SolverHelper::mod(int x, int y){
		int vx=1 ;
		bool kx = mng.isIntVarKnown(x, vx);
		int vy=1 ;
		bool ky = mng.isIntVarKnown(y, vy);
		if((kx && ky) || (kx &&vx==0) || (ky&&vy==0) || (ky&&vy==1) ){			
			Tvalue tv;
			tv.makeIntVal(YES, vy==0? 0: vx%vy);
			intClause(tv);
			return tv.getId();
		}
		

		if(doMemoization){
			int l = this->setStr(x, '%' ,y);
			int rv;
			int tt = mng.nextIntVar();
			if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
				return rv;
			}
		}
		return mng.mod(x, y);
	}

int SolverHelper::div(int x, int y){
		int vx=1 ;
		bool kx = mng.isIntVarKnown(x, vx);
		int vy=1 ;
		bool ky = mng.isIntVarKnown(y, vy);
		if((kx && ky) || (kx &&vx==0) || (ky&&vy==0) ){			
			Tvalue tv;
			tv.makeIntVal(YES, vy==0? 0: vx/vy);
			intClause(tv);
			return tv.getId();
		}
		if(ky&&vy==1){
			return x;
		}

		if(doMemoization){
			int l = this->setStr(x, '/' ,y);
			int rv;
			int tt = mng.nextIntVar();
			if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
				return rv;
			}
		}
		return mng.div(x, y);
	}

int SolverHelper::inteq(int x, int y){
	int vx ;		
	int vy ;
		
	if( mng.isIntVarKnown(x, vx) &&   mng.isIntVarKnown(y, vy) ){
		
		if(vx==vy){
			return YES;
		}else{
			return -YES;
		}		
	}

	if(doMemoization){
		int l = this->setStr(min(x,y), '=' ,max(x,y));
		int rv;
		int tt = lastVar + 1;
		if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
			return rv;
		}
	}
	int rv = newAnonymousVar ();
	Tvalue tv=rv;
	tv.makeSparse(*this);	
	int iv = intClause(tv);
	mng.inteq(x, y, iv);	
	bitToInt[rv] = iv;
	return rv;
}

int SolverHelper::intlt(int x, int y){
	int vx ;		
	int vy ;
		
	if( mng.isIntVarKnown(x, vx) &&   mng.isIntVarKnown(y, vy) ){
		
		if(vx<vy){
			return YES;
		}else{
			return -YES;
		}		
	}

	if(doMemoization){
		int l = this->setStr(x, '<' ,y);
		int rv;
		int tt = lastVar + 1;
		if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
			return rv;
		}
	}
	int rv = newAnonymousVar ();
	Tvalue tv=rv;
	tv.makeSparse(*this);
	int iv = intClause(tv);
	mng.intlt(x, y, iv);		
	return rv;
}

int SolverHelper::mux(iVar cond, int len, iVar* choices){
		int kc ;
		if(mng.isIntVarKnown(cond, kc)){
			if(kc >=0 && kc<len){
				return choices[kc];
			}
			Tvalue tv;
			tv.makeIntVal(YES, 0);
			intClause(tv);
			return tv.getId();
		}

		for(int i=0; i<len; ++i){
			if(choices[i]==cond){
				Tvalue tv;
				tv.makeIntVal(YES, i);
				intClause(tv);
				choices[i] = tv.getId();
			}
		}


		if(doMemoization){
			int l = this->setStriMux(cond, choices, len);
			int rv;
			int tt = mng.nextIntVar();
			if(this->intmemo.condAdd(&tmpbuf[0], l, tt, rv)){
				return rv;
			}
		}
		return mng.intmux(cond, len, choices);
	}


void SolverHelper::addHelperC(int l1, int l2){
	if(l1 == -l2)
		return;
	mng.addHelper2Clause(l1, l2);
}

/*
int
SolverHelper::select (int choices[], int control, int nchoices, int bitsPerChoice)
{
    int outvar = getVarCnt();	
    for(int i=0; i<bitsPerChoice; ++i){
	newAnonymousVar();
    }	
    for(int j=0; j<bitsPerChoice; ++j){
	mng.setVarClause( -(newAnonymousVar()));
	for(int i=0; i<nchoices; ++i){
	    int cvar = newAnonymousVar();
	    mng.addAndClause( cvar, control+i, choices[i]+j);
	    int cvar2 = newAnonymousVar();
	    mng.addOrClause( cvar2, cvar, cvar-1);
	}
	mng.addEqualsClause( outvar+j, getVarCnt()-1);
    }
    return outvar;
}

int
SolverHelper::selectMinGood (int choices[], int control, int nchoices, int bitsPerChoice)
{
    int outvar = select(choices, control, nchoices, bitsPerChoice);	
    int differences = getVarCnt();	
    int prev = newAnonymousVar();
    mng.setVarClause(-prev);
    for(int i=1; i<nchoices; ++i){
	int different = assertVectorsDiffer(choices[i-1], choices[i], bitsPerChoice);
	int cvar = newAnonymousVar();
	mng.addAndClause( cvar, control+i, different);
	int cvar2 = newAnonymousVar();
	mng.addOrClause( cvar2, cvar, prev);
	prev = cvar2;
    }
    mng.setVarClause( -prev);	
    return outvar;
}

int
SolverHelper::arbitraryPerm (int input, int insize, int controls[], int ncontrols, int csize)
{
    // ncontrols = sizeof(controls);
    Assert( insize <= csize, "This is an error");
    int OUTSIZE = ncontrols;
    int outvar = getVarCnt(); for(int i=0; i<OUTSIZE; ++i){ newAnonymousVar(); };
    for(int i=0; i<OUTSIZE; ++i){
	int cvarPrev = newAnonymousVar();
	mng.setVarClause( -cvarPrev);
	for(int j=0; j<insize; ++j){
	    int cvar = newAnonymousVar();
	    mng.addAndClause( cvar, controls[i]+j, input+j);
	    int cvar2 = newAnonymousVar();
	    mng.addOrClause( cvar2, cvar, cvarPrev);
	    cvarPrev = cvar2;
	}
	mng.addEqualsClause( outvar+i, cvarPrev);
    }
    return outvar;
}

*/