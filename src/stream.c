#include "stream.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void donk_read_bytes(donk_stream_input_t* stream, int count, void* bytes) {
	stream->read(stream, count, bytes);
}

void donk_write_bytes(donk_stream_output_t* stream, int count, void* bytes) {
	stream->write(stream, count, bytes);
}

static void input_file_read(donk_stream_input_t* stream, int count, void* bytes) {
	int read = fread(bytes, count, 1, stream->data);
	if (read != 1) {
		if (feof(stream->data)) {
			stream->state = DONK_STREAM_END;
		} else {
			stream->state = DONK_STREAM_READ_FAILED;
		}
	}
}

static void input_file_close(donk_stream_input_t* stream) {
	fclose(stream->data);
	memset(stream, 0, sizeof(donk_stream_input_t));
}

void donk_open_input_stream_from_file(donk_stream_input_t* stream, const char* filename) {
	memset(stream, 0, sizeof(donk_stream_input_t));
	
	stream->read = input_file_read;
	stream->close = input_file_close;

	stream->data = fopen(filename, "rb");
	if (!stream->data) {
		stream->state = DONK_STREAM_OPEN_FAILED;
		return;
	}
	
	stream->state = DONK_STREAM_READY;
}

static void output_file_write(struct donk_stream_output* stream, int count, void* bytes) {
	int written = fwrite(bytes, count, 1, stream->data);
	if (written != 1) {
		stream->state = DONK_STREAM_WRITE_FAILED;
	}
}

static void output_file_close(donk_stream_output_t* stream) {
	fclose(stream->data);
	memset(stream, 0, sizeof(donk_stream_output_t));
}

void donk_open_output_stream_from_file(donk_stream_output_t* stream, const char* filename) {
	memset(stream, 0, sizeof(donk_stream_output_t));
	
	stream->write = output_file_write;
	stream->close = output_file_close;

	stream->data = fopen(filename, "wb");
	if (!stream->data) {
		stream->state = DONK_STREAM_OPEN_FAILED;
		return;
	}
	
	stream->state = DONK_STREAM_READY;
}

struct memory_data {
	char* data;
	int capacity;
};

static void input_memory_read(donk_stream_input_t* stream, int count, void* bytes) {
	struct memory_data* data = (struct memory_data*)stream->data;
	
	int read_bytes = data->capacity - stream->cursor;
	if (read_bytes == count) {
		stream->state = DONK_STREAM_END;
	} else if (read_bytes > count) {
		read_bytes = count;
	} else {
		stream->state = DONK_STREAM_READ_FAILED;
	}
	
	memcpy(bytes, data->data + stream->cursor, read_bytes);
	stream->cursor += read_bytes;
}

static void input_memory_close(donk_stream_input_t* stream) {
	free(stream->data);
	memset(stream, 0, sizeof(donk_stream_input_t));
}

void donk_open_input_stream_in_memory(donk_stream_input_t* stream, void* data, int size) {
	memset(stream, 0, sizeof(donk_stream_input_t));
	
	stream->read = input_memory_read;
	stream->close = input_memory_close;

	struct memory_data* mem_data = malloc(sizeof(struct memory_data));
	memset(mem_data, 0, sizeof(struct memory_data));
	mem_data->data = data;
	mem_data->capacity = size;
	
	stream->data = mem_data;
	stream->state = DONK_STREAM_READY;
}

static void output_memory_write(struct donk_stream_output* stream, int count, void* bytes) {
	struct memory_data* data = (struct memory_data*)stream->data;
	
	int write_bytes = data->capacity - stream->cursor;
	if (write_bytes >= count) {
		write_bytes = count;
	} else {
		stream->state = DONK_STREAM_WRITE_FAILED;
	}
	
	memcpy(data->data + stream->cursor, bytes, write_bytes);
	stream->cursor += write_bytes;
}

static void output_memory_close(donk_stream_output_t* stream) {
	free(stream->data);
	memset(stream, 0, sizeof(donk_stream_output_t));
}

void donk_open_output_stream_in_memory(donk_stream_output_t* stream, void* data, int size) {
	memset(stream, 0, sizeof(donk_stream_output_t));
	
	stream->write = output_memory_write;
	stream->close = output_memory_close;

	struct memory_data* mem_data = malloc(sizeof(struct memory_data));
	memset(mem_data, 0, sizeof(struct memory_data));
	mem_data->data = data;
	mem_data->capacity = size;
	
	stream->data = mem_data;
	stream->state = DONK_STREAM_READY;
}

void donk_close_input_stream(donk_stream_input_t* stream) {
	stream->close(stream);
}

void donk_close_output_stream(donk_stream_output_t* stream) {
	stream->close(stream);
}