#include "crn_core.h"
#include "crn_buffer_stream.h"
#include "crn_texture_conversion.h"
#include "crn_stb_image.cpp"
#include "crntotga.h"
#include <cstdio>

unsigned int written_size;

void write_buffer(void* context, void* data, int size)
{
	memcpy((char*) context + written_size, data, size);
	written_size += size;
}

unsigned char* convert_crn_tga(const unsigned char* input_data, const unsigned int input_size, enum crunchFormat_t format, unsigned int *output_size)
{
	crnlib::texture_file_types::format src_file_format;

	switch (format)
	{
		case CRUNCH_CRN:
			src_file_format = crnlib::texture_file_types::cFormatCRN;
			break;
		case CRUNCH_DDS:
			src_file_format = crnlib::texture_file_types::cFormatDDS;
			break;
		case CRUNCH_KTX:
			src_file_format = crnlib::texture_file_types::cFormatKTX;
			break;
		default:
			return nullptr;
	}

	crnlib::buffer_stream in_stream(input_data, input_size);
	crnlib::data_stream_serializer serializer(in_stream);

	crnlib::mipmapped_texture work_tex;
	if (!work_tex.read_from_stream(serializer, src_file_format))
	{
		return nullptr;
	}

	crnlib::texture_conversion::convert_params convert_params;

	if (!work_tex.convert(crnlib::PIXEL_FMT_A8R8G8B8, crnlib::dxt_image::pack_params()))
	{
		return nullptr;
	}

	crnlib::image_u8 tmp;
	crnlib::image_u8* img = work_tex.get_level_image(0, 0, tmp);

	int width = work_tex.get_width();
	int height = work_tex.get_height();

	// HACK: Enough size for the RGBA pixmap + header + footer.
	int scratch_size = 1000 + (4 * width * height);
	unsigned char *scratch_data = (unsigned char*) malloc(scratch_size);

	if (!scratch_data)
	{
		return nullptr;
	}

	// Disable RLE compression to be faster.
	stbi_write_tga_with_rle = 0;

	written_size = 0;

	if (!stbi_write_tga_to_func(&write_buffer, scratch_data, work_tex.get_width(), work_tex.get_height(), 4, img->get_ptr()))
	{
		free(scratch_data);
		return nullptr;
	}

	unsigned char *output_data = (unsigned char*) malloc(written_size);

	if (!output_data)
	{
		free(scratch_data);
		return nullptr;
	}

	memcpy(output_data, scratch_data, written_size);
	*output_size = written_size;

#if 0
	FILE* output_file = fopen("/tmp/output.tga", "w+");
	fwrite(output_data, written_size, 1, output_file);
	fclose(output_file);
#endif

	free(scratch_data);
	return output_data;
}
