#pragma once

/**
 * Atomic operations.
 */

// Load and increment. Increase v, return old value.
nat atomicInc(volatile nat &v);
