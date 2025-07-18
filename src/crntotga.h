enum crunchFormat_t
{
	CRUNCH_NONE,
	CRUNCH_CRN,
	CRUNCH_DDS,
	CRUNCH_KTX,
};

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC unsigned char* convert_crn_tga(const unsigned char* input_data, const unsigned int input_size, enum crunchFormat_t format, unsigned int *output_size);

#undef EXTERNC
