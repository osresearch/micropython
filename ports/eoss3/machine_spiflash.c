/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "machine_spiflash.h"

#include "Fw_global_config.h"
#include "eoss3_dev.h"

#if MICROPY_PY_MACHINE_SPIFLASH


void mp_hal_pin_write(mp_hal_pin_obj_t x, int value)
{
	// hack to hook the pin writing for the cs pin
	//eoss3_spi_cs(!value);
}

static int
eoss3_spi_ioctl(
	void * self,
	uint32_t cmd
)
{
	switch(cmd)
	{
	case MP_QSPI_IOCTL_INIT:
		// enable the SPI hardware, leaving everything else
		// to the default settings.
		if(0) printf("%s: init %08x %08x %08x\n", __func__,
			(int) SPI_MS->CTRLR0,
			(int) SPI_MS->CTRLR1,
			(int) SPI_MS->IMR
		);
		SPI_MS->SSIENR = 0;
		SPI_MS->BAUDR = 32; // default? who knows.
		SPI_MS->CTRLR0 = CTRLR0_TMOD_EEPROM | CTRLR0_DFS_8_BIT;
		SPI_MS->CTRLR1 = 0; // byte at a time
		SPI_MS->IMR = 0;

		// turn off the SPI DMA controller since we're busy waiting
		DMA_SPI_MS->DMA_INTR_MASK = 0;

		return 0;
	case MP_QSPI_IOCTL_DEINIT:
		printf("%s: -----deinit-----\n", __func__);
		SPI_MS->SSIENR = 0;
		return 0;
	case MP_QSPI_IOCTL_BUS_ACQUIRE:
		//printf("%s: select\n", __func__);
		return 0;
    	case MP_QSPI_IOCTL_BUS_RELEASE:
		//printf("%s: unselect\n", __func__);
		SPI_MS->SSIENR = 0;
		SPI_MS->SER = 0;
		return 0;
	default:
		printf("%s: bad ioctl %08x\n", __func__, (int) cmd);
		return -1;
	}
}

static void
eoss3_spi_transfer(
	void * self,
	const uint8_t * tx,
	size_t tx_len,
	uint8_t * rx,
	size_t rx_len
)
{
	//printf("%s: tx %d, rx %d\n", __func__, tx_len, rx_len);

	if ((tx_len != 0 && !tx) || (rx_len != 0 && !rx))
	{
		printf("%s: parameter mismatch tx %p %d, rx %p %d\n",
			__func__, tx, tx_len, rx, rx_len);
		return;
	}
	if (tx_len == 0 && rx_len == 0)
		return;

	// select device and configure dma for the length of data
	SPI_MS->SSIENR = SSIENR_SSI_DISABLE;
	SPI_MS->SER = 0;

	// if there is a receive, round up rx_len to a multiple of four
	if (rx_len)
	{
		const size_t dma_len = ((rx_len + 3) / 4) * 4 - 1;
		SPI_MS->CTRLR0 = CTRLR0_TMOD_EEPROM | CTRLR0_DFS_8_BIT;
		SPI_MS->CTRLR1 = dma_len;
		SPI_MS->IMR |= ISR_RXFIM_ACTIVE;
		DMA_SPI_MS->DMA_DEST_ADDR = (uint32_t) rx;
		DMA_SPI_MS->DMA_XFER_CNT = dma_len;
		DMA_SPI_MS->DMA_INTR_MASK = DMA_RX_DATA_AVAIL_INTR_MSK;
		DMA_SPI_MS->DMA_CTRL = DMA_CTRL_START_BIT;
	} else {
		// no DMA, turn of RX path
		SPI_MS->CTRLR0 = CTRLR0_TMOD_TX | CTRLR0_DFS_8_BIT;
		SPI_MS->CTRLR1 = 0;
		SPI_MS->IMR = 0;
		DMA_SPI_MS->DMA_INTR_MASK = 0;
	}

	SPI_MS->SSIENR = SSIENR_SSI_EN;

	// wait for TxFifo empty and not busy
	while ((SPI_MS->SR & (SR_TFE | SR_BUSY)) != SR_TFE)
		;

	// send the tx data (should check maximum length)
	for(size_t i = 0 ; i < tx_len ; i++)
		SPI_MS->DR0 = *tx++;

	// select the output, which starts draining the FIFO and the DMA?
	SPI_MS->SER = SER_SS_0_N_SELECTED;
	//SPI_MS->IMR |= ISR_TXEIM_ACTIVE;

	// wait for TxFifo empty and not busy
	// which means that the DMA is over.
	while ((SPI_MS->SR & (SR_TFE | SR_BUSY)) != SR_TFE)
		;

	// Shut down the SPI hardware and DMA
	SPI_MS->SSIENR = SSIENR_SSI_DISABLE;
	SPI_MS->SER = 0;
	SPI_MS->IMR = 0;
	DMA_SPI_MS->DMA_INTR_MASK = 0;
	DMA_SPI_MS->DMA_CTRL = DMA_CTRL_STOP_BIT;
}

static void eoss3_write_cmd_data(void *self, uint8_t cmd, size_t len, uint32_t data)
{
	data <<= 8 * (4 - len);
	uint8_t data_buf[5] = {
		cmd,
		(data >> 24) & 0xFF,
		(data >> 16) & 0xFF,
		(data >>  8) & 0xFF,
		(data >>  0) & 0xFF,
	};

	printf("%s: cmd=%02x data=%08x len=%d\n", __func__, (int) cmd, (int) data, (int) len);
	eoss3_spi_transfer(self, data_buf, 1 + len, NULL, 0);
}

static void eoss3_write_cmd_addr_data(void *self, uint8_t cmd, uint32_t addr, size_t len, const uint8_t *src)
{
	uint8_t cmd_buf[4+len];
	cmd_buf[0] = cmd;
	cmd_buf[1] = (addr >> 16) & 0xFF;
	cmd_buf[2] = (addr >>  8) & 0xFF;
	cmd_buf[3] = (addr >>  0) & 0xFF;

	memcpy(cmd_buf + 4, src, len);

	printf("%s: cmd=%02x addr=%06x len=%d\n", __func__, (int) cmd, (int) addr, (int) len);
	eoss3_spi_transfer(self, cmd_buf, sizeof(cmd_buf), NULL, 0);
}

static uint32_t eoss3_read_cmd(void *self, uint8_t cmd, size_t len)
{
	uint8_t buf[4];

	eoss3_spi_transfer(self, &cmd, 1, buf, len);

	// don't print the nearly constant RDSR messages
	if (cmd != 0x05)
	printf("%s: cmd=%02x buf=[%02x %02x %02x %02x]\n",
		__func__, cmd,
		(int) buf[0],
		(int) buf[1],
		(int) buf[2],
		(int) buf[3]
	);

	if (len == 1)
		return buf[0] << 0;
	if (len == 2)
		return buf[1] << 8 | buf[0] << 0;
	if (len == 3)
		return buf[2] << 16 | buf[1] << 8 | buf[0] << 0;
	if (len == 4)
		return buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0] << 0;
	return buf[0];
}

static void eoss3_read_cmd_qaddr_qdata(void *self, uint8_t cmd, uint32_t addr, size_t len, uint8_t *dest)
{
	// remap quad read to single read
	if (cmd == 0xEB)
		cmd = 0x03;

	//uint8_t addr_buf[4];
	//size_t addr_len = mp_spi_set_addr_buff(addr_buf, addr);
	uint8_t cmd_buf[4] = {
		cmd,
		(addr >> 16) & 0xFF,
		(addr >>  8) & 0xFF,
		(addr >>  0) & 0xFF,
	};

	eoss3_spi_transfer(self, cmd_buf, sizeof(cmd_buf), dest, len);
}

static const mp_qspi_proto_t __attribute__((used)) eoss3_qspi_protocol = {
	.ioctl			= eoss3_spi_ioctl,
	.write_cmd_data		= eoss3_write_cmd_data,
	.write_cmd_addr_data	= eoss3_write_cmd_addr_data,
	.read_cmd		= eoss3_read_cmd,
	.read_cmd_qaddr_qdata	= eoss3_read_cmd_qaddr_qdata,
};


static const mp_spiflash_config_t eoss3_spiflash_config = {
	.bus_kind	= MP_SPIFLASH_BUS_QSPI,
	.bus.u_qspi	= {
		.data		= NULL, // unused
		.proto		= &eoss3_qspi_protocol,
	},
	.cache		= NULL,
};


STATIC mp_obj_t
mp_machine_spiflash_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *all_args
)
{
	// create new object
	mp_machine_spiflash_obj_t *self = m_new_obj(mp_machine_spiflash_obj_t);
	self->base.type = &mp_machine_spiflash_type;

	// set parameters
	self->spi_flash.config = &eoss3_spiflash_config;
   
	// initialize it
	printf("%s: calling init\n", __func__);
	mp_spiflash_init(&self->spi_flash);

	return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t mp_machine_spiflash_read(mp_obj_t self_obj, mp_obj_t addr_obj, mp_obj_t buf_obj)
{
    mp_machine_spiflash_obj_t * self = MP_OBJ_TO_PTR(self_obj);
    const unsigned addr = mp_obj_get_int(addr_obj);

    mp_buffer_info_t buf;
    mp_get_buffer_raise(buf_obj, &buf, MP_BUFFER_READ);
    const unsigned len = buf.len;

    mp_spiflash_read(&self->spi_flash, addr, len, buf.buf);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_machine_spiflash_read_obj, mp_machine_spiflash_read);

STATIC mp_obj_t mp_machine_spiflash_write(mp_obj_t self_obj, mp_obj_t addr_obj, mp_obj_t buf_obj)
{
    mp_machine_spiflash_obj_t * self = MP_OBJ_TO_PTR(self_obj);
    const unsigned addr = mp_obj_get_int(addr_obj);

    mp_buffer_info_t buf;
    mp_get_buffer_raise(buf_obj, &buf, MP_BUFFER_READ);
    const unsigned len = buf.len;

    mp_spiflash_write(&self->spi_flash, addr, len, buf.buf);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_machine_spiflash_write_obj, mp_machine_spiflash_write);

STATIC mp_obj_t mp_machine_spiflash_erase(mp_obj_t self_obj, mp_obj_t addr_obj)
{
    mp_machine_spiflash_obj_t * self = MP_OBJ_TO_PTR(self_obj);
    const unsigned addr = mp_obj_get_int(addr_obj);

    int ret = mp_spiflash_erase_block(&self->spi_flash, addr);
    if (ret != 0)
	mp_raise_msg(&mp_type_RuntimeError, NULL); //"erase block failed");

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_spiflash_erase_obj, mp_machine_spiflash_erase);


STATIC const mp_rom_map_elem_t machine_spiflash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spiflash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spiflash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_erase), MP_ROM_PTR(&mp_machine_spiflash_erase_obj) },
};

MP_DEFINE_CONST_DICT(mp_machine_spiflash_locals_dict, machine_spiflash_locals_dict_table);

const mp_obj_type_t mp_machine_spiflash_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPIFlash,
    //.print = mp_machine_spiflash_print,
    .make_new = mp_machine_spiflash_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_machine_spiflash_locals_dict,
};

#endif // MICROPY_PY_MACHINE_SPI
