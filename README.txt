================================================================================
The BOSS (Basic Operating SyStem)
A simple multitasking text-based operating system for x86

Author: Liam Mitchell
================================================================================

Welcome to the source repository for the BOSS!

Contents:
  1. Repository information
  2. Installing the operating system on a virtual machine
  3. Building the kernel
  4. Kernel features walkthrough

** 1. REPOSITORY INFORMATION **

At the root of the repository are several core files:

  - Makefile: the kernel makefile
  - include.mk: variable definitions for make
  - link.ld: the kernel linker script

The root directory also contains several subdirectories. Most of those are
detailed below, with the exception of the src/ subdirectory (which is covered
in-depth in section 4).

  - bin/
      Pre-compiled kernel binary and ISO image.

  - iso/
      Supporting files necessary for generation of ISO image by genisoimage.
      The generated ISO filesystem contains necessary information for booting
      via GRUB, as well as the generated initrd image (loaded as a module by
      GRUB).

  - scripts/
      Scripts and files to support the build process. Includes the init/
      directory, which is used by gen-initrd.sh to build the initrd image
      for inclusion in the iso.

  - src/
      Kernel source directory.


** 2. INSTALLING THE OPERATING SYSTEM ON A VIRTUAL MACHINE **

WARNING: This has only been tested on VirtualBox!

To load the operating system on a virtual machine of your choice, simply load
it into the disk drive of the VM and select the ISO boot method as your primary
choice. There is a bug within the paging setup code that assumes 4GB of memory,
so ensure you set your VM's system memory to 4GB.

* 2.1 TRASH * Once the operating system boots and prints a copious amount of
debug information to the screen, it will start trash, the BOSS's default shell.
trash is an extremely simple shell written in x86 assembler, which displays
the BOSS's standard input/output facilities, its process creation and
scheduling abilities, and its system call interface.

trash accepts only a single argument on the command line: an absolute path
to a flat binary file to load. Currently, since the only filesystems supported
are the dev/ and init/ filesystems, this means the only programs available
to be loaded are trash itself (/init/bin/trash), hello (/init/bin/hello) and
goodbye (/init/bin/goodbye). hello and goodbye will simply print a message to
the screen and exit - hello will exit successfully with return code 0, while
goodbye will exit with code 1. trash will wait for its child to exit, print
the return value and prompt for another program.

trash is also unable to backspace (a crippling flaw - but nobody said it was
a *good* shell ;) so you better get it right the first time!

** 3. BUILDING THE KERNEL **

Should you wish to undertake the arduous task of building the kernel yourself,
you will require several tools:

  - a GCC cross toolchain for the i686-elf-gcc target (gcc, as, ld)
  - nasm
  - make
  - bash
  - python
  - genisoimage
  - coreutils (cp, mv, echo, etc)

These tools will need to be in your PATH.

Once the proper environment is set up, it should be as simple as running 'make'
from the top level of the kernel source tree (ie. the git repository root,
where the Makefile resides).

** 4. KERNEL FEATURES WALKTHROUGH **

Major features of the kernel are as follows:

  - Fully preemptive round-robin scheduler, which allows fancy things like
      threads sleeping in the kernel and yielding before preemption ;)

      Core scheduler code can be found in src/tasks/task.c. Context saving code
      is found unfortunately split between switch_context() in src/tasks/task.h
      and the IRQ/ISR handlers in src/interrupts/interrupts.s.

      The scheduler uses one kernel stack per thread, allocating a single page
      for each thread's kernel stack.

      Process management facilities can be found in src/tasks/, in fork.c,
      exec.c, wait.c and exit.c. These files map pretty much 1-1 to the
      Linux system calls of the same name.

- Virtual UNIX-like filesystem, allowing classic mountpoints for different
      types of filesystem within the same directory structure.

      Core VFS code can be found in src/filesystem/vfs.c. Specific filesystem
      implementations are contained in subfolders within the filesystem/
      directory (ie. initrd/, dev/). src/filesystem/fs.h contains core
      filesystem data structures and definitions, while src/filesystem/fs.c
      implements several of these core functions.

  - Loads itself into the higher half, to allow program linking to take place
      from the bottom of memory without disruption.

      src/boot/boot.s does most of the hard work of implementing the higher
      half kernel, setting up the paging structures for the rest of the
      kernel and mapping the kernel into the upper 1GB. The virtual load
      address for the kernel is at 0xC0000000 (as per the usual... :P)

  - Uses a simple implementation of malloc which allows for allocation of
      contiguous sections of DMA memory (for devices drivers which require
      physically contiguous RAM).

      Core kernel memory allocation is implemented in src/memory/kmalloc.c,
      and DMA implementations can be found in src/memory/kmalloc-dma.c. The
      supporting physical page allocation functions can be found in
      src/memory/vmm.c, and the functions used to allocate physical frames
      for those pages are in src/memory/pmm.c.

  - Extensible system call interface (currently being extended :D )

      Similar to Linux, uses registers to pass system call parameters (in the
      same order, too). Enables simple system call extension by just adding
      functions to the array void *syscalls[] in src/interrupts/syscalls.c.

Thanks for your interest in the BOSS! Feel free to email any concerns, requests,
reports, stories or really whatever else to me (Liam Mitchell) at
liamdmitchell [at] gmail [dot] com, or feel free to add me on Discord
@commandercoriandersalamander.
