#include "mcp2221_hidapi.h"

#include <stdio.h>
#include <string.h>


int main(int argc, char const *argv[])
{
  if ((argc == 0) && (argv == NULL)) {}

  struct MCP2221_t *handle = mcp2221_hidapi_init();
  if (handle == NULL)
  {
    printf("no handle!\n");
    return 1;
  }

  char serial[32];
  mcp2221_hidapi_read_usb_serial_number(handle, serial, 32);
  printf ("serial = '%s'\n", serial);

  printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
  for (uint8_t slave_address = 0x00; slave_address < 0x7F; slave_address++)
  {
    uint8_t lo_nibble = slave_address & 0x0F;
    if (lo_nibble == 0)
    {
      printf("\n%02X:", slave_address);
    }
    if (slave_address <= 0x02)
    {
      printf("   ");
      continue;
    }
    if (slave_address >= 0x78)
    {
      printf("   ");
      continue;
    }

    int16_t r = mcp2221_hidapi_i2c_slave_available(handle, slave_address);
    if (r == 0)
    {
      printf(" %02X", slave_address);
    } else
    {
      printf(" --");
    }
  }
  printf("\n");

  return 0;
}
