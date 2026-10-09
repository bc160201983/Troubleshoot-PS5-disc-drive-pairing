/* SPDX-License-Identifier: GPL-3.0-or-later
 * Invoke only dynlib_dlsym (591) through the verified loader syscall stub.
 * No direct syscall instruction is emitted into the payload.
 */
.intel_syntax noprefix
.text
.global probe_dynlib_dlsym
.type probe_dynlib_dlsym, @function
probe_dynlib_dlsym:
    mov rax, 591
    mov r10, rcx
    jmp qword ptr [rip + probe_syscall_instruction]

.global probe_write_stdout
.type probe_write_stdout, @function
probe_write_stdout:
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, 1
    mov rax, 4
    jmp qword ptr [rip + probe_syscall_instruction]
