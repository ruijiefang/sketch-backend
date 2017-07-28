#include "ValueGrad.h"


void ValueGrad::vg_plus(ValueGrad* m, ValueGrad* f, ValueGrad* o) {
	float mval = m->getVal();
	float fval = f->getVal();
	float val = mval + fval;
	o->update(val);
	
	gsl_vector* mgrads = m->getGrad();
	gsl_vector* fgrads = f->getGrad();
	GradUtil::compute_plus_grad(mgrads, fgrads, o->getGrad());
	o->bound();
}

void ValueGrad::vg_times(ValueGrad* m, ValueGrad* f, ValueGrad* o) {
	float mval = m->getVal();
	float fval = f->getVal();
	float val = mval*fval;
	o->update(val);
	
	gsl_vector* mgrads = m->getGrad();
	gsl_vector* fgrads = f->getGrad();
	GradUtil::compute_mult_grad(mval, fval, mgrads, fgrads, o->getGrad());
	o->bound();
}

void ValueGrad::vg_div(ValueGrad* m, ValueGrad* f, ValueGrad* o) {
	float mval = m->getVal();
	float fval = f->getVal();
	float val = mval/fval;
	o->update(val);
	
	gsl_vector* mgrads = m->getGrad();
	gsl_vector* fgrads = f->getGrad();
	GradUtil::compute_div_grad(mval, fval, mgrads, fgrads, o->getGrad());
	o->bound();
}

void ValueGrad::vg_neg(ValueGrad* m, ValueGrad* o) {
	o->update(- m->getVal());
	GradUtil::compute_neg_grad(m->getGrad(), o->getGrad());
	o->bound();
}

// Approximate ite with a sigmoid
// o = f*sigmoid(d) + m*sigmoid(-d)
void ValueGrad::vg_ite(ValueGrad* m, ValueGrad* f, DistanceGrad* d, ValueGrad* o) {
	float s1 = GradUtil::sigmoid(d->dist, d->grad, GradUtil::tmp);
	float v1 = s1 * f->getVal();
	GradUtil::compute_mult_grad(s1, f->getVal(), GradUtil::tmp, f->getGrad(), o->getGrad());
	
	GradUtil::compute_neg_grad(d->grad, GradUtil::tmp1);
	float s2 = GradUtil::sigmoid(-d->dist, GradUtil::tmp1, GradUtil::tmp);
	float v2 = s2*m->getVal();
	GradUtil::compute_mult_grad(s2, m->getVal(), GradUtil::tmp, m->getGrad(), GradUtil::tmp1);
	gsl_blas_daxpy(1.0, GradUtil::tmp1, o->getGrad());
	
	o->update(v1 + v2);
	o->bound();
}

// Approximate ite with d*f + (1 - d)*m
void ValueGrad::vg_ite(ValueGrad* m, ValueGrad* f, ValueGrad* d, ValueGrad* o) {
	float v1 = d->getVal() * f->getVal();
	GradUtil::compute_mult_grad(d->getVal(), f->getVal(), d->getGrad(), f->getGrad(), o->getGrad());
	
	GradUtil::compute_neg_grad(d->getGrad(), GradUtil::tmp);
	float v2 = (1 - d->getVal()) * m->getVal();
	GradUtil::compute_mult_grad(1 - d->getVal(), m->getVal(), GradUtil::tmp, m->getGrad(), GradUtil::tmp1);
	gsl_blas_daxpy(1.0, GradUtil::tmp1, o->getGrad());
	
	o->update(v1 + v2);
	o->bound();
}

void ValueGrad::vg_ite(DistanceGrad* m, DistanceGrad* f, ValueGrad* d, DistanceGrad* o) {
	Assert(m->set && f->set, "Something is wrong dsaf");
	float v1 = d->getVal() * f->dist;
	GradUtil::compute_mult_grad(d->getVal(), f->dist, d->getGrad(), f->grad, o->grad);
	GradUtil::compute_neg_grad(d->getGrad(), GradUtil::tmp);
	float v2 = (1 - d->getVal()) * m->dist;
	GradUtil::compute_mult_grad(1 - d->getVal(), m->dist, GradUtil::tmp, m->grad, GradUtil::tmp1);
	gsl_blas_daxpy(1.0, GradUtil::tmp1, o->grad);
	o->dist = v1 + v2;
	o->set = true;
}


void ValueGrad::vg_equal(ValueGrad* m, ValueGrad* f, DistanceGrad* o) {
	Assert(false, "NYI: value grad equal");
}

void ValueGrad::vg_lt(ValueGrad* m, ValueGrad* f, DistanceGrad* o) {
	o->dist = f->getVal()  - m->getVal();
	gsl_vector_memcpy(o->grad, f->getGrad());
	gsl_vector_sub(o->grad, m->getGrad());
	o->set = true;
}

void ValueGrad::vg_arctan(ValueGrad* m, ValueGrad* o) {
	float x = m->getVal();
	o->update(atan(x));
	GradUtil::compute_arctan_grad(x, m->getGrad(), o->getGrad());
	o->bound();
}

void ValueGrad::vg_sin(ValueGrad* m, ValueGrad* o) {
	float x = m->getVal();
	o->update(sin(x));
	GradUtil::compute_sin_grad(x, m->getGrad(), o->getGrad());
	o->bound();
}

void ValueGrad::vg_cos(ValueGrad* m, ValueGrad* o) {
	float x = m->getVal();
	o->update(cos(x));
	GradUtil::compute_cos_grad(x, m->getGrad(), o->getGrad());
	o->bound();
}

void ValueGrad::vg_tan(ValueGrad* m, ValueGrad* o) {
	float x = m->getVal();
	o->update(tan(x));
	GradUtil::compute_tan_grad(x, m->getGrad(), o->getGrad());
	o->bound();}

void ValueGrad::vg_exp(ValueGrad* m, ValueGrad* o) {
	float x = m->getVal();
	o->update(exp(x));
	GradUtil::compute_exp_grad(x, m->getGrad(), o->getGrad());
	o->bound();
}

void ValueGrad::vg_sqrt(ValueGrad* m, ValueGrad* o) {
	float x = m->getVal();
	o->update(sqrt(x));
	GradUtil::compute_sqrt_grad(x, m->getGrad(), o->getGrad());
	o->bound();
}

void ValueGrad::vg_cast_int_float(ValueGrad* m, ValueGrad* o) {
	vg_copy(m, o);
}

// Copy i1 into i2
void ValueGrad::vg_copy(ValueGrad* i1, ValueGrad* i2) {
	i2->update(i1->getVal());
	gsl_blas_dcopy(i1->getGrad(), i2->getGrad());
}

// o = m + f - mf
void ValueGrad::vg_or(ValueGrad* m, ValueGrad* f, ValueGrad* o) {
	float mval = m->getVal();
	float fval = f->getVal();
	float val = mval + fval - mval*fval;
	o->update(val);
	GradUtil::compute_plus_grad(m->getGrad(), f->getGrad(), o->getGrad());
	GradUtil::compute_mult_grad(mval, fval, m->getGrad(), f->getGrad(), GradUtil::tmp);
	gsl_blas_daxpy(-1.0, GradUtil::tmp, o->getGrad());
}

// o = mf
void ValueGrad::vg_and(ValueGrad* m, ValueGrad* f, ValueGrad* o) {
	float mval = m->getVal();
	float fval = f->getVal();
	float val = mval*fval;
	o->update(val);
	GradUtil::compute_mult_grad(mval, fval, m->getGrad(), f->getGrad(), o->getGrad());
}

// o = 1 - m
void ValueGrad::vg_not(ValueGrad* m, ValueGrad* o) {
	o->update(1 - m->getVal());
	GradUtil::compute_neg_grad(m->getGrad(), o->getGrad());
}
