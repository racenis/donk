#ifndef DONK_STREAM_H
#define DONK_STREAM_H

enum {
	DONK_STREAM_INVALID,
	DONK_STREAM_READY,
	DONK_STREAM_OPEN_FAILED,
	DONK_STREAM_READ_FAILED,
	DONK_STREAM_WRITE_FAILED,
	DONK_STREAM_END,
};

typedef struct donk_stream_input {
	void (*read)(struct donk_stream_input* stream, int count, void* bytes);
	void (*close)(struct donk_stream_input* stream);
	
	int state;
	int cursor;
	
	void* data;
} donk_stream_input_t;

typedef struct donk_stream_output {
	void (*write)(struct donk_stream_output* stream, int count, void* bytes);
	void (*close)(struct donk_stream_output* stream);
	
	int state;
	int cursor;
	
	void* data;
} donk_stream_output_t;

void donk_read_bytes(donk_stream_input_t* stream, int count, void* bytes);
void donk_write_bytes(donk_stream_output_t* stream, int count, void* bytes);

void donk_open_input_stream_from_file(donk_stream_input_t* stream, const char* filename);
void donk_open_output_stream_from_file(donk_stream_output_t* stream, const char* filename);

void donk_open_input_stream_in_memory(donk_stream_input_t* stream, void* data, int size);
void donk_open_output_stream_in_memory(donk_stream_output_t* stream, void* data, int size);

void donk_close_input_stream(donk_stream_input_t* stream);
void donk_close_output_stream(donk_stream_output_t* stream);

// TODO: add memory stream

#endif // DONK_STREAM_H