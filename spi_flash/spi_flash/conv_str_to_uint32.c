#include <stdint.h>
#include <string.h>

//------------------------------------------------------------------------------
// Conver char to bin, str(hex) -> bin
// return 
// =0 OK
// !=0 ERROR
//------------------------------------------------------------------------------
int conv_char_to_byte(const uint8_t sin, uint8_t * bout)
{
    if (sin >= 'a' && sin <= 'f'){
        *bout = (uint8_t)sin - 0x57;
        return 0;
    } 
    
    if (sin >= 'A' && sin <= 'F'){
        *bout = (uint8_t)sin - 0x37;
        return 0;
    } 

    if (sin >= '0' && sin <= '9'){
        *bout = (uint8_t)sin - 0x30;
        return 0;
    } 

    return 1;
}

//------------------------------------------------------------------------------
// convertachiya stroki (do 8 char) v chislo,  str(hex) -> bin
// format stroki 0x[0-9a-f] 0xa0000001 i tomu podobnoe
// return
// = 0 - ERROR
// >0 dlinna stroki v bytes
//
// const uint8_t * sin - vhodnaya stroka, dlinna stroki dolhna bit ne bolee 2+8 char
// uint32_t * bout     - vihodnoe slovo 32 bit Unsignaed (4 bytes)
//
//------------------------------------------------------------------------------
// not tested
#define CONV_STR_SIZE_MAX    (10) // 0x12345678
int conv_str_to_uint32(const uint8_t * sin, uint32_t * bout)
{
    int res = 0;
    uint32_t l,i;
    uint32_t conv = 0;
    uint8_t c;

    l = strlen( (const char*)sin );

    if (l == 0 || l <= 2 || l > CONV_STR_SIZE_MAX){
        return 0;
    }
    
    i = 0;
    c = *(sin + i);
    if (c != '0') return 0;

    i++;
    c = *(sin + i);
    if (c != 'x' && c != 'X') return 0;

    i++;
    while ( (c=*(sin + i)) != '\0'){
        res = conv_char_to_byte( c, &c);
        if (res) return 0; // error
        conv = conv << 4;
        conv = conv | c;
        i++;
    }   
    
    *bout = conv;
    return (i - 2) / 2 + (i - 2) % 2; // return colichestvo preobrazovanih bytes v vhodnoy stroke (-2 bytes 0x)
}
