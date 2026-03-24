# linux-vulnserver

A Linux-native port of [vulnserver](https://github.com/stephenbradshaw/vulnserver) by Stephen Bradshaw, rewritten for buffer overflow exploitation exercises on Kali Linux.

> **WARNING: This software is intentionally vulnerable.**
> Run only in isolated lab environments. Never expose to untrusted networks.

## Background

The original vulnserver is a Windows TCP server designed for practicing classic stack-based buffer overflow exploitation. This port reproduces the same vulnerable command set as a 32-bit Linux ELF binary, removing the need for Wine or a Windows VM. It includes a deliberate `JMP ESP` gadget compiled into the binary so that standard exploitation techniques (fuzzing → offset → EIP overwrite → bad chars → JMP ESP → shellcode) work without relying on external DLLs.

## Prerequisites

```bash
sudo apt install gcc gcc-multilib
```

## Compilation

```bash
gcc vulnserver.c -o vulnserver \
    -m32 -fno-stack-protector -z execstack -no-pie -lpthread
```

| Flag | Purpose |
|------|---------|
| `-m32` | Compile as 32-bit x86 binary |
| `-fno-stack-protector` | Disable stack canaries |
| `-z execstack` | Mark the stack as executable (no NX/DEP) |
| `-no-pie` | Fixed load address — gadget addresses are stable across runs |
| `-lpthread` | Link pthreads for per-connection threads |

## Usage

```bash
./vulnserver          # listens on port 9999 (default)
./vulnserver 8888     # custom port
```

Connect with netcat to verify:

```bash
nc 127.0.0.1 9999
```

You should see:

```
Welcome to Vulnerable Server! Enter HELP for help.
```

## Vulnerable commands

The `TRUN` command is the primary exploitation target. Sending a payload containing a `.` character causes the input to be copied into a 2000-byte stack buffer via `strcpy()` with no bounds checking.

Other commands (`GTER`, `KSTET`, `LTER`, `GMON`) trigger similar vulnerabilities with different buffer sizes and constraints.

## Credits

Original vulnserver by **Stephen Bradshaw** — [thegreycorner.com](http://www.thegreycorner.com/)
Source: https://github.com/stephenbradshaw/vulnserver

This Linux port was created for the **Security and Testing in Information Technology (SPTI)** course at the Escuela Colombiana de Ingenieria Julio Garavito.
