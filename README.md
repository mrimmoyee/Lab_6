# Directory Structure
The repository follows the standard mCertiKOS structure, with key directories and their purposes:

| Directory       | Description                                                                 |
|-----------------|-----------------------------------------------------------------------------|
| `kern/`         | Core kernel code, including process management, trap handling, and syscalls. |
| `vmm/`          | Virtual memory management modules.                                          |
| `vmm/MPTComm/`  | Common utilities for virtual memory management (e.g., page table functions). |
| `vmm/MPTInit/`  | Initialization code for virtual memory, including paging setup.              |
| `vmm/MPTIntro/` | Entry points for VMM operations, such as page fault and trap handling.       |
| `vmm/MPTKern/`  | Kernel-specific memory mappings.                                            |
| `vmm/MPTNew/`   | Page allocation and mapping functions.                                      |
| `user/`         | User-space programs and test cases (e.g., `ping`, `pong`, shell).           |
| `lib/`          | Shared libraries and utilities for kernel and user-space code.              |


- **Operating System**: Ubuntu (recommended, e.g., 22.04 LTS) running in VirtualBox or a similar environment.
- **Tools**:
  - GCC (with multilib support for 32-bit compilation).
  - Make.
  - QEMU for emulation.
  - Git for version control.
- Install dependencies on Ubuntu:
  ```bash
  sudo apt update
  sudo apt install build-essential git qemu-system gcc-multilib



  **Setup Instructions**
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/mrimmoyee/Lab_6.git
   cd Lab_6
   ```

2. **Build the Kernel**:
   - Run the build command to compile the kernel and user programs:
     ```bash
     make
     ```
   - Ensure all dependencies are resolved (check `Makefile` for specific requirements).

3. **Run the Kernel**:
   - Emulate the kernel using QEMU:
     ```bash
     make qemu
     ```
   - For debugging with GDB:
     ```bash
     make qemu-gdb
     ```
     Then, in another terminal:
     ```bash
     go to the mcertikOS directory
     sudo make clean
     sudo make TEST=1
     ```

