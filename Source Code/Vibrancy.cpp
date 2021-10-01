/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*	Vignette.cpp	

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to 
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017

*/

#include "Vibrancy.h"

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc

	if (in_data->appl_id == 'PrMr') {
		AEFX_SuiteScoper<PF_PixelFormatSuite1> pixelFormatSuite = AEFX_SuiteScoper<PF_PixelFormatSuite1>(in_data, kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1, out_data);

		// add supported pixel formats
		(*pixelFormatSuite->ClearSupportedPixelFormats)(in_data->effect_ref);

		(*pixelFormatSuite->AddSupportedPixelFormat) (in_data->effect_ref, PrPixelFormat_VUYA_4444_32f);
		(*pixelFormatSuite->AddSupportedPixelFormat) (in_data->effect_ref, PrPixelFormat_BGRA_4444_32f);
		(*pixelFormatSuite->AddSupportedPixelFormat) (in_data->effect_ref, PrPixelFormat_VUYA_4444_32f);
		(*pixelFormatSuite->AddSupportedPixelFormat) (in_data->effect_ref, PrPixelFormat_BGRA_4444_8u);
	}
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	PF_ADD_COLOR("Colour",
		0,
		PF_MAX_CHAN8,
		PF_MAX_CHAN8,
		COLOUR_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX("Vibrance", 
							0.0, 
							100.0, 
							0.0, 
							100.0, 
							100.0,
							PF_Precision_TEN_THOUSANDTHS,
							0,
							0,
							VIBRANCE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX("Gamma",
		0.000,
		3.000,
		0.000,
		3.000,
		1.000,
		PF_Precision_THOUSANDTHS,
		0,
		0,
		GAMMA_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOXX("Fill BG", FALSE, NULL, FILLBG_DISK_ID);

	
	out_data->num_params = VIBRANCY_NUM_PARAMS;

	return err;
}

static PF_Err
GammaFunc16 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel16	*inP, 
	PF_Pixel16	*outP)
{
	PF_Err		err = PF_Err_NONE;

	VibrancyInfo* giP = reinterpret_cast<VibrancyInfo*>(refcon);
	double gamma = giP->gamma;
	int	   bgInt = giP->fillBGInt;

	double red, green, blue;
	red = pow(inP->red*.00778, gamma)* 128.498;
	if (red > 32767) {
		red = 32767;
	}
	green = pow(inP->green * .00778, gamma) * 128.498;
	if (green > 32767) {
		green = 32767;
	}
	blue = pow(inP->blue * .00778, gamma) * 128.498;
	if (blue > 32767) {
		blue = 32767;
	}

	if (isnan(red)) {
		red = 0;
	}
	if (isnan(green)) {
		green = 0;
	}
	if (isnan(blue)) {
		blue = 0;
	}

	if (bgInt == 1) {
		if (inP->alpha != 32767) {
			outP->red = A_u_short(red * (inP->alpha / 32767.0));
			outP->green = A_u_short(green * (inP->alpha / 32767.0));
			outP->blue = A_u_short(blue * (inP->alpha / 32767.0));
			outP->alpha = 32767;
		}
		else {
			outP->red = A_u_short(red);
			outP->green = A_u_short(green);
			outP->blue = A_u_short(blue);
			outP->alpha = inP->alpha;
		}
	}
	else {
		outP->red = A_u_short(red);
		outP->green = A_u_short(green);
		outP->blue = A_u_short(blue);
		outP->alpha = inP->alpha;
	}

	return err;
}

static PF_Err
GammaFunc8 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel8	*inP, 
	PF_Pixel8	*outP)
{
	PF_Err		err = PF_Err_NONE;

	VibrancyInfo* giP = reinterpret_cast<VibrancyInfo*>(refcon);
	double gamma = giP->gamma;
	int	   bgInt = giP->fillBGInt;

	double red, green, blue;
	red = pow(inP->red, gamma);
	if (red > 255) {
		red = 255;
	}
	green = pow(inP->green, gamma);
	if (green > 255) {
		green = 255;
	}
	blue = pow(inP->blue, gamma);
	if (blue > 255) {
		blue = 255;
	}

	if (isnan(red)) {
		red = 0;
	}
	if (isnan(green)) {
		green = 0;
	}
	if (isnan(blue)) {
		blue = 0;
	}


	if (bgInt == 1) {
		if (inP->alpha != 255) {
			outP->red = A_u_char(red * (inP->alpha / 255.0));
			outP->green = A_u_char(green * (inP->alpha / 255.0));
			outP->blue = A_u_char(blue * (inP->alpha / 255.0));
			outP->alpha = 255;
		}
		else {
			outP->red = A_u_char(red);
			outP->green = A_u_char(green);
			outP->blue = A_u_char(blue);
			outP->alpha = inP->alpha;
		}
	} else {
		outP->red = A_u_char(red);
		outP->green = A_u_char(green);
		outP->blue = A_u_char(blue);
		outP->alpha = inP->alpha;
	}


	return err;
}

static bool isImportantColourChannel8(PF_Pixel8 pixel, int channelInt) {
	// channelInt guide
	// 0 == alpha
	// 1 == red
	// 2 == green
	// 3 == blue
	bool result = false;
	bool oneWayTie = false;
	bool twoWayTie = false;
	bool threeWayTie = false;
	int threeWay[3]{};
	int channels[3];
	int middleValue, bottomValue, topValue;
	int topChannelInt = 0,
		middleChannelInt = 0,
		bottomChannelInt = 0;
	// if any of the above ints are 0, do not use that channel

	int tieCounter = 0;
	if (pixel.red == pixel.green) {
		tieCounter++;
	}
	if (pixel.red == pixel.blue) {
		tieCounter++;
	}
	if (pixel.green == pixel.blue) {
		tieCounter++;
	}

	switch (tieCounter) {
	case 0:
		oneWayTie = true;
		break;
	case 1:
		twoWayTie = true;
		break;
	case 2:
		threeWayTie = true;
		break;
	}

	if (oneWayTie == true) {
		// find out where the red channel sits
		if (pixel.red < pixel.green && pixel.red < pixel.blue) {
			bottomChannelInt = 1;
		}
		else if (pixel.red < pixel.green && pixel.red > pixel.blue) {
			middleChannelInt = 1;
		}
		else {
			topChannelInt = 1;
		}

		// find out where the green channel sits
		if (pixel.green < pixel.red && pixel.green < pixel.blue) {
			bottomChannelInt = 2;
		}
		else if (pixel.green < pixel.red && pixel.green > pixel.blue) {
			middleChannelInt = 2;
		}
		else {
			topChannelInt = 2;
		}

		// find out where the blue channel sits
		if (pixel.blue < pixel.red && pixel.blue < pixel.green) {
			bottomChannelInt = 3;
		}
		else if (pixel.blue < pixel.red && pixel.blue > pixel.green) {
			middleChannelInt = 3;
		}
		else {
			topChannelInt = 3;
		}
		channels[0] = middleChannelInt;
		channels[1] = topChannelInt;
	}
	else if (twoWayTie == true) {
		if (pixel.red == pixel.green) {
			channels[0] = 1;
			channels[1] = 2;
		}
		else if (pixel.red == pixel.green) {
			channels[0] = 1;
			channels[1] = 3;
		}
		else {
			channels[0] = 2;
			channels[1] = 3;
		}
	}
	else if (threeWayTie == true) {
		channels[0] = 1;
		channels[1] = 2;
		channels[2] = 3;
	}


	switch (middleChannelInt) {
	case 1:
		middleValue = pixel.red;
		break;
	case 2:
		middleValue = pixel.green;
		break;
	case 3:
		middleValue = pixel.blue;
		break;
	}

	switch (bottomChannelInt) {
	case 1:
		bottomValue = pixel.red;
		break;
	case 2:
		bottomValue = pixel.green;
		break;
	case 3:
		bottomValue = pixel.blue;
		break;
	}

	switch (topChannelInt) {
	case 1:
		topValue = pixel.red;
		break;
	case 2:
		topValue = pixel.green;
		break;
	case 3:
		topValue = pixel.blue;
		break;
	}



	switch (channelInt) {
	case 0:

		break;
	case 1:
		if (channels[0] == 1 || channels[1] == 1 || channels[2] == 1) {
			result = true;
		}
		break;
	case 2:
		if (channels[0] == 2 || channels[1] == 2 || channels[2] == 2) {
			result = true;
		}
		break;
	case 3:
		if (channels[0] == 3 || channels[1] == 3 || channels[2] == 3) {
			result = true;
		}
		break;
	}

	return result;
}

static PF_Err
TintFunc8(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel8* inP,
	PF_Pixel8* outP)
{
	PF_Err		err = PF_Err_NONE;

	VibrancyInfo* giP = reinterpret_cast<VibrancyInfo*>(refcon);
	double gamma = giP->gamma;
	PF_Pixel8 colour = giP->colour8;
	double	  vibrance = (giP->vibrance / 100.0);

	A_long	multiplier;
	bool redChannelB = false;
	bool greenChannelB = false;
	bool blueChannelB = false;

	if (isImportantColourChannel8(colour, 1)) {
		redChannelB = true;
	}
	if (isImportantColourChannel8(colour, 2)) {
		greenChannelB = true;
	}
	if (isImportantColourChannel8(colour, 3)) {
		blueChannelB = true;
	}

		if (inP->alpha > 0) {
			/*multiplier = colour.red / (inP->red+1);
			outP->red = A_u_char(colour.red * multiplier);
			multiplier = colour.green / (inP->green+1);
			outP->green = A_u_char(colour.green * multiplier);
			multiplier = colour.blue / (inP->blue+1);
			outP->blue = A_u_char(colour.blue * multiplier);
			outP->alpha = inP->alpha;*/

			if (redChannelB == true) {
				outP->red = colour.red * vibrance;
			}
			else {
				outP->red = colour.red;
			}
			if (greenChannelB == true) {
				outP->green = colour.green * vibrance;
			}
			else {
				outP->green = colour.green;
			}
			if (blueChannelB == true) {
				outP->blue = colour.blue * vibrance;
			}
			else {
				outP->blue = colour.blue;
			}

			outP->alpha = inP->alpha;

		}
		else {
			outP->red = inP->red;
			outP->green = inP->green;
			outP->blue = inP->blue;
			outP->alpha = inP->alpha;
		}

	
	

	return err;
}

static PF_Err
TintFuncBGRA8(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel8* inP,
	PF_Pixel8* outP)
{
	PF_Err		err = PF_Err_NONE;

	VibrancyInfo* giP = reinterpret_cast<VibrancyInfo*>(refcon);
	double gamma = giP->gamma;
	PF_Pixel8 colour = giP->colour8;
	double	  vibrance = (giP->vibrance * .01);

	A_long	multiplier;
	bool redChannelB = false;
	bool greenChannelB = false;
	bool blueChannelB = false;

	if (isImportantColourChannel8(colour, 1)) {
		redChannelB = true;
	}
	if (isImportantColourChannel8(colour, 2)) {
		greenChannelB = true;
	}
	if (isImportantColourChannel8(colour, 3)) {
		blueChannelB = true;
	}

	if (inP->blue > 0) {
		/*multiplier = colour.red / (inP->red+1);
		outP->red = A_u_char(colour.red * multiplier);
		multiplier = colour.green / (inP->green+1);
		outP->green = A_u_char(colour.green * multiplier);
		multiplier = colour.blue / (inP->blue+1);
		outP->blue = A_u_char(colour.blue * multiplier);
		outP->alpha = inP->alpha;*/

		if (redChannelB == true) {
			outP->green = colour.red * vibrance;
		}
		else {
			outP->green = colour.red;
		}
		if (greenChannelB == true) {
			outP->red = colour.green * vibrance;
		}
		else {
			outP->red = colour.green;
		}
		if (blueChannelB == true) {
			outP->alpha = colour.blue * vibrance;
		}
		else {
			outP->alpha = colour.blue;
		}

		outP->blue = inP->blue;

	}
	else {
		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
		outP->alpha = inP->alpha;
	}




	return err;
}

static PF_Err
TintFuncVUYA32(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP)
{
	PF_Err		err = PF_Err_NONE;

	VibrancyInfo* giP = reinterpret_cast<VibrancyInfo*>(refcon);
	double gamma = giP->gamma;
	PF_Pixel8 colour = giP->colour8;
	PF_Pixel32 colour32;
	colour32.red = colour.red * .0039;
	colour32.green = colour.green * .0039;
	colour32.blue = colour.blue * .0039;
	colour32.alpha = colour.alpha * .0039;
	double	  vibrance = (giP->vibrance * .01);

	PF_Pixel_VUYA_32f convertedPixel;
	convertedPixel.luma = inP->green;
	convertedPixel.Pb = inP->red;
	convertedPixel.Pr = inP->alpha;
	convertedPixel.alpha = inP->blue;

	PF_FpLong R = convertedPixel.luma + 1.403 * convertedPixel.Pr;
	PF_FpLong G = convertedPixel.luma - 0.344 * convertedPixel.Pb - 0.714 * convertedPixel.Pr;
	PF_FpLong B = convertedPixel.luma + 1.77 * convertedPixel.Pb;

	A_long	multiplier;
	bool redChannelB = false;
	bool greenChannelB = false;
	bool blueChannelB = false;

	PF_FpLong newR, newG, newB;

	if (isImportantColourChannel8(colour, 1)) {
		redChannelB = true;
	}
	if (isImportantColourChannel8(colour, 2)) {
		greenChannelB = true;
	}
	if (isImportantColourChannel8(colour, 3)) {
		blueChannelB = true;
	}

	if (inP->blue > 0) {
		/*multiplier = colour.red / (inP->red+1);
		outP->red = A_u_char(colour.red * multiplier);
		multiplier = colour.green / (inP->green+1);
		outP->green = A_u_char(colour.green * multiplier);
		multiplier = colour.blue / (inP->blue+1);
		outP->blue = A_u_char(colour.blue * multiplier);
		outP->alpha = inP->alpha;*/

		if (redChannelB == true) {
			newR = colour32.red * vibrance;
		}
		else {
			newR = colour32.red;
		}
		if (greenChannelB == true) {
			newG = colour32.green * vibrance;
		}
		else {
			newG = colour32.green;
		}
		if (blueChannelB == true) {
			newB = colour32.blue * vibrance;
		}
		else {
			newB = colour32.blue;
		}

		convertedPixel.luma = 0.299 * newR + 0.587 * newG + 0.114 * newB;
		convertedPixel.Pb = (newB - convertedPixel.luma) * 0.565;
		convertedPixel.Pr = (newR - convertedPixel.luma) * 0.713;
		convertedPixel.alpha = 1.0;

		outP->red = convertedPixel.Pb;
		outP->green = convertedPixel.luma;
		outP->alpha = convertedPixel.Pr;


		outP->blue = inP->blue;

	}
	else {
		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
		outP->alpha = inP->alpha;
	}




	return err;
}

static PF_Err
TintFunc16(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel16* inP,
	PF_Pixel16* outP)
{
	PF_Err		err = PF_Err_NONE;

	VibrancyInfo* giP = reinterpret_cast<VibrancyInfo*>(refcon);
	double gamma = giP->gamma;
	PF_Pixel16 colour = giP->colour16;
	double	  vibrance = (giP->vibrance / 100.0);

	A_long	multiplier;
	bool redChannelB = false;
	bool greenChannelB = false;
	bool blueChannelB = false;

	if (isImportantColourChannel8(giP->colour8, 1)) {
		redChannelB = true;
	}
	if (isImportantColourChannel8(giP->colour8, 2)) {
		greenChannelB = true;
	}
	if (isImportantColourChannel8(giP->colour8, 3)) {
		blueChannelB = true;
	}

	if (inP->alpha > 0) {
		/*multiplier = colour.red / (inP->red+1);
		outP->red = A_u_char(colour.red * multiplier);
		multiplier = colour.green / (inP->green+1);
		outP->green = A_u_char(colour.green * multiplier);
		multiplier = colour.blue / (inP->blue+1);
		outP->blue = A_u_char(colour.blue * multiplier);
		outP->alpha = inP->alpha;*/

		if (redChannelB == true) {
			outP->red = colour.red * vibrance;
		}
		else {
			outP->red = colour.red;
		}
		if (greenChannelB == true) {
			outP->green = colour.green * vibrance;
		}
		else {
			outP->green = colour.green;
		}
		if (blueChannelB == true) {
			outP->blue = colour.blue * vibrance;
		}
		else {
			outP->blue = colour.blue;
		}

		outP->alpha = inP->alpha;

	}
	else {
		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
		outP->alpha = inP->alpha;
	}


	return err;
}

PF_Pixel16 convertColour8To16(PF_Pixel8 inputColour) {
	PF_Pixel16 outputColour;
	outputColour.red = inputColour.red * 128.498;
	outputColour.green = inputColour.green * 128.498;
	outputColour.blue = inputColour.blue * 128.498;
	outputColour.alpha = inputColour.alpha * 128.498;

	return outputColour;
}

static PF_Err
IterateFloat(
	PF_InData* in_data,
	long				progress_base,
	long				progress_final,
	PF_EffectWorld* src,
	void* refcon,
	PF_Err(*pix_fn)(void* refcon, A_long x, A_long y, PF_PixelFloat* in, PF_PixelFloat* out),
	PF_EffectWorld* dst)
{
	PF_Err	err = PF_Err_NONE;
	char* localSrc, * localDst;
	localSrc = reinterpret_cast<char*>(src->data);
	localDst = reinterpret_cast<char*>(dst->data);

	for (int y = progress_base; y < progress_final; y++)
	{
		for (int x = 0; x < src->width; x++)
		{
			pix_fn(refcon,
				static_cast<A_long> (x),
				static_cast<A_long> (y),
				reinterpret_cast<PF_PixelFloat*>(localSrc),
				reinterpret_cast<PF_PixelFloat*>(localDst));
			localSrc += 16;
			localDst += 16;
		}
		localSrc += (src->rowbytes - src->width * 16);
		localDst += (dst->rowbytes - src->width * 16);
	}

	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	VibrancyInfo			giP;
	AEFX_CLR_STRUCT(giP);
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top;

	giP.colour8 = params[VIBRANCY_COLOUR]->u.cd.value;
	giP.colour16 = convertColour8To16(giP.colour8);
	giP.vibrance = params[VIBRANCY_VIBRANCE]->u.fs_d.value;
	giP.gamma = params[VIBRANCY_GAMMA]->u.fs_d.value;
	if (params[VIBRANCY_FILLBG]->u.bd.value == FALSE) {
		giP.fillBGInt = 0;
	}
	else {
		giP.fillBGInt = 1;
	}


	if (in_data->appl_id != 'PrMr') {
		if (PF_WORLD_IS_DEEP(output)) {

			ERR(suites.Iterate16Suite1()->iterate(in_data,
				0,								// progress base
				linesL,							// progress final
				&params[VIBRANCY_INPUT]->u.ld,	// src 
				NULL,							// area - null for all pixels
				(void*)&giP,					// refcon - your custom data pointer
				TintFunc16,				// pixel function pointer
				output));


			//ERR(suites.Iterate16Suite1()->iterate(in_data,
			//	0,								// progress base
			//	linesL,							// progress final
			//	output,	// src 
			//	NULL,							// area - null for all pixels
			//	(void*)&giP,					// refcon - your custom data pointer
			//	GammaFunc16,				// pixel function pointer
			//	output));
		}
		else {
			ERR(suites.Iterate8Suite1()->iterate(in_data,
				0,								// progress base
				linesL,							// progress final
				&params[VIBRANCY_INPUT]->u.ld,	// src 
				NULL,							// area - null for all pixels
				(void*)&giP,					// refcon - your custom data pointer
				TintFunc8,				// pixel function pointer
				output));

			ERR(suites.Iterate8Suite1()->iterate(in_data,
				0,								// progress base
				linesL,							// progress final
				output,	// src 
				NULL,							// area - null for all pixels
				(void*)&giP,					// refcon - your custom data pointer
				GammaFunc8,				// pixel function pointer
				output));
		}
	}
	else {
		// definitely is premiere
		AEFX_SuiteScoper<PF_PixelFormatSuite1> pixelFormatSuite =
			AEFX_SuiteScoper<PF_PixelFormatSuite1>(in_data,
				kPFPixelFormatSuite,
				kPFPixelFormatSuiteVersion1,
				out_data);

		PrPixelFormat destinationPixelFormat = PrPixelFormat_BGRA_4444_8u;

		pixelFormatSuite->GetPixelFormat(output, &destinationPixelFormat);

		AEFX_SuiteScoper<PF_Iterate8Suite1> iterate8Suite =
			AEFX_SuiteScoper<PF_Iterate8Suite1>(in_data,
				kPFIterate8Suite,
				kPFIterate8SuiteVersion1,
				out_data);

		switch (destinationPixelFormat)
		{

		case PrPixelFormat_BGRA_4444_8u:
			/// good
			iterate8Suite->iterate(in_data,
				0,								// progress base
				output->height,							// progress final
				&params[VIBRANCY_INPUT]->u.ld,	// src 
				NULL,							// area - null for all pixels
				(void*)&giP,					// refcon - your custom data pointer
				TintFuncBGRA8,				// pixel function pointer
				output);

			break;
		case PrPixelFormat_VUYA_4444_8u:

			break;
		case PrPixelFormat_BGRA_4444_32f:

			break;
		case PrPixelFormat_VUYA_4444_32f:
			/// good
			IterateFloat(in_data, 0, output->height, &params[VIBRANCY_INPUT]->u.ld, (void*)&giP, TintFuncVUYA32, output);
			break;
		default:
			//	Return error, because we don't know how to handle the specified pixel type
			return PF_Err_UNRECOGNIZED_PARAM_TYPE;
		}
	}

	return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"Vibrancy", // Name
		"NT Vibrancy", // Match Name
		"NTProductions", // Category
		AE_RESERVED_INFO); // Reserved Info

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

