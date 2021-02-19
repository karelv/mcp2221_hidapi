#include "mcp2221_hidapi.h"
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <stdio.h>



struct MCP2221_t *
mcp2221_hidapi_init_(int16_t index, const char *path,  int vid, int pid)
{
  if ((path == NULL) && (index < 0))
  {
    printf ("mcp2221_hidapi_init: ERROR, specify at least index or path!\n");
    return NULL;
  }

  struct hid_device_info* all_devices = hid_enumerate(vid, pid);

  if (all_devices == NULL)
  {
    printf ("mcp2221_hidapi_init: ERROR, No MCP2221(A) found!\n");
    return NULL;
  }

  int i = 0;
  for (struct hid_device_info *cur_dev = all_devices; cur_dev != NULL; cur_dev = cur_dev->next)
  {
    printf("dev[%i](%04X %04X) => '%s'\n", i, cur_dev->vendor_id, cur_dev->product_id, cur_dev->path);
    i++;
  }    

  struct MCP2221_t *handle = malloc(sizeof(struct MCP2221_t));
  static int8_t is_init = 0;
  memset(handle, 0, sizeof (struct MCP2221_t));
  handle->i2c_frequency_hz_ = 100000; // defaulting to 100kHz.

  if (!is_init)
  {
    if (hid_init() < 0)
    {
      printf("mcp2221_hidapi_init: ERROR, HIDAPI Init failed\n");
      return NULL;
    }
    is_init = 1;
  }

  if (index >= 0)
  {
    int i = 0;
    for (struct hid_device_info *cur_dev = all_devices; cur_dev != NULL; cur_dev = cur_dev->next, i++)
    {
      if (i == index)
      {
        handle->hid_ = hid_open_path(cur_dev->path);
        hid_free_enumeration(all_devices);

        if (handle->hid_ == NULL)
        {
          printf ("mcp2221_hidapi_init: ERROR, no handle!\n");
          free(handle);
          return NULL;
        }
        mcp2221_hidapi_i2c_cancel(handle);
        mcp2221_hidapi_i2c_set_frequency(handle, handle->i2c_frequency_hz_);
        mcp2221_hidapi_i2c_cancel(handle);
        mcp2221_hidapi_i2c_test_lines(handle);
        return handle;
      }
    }

    printf ("mcp2221_hidapi_init: ERROR, index not found!\n");
    hid_free_enumeration(all_devices);
    free(handle);
    return NULL;
  }

  // index is <0, so path must be specified; null pointer is checked above.
  for (struct hid_device_info *cur_dev = all_devices; cur_dev != NULL; cur_dev = cur_dev->next)
  {
    if (!strcmp(cur_dev->path, path))
    {
      handle->hid_ = hid_open_path(cur_dev->path);
      hid_free_enumeration(all_devices);

      if (handle->hid_ == NULL)
      {
        printf ("mcp2221_hidapi_init: ERROR, no handle!\n");
        free(handle);
        return NULL;
      }

      mcp2221_hidapi_i2c_cancel(handle);
      mcp2221_hidapi_i2c_set_frequency(handle, handle->i2c_frequency_hz_);
      mcp2221_hidapi_i2c_cancel(handle);
      mcp2221_hidapi_i2c_test_lines(handle);
      return handle;        
    }
  }
  printf ("mcp2221_hidapi_init: ERROR, path not found!\n");
  hid_free_enumeration(all_devices);
  free(handle);
  return NULL;
}


struct MCP2221_t *
mcp2221_hidapi_init_by_path(const char *path)
{
  return mcp2221_hidapi_init_(-1, path, 0x04d8, 0x00dd);
}


struct MCP2221_t *
mcp2221_hidapi_init_by_index(uint8_t index)
{
  return mcp2221_hidapi_init_(index, NULL, 0x04d8, 0x00dd);
}


struct MCP2221_t *
mcp2221_hidapi_init()
{
  return mcp2221_hidapi_init_(0, NULL, 0x04d8, 0x00dd);
}


void
mcp2221_hidapi_tear_down(struct MCP2221_t *handle)
{
  if (handle != NULL)
  {
    if (handle->hid_ != NULL)
    {
      hid_close(handle->hid_);
    }
    free(handle);
  }
  return;
}



int16_t
mcp2221_hidapi_i2c_smb(struct MCP2221_t *handle, uint8_t use_pec)
{
 if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  handle->use_pec_ = use_pec;
  return 0;
}


int16_t
mcp2221_hidapi_i2c_test_lines(struct MCP2221_t *handle)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));

  buf[0+1] = 0x10; // CMD Status / Set param
  hid_write(handle->hid_, buf, 65);

  memset(buf, 0, sizeof(buf));
  hid_read(handle->hid_, buf, 65);

  if (buf[23] != 1)
  {
    printf("mcp2221_hidapi_i2c_test_lines: SCL is stuck to low!\n");
    return -1;
  }
  if (buf[22] != 1)
  {
    printf("mcp2221_hidapi_i2c_test_lines: SDA is stuck to low!\n");
    return -2;
  }

  return 0;
}


int16_t
mcp2221_hidapi_i2c_set_frequency(struct MCP2221_t *handle, uint32_t frequency_hz)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));

  buf[0+1] = 0x10; // CMD Status / Set param
  buf[1+1] = 0x00; // don't care
  buf[2+1] = 0x00; // 0x10 ==> cancel current transaction
  buf[3+1] = 0x20; // when 0x20 ==> next is clock divider
  buf[4+1] = (12000000 / frequency_hz) - 3; // clock speed.
  hid_write(handle->hid_, buf, 65);

  memset(buf, 0, sizeof(buf));
  hid_read(handle->hid_, buf, 65);

  handle->i2c_frequency_hz_ = frequency_hz;

  return 0;
}


int16_t
mcp2221_hidapi_i2c_cancel(struct MCP2221_t *handle)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));
  buf[0+1] = 0x10; // CMD Status / Set param
  buf[1+1] = 0x00; // don't care
  buf[2+1] = 0x10; // 0x10 ==> cancel current transaction
  hid_write(handle->hid_, buf, 65);

  memset(buf, 0, sizeof(buf));
  int n = hid_read(handle->hid_, buf, 65);

  if (n != 64)
  {
    printf("mcp2221_hidapi_i2c_cancel: ERROR, reading result\n");
    return -1;
  }

  return 0;
}


int16_t
mcp2221_hidapi_reset(struct MCP2221_t *handle)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));

  buf[0+1] = 0x70;
  buf[1+1] = 0xAB;
  buf[2+1] = 0xCD;
  buf[3+1] = 0xEF;
  hid_write(handle->hid_, buf, 65);

  usleep(1000000);
  // no answer is expected!

  return 0;
}


int16_t
mcp2221_hidapi_i2c_state_check(struct MCP2221_t *handle)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));
  buf[0+1] = 0x10; // CMD Status / Set param
  hid_write(handle->hid_, buf, 65);

  memset(buf, 0, sizeof(buf));
  hid_read(handle->hid_, buf, 65);

  return buf[8];
}


int16_t
mcp2221_hidapi_i2c_write_(struct MCP2221_t *handle, uint8_t cmd, uint8_t slave_address, const uint8_t *data, uint16_t size)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));
  buf[0+1] = cmd;
  buf[1+1] = size & 0x00FF;
  buf[2+1] = (size>>8) & 0x00FF;
  buf[3+1] = (slave_address << 1);
  if (size > 60) size = 60;
  for (uint8_t i=0; i<size; i++)
  {
    buf[4+1+i] = data[i];
  }

  hid_write(handle->hid_, buf, 65);

  memset(buf, 0, sizeof(buf));
  hid_read(handle->hid_, buf, 65);

  return 0;
}


int16_t
mcp2221_hidapi_i2c_write(struct MCP2221_t *handle, uint8_t slave_address, const uint8_t *data, uint16_t size)
{
  return mcp2221_hidapi_i2c_write_(handle, 0x90, slave_address, data, size);
}


int16_t
mcp2221_hidapi_i2c_write_repeated(struct MCP2221_t *handle, uint8_t slave_address, const uint8_t *data, uint16_t size)
{
  return mcp2221_hidapi_i2c_write_(handle, 0x92, slave_address, data, size);
}


int16_t
mcp2221_hidapi_i2c_write_no_stop(struct MCP2221_t *handle, uint8_t slave_address, const uint8_t *data, uint16_t size)
{
  return mcp2221_hidapi_i2c_write_(handle, 0x94, slave_address, data, size);
}


int16_t
mcp2221_hidapi_i2c_read_(struct MCP2221_t *handle, uint8_t cmd, uint8_t slave_address, uint8_t *data, uint16_t size)
{
  if (handle == NULL) 
  {
    printf("%s: ERROR: No handle\n", __func__);
    return -9999;
  }
  uint8_t buf[65];
  memset(buf, 0, sizeof(buf));
  buf[0+1] = cmd;
  buf[1+1] = size & 0x00FF;
  buf[2+1] = (size>>8) & 0x00FF;
  buf[3+1] = (slave_address << 1);
  hid_write(handle->hid_, buf, 65);

  memset(buf, 0, sizeof(buf));
  hid_read(handle->hid_, buf, 65);

  if (buf[1] != 0x00)
  { // no ACK => cancel current operation...
    mcp2221_hidapi_i2c_cancel(handle);
    return -1;
  }

  for (int chunk_i = 0; chunk_i * 60 < size; chunk_i++)
  {
    int16_t chunk_size = size;
    chunk_size -= chunk_i * 60;
    if (chunk_size > 60) chunk_size = 60;
    if (chunk_size <= 0) break;
    if (chunk_size > 4)
    {
      usleep(1500); // wait 1.5 ms
    }

    memset(buf, 0, sizeof(buf));
    buf[0+1] = 0x40;
    hid_write(handle->hid_, buf, 65);

    memset(buf, 0, sizeof(buf));
    hid_read(handle->hid_, buf, 65);

    if (buf[1] != 0)
    {
      mcp2221_hidapi_i2c_cancel(handle);
      return -1;
    }

    if (buf[3] != chunk_size) // more than 60 bytes can be read in sequence...
    {
      mcp2221_hidapi_i2c_cancel(handle);
      return -2;
    }

    for (int i=0; i<chunk_size; i++)
    {
      data[(chunk_i * 60) + i] = buf[i+4];
    }
  }
  return 0;
}


int16_t
mcp2221_hidapi_i2c_read(struct MCP2221_t *handle, uint8_t slave_address, uint8_t *data, uint16_t size)
{
  return mcp2221_hidapi_i2c_read_(handle, 0x91, slave_address, data, size);
}


int16_t
mcp2221_hidapi_i2c_read_repeated(struct MCP2221_t *handle, uint8_t slave_address, uint8_t *data, uint16_t size)
{
  return mcp2221_hidapi_i2c_read_(handle, 0x93, slave_address, data, size);
}



int16_t
mcp2221_hidapi_i2c_slave_available(struct MCP2221_t *handle, uint8_t slave_address)
{
  uint8_t data[2];
  return mcp2221_hidapi_i2c_read(handle, slave_address, data, 1);
}


int16_t
mcp2221_hidapi_i2c_sent_general_reset(struct MCP2221_t *handle)
{ // See I2C Spec => 3.1.14 Software reset
  // http://www.nxp.com/documents/user_manual/UM10204.pdf
  uint8_t data[1];
  data[0] = 0x06;
  uint8_t slave_address = 0x00;
  return mcp2221_hidapi_i2c_write(handle, slave_address, data, 1);
}


int16_t
mcp2221_hidapi_i2c_memory_write(struct MCP2221_t *handle, uint8_t slave_address, uint16_t memory_address, uint16_t data, uint16_t delay_ms)
{
  uint8_t buf[4];
  memset(buf, 0, sizeof(buf));
  buf[0] = (memory_address >> 8) & 0xFF;
  buf[1] = memory_address & 0xFF;
  buf[2] = (data >> 8) & 0xFF;
  buf[3] = data & 0xFF;

  int16_t r = mcp2221_hidapi_i2c_write(handle, slave_address, buf, 4);
  if (delay_ms > 0)
  {
    usleep(delay_ms*1000);
  }
  return r;
}


int16_t
mcp2221_hidapi_i2c_memory_read(struct MCP2221_t *handle, uint8_t slave_address, uint16_t memory_address, uint8_t *data, uint16_t size)
{
  uint8_t buf[2];
  memset(buf, 0, sizeof(buf));  
  buf[0] = (memory_address >> 8) & 0xFF;
  buf[1] = memory_address & 0xFF;

  int16_t r = mcp2221_hidapi_i2c_write_no_stop(handle, slave_address, buf, 2);
  if (r != 0)
  {
    mcp2221_hidapi_i2c_cancel(handle);
    return r;
  }
  return mcp2221_hidapi_i2c_read_repeated(handle, slave_address, data, size);
}


int16_t
mcp2221_hidapi_i2c_memory_read_uint16(struct MCP2221_t *handle, uint8_t slave_address, uint16_t memory_address, uint16_t *data, uint16_t size)
{
  uint8_t *data8 = (uint8_t *)data;
  int16_t r = mcp2221_hidapi_i2c_memory_read(handle, slave_address, memory_address, data8, size*2);
  for (uint16_t i=0; i<size; i++)
  { // swap bytes: 
    data[i] = (256 * data8[i*2 + 0] + data8[i*2 + 1]);
  }
  return r;
}


int16_t
mcp2221_hidapi_i2c_write_byte(struct MCP2221_t *handle, uint8_t slave_address, uint8_t data)
{
  uint8_t buf[1];
  buf[0] = data;
  return mcp2221_hidapi_i2c_write(handle, slave_address, buf, 1);
}


int16_t
mcp2221_hidapi_i2c_write_word(struct MCP2221_t *handle, uint8_t slave_address, uint16_t data)
{
  uint8_t buf[2];
  buf[0] = data & 0xFF;
  buf[1] = (data >> 8) & 0xFF;
  return mcp2221_hidapi_i2c_write(handle, slave_address, buf, 2);
}


int16_t
mcp2221_hidapi_i2c_read_byte(struct MCP2221_t *handle, uint8_t slave_address, uint8_t *data)
{
  return mcp2221_hidapi_i2c_read(handle, slave_address, data, 1);
}


int16_t
mcp2221_hidapi_i2c_read_word(struct MCP2221_t *handle, uint8_t slave_address, uint16_t *data)
{
  uint8_t *data8 = (uint8_t *)data;
  int16_t r = mcp2221_hidapi_i2c_read(handle, slave_address, data8, 2);

  // swap bytes: 
  *data = (256 * data8[0] + data8[1]);
  return r;
}
