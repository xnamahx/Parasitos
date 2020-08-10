#include "c74_msp.h"
#include "clouds/dsp/granular_processor.h"
#include <iostream>

using namespace c74::max;

static const char* clds_version = "0.5"; 

static t_class* this_class = nullptr;

inline double constrain(double v, double vMin, double vMax) {
	return std::max<double>(vMin, std::min<double>(vMax, v));
}

struct t_parasito {
	t_pxobject m_obj;

	double  f_dummy;

	double f_freeze;
	double f_trig;
	double f_position;
	double f_size;
	double f_pitch;
	double f_density;
	double f_texture;
	double f_mix;
	double f_spread;
	double f_feedback;
	double f_reverb;
	double f_mode;
	double f_mono;
	double f_silence;
	double f_bypass;
	double f_lofi;
	double f_num_channels;

	clouds::GranularProcessor processor;
	bool ltrig;
	clouds::FloatFrame* ibuf;
	clouds::FloatFrame* obuf;
	int iobufsz;

/*	static const int LARGE_BUF = 524288;
	static const int SMALL_BUF = 262144;*/
	static const int LARGE_BUF = 118784;
	static const int SMALL_BUF = 65536 - 128;
	uint8_t* large_buf;
	int      large_buf_size;
	uint8_t* small_buf;
	int      small_buf_size;
};



void parasito_perform64(t_parasito* self, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam) {
    double    *in = ins[0];     // first inlet
    double    *in2 = ins[1];     // first inlet
    double    *out = outs[0];   // first outlet
    double    *out2 = outs[1];   // first outlet

	if (sampleframes > self->iobufsz) {
		self->iobufsz = sampleframes;
		self->ibuf = new clouds::FloatFrame[self->iobufsz];
		self->obuf = new clouds::FloatFrame[self->iobufsz];
		self->processor.reset_buffers();
	}

	for (auto i=0; i<self->iobufsz; i++){
		self->ibuf[i].l = *in++;
		self->ibuf[i].r = *in2++;
	}

	self->processor.Prepare();
	self->processor.Process(self->ibuf, self->obuf, self->iobufsz);

	for (int i = 0; i < self->iobufsz; i++) {
		*out++ = self->obuf[i].l;
        *out2++ = self->obuf[i].r;
	}

}

void* parasito_new(void) {
	t_parasito* self = (t_parasito*)object_alloc(this_class);
	outlet_new(self, "signal");
	outlet_new(self, "signal");
	inlet_new(self, NULL);

	dsp_setup((t_pxobject*)self, 2);

	self->iobufsz = 64;
	self->ibuf = new clouds::FloatFrame[self->iobufsz];
	self->obuf = new clouds::FloatFrame[self->iobufsz];
	self->large_buf_size = t_parasito::LARGE_BUF;
	self->large_buf = new uint8_t[self->large_buf_size];
	self->small_buf_size = t_parasito::SMALL_BUF;
	self->small_buf = new uint8_t[self->small_buf_size];

	self->processor.Init(self->large_buf,self->LARGE_BUF,self->small_buf,self->SMALL_BUF);
	self->processor.mutable_parameters()->dry_wet = 1.0f;

	return (void *)self;
}

void parasito_free(t_parasito* self) {
	dsp_free((t_pxobject*)self);
}

void parasito_dsp64(t_parasito* self, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags) {
	object_method_direct(void, (t_object*, t_object*, t_perfroutine64, long, void*),
						 dsp64, gensym("dsp_add64"), (t_object*)self, (t_perfroutine64)parasito_perform64, 0, NULL);
}


void parasito_assist(t_parasito* self, void* unused, t_assist_function io, long index, char* string_dest) {
	if (io == ASSIST_INLET) {
		switch (index) {
			case 1: 
				strncpy(string_dest,"(signal) L IN", ASSIST_STRING_MAXSIZE); 
				break;
			case 2: 
				strncpy(string_dest,"(signal) R IN", ASSIST_STRING_MAXSIZE); 
				break;
		}
	}
	else if (io == ASSIST_OUTLET) {
		switch (index) {
			case 0: 
				strncpy(string_dest,"(signal) L Output", ASSIST_STRING_MAXSIZE); 
				break;
			case 1: 
				strncpy(string_dest,"(signal) R Output", ASSIST_STRING_MAXSIZE); 
				break;
		}
	}
}

void parasito_freeze(t_parasito *x, double f)
{
  	x->f_freeze = f > 0.5f ? true : false;
	x->processor.mutable_parameters()->freeze = x->f_freeze;
}

void parasito_reverb(t_parasito *x, double f)
{
  	x->f_reverb = f;
	x->processor.mutable_parameters()->reverb = constrain(x->f_reverb, 0.0f, 1.0f);
}

void parasito_diffusion(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_diffusion = constrain(f, 0.0f, 1.0f);
}

void parasito_size(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_size = constrain(f, 0.0f, 1.0f);
}

void parasito_mod_rate(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_mod_rate = constrain(f, 0.0f, 1.0f);
}

void parasito_mod_amount(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_mod_amount = constrain(f, 0.0f, 1.0f);
}

void parasito_ratio(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_ratio = constrain(f, -1.0f, 1.0f) * 12.0f;
}

void parasito_pitch(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_pitch = f;
}

void parasito_density(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_density = constrain(f, 0.0f, 1.0f);
}

void parasito_texture(t_parasito *x, double f)
{
	x->processor.mutable_parameters()->oliverb_texture = constrain(f, 0.0f, 1.0f);
}


void parasito_mix(t_parasito *x, double f)
{
	x->f_mix = f;
	x->processor.mutable_parameters()->dry_wet = constrain(x->f_mix, 0.0f, 1.0f);
}

void parasito_samplerate(t_parasito *x, double f)
{
	x->processor.sample_rate(f);
}



void ext_main(void* r) {
	this_class = class_new("parasito~", (method)parasito_new, (method)parasito_free, sizeof(t_parasito), NULL, A_GIMME, 0);

	class_addmethod(this_class,(method) parasito_assist, "assist",	A_CANT,		0);
	class_addmethod(this_class,(method) parasito_dsp64, "dsp64",	A_CANT,		0);
	
	class_addmethod(this_class,(method) parasito_reverb, "reverb", A_DEFFLOAT, 0);
	class_addmethod(this_class,(method) parasito_samplerate, "samplerate", A_DEFFLOAT, 0);

	class_addmethod(this_class,(method) parasito_diffusion, "diffusion", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_size, "size", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_mod_rate, "mod_rate", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_mod_amount, "mod_amount", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_ratio, "ratio", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_pitch, "pitch", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_density, "density", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_texture, "texture", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_freeze, "freeze", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasito_mix, "mix", A_DEFFLOAT,0);

	class_dspinit(this_class);
	class_register(CLASS_BOX, this_class);
}