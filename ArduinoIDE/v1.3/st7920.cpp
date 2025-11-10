/*
 * ST7920 Driver for Arduino IDE (Standard 128x64 ST7920 Displays Only!)
 * Version 1.3
 *
 * Author: Rafael Sabe
 * Email: rafaelmsabe@gmail.com
 */

#include "st7920.hpp"
#include <string.h>

ST7920::ST7920(uint8_t db0, uint8_t db1, uint8_t db2, uint8_t db3, uint8_t db4, uint8_t db5, uint8_t db6, uint8_t db7, uint8_t rs, uint8_t e)
{
	this->resetPinout(db0, db1, db2, db3, db4, db5, db6, db7, rs, e);
}

ST7920::~ST7920(void)
{
}

bool ST7920::begin(void)
{
	if(this->_status > 0) return true;

	this->_status = this->STATUS_UNINITIALIZED;

	if(!this->_validate_pins())
	{
		this->_status = this->STATUS_ERROR;
		return false;
	}

	pinMode(this->_pins.e, OUTPUT);
	digitalWrite(this->_pins.e, 0);

	pinMode(this->_pins.rs, OUTPUT);

	this->_set_dataline_mode(true);

	/*Default Initialization*/
	this->_set_instruction_mode(false);
	this->_send_byte(false, 0x01, this->_CMD_SHORT_DELAY_US);
	this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);
	this->_send_byte(false, 0x0c, this->_CMD_SHORT_DELAY_US);

	memset(this->_page_buffer, 0x0, this->_BUFFER_SIZE_BYTES);

	this->_status = this->STATUS_INITIALIZED;
	return true;
}

void ST7920::resetPinout(uint8_t db0, uint8_t db1, uint8_t db2, uint8_t db3, uint8_t db4, uint8_t db5, uint8_t db6, uint8_t db7, uint8_t rs, uint8_t e)
{
	memset(&(this->_pins), 0xff, sizeof(struct _st7920_pinout));

	this->_status = this->STATUS_UNINITIALIZED;

	this->_pins.db0 = db0;
	this->_pins.db1 = db1;
	this->_pins.db2 = db2;
	this->_pins.db3 = db3;
	this->_pins.db4 = db4;
	this->_pins.db5 = db5;
	this->_pins.db6 = db6;
	this->_pins.db7 = db7;
	this->_pins.rs = rs;
	this->_pins.e = e;

	return;
}

intptr_t ST7920::getStatus(void)
{
	return this->_status;
}

bool ST7920::enableGraphicDisplay(bool enable)
{
	if(this->_status < 1) return false;

	this->_set_instruction_mode(true);
	this->_graphic_display_enabled = enable;
	this->_set_instruction_mode(true);

	return true;
}

intptr_t ST7920::graphicDisplayIsEnabled(void)
{
	if(this->_status < 1) return -1;

	if(this->_graphic_display_enabled) return 1;

	return 0;
}

bool ST7920::bufferSetPixel(uintptr_t cx, uintptr_t cy, bool lit)
{
	uintptr_t buffer_index = 0u;
	uintptr_t pixel_offset = 0u;

	if(this->_status < 1) return false;

	if(!this->_phys_cx_cy_to_virt_bufindex_pageindex_cy_offset(cx, cy, &buffer_index, NULL, NULL, &pixel_offset)) return false;

	if(lit) this->_page_buffer[buffer_index] |= (1 << pixel_offset);
	else this->_page_buffer[buffer_index] &= ~(1 << pixel_offset);

	return true;
}

intptr_t ST7920::bufferGetPixel(uintptr_t cx, uintptr_t cy)
{
	uintptr_t buffer_index = 0u;
	uintptr_t pixel_offset = 0u;

	if(this->_status < 1) return -1;

	if(!this->_phys_cx_cy_to_virt_bufindex_pageindex_cy_offset(cx, cy, &buffer_index, NULL, NULL, &pixel_offset)) return -1;

	if(this->_page_buffer[buffer_index] & (1 << pixel_offset)) return 1;

	return 0;
}

bool ST7920::bufferTogglePixel(uintptr_t cx, uintptr_t cy)
{
	uintptr_t buffer_index = 0u;
	uintptr_t pixel_offset = 0u;

	if(this->_status < 1) return false;

	if(!this->_phys_cx_cy_to_virt_bufindex_pageindex_cy_offset(cx, cy, &buffer_index, NULL, NULL, &pixel_offset)) return false;

	this->_page_buffer[buffer_index] ^= (1 << pixel_offset);

	return true;
}

bool ST7920::bufferSetPage(uintptr_t page_index, uintptr_t cy, uint16_t page_value)
{
	uintptr_t buffer_index = 0u;

	if(this->_status < 1) return false;

	if(!this->_phys_pageindex_cy_to_virt_bufindex_pageindex_cy(page_index, cy, &buffer_index, NULL, NULL)) return false;

	this->_page_buffer[buffer_index] = page_value;
	return true;
}

int32_t ST7920::bufferGetPage(uintptr_t page_index, uintptr_t cy)
{
	uintptr_t buffer_index = 0u;

	if(this->_status < 1) return -1;

	if(!this->_phys_pageindex_cy_to_virt_bufindex_pageindex_cy(page_index, cy, &buffer_index, NULL, NULL)) return -1;

	return (int32_t) this->_page_buffer[buffer_index];
}

bool ST7920::bufferTogglePage(uintptr_t page_index, uintptr_t cy, uint16_t toggle_value)
{
	uintptr_t buffer_index = 0u;

	if(this->_status < 1) return false;

	if(!this->_phys_pageindex_cy_to_virt_bufindex_pageindex_cy(page_index, cy, &buffer_index, NULL, NULL)) return false;

	if(!toggle_value) return true;

	this->_page_buffer[buffer_index] ^= toggle_value;
	return true;
}

bool ST7920::bufferSetAll(bool lit)
{
	if(this->_status < 1) return false;

	if(lit) memset(this->_page_buffer, 0xff, this->_BUFFER_SIZE_BYTES);
	else memset(this->_page_buffer, 0x00, this->_BUFFER_SIZE_BYTES);

	return true;
}

bool ST7920::bufferToggleAll(void)
{
	uintptr_t buffer_index = 0u;

	if(this->_status < 1) return false;

	for(buffer_index = 0u; buffer_index < this->_BUFFER_SIZE_PAGES; buffer_index++) this->_page_buffer[buffer_index] = ~(this->_page_buffer[buffer_index]);

	return true;
}

bool ST7920::bufferPaintPixel(uintptr_t cx, uintptr_t cy)
{
	uintptr_t page_index = 0u;

	page_index = cx/this->_PAGE_SIZE_PIXELS;

	return this->bufferPaintPage(page_index, cy);
}

bool ST7920::bufferPaintPage(uintptr_t page_index, uintptr_t cy)
{
	uintptr_t buffer_index = 0u;
	uintptr_t v_pageindex = 0u;
	uintptr_t v_cy = 0u;
	uint16_t page_value = 0u;

	if(this->_status < 1) return false;

	if(!this->_phys_pageindex_cy_to_virt_bufindex_pageindex_cy(page_index, cy, &buffer_index, &v_pageindex, &v_cy)) return false;

	v_pageindex &= 0xff;
	v_cy &= 0xff;

	page_value = this->_page_buffer[buffer_index];

	this->_set_instruction_mode(true);

	this->_send_byte(false, (uint8_t) (0x80 | v_cy), this->_CMD_SHORT_DELAY_US);
	this->_send_byte(false, (uint8_t) (0x80 | v_pageindex), this->_CMD_SHORT_DELAY_US);

	this->_send_byte(true, (uint8_t) (page_value >> 8), this->_CMD_SHORT_DELAY_US);
	this->_send_byte(true, (uint8_t) (page_value & 0xff), this->_CMD_SHORT_DELAY_US);

	return true;
}

bool ST7920::bufferPaintAll(void)
{
	uintptr_t buffer_index = 0u;
	uint16_t page_value = 0u;
	uint8_t v_cy = 0u;
	uint8_t v_pageindex = 0u;

	if(this->_status < 1) return false;

	this->_set_instruction_mode(true);

	for(v_cy = 0u; v_cy < ((uint8_t) this->_HEIGHT_PIXELS); v_cy++)
	{
		this->_send_byte(false, (0x80 | v_cy), this->_CMD_SHORT_DELAY_US);
		this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);

		for(v_pageindex = 0u; v_pageindex < ((uint8_t) this->_WIDTH_PAGES); v_pageindex++)
		{
			if(buffer_index >= this->_BUFFER_SIZE_PAGES) return false; /*Not really necessary, but just to be safe...*/

			page_value = this->_page_buffer[buffer_index];

			this->_send_byte(true, (uint8_t) (page_value >> 8), this->_CMD_SHORT_DELAY_US);
			this->_send_byte(true, (uint8_t) (page_value & 0xff), this->_CMD_SHORT_DELAY_US);

			buffer_index++;
		}
	}

	return true;
}

bool ST7920::clearGraphics(void)
{
	if(this->_status < 1) return false;

	this->bufferSetAll(false);
	this->bufferPaintAll();
	return true;
}

bool ST7920::setDisplayMode(intptr_t display_mode)
{
	if(this->_status < 1) return false;

	this->_set_instruction_mode(false);

	switch(display_mode)
	{
		case this->DISPLAYMODE_DISPLAY_OFF:
			this->_send_byte(false, 0x08, this->_CMD_SHORT_DELAY_US);
			return true;

		case this->DISPLAYMODE_DISPLAY_ON_CURSOR_OFF:
			this->_send_byte(false, 0x0c, this->_CMD_SHORT_DELAY_US);
			return true;

		case this->DISPLAYMODE_DISPLAY_ON_CURSOR_ON:
			this->_send_byte(false, 0x0e, this->_CMD_SHORT_DELAY_US);
			return true;

		case this->DISPLAYMODE_DISPLAY_ON_CURSOR_BLINK:
			this->_send_byte(false, 0x0f, this->_CMD_SHORT_DELAY_US);
			return true;
	}

	return false;
}

bool ST7920::clearText(void)
{
	if(this->_status < 1) return false;

	this->fillScreenChar(' ');

	this->_set_instruction_mode(false);
	this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);
	return true;
}

bool ST7920::cursorHome(void)
{
	if(this->_status < 1) return false;

	this->_set_instruction_mode(false);
	this->_send_byte(false, 0x02, this->_CMD_SHORT_DELAY_US);

	return true;
}

bool ST7920::setTextCursorPosition(uintptr_t cx, uintptr_t cy)
{
	uintptr_t numptr = 0u;
	bool add_space = false;

	if(this->_status < 1) return false;

	if(!this->_phys_text_cx_cy_to_virt_wtext_cx_cy_addspace(cx, cy, &cx, &cy, &add_space)) return false;

	this->_set_instruction_mode(false);

	if(cy) this->_send_byte(false, 0x90, this->_CMD_SHORT_DELAY_US);
	else this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);

	numptr = 0u;
	while(numptr < cx)
	{
		this->_send_byte(false, 0x14, this->_CMD_SHORT_DELAY_US);
		numptr++;
	}

	if(add_space) this->_send_byte(true, ' ', this->_CMD_SHORT_DELAY_US);

	return true;
}

bool ST7920::setWTextCursorPosition(uintptr_t cx, uintptr_t cy)
{
	uintptr_t numptr = 0u;

	if(this->_status < 1) return false;

	if(!this->_phys_wtext_cx_cy_to_virt_wtext_cx_cy(cx, cy, &cx, &cy)) return false;

	this->_set_instruction_mode(false);

	if(cy) this->_send_byte(false, 0x90, this->_CMD_SHORT_DELAY_US);
	else this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);

	numptr = 0u;
	while(numptr < cx)
	{
		this->_send_byte(false, 0x14, this->_CMD_SHORT_DELAY_US);
		numptr++;
	}

	return true;
}

bool ST7920::printChar(char c)
{
	if(this->_status < 1) return false;

	this->_set_instruction_mode(false);
	this->_send_byte(true, (uint8_t) c, this->_CMD_SHORT_DELAY_US);

	return true;
}

bool ST7920::printText(const char *text)
{
	uintptr_t n_len = 0u;

	if(this->_status < 1) return false;
	if(text == NULL) return false;

	while(text[n_len] != '\0') n_len++;

	return this->printText(text, n_len);
}

bool ST7920::printText(const char *text, uintptr_t length)
{
	uintptr_t n_char = 0u;

	if(this->_status < 1) return false;
	if(text == NULL) return false;

	this->_set_instruction_mode(false);

	n_char = 0u;
	while(n_char < length)
	{
		this->_send_byte(true, (uint8_t) text[n_char], this->_CMD_SHORT_DELAY_US);
		n_char++;
	}

	return true;
}

bool ST7920::printWChar(uint16_t wc)
{
	if(this->_status < 1) return false;

	this->_set_instruction_mode(false);

	this->_send_byte(true, (uint8_t) (wc >> 8), this->_CMD_SHORT_DELAY_US);
	this->_send_byte(true, (uint8_t) (wc & 0xff), this->_CMD_SHORT_DELAY_US);

	return true;
}

bool ST7920::printWText(const uint16_t *wtext)
{
	uintptr_t n_len = 0u;

	if(this->_status < 1) return false;
	if(wtext == NULL) return false;

	while(wtext[n_len] != '\0') n_len++;

	return this->printWText(wtext, n_len);
}

bool ST7920::printWText(const uint16_t *wtext, uintptr_t length)
{
	uintptr_t n_wchar = 0u;
	uint16_t wchar = 0u;

	if(this->_status < 1) return false;
	if(wtext == NULL) return false;

	this->_set_instruction_mode(false);

	n_wchar = 0u;
	while(n_wchar < length)
	{
		wchar = wtext[n_wchar];

		this->_send_byte(true, (uint8_t) (wchar >> 8), this->_CMD_SHORT_DELAY_US);
		this->_send_byte(true, (uint8_t) (wchar & 0xff), this->_CMD_SHORT_DELAY_US);

		n_wchar++;
	}

	return true;
}

bool ST7920::fillScreenChar(char c)
{
	uintptr_t n_char = 0u;

	if(this->_status < 1) return false;

	this->_set_instruction_mode(false);

	this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);
	for(n_char = 0u; n_char < this->_N_CHARS; n_char++) this->_send_byte(true, (uint8_t) c, this->_CMD_SHORT_DELAY_US);

	this->_send_byte(false, 0x90, this->_CMD_SHORT_DELAY_US);
	for(n_char = 0u; n_char < this->_N_CHARS; n_char++) this->_send_byte(true, (uint8_t) c, this->_CMD_SHORT_DELAY_US);

	return true;
}

bool ST7920::fillScreenWChar(uint16_t wc)
{
	uintptr_t n_wchar = 0u;

	if(this->_status < 1) return false;

	this->_set_instruction_mode(false);

	this->_send_byte(false, 0x80, this->_CMD_SHORT_DELAY_US);
	for(n_wchar = 0u; n_wchar < this->_N_WCHARS; n_wchar++)
	{
		this->_send_byte(true, (uint8_t) (wc >> 8), this->_CMD_SHORT_DELAY_US);
		this->_send_byte(true, (uint8_t) (wc & 0xff), this->_CMD_SHORT_DELAY_US);
	}

	this->_send_byte(false, 0x90, this->_CMD_SHORT_DELAY_US);
	for(n_wchar = 0u; n_wchar < this->_N_WCHARS; n_wchar++)
	{
		this->_send_byte(true, (uint8_t) (wc >> 8), this->_CMD_SHORT_DELAY_US);
		this->_send_byte(true, (uint8_t) (wc & 0xff), this->_CMD_SHORT_DELAY_US);
	}

	return true;
}

bool ST7920::clearDisplay(void)
{
	if(this->_status < 1) return false;

	this->clearGraphics();

	this->_set_instruction_mode(false);
	this->_send_byte(false, 0x01, this->_CMD_SHORT_DELAY_US);

	return true;
}

void ST7920::_set_instruction_mode(bool ext)
{
	uint8_t mode = 0x0;

	if(ext)
	{
		mode = this->_EXT_INSTRUCTION_BYTE;
		if(this->_graphic_display_enabled) mode |= this->_GRAPHIC_DISPLAY_ENABLE_BIT;
	}
	else mode = this->_BASIC_INSTRUCTION_BYTE;

	this->_send_byte(false, mode, this->_CMD_LONG_DELAY_US);
	return;
}

void ST7920::_send_byte(bool reg, uint8_t byte, uintptr_t cmddelay_us)
{
	digitalWrite(this->_pins.e, 0);
	digitalWrite(this->_pins.rs, reg);
	delayMicroseconds(this->_EN_DELAY_US);
	this->_write_byte(byte);
	digitalWrite(this->_pins.e, 1);
	delayMicroseconds(this->_EN_DELAY_US);
	digitalWrite(this->_pins.e, 0);
	delayMicroseconds(cmddelay_us);

	return;
}

void ST7920::_write_byte(uint8_t byte)
{
	digitalWrite(this->_pins.db7, (byte & 0x80));
	digitalWrite(this->_pins.db6, (byte & 0x40));
	digitalWrite(this->_pins.db5, (byte & 0x20));
	digitalWrite(this->_pins.db4, (byte & 0x10));
	digitalWrite(this->_pins.db3, (byte & 0x08));
	digitalWrite(this->_pins.db2, (byte & 0x04));
	digitalWrite(this->_pins.db1, (byte & 0x02));
	digitalWrite(this->_pins.db0, (byte & 0x01));

	return;
}

void ST7920::_set_dataline_mode(bool output)
{
	uint8_t mode = 0u;

	if(output) mode = OUTPUT;
	else mode = INPUT;

	pinMode(this->_pins.db0, mode);
	pinMode(this->_pins.db1, mode);
	pinMode(this->_pins.db2, mode);
	pinMode(this->_pins.db3, mode);
	pinMode(this->_pins.db4, mode);
	pinMode(this->_pins.db5, mode);
	pinMode(this->_pins.db6, mode);
	pinMode(this->_pins.db7, mode);

	return;
}

bool ST7920::_validate_pins(void)
{
	uintptr_t n_pin = 0u;
	uint8_t *p_pins = (uint8_t*) &(this->_pins);

	/*DB0 - DB7*/
	for(n_pin = 0u; n_pin < 8u; n_pin++) if(p_pins[n_pin] == 0xff) return false;

	/*RS*/
	if(p_pins[8] == 0xff) return false;

	/*Ignore RW for now... (p_pins[9])*/

	/*E*/
	if(p_pins[10] == 0xff) return false;

	return true;
}

bool ST7920::_phys_cx_cy_to_virt_bufindex_pageindex_cy_offset(uintptr_t cx, uintptr_t cy, uintptr_t *p_bufferindex, uintptr_t *p_pageindex, uintptr_t *p_cy, uintptr_t *p_offset)
{
	uintptr_t buffer_index = 0u;
	uintptr_t page_index = 0u;

	if((cx >= this->WIDTH) || (cy >= this->HEIGHT)) return false;

	page_index = cx/this->_PAGE_SIZE_PIXELS;
	cx %= this->_PAGE_SIZE_PIXELS;
	cx = this->_PAGE_SIZE_PIXELS - cx - 1u;

	if(cy >= this->_HEIGHT_PIXELS)
	{
		cy -= this->_HEIGHT_PIXELS;
		page_index += this->WIDTH_PAGES;
	}

	buffer_index = this->_WIDTH_PAGES*cy + page_index;

	if(p_bufferindex != NULL) *p_bufferindex = buffer_index;
	if(p_pageindex != NULL) *p_pageindex = page_index;
	if(p_cy != NULL) *p_cy = cy;
	if(p_offset != NULL) *p_offset = cx;

	return true;
}

bool ST7920::_phys_pageindex_cy_to_virt_bufindex_pageindex_cy(uintptr_t page_index, uintptr_t cy, uintptr_t *p_bufferindex, uintptr_t *p_pageindex, uintptr_t *p_cy)
{
	uintptr_t buffer_index = 0u;

	if((page_index >= this->WIDTH_PAGES) || (cy >= this->HEIGHT)) return false;

	if(cy >= this->_HEIGHT_PIXELS)
	{
		cy -= this->_HEIGHT_PIXELS;
		page_index += this->WIDTH_PAGES;
	}

	buffer_index = this->_WIDTH_PAGES*cy + page_index;

	if(p_bufferindex != NULL) *p_bufferindex = buffer_index;
	if(p_pageindex != NULL) *p_pageindex = page_index;
	if(p_cy != NULL) *p_cy = cy;

	return true;
}

bool ST7920::_phys_text_cx_cy_to_virt_wtext_cx_cy_addspace(uintptr_t cx, uintptr_t cy, uintptr_t *p_cx, uintptr_t *p_cy, bool *p_addspace)
{
	bool add_space = false;

	if((cx >= this->N_CHARS) || (cy >= this->N_LINES)) return false;

	if(cx & 0x1) add_space = true;

	cx = (cx >> 1);

	if(cy >= this->_N_LINES)
	{
		cy -= this->_N_LINES;
		cx += this->N_WCHARS;
	}

	if(p_cx != NULL) *p_cx = cx;
	if(p_cy != NULL) *p_cy = cy;
	if(p_addspace != NULL) *p_addspace = add_space;

	return true;
}

bool ST7920::_phys_wtext_cx_cy_to_virt_wtext_cx_cy(uintptr_t cx, uintptr_t cy, uintptr_t *p_cx, uintptr_t *p_cy)
{
	if((cx >= this->N_WCHARS) || (cy >= this->N_LINES)) return false;

	if(cy >= this->_N_LINES)
	{
		cy -= this->_N_LINES;
		cx += this->N_WCHARS;
	}

	if(p_cx != NULL) *p_cx = cx;
	if(p_cy != NULL) *p_cy = cy;

	return true;
}

