.section .rodata
.global secret_data
secret_data:
.incbin "secret.txt"
.byte 0