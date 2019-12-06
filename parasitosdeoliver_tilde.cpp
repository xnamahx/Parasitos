#include "c74_msp.h"
#include "clouds/dsp/granular_processor.h"
#include <iostream>

using namespace c74::max;

static const char* clds_version = "0.5"; 

static t_class* this_class = nullptr;

inline double constrain(double v, double vMin, double vMax) {
	return std::max<double>(vMin, std::min<double>(vMax, v));
}

inline int min(int a, int b) {
	return (a < b) ? a : b;
}

/** Returns the maximum of `a` and `b` */
inline int max(int a, int b) {
	return (a > b) ? a : b;
}

inline int clamp(int x, int a, int b) {
	return min(max(x, a), b);
}

inline short TO_SHORTFRAME(float v) { return (short(v * 16384.0f)); }
inline float FROM_SHORTFRAME(short v) { return (float(v) / 16384.0f); }

struct t_parasitosdeoliver {
	t_object  x_obj;

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

	//inlet<>  x_in_left{ this, "(signal) Sample index" };
	/*t_inlet*  x_in_right{ this, "(signal) Sample index" };
	t_outlet* x_out_left{ this, "(signal) Sample value at index", "signal" };
	t_outlet* x_out_right{ this, "(signal) Sample value at index", "signal" };*/
	// CLASS_MAINSIGNALIN  = in_left;
	/*t_inlet*  x_in_right;
	t_outlet* x_out_left;
	t_outlet* x_out_right;*/

	clouds::GranularProcessor processor;
	bool ltrig;
	clouds::ShortFrame* ibuf;
	clouds::ShortFrame* obuf;
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



void parasitosdeoliver_perform64(t_parasitosdeoliver* self, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam) {
    if (numouts>0 && numins>0)
    {
	    double    *in = ins[0];     // first inlet
	    double    *in2 = ins[1];     // first inlet
	    double    *out = outs[0];   // first outlet
	    double    *out2 = outs[1];   // first outlet

		/*if (sampleframes > self->iobufsz) {
			delete[] self->ibuf;
			delete[] self->obuf;
			self->iobufsz = sampleframes;
			self->ibuf = new clouds::ShortFrame[self->iobufsz];
			self->obuf = new clouds::ShortFrame[self->iobufsz];
		}*/

		for (auto i=0; i<self->iobufsz; ++i){
			self->ibuf[i].l = clamp((*in++) * 32767.0f, -32768.0f, 32767.0f);
			self->ibuf[i].r = clamp((*in2++) * 32767.0f, -32768.0f, 32767.0f);
		}

		self->processor.Prepare();
		self->processor.Process(self->ibuf, self->obuf, self->iobufsz);

		for (int i = 0; i < self->iobufsz; i++) {
			*out++ = self->obuf[i].l / 32768.0f;
	        *out2++ = self->obuf[i].r / 32768.0f;
		}

	}
}

void* parasitosdeoliver_new(void) {
	t_parasitosdeoliver* self = (t_parasitosdeoliver*)object_alloc(this_class);


	self->iobufsz = 64;
	self->ibuf = new clouds::ShortFrame[self->iobufsz];
	self->obuf = new clouds::ShortFrame[self->iobufsz];
	self->large_buf_size = t_parasitosdeoliver::LARGE_BUF;
	self->large_buf = new uint8_t[self->large_buf_size];
	self->small_buf_size = t_parasitosdeoliver::SMALL_BUF;
	self->small_buf = new uint8_t[self->small_buf_size];

	self->processor.Init(self->large_buf,self->LARGE_BUF,self->small_buf,self->SMALL_BUF);
	self->processor.mutable_parameters()->dry_wet = 1.0f;

	outlet_new(self, "signal");
	outlet_new(self, "signal");
	inlet_new(self, NULL);

	dsp_setup((t_pxobject*)self, 2);

	return (void *)self;
}

void parasitosdeoliver_free(t_parasitosdeoliver* self) {
	dsp_free((t_pxobject*)self);
}

void parasitosdeoliver_dsp64(t_parasitosdeoliver* self, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags) {
	object_method_direct(void, (t_object*, t_object*, t_perfroutine64, long, void*),
						 dsp64, gensym("dsp_add64"), (t_object*)self, (t_perfroutine64)parasitosdeoliver_perform64, 0, NULL);
}


void parasitosdeoliver_assist(t_parasitosdeoliver* self, void* unused, t_assist_function io, long index, char* string_dest) {
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

void parasitosdeoliver_reverb(t_parasitosdeoliver *x, double f)
{
  	x->f_reverb = f;
	x->processor.mutable_parameters()->reverb = constrain(x->f_reverb, 0.0f, 1.0f);
}

void parasitosdeoliver_diffusion(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_diffusion = constrain(f, 0.0f, 1.0f);
}

void parasitosdeoliver_size(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_size = constrain(f, 0.0f, 1.0f);
}

void parasitosdeoliver_mod_rate(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_mod_rate = constrain(f, 0.0f, 1.0f);
}

void parasitosdeoliver_mod_amount(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_mod_amount = constrain(f, 0.0f, 1.0f);
}

void parasitosdeoliver_ratio(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_ratio = constrain(f, 0.0f, 1.0f) * 12.0f;
}

void parasitosdeoliver_pitch(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_pitch = constrain(f, 0.0f, 1.0f);
}

void parasitosdeoliver_density(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_density = constrain(f, 0.0f, 1.0f);
}

void parasitosdeoliver_texture(t_parasitosdeoliver *x, double f)
{
	x->processor.mutable_parameters()->oliverb_texture = constrain(f, 0.0f, 1.0f);
}


void parasitosdeoliver_mix(t_parasitosdeoliver *x, double f)
{
	x->f_mix = f;
	x->processor.mutable_parameters()->dry_wet = constrain(x->f_mix, 0.0f, 1.0f);
}

void parasitosdeoliver_samplerate(t_parasitosdeoliver *x, double f)
{
	x->processor.sample_rate(f);
}



void ext_main(void* r) {
	this_class = class_new("parasitosdeoliver~", (method)parasitosdeoliver_new, (method)parasitosdeoliver_free, sizeof(t_parasitosdeoliver), NULL, A_GIMME, 0);

	class_addmethod(this_class,(method) parasitosdeoliver_assist, "assist",	A_CANT,		0);
	class_addmethod(this_class,(method) parasitosdeoliver_dsp64, "dsp64",	A_CANT,		0);
	
	class_addmethod(this_class,(method) parasitosdeoliver_reverb, "reverb", A_DEFFLOAT, 0);
	class_addmethod(this_class,(method) parasitosdeoliver_samplerate, "samplerate", A_DEFFLOAT, 0);

	class_addmethod(this_class,(method) parasitosdeoliver_diffusion, "diffusion", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_size, "size", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_mod_rate, "mod_rate", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_mod_amount, "mod_amount", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_ratio, "ratio", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_pitch, "pitch", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_density, "density", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_texture, "texture", A_DEFFLOAT,0);
	class_addmethod(this_class,(method) parasitosdeoliver_mix, "mix", A_DEFFLOAT,0);

	class_dspinit(this_class);
	class_register(CLASS_BOX, this_class);
}