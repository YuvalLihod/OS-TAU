# Simulaton of an OS that handles a multi-level (trie-based) page table.
Our simulated OS targets an imaginary 64-bit x86-like CPU.

## Virtual addresses
The virtual address size of our hardware is 64 bits, of which only the lower
57 bits are used for translation. The top 7 bits are guaranteed to be identical to bit 56.

## Physical addresses
The physical address size of our hardware is also 64 bits.
