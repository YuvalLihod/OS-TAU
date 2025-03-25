# Message Slot Kernel Module

Implemention of a kernel module that provides a new IPC mechanism, called a message slot.\n
A message slot is a character device file through which processes communicate.\n
A message slot device has multiple message channels active concurrently, which can be used by multiple processes.
