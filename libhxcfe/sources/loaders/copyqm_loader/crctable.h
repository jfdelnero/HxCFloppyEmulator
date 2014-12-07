#define QM_HEADER_SIZE 133

/* CRC table for 0x104C11DB7, bit reverse algorithm */
extern const uint32_t crc32r_table[256];
uint32_t get_u32( unsigned char* buf, int pos );
unsigned int get_u16( unsigned char* buf, int pos );
int get_i16( unsigned char* buf, int pos );
void drv_qm_update_crc( uint32_t* crc, unsigned char byte );

