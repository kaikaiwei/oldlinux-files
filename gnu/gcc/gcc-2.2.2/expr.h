/* Definitions for code generation pass of GNU compiler.
   Copyright (C) 1987, 1991 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef __STDC__
#ifndef const
#define const
#endif
#endif

/* The default branch cost is 1.  */
#ifndef BRANCH_COST
#define BRANCH_COST 1
#endif

/* Macros to access the slots of a QUEUED rtx.
   Here rather than in rtl.h because only the expansion pass
   should ever encounter a QUEUED.  */

/* The variable for which an increment is queued.  */
#define QUEUED_VAR(P) XEXP (P, 0)
/* If the increment has been emitted, this is the insn
   that does the increment.  It is zero before the increment is emitted.  */
#define QUEUED_INSN(P) XEXP (P, 1)
/* If a pre-increment copy has been generated, this is the copy
   (it is a temporary reg).  Zero if no copy made yet.  */
#define QUEUED_COPY(P) XEXP (P, 2)
/* This is the body to use for the insn to do the increment.
   It is used to emit the increment.  */
#define QUEUED_BODY(P) XEXP (P, 3)
/* Next QUEUED in the queue.  */
#define QUEUED_NEXT(P) XEXP (P, 4)

/* This is the 4th arg to `expand_expr'.
   EXPAND_SUM means it is ok to return a PLUS rtx or MULT rtx.
   EXPAND_INITIALIZER is similar but also record any labels on forced_labels.
   EXPAND_CONST_ADDRESS means it is ok to return a MEM whose address
    is a constant that is not a legitimate address.  */
enum expand_modifier {EXPAND_NORMAL, EXPAND_SUM,
		      EXPAND_CONST_ADDRESS, EXPAND_INITIALIZER};

/* List of labels that must never be deleted.  */
extern rtx forced_labels;

/* List (chain of EXPR_LISTs) of pseudo-regs of SAVE_EXPRs.
   So we can mark them all live at the end of the function, if stupid.  */
extern rtx save_expr_regs;

extern int current_function_calls_alloca;
extern int current_function_outgoing_args_size;

/* This is the offset from the arg pointer to the place where the first
   anonymous arg can be found, if there is one.  */
extern rtx current_function_arg_offset_rtx;

/* This is nonzero if the current function uses the constant pool.  */
extern int current_function_uses_const_pool;

/* This is nonzero if the current function uses pic_offset_table_rtx.  */
extern int current_function_uses_pic_offset_table;

/* The arg pointer hard register, or the pseudo into which it was copied.  */
extern rtx current_function_internal_arg_pointer;

/* Nonzero means stack pops must not be deferred, and deferred stack
   pops must not be output.  It is nonzero inside a function call,
   inside a conditional expression, inside a statement expression,
   and in other cases as well.  */
extern int inhibit_defer_pop;

/* Number of function calls seen so far in current function.  */

extern int function_call_count;

/* RTX for stack slot that holds the current handler for nonlocal gotos.
   Zero when function does not have nonlocal labels.  */

extern rtx nonlocal_goto_handler_slot;

/* RTX for stack slot that holds the stack pointer value to restore
   for a nonlocal goto.
   Zero when function does not have nonlocal labels.  */

extern rtx nonlocal_goto_stack_level;

/* List (chain of TREE_LIST) of LABEL_DECLs for all nonlocal labels
   (labels to which there can be nonlocal gotos from nested functions)
   in this function.  */

#ifdef TREE_CODE   /* Don't lose if tree.h not included.  */
extern tree nonlocal_labels;
#endif

#define NO_DEFER_POP (inhibit_defer_pop += 1)
#define OK_DEFER_POP (inhibit_defer_pop -= 1)

/* Number of units that we should eventually pop off the stack.
   These are the arguments to function calls that have already returned.  */
extern int pending_stack_adjust;

/* A list of all cleanups which belong to the arguments of
   function calls being expanded by expand_call.  */
#ifdef TREE_CODE   /* Don't lose if tree.h not included.  */
extern tree cleanups_this_call;
#endif

#ifdef TREE_CODE /* Don't lose if tree.h not included.  */
/* Structure to record the size of a sequence of arguments
   as the sum of a tree-expression and a constant.  */

struct args_size
{
  int constant;
  tree var;
};
#endif

/* Add the value of the tree INC to the `struct args_size' TO.  */

#define ADD_PARM_SIZE(TO, INC)	\
{ tree inc = (INC);				\
  if (TREE_CODE (inc) == INTEGER_CST)		\
    (TO).constant += TREE_INT_CST_LOW (inc);	\
  else if ((TO).var == 0)			\
    (TO).var = inc;				\
  else						\
    (TO).var = size_binop (PLUS_EXPR, (TO).var, inc); }

#define SUB_PARM_SIZE(TO, DEC)	\
{ tree dec = (DEC);				\
  if (TREE_CODE (dec) == INTEGER_CST)		\
    (TO).constant -= TREE_INT_CST_LOW (dec);	\
  else if ((TO).var == 0)			\
    (TO).var = size_binop (MINUS_EXPR, integer_zero_node, dec); \
  else						\
    (TO).var = size_binop (MINUS_EXPR, (TO).var, dec); }

/* Convert the implicit sum in a `struct args_size' into an rtx.  */
#define ARGS_SIZE_RTX(SIZE)						\
((SIZE).var == 0 ? gen_rtx (CONST_INT, VOIDmode, (SIZE).constant)	\
 : expand_expr (size_binop (PLUS_EXPR, (SIZE).var,			\
			    size_int ((SIZE).constant)),		\
		0, VOIDmode, 0))

/* Convert the implicit sum in a `struct args_size' into a tree.  */
#define ARGS_SIZE_TREE(SIZE)						\
((SIZE).var == 0 ? size_int ((SIZE).constant)				\
 : size_binop (PLUS_EXPR, (SIZE).var, size_int ((SIZE).constant)))

/* Supply a default definition for FUNCTION_ARG_PADDING:
   usually pad upward, but pad short, non-BLKmode args downward on
   big-endian machines.  */

enum direction {none, upward, downward};  /* Value has this type.  */

#ifndef FUNCTION_ARG_PADDING
#if BYTES_BIG_ENDIAN
#define FUNCTION_ARG_PADDING(MODE, TYPE)				\
  (((MODE) == BLKmode							\
    ? ((TYPE) && TREE_CODE (TYPE_SIZE (TYPE)) == INTEGER_CST		\
       && int_size_in_bytes (TYPE) < PARM_BOUNDARY / BITS_PER_UNIT)	\
    : GET_MODE_BITSIZE (MODE) < PARM_BOUNDARY)				\
   ? downward : upward)
#else
#define FUNCTION_ARG_PADDING(MODE, TYPE) upward
#endif
#endif

/* Supply a default definition for FUNCTION_ARG_BOUNDARY.  Normally, we let
   FUNCTION_ARG_PADDING, which also pads the length, handle any needed
   alignment.  */
  
#ifndef FUNCTION_ARG_BOUNDARY
#define FUNCTION_ARG_BOUNDARY(MODE, TYPE)	PARM_BOUNDARY
#endif

/* Nonzero if we do not know how to pass TYPE solely in registers.
   We cannot do so in the following cases:

   - if the type has variable size
   - if the type is marked as addressable (it is required to be constructed
     into the stack)
   - if the padding and mode of the type is such that a copy into a register
     would put it into the wrong part of the register
   - when STRICT_ALIGNMENT and the type is BLKmode and is is not
     aligned to a boundary corresponding to what can be loaded into a
     register.  */

#define MUST_PASS_IN_STACK_BAD_ALIGN(MODE,TYPE)			\
  (STRICT_ALIGNMENT && MODE == BLKmode			        \
   && TYPE_ALIGN (TYPE) < (BIGGEST_ALIGNMENT < BITS_PER_WORD	\
			   ? BIGGEST_ALIGNMENT : BITS_PER_WORD))
  
/* Which padding can't be supported depends on the byte endianness.  */

/* A value in a register is implicitly padded at the most significant end.
   On a big-endian machine, that is the lower end in memory.
   So a value padded in memory at the upper end can't go in a register.
   For a little-endian machine, the reverse is true.  */

#if BYTES_BIG_ENDIAN
#define MUST_PASS_IN_STACK_BAD_PADDING	upward
#else
#define MUST_PASS_IN_STACK_BAD_PADDING	downward
#endif

#define MUST_PASS_IN_STACK(MODE,TYPE)			\
  ((TYPE) != 0						\
   && (TREE_CODE (TYPE_SIZE (TYPE)) != INTEGER_CST	\
       || TREE_ADDRESSABLE (TYPE)			\
       || ((MODE) == BLKmode 				\
	   && (FUNCTION_ARG_PADDING (MODE, TYPE)	\
	       == MUST_PASS_IN_STACK_BAD_PADDING))	\
       || MUST_PASS_IN_STACK_BAD_ALIGN (MODE, TYPE)))

/* Nonzero if type TYPE should be returned in memory
   (even though its mode is not BLKmode).
   Most machines can use the following default definition.  */

#ifndef RETURN_IN_MEMORY
#define RETURN_IN_MEMORY(TYPE) 0
#endif

/* Optabs are tables saying how to generate insn bodies
   for various machine modes and numbers of operands.
   Each optab applies to one operation.
   For example, add_optab applies to addition.

   The insn_code slot is the enum insn_code that says how to
   generate an insn for this operation on a particular machine mode.
   It is CODE_FOR_nothing if there is no such insn on the target machine.

   The `lib_call' slot is the name of the library function that
   can be used to perform the operation.

   A few optabs, such as move_optab and cmp_optab, are used
   by special code.  */

/* Everything that uses expr.h needs to define enum insn_code
   but we don't list it in the Makefile dependencies just for that.  */
#include "insn-codes.h"

typedef struct optab
{
  enum rtx_code code;
  struct {
    enum insn_code insn_code;
    rtx libfunc;
  } handlers [NUM_MACHINE_MODES];
} * optab;

/* Given an enum insn_code, access the function to construct
   the body of that kind of insn.  */
#ifdef FUNCTION_CONVERSION_BUG
/* Some compilers fail to convert a function properly to a
   pointer-to-function when used as an argument.
   So produce the pointer-to-function directly.
   Luckily, these compilers seem to work properly when you
   call the pointer-to-function.  */
#define GEN_FCN(CODE) (insn_gen_function[(int) (CODE)])
#else
#define GEN_FCN(CODE) (*insn_gen_function[(int) (CODE)])
#endif

extern rtx (*const insn_gen_function[]) ();

extern optab add_optab;
extern optab sub_optab;
extern optab smul_optab;	/* Signed and floating-point multiply */
extern optab smul_widen_optab;	/* Signed multiply with result 
				   one machine mode wider than args */
extern optab umul_widen_optab;
extern optab sdiv_optab;	/* Signed divide */
extern optab sdivmod_optab;	/* Signed divide-and-remainder in one */
extern optab udiv_optab;
extern optab udivmod_optab;
extern optab smod_optab;	/* Signed remainder */
extern optab umod_optab;
extern optab flodiv_optab;	/* Optab for floating divide. */
extern optab ftrunc_optab;	/* Convert float to integer in float fmt */
extern optab and_optab;		/* Logical and */
extern optab ior_optab;		/* Logical or */
extern optab xor_optab;		/* Logical xor */
extern optab ashl_optab;	/* Arithmetic shift left */
extern optab ashr_optab;	/* Arithmetic shift right */
extern optab lshl_optab;	/* Logical shift left */
extern optab lshr_optab;	/* Logical shift right */
extern optab rotl_optab;	/* Rotate left */
extern optab rotr_optab;	/* Rotate right */
extern optab smin_optab;	/* Signed and floating-point minimum value */
extern optab smax_optab;	/* Signed and floating-point maximum value */
extern optab umin_optab;	/* Unsigned minimum value */
extern optab umax_optab;	/* Unsigned maximum value */

extern optab mov_optab;		/* Move instruction.  */
extern optab movstrict_optab;	/* Move, preserving high part of register.  */

extern optab cmp_optab;		/* Compare insn; two operands.  */
extern optab tst_optab;		/* tst insn; compare one operand against 0 */

/* Unary operations */
extern optab neg_optab;		/* Negation */
extern optab abs_optab;		/* Abs value */
extern optab one_cmpl_optab;	/* Bitwise not */
extern optab ffs_optab;		/* Find first bit set */
extern optab sqrt_optab;	/* Square root */
extern optab strlen_optab;	/* String length */

/* Passed to expand_binop and expand_unop to say which options to try to use
   if the requested operation can't be open-coded on the requisite mode.
   Either OPTAB_LIB or OPTAB_LIB_WIDEN says try using a library call.
   Either OPTAB_WIDEN or OPTAB_LIB_WIDEN says try using a wider mode.
   OPTAB_MUST_WIDEN says try widening and don't try anything else.  */

enum optab_methods
{
  OPTAB_DIRECT,
  OPTAB_LIB,
  OPTAB_WIDEN,
  OPTAB_LIB_WIDEN,
  OPTAB_MUST_WIDEN
};

/* SYMBOL_REF rtx's for the library functions that are called
   implicitly and not via optabs.  */

extern rtx extendsfdf2_libfunc;
extern rtx truncdfsf2_libfunc;
extern rtx memcpy_libfunc;
extern rtx bcopy_libfunc;
extern rtx memcmp_libfunc;
extern rtx bcmp_libfunc;
extern rtx memset_libfunc;
extern rtx bzero_libfunc;
extern rtx eqsf2_libfunc;
extern rtx nesf2_libfunc;
extern rtx gtsf2_libfunc;
extern rtx gesf2_libfunc;
extern rtx ltsf2_libfunc;
extern rtx lesf2_libfunc;
extern rtx eqdf2_libfunc;
extern rtx nedf2_libfunc;
extern rtx gtdf2_libfunc;
extern rtx gedf2_libfunc;
extern rtx ltdf2_libfunc;
extern rtx ledf2_libfunc;
extern rtx floatdisf_libfunc;
extern rtx floatsisf_libfunc;
extern rtx floatdidf_libfunc;
extern rtx floatsidf_libfunc;
extern rtx fixsfsi_libfunc;
extern rtx fixsfdi_libfunc;
extern rtx fixdfsi_libfunc;
extern rtx fixdfdi_libfunc;
extern rtx fixunssfsi_libfunc;
extern rtx fixunssfdi_libfunc;
extern rtx fixunsdfsi_libfunc;
extern rtx fixunsdfdi_libfunc;

typedef rtx (*rtxfun) ();

/* Indexed by the rtx-code for a conditional (eg. EQ, LT,...)
   gives the gen_function to make a branch to test that condition.  */

extern rtxfun bcc_gen_fctn[NUM_RTX_CODE];

/* Indexed by the rtx-code for a conditional (eg. EQ, LT,...)
   gives the insn code to make a store-condition insn
   to test that condition.  */

extern enum insn_code setcc_gen_code[NUM_RTX_CODE];

/* Expand a binary operation given optab and rtx operands.  */
extern rtx expand_binop ();

/* Expand a binary operation with both signed and unsigned forms.  */
extern rtx sign_expand_binop ();

/* Expand a unary arithmetic operation given optab rtx operand.  */
extern rtx expand_unop ();

/* Arguments MODE, RTX: return an rtx for the negation of that value.
   May emit insns.  */
extern rtx negate_rtx ();

/* Expand a logical AND operation.  */
extern rtx expand_and ();

/* Emit a store-flag operation.  */
extern rtx emit_store_flag ();

/* Return the CODE_LABEL rtx for a LABEL_DECL, creating it if necessary.  */
extern rtx label_rtx ();

/* Given a JUMP_INSN, return a description of the test being made.  */
extern rtx get_condition ();

/* Return the INSN_CODE to use for an extend operation.  */
extern enum insn_code can_extend_p ();

/* Initialize the tables that control conversion between fixed and
   floating values.  */
extern void init_fixtab ();
extern void init_floattab ();

/* Generate code for a FIX_EXPR.  */
extern void expand_fix ();

/* Generate code for a FLOAT_EXPR.  */
extern void expand_float ();

/* Create but don't emit one rtl instruction to add one rtx into another.
   Modes must match; operands must meet the operation's predicates.
   Likewise for subtraction and for just copying.
   These do not call protect_from_queue; caller must do so.  */
extern rtx gen_add2_insn ();
extern rtx gen_sub2_insn ();
extern rtx gen_move_insn ();

/* Emit one rtl instruction to store zero in specified rtx.  */
extern void emit_clr_insn ();

/* Emit one rtl insn to store 1 in specified rtx assuming it contains 0.  */
extern void emit_0_to_1_insn ();

/* Emit one rtl insn to compare two rtx's.  */
extern void emit_cmp_insn ();

/* Generate rtl to compare two rtx's, will call emit_cmp_insn.  */
extern rtx compare_from_rtx ();

/* Emit some rtl insns to move data between rtx's, converting machine modes.
   Both modes must be floating or both fixed.  */
extern void convert_move ();

/* Convert an rtx to specified machine mode and return the result.  */
extern rtx convert_to_mode ();

/* Emit code to push some arguments and call a library routine,
   storing the value in a specified place.  Calling sequence is
   complicated.  */
extern void emit_library_call ();

/* Given an rtx that may include add and multiply operations,
   generate them as insns and return a pseudo-reg containing the value.
   Useful after calling expand_expr with 1 as sum_ok.  */
extern rtx force_operand ();

/* Return an rtx for the size in bytes of the value of an expr.  */
extern rtx expr_size ();

/* Return an rtx for the sum of an rtx and an integer.  */
extern rtx plus_constant ();

extern rtx lookup_static_chain ();

/* Return an rtx like arg but sans any constant terms.
   Returns the original rtx if it has no constant terms.
   The constant terms are added and stored via a second arg.  */
extern rtx eliminate_constant_term ();

/* Convert arg to a valid memory address for specified machine mode,
   by emitting insns to perform arithmetic if nec.  */
extern rtx memory_address ();

/* Like `memory_address' but pretent `flag_force_addr' is 0.  */
extern rtx memory_address_noforce ();

/* Return a memory reference like MEMREF, but with its mode changed
   to MODE and its address changed to ADDR.
   (VOIDmode means don't change the mode.
   NULL for ADDR means don't change the address.)  */
extern rtx change_address ();

/* Return a memory reference like MEMREF, but which is known to have a
   valid address.  */

extern rtx validize_mem ();

/* Convert a stack slot address ADDR valid in function FNDECL
   into an address valid in this function (using a static chain).  */
extern rtx fix_lexical_addr ();

/* Return the address of the trampoline for entering nested fn FUNCTION.  */
extern rtx trampoline_address ();

/* Assemble the static constant template for function entry trampolines.  */
extern rtx assemble_trampoline_template ();

/* Return 1 if two rtx's are equivalent in structure and elements.  */
extern int rtx_equal_p ();

/* Given rtx, return new rtx whose address won't be affected by
   any side effects.  It has been copied to a new temporary reg.  */
extern rtx stabilize ();

/* Given an rtx, copy all regs it refers to into new temps
   and return a modified copy that refers to the new temps.  */
extern rtx copy_all_regs ();

/* Copy given rtx to a new temp reg and return that.  */
extern rtx copy_to_reg ();

/* Like copy_to_reg but always make the reg Pmode.  */
extern rtx copy_addr_to_reg ();

/* Like copy_to_reg but always make the reg the specified mode MODE.  */
extern rtx copy_to_mode_reg ();

/* Copy given rtx to given temp reg and return that.  */
extern rtx copy_to_suggested_reg ();

/* Copy a value to a register if it isn't already a register.
   Args are mode (in case value is a constant) and the value.  */
extern rtx force_reg ();

/* Return given rtx, copied into a new temp reg if it was in memory.  */
extern rtx force_not_mem ();

/* Remove some bytes from the stack.  An rtx says how many.  */
extern void adjust_stack ();

/* Add some bytes to the stack.  An rtx says how many.  */
extern void anti_adjust_stack ();

/* This enum is used for the following two functions.  */
enum save_level {SAVE_BLOCK, SAVE_FUNCTION, SAVE_NONLOCAL};

/* Save the stack pointer at the specified level.  */
extern void emit_stack_save ();

/* Restore the stack pointer from a save area of the specified level.  */
extern void emit_stack_restore ();

/* Allocate some space on the stack dynamically and return its address.  An rtx
   says how many bytes.  */
extern rtx allocate_dynamic_stack_space ();

/* Emit code to copy function value to a new temp reg and return that reg.  */
extern rtx function_value ();

/* Return an rtx that refers to the value returned by a function
   in its original home.  This becomes invalid if any more code is emitted.  */
extern rtx hard_function_value ();

/* Return an rtx that refers to the value returned by a library call
   in its original home.  This becomes invalid if any more code is emitted.  */
extern rtx hard_libcall_value ();

/* Emit code to copy function value to a specified place.  */
extern void copy_function_value ();

/* Given an rtx, return an rtx for a value rounded up to a multiple
   of STACK_BOUNDARY / BITS_PER_UNIT.  */
extern rtx round_push ();

/* Push a block of length SIZE (perhaps variable)
   and return an rtx to address the beginning of the block.  */
extern rtx push_block ();

/* Generate code for computing expression EXP,
   and storing the value into TARGET.
   If SUGGEST_REG is nonzero, copy the value through a register
   and return that register, if that is possible.  */
extern rtx store_expr ();

extern rtx prepare_call_address ();
extern rtx expand_call ();
extern void emit_call_1 ();

extern void emit_block_move ();
extern void emit_push_insn ();
extern void use_regs ();
extern void move_block_to_reg ();

extern rtx store_bit_field ();
extern rtx extract_bit_field ();
extern rtx expand_shift ();
extern rtx expand_mult ();
extern rtx expand_divmod ();
extern rtx expand_mult_add ();
extern rtx expand_stmt_expr ();
extern rtx emit_no_conflict_block ();
extern void emit_libcall_block ();

extern void jumpifnot ();
extern void jumpif ();
extern void do_jump ();

extern rtx assemble_static_space ();

extern void locate_and_pad_parm ();

extern rtx expand_inline_function ();

/* Hook called by expand_expr for language-specific tree codes.
   It is up to the language front end to install a hook
   if it has any such codes that expand_expr needs to know about.  */
extern rtx (*lang_expand_expr) ();
