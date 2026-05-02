#include "container.h"

/*
	TODO: IMPLEMENT THE CONTAINERS
	
	A container will simply be a sequence of frames. Initially we'll have two
	containers -- a file container and a stream container.
	
	The file container will consist of a header, a sequence of frames and a
	checksum at the end. The first frames will consist of palettes and coding
	tables. They won't be repeated. The decoder will assume that the file is not
	corrupted.
	
	The stream container will not contain any predefined frame sequence and
	instead will just decode frames as they come in.
*/