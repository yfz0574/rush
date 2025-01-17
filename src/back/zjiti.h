﻿#pragma once

#include "zjitb.h"

//jit指令类
struct zjiti
{
	static rstr error()
	{
		rserror();
		return rstr();
	}

	static rstr b_call(const tins& ins,uchar* start)
	{
		if(ins.get_first_type()==topnd::c_imme)
		{
#ifdef _WIN64
			return b_mov64_rax_i(ins.first.val64())+b_jmp_rax(0xff,0xd0);
#else
			return zjitb::build_relative(ins,start,0xe8);
#endif
		}
		elif(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0xff,0xd0);
		}
		elif(ins.get_first_type()==topnd::c_addr)
		{
			return zjitb::build_a(ins,0xff,0x90);
		}
		return error();
	}

	static rstr b_jmp_rax(int one,int two)
	{
		tins b;
		b.first.off=treg::c_rax;
		return zjitb::build_r(b,one,two);
	}

	static rstr b_retn()
	{
		return zjitb::build_one(0xc3);
	}

	static rstr b_reti(const tins& ins)
	{
		rstr s;
		s.set_size(3);
		s[0]=0xc2;
		if(ins.first.val>65535)
		{
			return rstr();
		}
		*(ushort*)(&s[1])=(ushort)(ins.first.val);//只有16位的reti，截断
		return r_move(s);
	}

	static rstr b_push(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_imme)
		{
			return zjitb::build_i(ins,0x68);
		}
		elif(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0x50);
		}
		elif(ins.get_first_type()==topnd::c_addr)
		{
			return zjitb::build_a(ins,0xff,0xb0);
		}
		return error();
	}

	static rstr b_pop(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0x58);
		}
		return error();
	}

	static rstr b_jmp(const tins& ins,uchar* start)
	{
		if(ins.get_first_type()==topnd::c_imme)
		{
#ifdef _WIN64
			return b_mov64_rax_i(ins.first.val64())+b_jmp_rax(0xff,0xe0);
#else
			return zjitb::build_relative(ins,start,0xe9);
#endif
		}
		elif(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0xff,0xe0);
		}
		elif(ins.get_first_type()==topnd::c_addr)
		{
			return zjitb::build_a(ins,0xff,0xa0);
		}
		return error();
	}

	static rstr b_jz(const tins& ins,uchar* start)
	{
		if(ins.get_first_type()==topnd::c_imme)
		{
#ifdef _WIN64
			return b_mov64_rax_i(ins.first.val64())+zjitb::build_two(0x75,0x02)+b_jmp_rax(0xff,0xe0);
#else
			return zjitb::build_relative(ins,start,0x0f,0x84);
#endif	
		}
		return error();
	}

	static rstr b_jnz(const tins& ins,uchar* start)
	{
		if(ins.get_first_type()==topnd::c_imme)
		{
#ifdef _WIN64
			return b_mov64_rax_i(ins.first.val64())+zjitb::build_two(0x74,0x02)+b_jmp_rax(0xff,0xe0);
#else
			return zjitb::build_relative(ins,start,0x0f,0x85);
#endif	
		}
		return error();
	}

	static rstr b_nop()
	{
		return zjitb::build_one(0x90);
	}

	static rstr b_lea(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg&&ins.get_second_type()==topnd::c_addr)
		{
#ifdef _WIN64
			return zjitb::build_one(0x48)+zjitb::build_ra(ins,0x8d,0x80);
#else
			return zjitb::build_ra(ins,0x8d,0x80);
#endif
		}
		return error();
	}

	static rstr b_mov(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ri(ins,0xb8);
			}
			elif(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x8b,0xc0);
			}
			elif(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x8b,0x80);
			}
		}
		elif(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
#ifdef _WIN64
				ifn(ins.first.is_reg64())
				{
					return zjitb::build_one(0x67)+zjitb::build_ai(ins,0xc7,0x80);
				}
#endif
				return zjitb::build_ai(ins,0xc7,0x80);
			}
			elif(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x89,0x80);
			}
		}
		return error();
	}

	static rstr b_mov64(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ri_64(ins,0x48,0xb8);
			}
			elif(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x48,0x8b,0xc0);
			}
			elif(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x48,0x8b,0x80);
			}
		}
		elif(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0xc7,0x80);
			}
			elif(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x48,0x89,0x80);
			}
		}
		return error();
	}

	static rstr b_mov64_rax_i(int64 val)
	{
		tins a;
		a.type=tins::c_mov64_ri;
		a.first.off=treg::c_rax;
		a.second.val64()=val;
		return b_mov64(a);
	}

	static rstr b_mov8_cl_addr(const tins& ins)
	{
		rstr s;
		if(ins.second.off==treg::c_esp||ins.second.off==treg::c_rsp)
		{
			s.set_size(7);
		}
		else
		{
			s.set_size(6);
		}
		s[0]=0x8a;
		s[1]=0x88;
		zjitb::set_addr_bit(ins.second.off,&s[1],ins.second.val);
		return r_move(s);
	}

	static rstr b_mov8_addr_cl(const tins& ins)
	{
		rstr s;
		if(ins.first.off==treg::c_esp||ins.first.off==treg::c_rsp)
		{
			s.set_size(7);
		}
		else
		{
			s.set_size(6);
		}
		s[0]=0x88;
		s[1]=0x88;
		zjitb::set_addr_bit(ins.first.off,&s[1],ins.first.val);
		return r_move(s);
	}

	static rstr b_add(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
#ifdef _WIN64
				if(ins.first.is_reg64())
				{
					return zjitb::build_one(0x48)+zjitb::build_ri(ins,0x81,0xc0);
				}
#endif
				return zjitb::build_ri(ins,0x81,0xc0);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x03,0xc0);
			}
			if(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x03,0x80);
			}
		}
		if(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0x81,0x80);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x01,0x80);
			}
		}
		return error();
	}

	static rstr b_sub(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
#ifdef _WIN64
				if(ins.first.is_reg64())
				{
					return zjitb::build_one(0x48)+zjitb::build_ri(ins,0x81,0xe8);
				}
#endif
				return zjitb::build_ri(ins,0x81,0xe8);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x2b,0xc0);
			}
			if(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x2b,0x80);
			}
		}
		if(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0x81,0xa8);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x29,0x80);
			}
		}
		return error();
	}

	static rstr b_imul(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg&&ins.get_second_type()==topnd::c_addr)
		{
			return zjitb::build_ra(ins,0x0f,0xaf,0x80);
		}
		return error();
	}

	static rstr b_idiv(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0xf7,0xf8);
		}
		return error();
	}

	static rstr b_udiv(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0xf7,0xf0);
		}
		return error();
	}

	static rstr b_cdq()
	{
		return zjitb::build_one(0x99);
	}

	static rstr b_sete_bl()
	{
		return zjitb::build_three(0x0f,0x94,0xc3);
	}

	static rstr b_setne_bl()
	{
		return zjitb::build_three(0x0f,0x95,0xc3);
	}

	static rstr b_setg_bl()
	{
		return zjitb::build_three(0x0f,0x9f,0xc3);
	}

	static rstr b_setge_bl()
	{
		return zjitb::build_three(0x0f,0x9d,0xc3);
	}

	static rstr b_setl_bl()
	{
		return zjitb::build_three(0x0f,0x9c,0xc3);
	}

	static rstr b_setle_bl()
	{
		return zjitb::build_three(0x0f,0x9e,0xc3);
	}

	static rstr b_seta_bl()
	{
		return zjitb::build_three(0x0f,0x97,0xc3);
	}

	static rstr b_setae_bl()
	{
		return zjitb::build_three(0x0f,0x93,0xc3);
	}

	static rstr b_setb_bl()
	{
		return zjitb::build_three(0x0f,0x92,0xc3);
	}

	static rstr b_setbe_bl()
	{
		return zjitb::build_three(0x0f,0x96,0xc3);
	}

	static rstr b_cmp(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ri(ins,0x81,0xf8);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x3b,0xc0);
			}
			if(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x3b,0x80);
			}
		}
		if(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0x81,0xb8);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x39,0x80);
			}
		}
		return error();
	}

	static rstr b_and(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ri(ins,0x81,0xe0);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x23,0xc0);
			}
			if(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x23,0x80);
			}
		}
		if(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0x81,0xa0);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x21,0x80);
			}
		}
		return error();
	}

	static rstr b_or(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ri(ins,0x81,0xc8);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x0b,0xc0);
			}
			if(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x0b,0x80);
			}
		}
		if(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0x81,0x88);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x09,0x80);
			}
		}
		return error();
	}

	static rstr b_xor(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ri(ins,0x81,0xf0);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_rr(ins,0x33,0xc0);
			}
			if(ins.get_second_type()==topnd::c_addr)
			{
				return zjitb::build_ra(ins,0x33,0x80);
			}
		}
		if(ins.get_first_type()==topnd::c_addr)
		{
			if(ins.get_second_type()==topnd::c_imme)
			{
				return zjitb::build_ai(ins,0x81,0xb0);
			}
			if(ins.get_second_type()==topnd::c_reg)
			{
				return zjitb::build_ar(ins,0x31,0x80);
			}
		}
		return error();
	}

	static rstr b_not(const tins& ins)
	{
		if(ins.get_first_type()==topnd::c_reg)
		{
			return zjitb::build_r(ins,0xf7,0xd0);
		}
		elif(ins.get_first_type()==topnd::c_addr)
		{
			return zjitb::build_a(ins,0xf7,0x90);
		}
		return error();
	}

	static rstr b_shl_eax_cl()
	{
		return zjitb::build_two(0xd3,0xe0);
	}

	static rstr b_shr_eax_cl()
	{
		return zjitb::build_two(0xd3,0xe8);
	}

	static rstr b_sar_eax_cl()
	{
		return zjitb::build_two(0xd3,0xf8);
	}
};
