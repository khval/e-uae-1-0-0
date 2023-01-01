
struct video_convert_names
{
	const char *name;
	void (*fn) (void *src, void *dest, int size);
};

const char *get_name_converter_fn_ptr( void *fn_ptr);

void init_lookup_15bit_to_16bit_le( void );
void init_lookup_15bit_to_16bit_be( void );

void convert_15bit_to_16bit_le( uint16 *from, uint16 *to,int  pixels );
void convert_32bit_to_16bit_le( uint32 *from, uint16 *to,int  pixels );

void convert_15bit_to_16bit_be( uint16 *from, uint16 *to,int  pixels );
void convert_32bit_to_16bit_be( uint32 *from, uint16 *to,int  pixels );

void convert_8bit_lookup_to_16bit( char *from, uint16 *to,int  pixels );
void convert_8bit_lookup_to_16bit_2pixels( uint16 *from, uint32 *to,int  pixels );		// from 2x 8bit pixels to 2x 16bit pixels.
void convert_16bit_lookup_to_16bit( uint16 *from, uint16 *to,int  pixels );

void convert_8bit_to_32bit(  char *from, uint32 *to,int  pixels );
void convert_8bit_lookup_to_32bit_2pixels( uint16 *from, double *to,int  pixels );

void convert_15bit_to_32bit( uint16 *from, uint32 *to,int  pixels );
void convert_16bit_to_32bit( uint16 *from, uint32 *to,int  pixels );

extern uint16 *lookup16bit;


