# include "core/ring-buffer.h"

// ring_buffer_setup assumes size is a power of 2.
// eg. size=8 will yield a mask of 0b111 .
void ring_buffer_setup(ring_buffer_t* rb, uint8_t* buffer, uint32_t size) {
	rb->buffer = buffer;
	rb->read_index = 0;
	rb->write_index = 0;
	rb->mask = size - 1;
}

bool ring_buffer_empty(ring_buffer_t* rb) {
	return rb->read_index == rb->write_index;
}

bool ring_buffer_read(ring_buffer_t* rb, uint8_t* byte) {
	uint32_t rp = rb->read_index;
	uint32_t wp = rb->write_index;
	if (rp == wp) return false;

	byte = (rb->buffer + rp);
	(void)byte;
	rp = (rp+1) & rb->mask;
	rb->read_index = rp; // atomically update read_index
	return true;
}

bool ring_buffer_write(ring_buffer_t* rb, uint8_t byte) {
	uint32_t rp = rb->read_index;
	uint32_t wp = rb->write_index;
	uint32_t next_wp = (wp+1) & rb->mask;
	if (next_wp == rp) return false;  // buffer full

	rb->buffer[wp] = byte;
	rb->write_index = next_wp;
	return true;

}
