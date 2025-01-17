﻿#pragma once

#include "../front/ypre.h"
#include "zjiti.h"
#include "zjitf.h"
#include "znasm.h"

struct zjit
{
	static rbool run(tsh& sh)
	{
#ifndef _RS
		tfunc* ptfi=yfind::find_func(*sh.pmain,"main_c");
		if(ptfi==null)
		{
			rf::printl("main not find");
			return false;
		}
		ifn(compile_func_to_x86(sh,*ptfi,tenv()))
		{
			return false;
		}
		((void (*)())(ptfi->code))();
#endif
		return true;
	}

	//将一个函数翻译成X86代码
	static rbool compile_func_to_x86(tsh& sh,tfunc& tfi,tenv env)
	{
		ifn(tfi.vasm.empty())
		{
			return true;
		}
		ifn(zbin::compile_vword_to_vasm(sh,tfi,env))
		{
			return false;
		}
#ifdef _WIN64
		int size=tfi.vasm.count()*12;
#else
		int size=tfi.vasm.count()*6;//todo 估算法并不是最好的方法
#endif
		size=r_ceil_div(size,4096)*4096;
		if(tfi.code==null)
		{
			tfi.code=xf::alloc_v(size);
			if(tfi.code==null)
			{
				return false;
			}
			sh.s_point.insert(taddr(
				(uint)(tfi.code),(uint)(tfi.code+size),&tfi));
		}
		int cur=0;
		for(int i=0;i<tfi.vasm.count();i++)
		{
			if(ybase::is_tag<rstr>(tfi.vasm[i].vstr))
			{
				continue;
			}
			tfi.vasm[i].ptfi=&tfi;
			ifn(parse_asm(sh,tfi.vasm[i]))
			{
				return false;
			}
			tfi.vasm[i].start=tfi.code+cur;
			rstr s=trans_asm_to_x86(sh,tfi.vasm[i],tfi.code+cur);
			if(s.empty())
			{
				rserror(tfi.vasm[i],"can't build jit ins");
				return false;
			}
			if(cur+s.count()>size)
			{
				rserror(tfi,"func too big");
				return false;
			}
			xf::memcpy(tfi.code+cur,s.begin(),s.count());
			cur+=s.count();
		}
		for(int i=0;i<tfi.vasm.count();i++)
		{
			ifn(fix_addr(sh,tfi.vasm[i],tfi.vasm))
			{
				return false;
			}
		}
		return true;
	}

	static rbool fix_addr(tsh& sh,tasm& oasm,rbuf<tasm>& vasm)
	{
		ifn(oasm.vstr.count()==2&&
			ybase::is_jmp_ins(oasm.ins.get_index())&&
			!sh.key.is_asm_reg(oasm.vstr[1]))
		{
			return true;
		}
		int i;
		for(i=0;i<vasm.count();i++)
		{
			if(ybase::is_tag<rstr>(vasm[i].vstr)&&
				vasm[i].vstr[0]==oasm.vstr[1])
			{
				break;
			}
		}
		if(i>=vasm.count())
		{
			rserror(oasm);
			return false;
		}
		int j;
		for(j=i+1;j<vasm.count();j++)
		{
			ifn(ybase::is_tag<rstr>(vasm[j].vstr))
			{
				break;
			}
		}
		if(j>=vasm.count())
		{
			rserror(oasm);
			return false;
		}
#ifdef _WIN64
		oasm.ins.first.val64()=(int64)(vasm[j].start);
#else
		oasm.ins.first.val=(uint)(vasm[j].start);
#endif
		uchar* real=oasm.start;
		rstr s=trans_asm_to_x86(sh,oasm,real);
		if(s.empty())
		{
			return false;
		}
		xf::memcpy(real,s.begin(),s.count());
		return true;
	}

	static rstr trans_asm_to_x86(tsh& sh,tasm& item,uchar* start)
	{
#ifndef _RS
		tins& ins=item.ins;
		rbuf<rstr>& vstr=item.vstr;
		rstr s;
		switch(ins.get_index())
		{
		case tkey::c_rbyte:
			return zjitb::build_one(ins.first.val);
		case tkey::c_calle:
			//todo 应判断start与item.start差值是否超过2G
			return proc_calle(sh,ins,start);
		case tkey::c_call:
			return zjiti::b_call(ins,start);
		case tkey::c_retn:
			return zjiti::b_retn();
		case tkey::c_reti:
			return zjiti::b_reti(ins);
		case tkey::c_push:
			return zjiti::b_push(ins);
		case tkey::c_pop:
			return zjiti::b_pop(ins);
		case tkey::c_jmp:
			return zjiti::b_jmp(ins,start);
		case tkey::c_jebxz:
			//没有cmp指令，按照nop处理
			return (rsj4(b_cmp,rskey(c_ebx),rsoptr(c_comma),"0")+
				zjiti::b_jz(ins,start+6));
		case tkey::c_jebxnz:
			return (rsj4(b_cmp,rskey(c_ebx),rsoptr(c_comma),"0")+
				zjiti::b_jnz(ins,start+6));
		case tkey::c_nop:
			return zjiti::b_nop();
		case tkey::c_lea:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_lea(ins);
			}
			//应该检查b_lea是否返回空串
#ifdef _WIN64
			return (rsj4(b_lea,rskey(c_rcx),rsoptr(c_comma),rsjb)+
				rsj4(b_mov64,rsja,rsoptr(c_comma),rskey(c_rcx)));
#else
			return (rsj4(b_lea,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_ecx)));
#endif
		case tkey::c_mov:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_mov(ins);
			}
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_ecx)));
		case tkey::c_mov8:
			return (rsj4(b_mov8_cl_addr,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_mov8_addr_cl,rsja,rsoptr(c_comma),rskey(c_ecx)));
		case tkey::c_mov64:
#ifdef _WIN64
			if(znasm::count_mbk_l(vstr)==2)
			{
				return (rsj4(b_mov64,rskey(c_rcx),rsoptr(c_comma),rsjb)+
					rsj4(b_mov64,rsja,rsoptr(c_comma),rskey(c_rcx)));
			}
			return zjiti::b_mov64(ins);
#else
			return zjiti::error();
#endif
		case tkey::c_add:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_add(ins);
			}
#ifdef _WIN64
			return zjiti::error();
#else
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_add,rsja,rsoptr(c_comma),rskey(c_ecx)));
#endif
		case tkey::c_sub:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_sub(ins);
			}
#ifdef _WIN64
			return zjiti::error();
#else
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_sub,rsja,rsoptr(c_comma),rskey(c_ecx)));
#endif
		case tkey::c_imul:
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsja)+
				rsj4(b_imul,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_ecx)));
		case tkey::c_idiv:
			return (rsj4(b_mov,rskey(c_eax),rsoptr(c_comma),rsja)+
				zjiti::b_cdq()+
				rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_idiv,rskey(c_ecx),rsoptr(c_comma),"0")+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_eax)));
		case tkey::c_imod:
			return (rsj4(b_mov,rskey(c_eax),rsoptr(c_comma),rsja)+
				zjiti::b_cdq()+
				rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_idiv,rskey(c_ecx),rsoptr(c_comma),"0")+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_edx)));
		case tkey::c_udiv:
			return (rsj4(b_mov,rskey(c_eax),rsoptr(c_comma),rsja)+
				rsj4(b_xor,rskey(c_edx),rsoptr(c_comma),rskey(c_edx))+
				rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_udiv,rskey(c_ecx),rsoptr(c_comma),"0")+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_eax)));
		case tkey::c_umod:
			return (rsj4(b_mov,rskey(c_eax),rsoptr(c_comma),rsja)+
				rsj4(b_xor,rskey(c_edx),rsoptr(c_comma),rskey(c_edx))+
				rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_udiv,rskey(c_ecx),rsoptr(c_comma),"0")+
				rsj4(b_mov,rsja,rsoptr(c_comma),rskey(c_edx)));
		case tkey::c_cesb:
			return jcmp(sh,item,zjiti::b_sete_bl());
		case tkey::c_cnesb:
			return jcmp(sh,item,zjiti::b_setne_bl());
		case tkey::c_cgsb:
			return jcmp(sh,item,zjiti::b_setg_bl());
		case tkey::c_cgesb:
			return jcmp(sh,item,zjiti::b_setge_bl());
		case tkey::c_clsb:
			return jcmp(sh,item,zjiti::b_setl_bl());
		case tkey::c_clesb:
			return jcmp(sh,item,zjiti::b_setle_bl());
		case tkey::c_ucgsb:
			return jcmp(sh,item,zjiti::b_seta_bl());
		case tkey::c_ucgesb:
			return jcmp(sh,item,zjiti::b_setae_bl());
		case tkey::c_uclsb:
			return jcmp(sh,item,zjiti::b_setb_bl());
		case tkey::c_uclesb:
			return jcmp(sh,item,zjiti::b_setbe_bl());
		case tkey::c_band:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_and(ins);
			}
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_and,rsja,rsoptr(c_comma),rskey(c_ecx)));
		case tkey::c_bor:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_or(ins);
			}
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_or,rsja,rsoptr(c_comma),rskey(c_ecx)));
		case tkey::c_bxor:
			if(znasm::count_mbk_l(vstr)!=2)
			{
				return zjiti::b_xor(ins);
			}
			return (rsj4(b_mov,rskey(c_ecx),rsoptr(c_comma),rsjb)+
				rsj4(b_xor,rsja,rsoptr(c_comma),rskey(c_ecx)));
		case tkey::c_bnot:
			return zjiti::b_not(ins);
		}
#endif
		return zjiti::error();
	}

	static rstr jcmp(tsh& sh,tasm& item,const rstr& cont)
	{
		rstr s;
		if(znasm::count_mbk_l(item.vstr)==2)
		{
			s+=zjiti::b_mov(build_ins(sh,rskey(c_nop),rskey(c_ecx),
				rsoptr(c_comma),znasm::get_opnd2_v(item.vstr)));
			s+=zjiti::b_cmp(build_ins(sh,rskey(c_nop),
				znasm::get_opnd1_v(item.vstr),
				rsoptr(c_comma),rskey(c_ecx)));
		}
		else
		{
			s+=zjiti::b_cmp(item.ins);
		}
		s+=zjiti::b_mov(build_ins(sh,rskey(c_nop),rskey(c_ebx),rsoptr(c_comma),"0"));
		s+=cont;
		return r_move(s);
	}

	static rstr proc_calle(tsh& sh,tins& ins,uchar* start)
	{
#ifdef _WIN64
		char* name=(char*)(ins.first.val64());
#else
		char* name=(char*)(ins.first.val);
#endif
		void* addr=null;
		if(sh.dic_bind_func.find(name)!=null)
		{
			addr=sh.dic_bind_func[name];
		}
		if(addr==null)
		{
			//addr=zjitf::find_dll_full(name);
			return rstr();
		}
#ifdef _WIN64
		ins.first.val64()=(int64)addr;
#else
		ins.first.val=(int)addr;
#endif
		return zjiti::b_call(ins,start);
	}

	static rbool parse_asm(tsh& sh,tasm& item)
	{
		int type=sh.key.get_key_index(item.vstr.get_bottom());
		if(ybase::is_jmp_ins(type)&&item.vstr.count()==2&&
			!sh.key.is_asm_reg(item.vstr[1]))
		{
			item.ins.type=type*6;
			return true;
		}
		int i=zbin::find_comma(sh,item.vstr);
		int first;
		ifn(parse_opnd(sh,item,i-1,item.vstr.sub(1,i),item.ins.first,first))
		{
			return false;
		}
		int second;
		ifn(parse_opnd(sh,item,i+1,item.vstr.sub(i+1),item.ins.second,second))
		{
			return false;
		}
		item.ins.type=type*6;
		ifn(zbin::obtain_qrun_type(item.ins,first,second))
		{
			return false;
		}
		return true;
	}

	static rbool parse_opnd(tsh& sh,tasm& item,int index,const rbuf<rstr>& v,topnd& o,int& otype)
	{
		if(v.count()==7&&v[1]==rsoptr(c_addr))
		{
			tfunc* ptfi=znasm::find_call(sh,item);
			if(ptfi==null)
			{
				return false;
			}
			if(ptfi->is_function)
			{
				void* addr=null;
				if(sh.dic_bind_func.find("z_function")!=null)
				{
					addr=sh.dic_bind_func["z_function"];
				}
				otype=topnd::c_imme;
#ifdef _WIN64
				o.val64()=(int64)addr;
#else
				o.val=(uint)addr;
#endif
				return true;
			}
			ifn(compile_func_to_x86(sh,*ptfi,tenv()))
			{
				return false;
			}
			otype=topnd::c_imme;
#ifdef _WIN64
			o.val64()=(int64)(ptfi->code);
#else
			o.val=(uint)(ptfi->code);
#endif
			return true;
		}
		return zbin::parse_opnd(sh,item,index,v,o,otype);
	}

	static tins build_ins(tsh& sh,rstr s1,rstr s2,rstr s3,rstr s4)
	{
		tasm item;
		item.vstr+=s1;
		item.vstr+=s2;
		item.vstr+=s3;
		item.vstr+=s4;
		parse_asm(sh,item);
		return item.ins;
	}

	static tins build_ins(tsh& sh,rstr s1,rstr s2,rstr s3,rbuf<rstr> s4)
	{
		tasm item;
		item.vstr+=s1;
		item.vstr+=s2;
		item.vstr+=s3;
		item.vstr+=s4;
		parse_asm(sh,item);
		return item.ins;
	}

	static tins build_ins(tsh& sh,rstr s1,rbuf<rstr> s2,rstr s3,rstr s4)
	{
		tasm item;
		item.vstr+=s1;
		item.vstr+=s2;
		item.vstr+=s3;
		item.vstr+=s4;
		parse_asm(sh,item);
		return item.ins;
	}
};
