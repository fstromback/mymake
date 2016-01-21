#pragma once

/**
 * Platform agnostic pipe handling.
 */

#ifdef WINDOWS
typedef HANDLE Pipe;
#else
typedef int Pipe;
#endif

// Invalid pipe value.
extern const Pipe noPipe;


// Create a pipe. if 'forChild', the writing end of the pipe is inherited by child processes on Windows.
void createPipe(Pipe &read, Pipe &write, bool forChild);

// Close one end of a pipe.
void closePipe(Pipe p);

// Write to a pipe.
bool writePipe(Pipe to, void *data, nat size);


/**
 * Set of pipes that allows waiting until data on one of the pipes.
 * The PipeSet is quite different between platforms, so the declaration is in the .cpp file.
 */
class PipeSet;

// Create a pipe set. 'bufferSize' is the minimum buffer size that will be passed to 'readPipeSet'.
PipeSet *createPipeSet(nat bufferSize);

// Destroy a pipe set.
void destroyPipeSet(PipeSet *pipes);

// Add a pipe to a pipe set.
void addPipeSet(PipeSet *pipes, Pipe pipe);

// Remove a pipe from a pipe set.
void removePipeSet(PipeSet *pipes, Pipe pipe);

// Read data from one of the pipes.
void readPipeSet(PipeSet *pipes, void *to, nat &written, Pipe &from);
