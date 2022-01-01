#pragma once

/**
 * Atomic operations.
 */

// Load and increment. Increase v, return old value.
nat atomicInc(volatile nat &v);

// Atomic write.
void atomicWrite(volatile nat &v, nat write);

// Atomic read.
nat atomicRead(volatile nat &v);
