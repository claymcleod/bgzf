/**
 * Title: bgzf_read.c
 * Author: Clay McLeod
 * Description: This C file is an exploratory exercise to
 * 				understand how BAM files are structured.
 * 				This implementation is not written for speed,
 * 				nor is it written to work for BGZF in the 
 * 				general case. It is written to maximize
 * 				readability and understanding on vanilla
 * 				BAM files only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Get a file's size.
 */
int get_file_size(FILE *fp)
{
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

/**
 * Read a number of bytes from a file into a buffer.
 */
void read_bytes(uint8_t *buffer, int num_bytes, FILE *fp)
{
	fread(buffer, sizeof(uint8_t), num_bytes, fp);
}

/**
 * Print bytes out in a sequential fashion.
 */
void print_bytes(uint8_t *bytes, int num_bytes)
{
	for (int i=0; i < num_bytes; i++) 
		printf("%x ", bytes[i]);
}

/**
 * Convert two bytes to short (little endian).
 */
uint16_t bytes_to_short(uint8_t* arr, int offset)
{
	return (short)(((short) arr[offset+1]) << 8) | arr[offset];
}

/**
 * Convert four bytes to int (little endian).
 */
uint32_t bytes_to_int(uint8_t* arr, int offset)
{
	return (short)( (((short) arr[offset+3]) << 24) |
					(((short) arr[offset+2]) << 16) |
					(((short) arr[offset+1]) << 8) |
					arr[offset]);
}

/**
 * Read one BGZF block
 */
int read_bgzf_block(FILE *fp)
{
	static int total_bytes_read=0;

	/**
	 * Header
	 */

	int header_size=12;
	uint8_t header[header_size];
	read_bytes(header, header_size, fp);
	uint16_t extra_length = bytes_to_short(header, 10);

	if (header[0] != 0x1f || header[1] != 0x8b) {
		fputs("Invalid BGZF header magic!\n", stderr);
		exit(1);
	}

	if (header[2] != 0x08 || header[3] != 0x04) {
		fputs("This is a BGZF file, but it's not BGZF!\n", stderr);
		exit(1);
	}

	printf("  [*] Header: "); print_bytes(header, header_size); printf("\n");
	printf("    - Modified time    => %d:%d:%d:%d\n", header[7], header[6], header[5], header[4]);
	printf("    - Extra flags      => %x\n", header[8]);
	printf("    - Operating system => %x\n", header[9]);
	printf("    - Extra length     => %u bytes\n", extra_length);

	/**
	 * Subfields
	 *
	 * Parse subfield which contains block information. Should only
	 * be one subfield BGZF block with a length of 6 bytes for standard
	 * BAM files, so I will ignore all cases with multiple subfields.
	 */

	uint8_t block[6];
	read_bytes(block, 6, fp);
	uint16_t subfield_length = bytes_to_short(block, 2); 
	uint16_t block_size = bytes_to_short(block, 4);

	printf("\n");
	printf("  [*] Block info:\n");
	printf("    - Subfield identifier 1 => %x\n", block[0]);	
	printf("    - Subfield identifier 2 => %x\n", block[1]);	
	printf("    - Subfield length       => %u\n", subfield_length);	
	printf("    - Block size (minus 1)  => %u\n", block_size);	

	/**
	 * Data
	 */

	printf("\n");
	uint16_t total_compressed_bytes=block_size-extra_length-19;
	uint8_t data_buf[total_compressed_bytes];
	uint8_t crc32_buf[4];
	uint8_t input_size_buf[4];

	/** We can just fseek over the bits rather than reading into mem **/
	fseek(fp, total_compressed_bytes, SEEK_CUR);
	total_bytes_read += total_compressed_bytes;
	//read_bytes(data_buf, total_compressed_bytes, fp);
	
	read_bytes(crc32_buf, 4, fp);
	read_bytes(input_size_buf, 4, fp);
	int crc32 = bytes_to_int(crc32_buf, 0);
	int input_size = bytes_to_int(input_size_buf, 0);
	printf("  [*] Data:\n");
	printf("    - Compressed # of bytes => %u\n", total_compressed_bytes);
	printf("    - CRC32                 => %u\n", crc32);
	printf("    - Raw input length      => %u\n", input_size);

	/** End block check **/
	if (crc32 == 0 && input_size == 0) {
		printf("\n\nCounted %d compressed data bytes in total.\n", total_bytes_read);	
		return 0;
	} 

	return 1;
}

/**
 * Parses a BGZF file
 */
void parse_bgzf(FILE *fp)
{
	int i=1;
	while(i > -1) {
		printf("\n");
		printf("######\n");
		printf("### BGZF Block %d\n", i);
		printf("######\n");
		printf("\n");
		if(!read_bgzf_block(fp))
			break;
		i++;
	}
}

/**
 * Main method
 **/
int main(int argc, char* argv[])
{
	if (argc != 2) {
		fputs("\nUsage: bgzf_read sample.bam\n\n", stderr);
		fputs("\tA delightfully ugly way explore a BGZF file.\n\n", stderr);
		return 1;
	}

	FILE *bgzfFile;
	bgzfFile = fopen(argv[1], "r");
	printf("File size: %d bytes", get_file_size(bgzfFile));
	parse_bgzf(bgzfFile);
	fclose(bgzfFile);
}
