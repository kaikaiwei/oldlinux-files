   1、MOVS指令

    创建MOVS指令是为了向程序员提供把字符串从一个内存位置传送到另一个内存位置的简单途

径。MOVS指令有3种格式：

* MOVSB：传送单一字节
* MOVSW：传送一个字（2字节）
* MOVSL：传送一个双字（4字节）

  MOVS指令使用隐含的源和目的操作数。隐含的源操作数是ESI寄存器。它指向源字符串的内存

位置。隐含的目标操作数是EDI寄存器。它指向字符串要被复制到的目标内存位置。

    有两种方式加载ESI和EDI值。第一种是使用间接寻址：

```
movl $output, %edi  /*这条指令把output标签的32位内存位置传送给EDI寄存器*/
```

    另一种方式是LEA指令，加载一个对象的有效地址：

```
leal output, %edi/*把output标签的32位内存位置加载到EDI寄存器中*/
```


```
.section .data
  value1:
    .ascii "This is a test string\n"

.section .bss
  .lcomm output, 23

.section .text
  .globl _start

_start:
  leal value1, %esi
  leal output, %edi
  movsb
  movsw
  movsl

  movl $1, %eax
  movl $0, %ebx
  int $0x80
```

    如果EFLAGS寄存器中的DF位被清零，那么每条MOVS指令执行之后ESI和EDI寄存器就会

递增。如果DF位被设置，那么每条MOVS指令执行之后ESI和EDI寄存器就会递减，可以使用下

面的命令：

* CLD用于将DF标志清零
* STD用于设置DF标志

  使用STD指令时，ESI和EDI寄存器在每一条MOVS指令执行之后递减，所以它们应该指向字

符串的末尾，而不是开头。
