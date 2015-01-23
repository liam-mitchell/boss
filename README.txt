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
choice.

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
the screen and loop forever (since no exit() system call is implemented yet).

Also due to no exit() system call, there is no ability to wait for a child
to finish executing - so trash always prints out its prompt immediately after
exec() instead of executing the child then reprinting its prompt.

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

These tools will need to be in your PATH.

Once the proper environment is set up, it should be as simple as running 'make'
from the top level of the kernel source tree (ie. the git repository root,
where the Makefile resides).

** 4. KERNEL FEATURES WALKTHROUGH **

