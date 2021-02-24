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

  mcp2221_hidapi_reset(handle);

  return 0;
}
