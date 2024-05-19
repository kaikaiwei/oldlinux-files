# 1、MOVS指令

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

# repne scasb=

repne是一个前缀，表示反转零标志位的情况下重复执行。用于scasb期间，用于检查ZF（零标志位）标记。

scasb是一个字符串比较指令，用于比较字符串中，第一个与al寄存器中的值。执行以下操作

1，将al与字符串中的一个字节进行比较

2，更新ZF标记，用来表示结果。如果相等，ZF为1。否则ZF为0。

3，DI寄存器向前或向后，取决于方向标志位DF（DF位0，DI增加。 DF位1，DI减少）

# str

=**[STR指令**](https://www.baidu.com/s?rsv_idx=1&wd=STR%E6%8C%87%E4%BB%A4&fenlei=256&usm=2&ie=utf-8&rsv_pq=b5919e3100967c03&oq=str%E6%8C%87%E4%BB%A4&rsv_t=34c7K%2FVOGlXMyrKcawsjF8YfNG9A9B0p45MIAawu61T5PuKe%2FDNK3FvDqVU&sa=re_dqa_zy&icon=1)在ARM汇编中用于将数据从一个寄存器写入内存中的指定地址。**=其语法为STR{[type**](https://www.baidu.com/s?rsv_idx=1&wd=type&fenlei=256&usm=2&ie=utf-8&rsv_pq=b5919e3100967c03&oq=str%E6%8C%87%E4%BB%A4&rsv_t=f8e0k9EHjRv0j9UCak7ViSysc%2BBxMYLiivvlqiZLA7wUk69Rs2viiTyMaFg&sa=re_dqa_zy&icon=1)}{[cond**](https://www.baidu.com/s?rsv_idx=1&wd=cond&fenlei=256&usm=2&ie=utf-8&rsv_pq=b5919e3100967c03&oq=str%E6%8C%87%E4%BB%A4&rsv_t=f8e0k9EHjRv0j9UCak7ViSysc%2BBxMYLiivvlqiZLA7wUk69Rs2viiTyMaFg&sa=re_dqa_zy&icon=1)}[Rd**](https://www.baidu.com/s?rsv_idx=1&wd=Rd&fenlei=256&usm=2&ie=utf-8&rsv_pq=b5919e3100967c03&oq=str%E6%8C%87%E4%BB%A4&rsv_t=fd49q34V1mNb06w7fhgiPWJdMb8iY2diEAWi5njdGLg8a32dgVBnqQK3fu4&sa=re_dqa_zy&icon=1),[[Rn**](https://www.baidu.com/s?rsv_idx=1&wd=%5BRn&fenlei=256&usm=2&ie=utf-8&rsv_pq=b5919e3100967c03&oq=str%E6%8C%87%E4%BB%A4&rsv_t=fd49q34V1mNb06w7fhgiPWJdMb8iY2diEAWi5njdGLg8a32dgVBnqQK3fu4&sa=re_dqa_zy&icon=1){,#offset}]。其中，type表示数据类型，如字节(B)、半字(H)、有符号字节(SB)、有符号半字(SH)，cond是条件码，可省略，Rd是要写入的寄存器，Rn是目标内存地址的基址寄存器，offset是偏移量。例如，将32位整数值0x12345678写入到内存地址为0x1000的位置，可以使用指令STR R2,[R1]，其中R2包含要写入的数据，R1指向目标内存地址。^1^^2^

此外，STR指令也广泛应用于字符串操作，如字符串的连接、复制、索引和切片等。这些操作涉及字符串的拼接、重复、访问和子串提取等功能，通过STR指令可以将字符串数据写入内存或从内存中读取。

在ARM体系中，STR指令与[LDR指令**](https://www.baidu.com/s?rsv_idx=1&wd=LDR%E6%8C%87%E4%BB%A4&fenlei=256&usm=2&ie=utf-8&rsv_pq=b5919e3100967c03&oq=str%E6%8C%87%E4%BB%A4&rsv_t=a242HMp3lq0DTY7mUG0c6%2F%2F9Toauj%2Fmvgv%2F9rqoG71FZXjscG0Pgs4PWNtU&sa=re_dqa_zy&icon=1)相对，LDR用于将内存中的数据载入到寄存器，而STR则用于将寄存器中的数据写入内存。这两个指令在内存与寄存器之间进行数据传输时非常有用。




# *(&eip)

*(&eip) =sa_handler;

载signal.c中，
