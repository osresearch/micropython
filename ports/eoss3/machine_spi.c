#include <stdio.h>
#include "py/mphal.h"
#include "mpconfigport.h"
#include "extmod/machine_spi.h"
#include "Fw_global_config.h"
#include "eoss3_dev.h"
//#include "eoss3_hal_spi.h" // this does not actually work

static void
eoss3_spi_init(
	mp_obj_base_t * obj,
	size_t n_args,
	const mp_obj_t *pos_args,
	mp_map_t *kw_args
)
{
	printf("%s: BAUDR=%02x\n", __func__, (int)SPI_MS->BAUDR);
	// enable the SPI hardware, leaving everything else
	// to the default settings.
	//SPI_MS->BAUDR = 100; // default? who knows.
		SPI_MS->SSIENR = 0;
		SPI_MS->BAUDR = 8; // default? who knows.
		SPI_MS->CTRLR0 = CTRLR0_TMOD_EEPROM | CTRLR0_DFS_8_BIT;
		SPI_MS->CTRLR1 = 0; // byte at a time
		SPI_MS->IMR = 0;

		// turn off the SPI DMA controller since we're busy waiting
		DMA_SPI_MS->DMA_INTR_MASK = 0;
}

static void
eoss3_spi_deinit(
	mp_obj_base_t * obj
)
{
	printf("%s\n", __func__);
	SPI_MS->SSIENR = 0;
}


static void eoss3_spi_transfer(
	mp_obj_base_t * obj,
	size_t len,
	const uint8_t * src,
	uint8_t * dest
)
{
	// select device and configure dma for the length of data
	SPI_MS->SSIENR = SSIENR_SSI_DISABLE;
	SPI_MS->SER = 0;
	SPI_MS->IMR = 0;
	SPI_MS->CTRLR1 = len - 1;
	SPI_MS->SSIENR = SSIENR_SSI_EN;

#if 1
	// wait for TxFifo empty and not busy
	while ((SPI_MS->SR & (SR_TFE | SR_BUSY)) != SR_TFE)
		;

	// send all of the data
	for(size_t i = 0 ; i < len ; i++)
	{
		const uint8_t out = src ? *src++ : 0xFF;
		SPI_MS->DR0 = out;
	}

	// now select the chip? wtf quicklogic
	SPI_MS->SER = SER_SS_0_N_SELECTED;

	// receive all of the data
	for(size_t i = 0 ; i < len ; i++)
	{
		unsigned spin_count = 0;
		while (!(SPI_MS->SR & SR_RFNE))
		{
			if (spin_count++ > 100)
			{
				printf("%s: RX FIFO failed\n", __func__);
				SPI_MS->SSIENR = SSIENR_SSI_DISABLE;
				return;
			}
		}

		const uint8_t in = SPI_MS->DR0;
		if (dest)
			*dest++ = in;
	}
#else
	while(len)
	{
		// fill up the fifo or send all of the data
		// Where is the 32 defined?
		size_t fifo_len = 1 - SPI_MS->TXFLR;
		if (fifo_len > len)
			fifo_len = len;

		printf("%s: len=%d\n", __func__, fifo_len);

		for(size_t i = 0 ; i < fifo_len ; i++)
		{
			const uint8_t out = src ? *src++ : 0xFF;
			SPI_MS->DR0 = out;
		}

		// wait for the fifo to drain
		printf("%s: wait\n", __func__);
		while((SPI_MS->SR & (SR_RFNE|SR_BUSY)) == SR_RFNE)
			;

		// copy the rx data out of the fifo
		for(size_t i = 0 ; i < fifo_len ; i++)
		{
			const uint8_t in = SPI_MS->DR0;
			if (dest)
				*dest++ = in;
		}

		// move to the next chunk
		len -= fifo_len;
		printf("%s: loop\n", __func__);
	}
#endif

	// unselect any devices
	SPI_MS->SSIENR = 0;
	SPI_MS->SER = 0;
	printf("%s: done\n", __func__);
}

extern const mp_obj_type_t mp_machine_eoss3_spi_type;

STATIC mp_obj_t
mp_machine_eoss3_spi_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *all_args
)
{
    // create new object
    mp_obj_base_t *self = m_new_obj(mp_obj_base_t);
    self->type = &mp_machine_eoss3_spi_type;

    return MP_OBJ_FROM_PTR(self);
}

static mp_machine_spi_p_t mp_machine_eoss3_spi_p = {
	.init		= eoss3_spi_init,
	.deinit		= eoss3_spi_deinit,
	.transfer	= eoss3_spi_transfer,
};

const mp_obj_type_t mp_machine_eoss3_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    //.print = mp_machine_soft_spi_print,
    .make_new = mp_machine_eoss3_spi_make_new,
    .protocol = &mp_machine_eoss3_spi_p,
    .locals_dict = (mp_obj_dict_t *)&mp_machine_spi_locals_dict,
};
