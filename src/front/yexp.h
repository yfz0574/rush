﻿#pragma once

#include "ymatch.h"
#include "yfunctl.h"

//表达式标准化类，返回表达式的类型
struct yexp
{
	//处理函数名调用
	static rbool proc_func_call(tsh& sh,const tsent& src,tsent& outopnd,
		tfunc& tfi,int level,int& i,tenv env)
	{
		tclass* ptci=tfi.ptci;
		rstr fname=src.vword[i].val;
		tfunc* ptfi=yfind::find_func(*ptci,fname);
		if(ptfi==null)
		{
			ptci=sh.pmain;
			ifn(proc_call(sh,src,null,outopnd,tfi,level,i,ptci,env))
			{
				rserror();
				return false;
			}
			return true;
		}
		//函数重载不能友元和非友元共存，友元标志很重要。
		if(ptfi->is_friend)
		{
			ifn(proc_call(sh,src,null,outopnd,tfi,level,i,ptci,env))
			{
				rserror();
				return false;
			}
		}
		else
		{
			tsent first;
			first.type=ptci->name+rsoptr(c_addr);
			first.vword+=rskey(c_this);
			ifn(proc_call(sh,src,&first,outopnd,tfi,level,i,ptci,env))
			{
				rserror();
				return false;
			}
		}
		return true;
	}

	//处理[]函数指针直接调用
	static rbool proc_point_call(tsh& sh,const tsent& src,tsent& outopnd,
		tfunc& tfi,int level,int& i,tenv env)
	{
		int left=i+1;
		int right=ybase::find_symm_mbk(sh,src.vword,left);
		if(right>=src.vword.count())
		{
			rserror(src,"call miss ]");
			return false;
		}
		outopnd.type=src.vword[i].val;
		outopnd.vword+=rsoptr(c_mbk_l);
		outopnd.vword+=rskey(c_pcall);
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=outopnd.type;
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=rsoptr(c_mbk_l);
		
		rbuf<tsent> vsent;
		ybase::split_param(sh,vsent,src.vword.sub(left+1,right),src);
		if(vsent.empty())
		{
			rserror(src,"call miss func");
			return false;
		}
		for(int j=0;j<vsent.count();j++)
		{
			if(j>=2)
			{
				outopnd.vword+=rsoptr(c_comma);
			}
			ifn(proc_exp(sh,vsent[j],tfi,level,env))
			{
				return false;
			}
			if(j>=1)
			{
				outopnd.vword+=vsent[j].type;
			}
		}
		outopnd.vword+=rsoptr(c_mbk_r);
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=vsent[0].vword;
		outopnd.vword+=rsoptr(c_comma);

		outopnd.vword+=rsoptr(c_mbk_l);
		for(int j=1;j<vsent.count();j++)
		{
			if(j!=1)
			{
				outopnd.vword+=rsoptr(c_comma);
			}
			outopnd.vword+=r_move(vsent[j].vword);
		}
		outopnd.vword+=rsoptr(c_mbk_r);

		outopnd.vword+=rsoptr(c_mbk_r);
		i=right;
		return true;
	}

	static rbool proc_temp_var(tsh& sh,const tsent& src,tsent& outopnd,
		tfunc& tfi,int level,int& i,tenv env)
	{
		int left=i+1;
		int right=ybase::find_symm_sbk(sh,src.vword,left);
		if(right>=src.vword.count())
		{
			rserror(src,"class name miss (");
			return false;
		}
		rbuf<tsent> vsent;
		ybase::split_param(sh,vsent,src.vword.sub(left+1,right),src);
		ifn(vsent.empty())
		{
			for(int j=0;j<vsent.count();j++)
			{
				ifn(proc_exp(sh,vsent[j],tfi,level,env))
				{
					return false;
				}
			}
		}
		outopnd.type=src.vword.get(i).val;
		outopnd.vword+=rsoptr(c_mbk_l);
		outopnd.vword+=rskey(c_btemp);
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=outopnd.type;
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=rsoptr(c_mbk_l);
		for(int j=0;j<vsent.count();j++)
		{
			if(j!=0)
			{
				outopnd.vword+=rsoptr(c_comma);
			}
			outopnd.vword+=r_move(vsent[j].vword);
		}
		outopnd.vword+=rsoptr(c_mbk_r);
		outopnd.vword+=rsoptr(c_mbk_r);
		i=right;
		return true;
	}
	
	//处理类名直接调用
	static rbool proc_class_call(tsh& sh,const tsent& src,tsent& outopnd,
		tfunc& tfi,int level,int& i,tenv env)
	{
		if(src.vword.get(i+1).val!=rsoptr(c_dot))
		{
			rserror(src,"class name miss .");
			return false;
		}
		rstr cname=src.vword[i].val;
		tclass* ptci=yfind::find_class(sh,cname);
		if(ptci==null)
		{
			rserror(src,"can't find class name");
			return false;
		}
		i+=2;
		return proc_call(sh,src,null,outopnd,tfi,level,i,ptci,env);
	}

	static rbool proc_mbk(tsh& sh,const tsent& first,const tsent& second,
		tsent& outopnd,tfunc& tfi,int level,tenv env)
	{
		rbuf<tsent> vsent;
		ybase::split_param(sh,vsent,second.vword,first);
		for(int j=0;j<vsent.count();j++)
		{
			ifn(proc_exp(sh,vsent[j],tfi,level,env))
			{
				return false;
			}
		}
		vsent.push_front(first);
		tclass* ptci=yfind::find_class_t(sh,first.type);
		if(ptci==null)
		{
			rserror(first,"can't find func mbk");
			return false;
		}
		tfunc* ptfi=ymatch::find_and_replace(sh,*ptci,"[]",vsent);
		if(null==ptfi)
		{
			rserror(first,"can't find func mbk");
			return false;
		}
		set_func(sh,outopnd,vsent,ptfi);
		return true;
	}

	static rbool proc_lisp(tsh& sh,const tsent& src,tsent& outopnd,
		tfunc& tfi,int level,tenv env)
	{
		if(src.vword.get_bottom().val!=rsoptr(c_mbk_l)&&
			src.vword.get_top().val!=rsoptr(c_mbk_r))
		{
			rserror(src,"lisp exp miss");
			return false;
		}
		outopnd.vword.clear();
		outopnd.pos=src.pos;
		tclass* ptci=null;
		tfunc* ptfi=null;
		rstr cname=src.vword.get(1).val;
		rbuf<rbuf<tword> > vlisp;
		get_vlisp(sh,src.vword,vlisp);
		rstr fname=rstr::join<rstr>(ybase::trans_vword_to_vstr(vlisp.get(1)),"");
		ptci=yfind::find_class(sh,cname);
		if(ptci!=null)
		{
			if(vlisp.count()!=3)
			{
				rserror(src,"lisp exp miss");
				return false;
			}
			ptfi=yfind::find_func_dec(*ptci,fname);
			rbuf<tsent> vsent;
			tsent sent;
			sent.pos=src.pos;

			rbuf<rbuf<tword> > vparam;
			get_vlisp(sh,vlisp[2],vparam);
			for(int j=0;j<vparam.count();j++)
			{
				sent.vword=vparam[j];
				ifn(proc_exp(sh,sent,tfi,level,env))
				{
					return false;
				}
				vsent.push(sent);
			}

			if(ptfi==null)
			{
				ptfi=ymatch::find_and_replace(sh,*ptci,fname,vsent);
				if(ptfi==null)
				{
					rserror(src,"lisp exp miss");
					return false;
				}
			}
			set_func(sh,outopnd,vsent,ptfi);
			return true;
		}
		if(cname==rskey(c_pcall)||cname==rskey(c_btemp))
		{
			outopnd.vword=src.vword;
			outopnd.type=fname;
			return true;
		}
		if(cname==rsoptr(c_dot))
		{
			tsent sent;
			if(vlisp.count()!=3)
			{
				rserror(src,"lisp exp miss");
				return false;
			}
			sent.vword=vlisp[1];
			sent.pos=src.pos;
			ifn(proc_exp(sh,sent,tfi,level,env))
			{
				return false;
			}
			ptci=yfind::find_class_t(sh,sent.type);
			if(ptci==null)
			{
				rserror(src,"lisp exp miss");
				return false;
			}
			tdata* ptdi=yfind::find_data_member(
				*ptci,vlisp[2].get(0).val);
			if(ptdi==null)
			{
				rserror(src,"lisp exp miss");
				return false;
			}
			outopnd.vword=src.vword;
			outopnd.type=ptdi->type;
			return true;
		}
		if(cname==rsoptr(c_addr))
		{
			outopnd.vword=src.vword;
			outopnd.type="rp<void>";
			return true;
		}
		rserror(src,"lisp exp miss");
		return false;
	}

	static void get_vsent(const rbuf<rbuf<tword> >& vlisp,rbuf<tsent>& vsent,
		const tsent& src)
	{
		vsent.clear();
		for(int i=0;i<vlisp.count();i++)
		{
			tsent sent;
			sent.pos=src.pos;
			sent.vword=vlisp[i];
			vsent.push(r_move(sent));
		}
	}

	static void get_vlisp(const tsh& sh,rbuf<tword> v,rbuf<rbuf<tword> >& vlisp)
	{
		vlisp=ybase::split_comma<tword>(sh,v.sub(1,v.count()-1));
	}
	
	//处理变参中括号调用或C变参，变参不能重载
	static rbool proc_vararg(tsh& sh,const tsent& src,const tsent* pfirst,tsent& outopnd,
		tfunc& tfi,int level,int& i,const tclass* ptci,tenv env)
	{
		rstr cname=ptci->name;
		rstr fname=src.vword[i].val;
		int left=i+1;
		int right;
		if(src.vword.get(left).val==rsoptr(c_sbk_l))
		{
			right=ybase::find_symm_sbk(sh,src.vword,left);
		}
		else
		{
			right=ybase::find_symm_mbk(sh,src.vword,left);
		}
		if(right>=src.vword.count())
		{
			rserror(src,"call miss )");
			return false;
		}
		tfunc* ptfi=yfind::find_func(*ptci,fname);
		if(null==ptfi)
		{
			rserror(src,"p_mbk_param can't find call");
			return false;
		}
		rbuf<tsent> vsent;
		ybase::split_param(sh,vsent,src.vword.sub(left+1,right),src);
		for(int j=0;j<vsent.count();j++)
		{
			ifn(proc_exp(sh,vsent[j],tfi,level,env))
			{
				return false;
			}
		}
		tsent tmp;
		tmp.pos=src.pos;
		tmp.vword.push(tword(rstr(vsent.count())));
		tmp.type=rskey(c_int);
		vsent.push_front(tmp);
		if(ptfi->is_function)
		{
			tmp.vword.clear();
			tmp.vword+=rstr((uint)ptfi)+"p";
			tmp.type="rp<void>";
			vsent.push_front(tmp);
		}
		//插入first还未处理引用的情况
		if(pfirst!=null)
		{
			vsent.push_front(*pfirst);//插入DOT前的对象
			ifn(ybase::is_point(pfirst->type))
			{
				vsent[0].vword.push_front(tword(rsoptr(c_addr)));
				ifn(proc_exp(sh,vsent[0],tfi,level,env))
				{
					return false;
				}
			}
		}
		tmp.vword.clear();
		tmp.vword+=tword(rsoptr(c_mbk_l));
		tmp.vword+=tword(rsoptr(c_addr));
		tmp.vword+=rsoptr(c_comma);
		tmp.vword+=tword(ptci->name);
		tmp.vword+=rsoptr(c_comma);
		tmp.vword+=tword(ptfi->name_dec);
		tmp.vword+=tword(rsoptr(c_mbk_r));

		outopnd.type=ptfi->retval.type;
		outopnd.vword+=tword(rsoptr(c_mbk_l));
		outopnd.vword+=tword(rskey(c_pcall));
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=tword(outopnd.type);
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=tword(rsoptr(c_mbk_l));
		for(int j=0;j<vsent.count();j++)
		{
			if(j!=0)
			{
				outopnd.vword+=rsoptr(c_comma);
			}
			outopnd.vword+=vsent[j].type;
		}
		outopnd.vword+=tword(rsoptr(c_mbk_r));
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=tmp.vword;
		outopnd.vword+=rsoptr(c_comma);

		outopnd.vword+=tword(rsoptr(c_mbk_l));
		for(int j=0;j<vsent.count();j++)
		{
			if(j!=0)
			{
				outopnd.vword+=rsoptr(c_comma);
			}
			outopnd.vword+=r_move(vsent[j].vword);
		}
		outopnd.vword+=tword(rsoptr(c_mbk_r));

		outopnd.vword+=tword(rsoptr(c_mbk_r));
		i=right;
		return true;
	}
	
	//无括号call
	static rbool proc_call_n(tsh& sh,const tsent& src,const tsent* pfirst,tsent& outopnd,
		tfunc& tfi,int level,int& i,const tclass* ptci,tenv env)
	{
		rstr cname=ptci->name;
		rstr fname=src.vword[i].val;
		tfunc* ptfi;
		int left=i;
		int right=left+1;
		if(i+1==src.vword.count()-1||
			yfind::is_end_part_optr(sh,src.vword.get(i+1).val))
		{
			ptfi=yfind::find_func_empty_param_m(
				*ptci,fname,pfirst==null);
			if(ptfi==null)
			{
				rserror(src,"p_call_n can't find call");
				return false;
			}
		}
		else
		{
			ptfi=yfind::find_func_multi_param_m(
				*ptci,fname,pfirst==null);
			if(ptfi==null)
			{
				ptfi=yfind::find_func_empty_param_m(
					*ptci,fname,pfirst==null);
			}
			else
			{
				right=src.vword.count()-1;
			}
			if(ptfi==null)
			{
				rserror(src,"p_call_n2 can't find call");
				return false;
			}
		}
		tsent param=src.sub(left+1,right);
		outopnd.vword.push(tword(cname));
		outopnd.vword.push(tword(rsoptr(c_dot)));
		outopnd.vword.push(tword(fname));
		outopnd.vword.push(tword(rsoptr(c_sbk_l)));
		if(pfirst!=null&&!ptfi->is_friend)
		{
			outopnd.vword+=pfirst->vword;
			ifn(param.empty())
			{
				outopnd.vword.push(tword(rsoptr(c_comma)));
				outopnd.vword+=r_move(param.vword);
			}
		}
		else
		{
			ifn(param.empty())
			{
				outopnd.vword+=r_move(param.vword);
			}
		}
		outopnd.vword.push(tword(rsoptr(c_sbk_r)));
		//无括号调用添加括号递归处理
		ifn(proc_exp(sh,outopnd,tfi,level,env))
		{
			return false;
		}
		i=right-1;
		return true;
	}

	static rbool proc_ftl(tsh& sh,tsent src,const tsent* pfirst,tsent& outopnd,
		tfunc& tfi,int level,int& i,tclass* ptci,tenv env)
	{
		int left=i+1;
		if(src.vword.get(left)!=rsoptr(c_tbk_l))
		{
			rserror(src,"call miss <");
			return false;
		}
		int right=ybase::find_symm_tbk(sh,src.vword,left);
		if(right>=src.vword.count())
		{
			rserror(src,"call miss >");
			return false;
		}
		rbuf<tword> temp=src.vword.sub(i,right+1);
		ifn(yfunctl::replace_ftl(sh,*tfi.ptci,temp,ptci))
		{
			return false;
		}
		src.vword[right]=temp.get_bottom();
		i=right;
		return proc_call(sh,src,pfirst,outopnd,tfi,level,i,ptci,env);
	}

	static rbool set_vsent(const tsh& sh,tpos pos,rbuf<tsent>& vsent,
		tfunc& tfi)
	{
		tsent sent;
		tword word;
		word.pos=pos;
		sent.pos=pos;
		tdynamic item;
		item.key=tfi.s_dynamic.count();
		for(int k=0;k<vsent.count();k++)
		{
			item.vparam+=ybase::trans_vword_to_vstr(vsent[k].vword);
		}
		tfi.s_dynamic.insert(item);

		vsent.clear();

		ybase::push_twi(sent.vword,word,rstr((uint)(&tfi.s_dynamic.find(item)->vparam)));
		vsent.push(sent);

		sent.vword.clear();
		ybase::push_twi(sent.vword,word,rstr((uint)(&tfi)));
		vsent.push(sent);

		sent.vword.clear();
		ybase::push_twi(sent.vword,word,rstr("_EBP"));
		vsent.push(sent);

		return true;
	}

	//todo: process this
	static rbool proc_infer(tsh& sh,const tsent& src,const tsent* pfirst,tsent& outopnd,
		tfunc& tfi,int level,int& i,tclass* ptci,tfunc* ptfi,tenv env)
	{
		extern rbool r_ysent_proc(tsh& sh,tfunc& tfi,tenv env);
		rstr fname=src.vword[i].val;
		int left=i+1;
		int right;
		right=ybase::find_symm_sbk(sh,src.vword,left);
		if(right>=src.vword.count())
		{
			rserror(src,"call miss )");
			return false;
		}
		rbuf<tsent> vsent;
		ybase::split_param(sh,vsent,src.vword.sub(left+1,right),src);
		for(int j=0;j<vsent.count();j++)
		{
			ifn(proc_exp(sh,vsent[j],tfi,level,env))
			{
				return false;
			}
		}
		if(pfirst!=null&&vsent.count()!=ptfi->param.count())
		{
			vsent.push_front(*pfirst);//插入DOT前的对象
		}
		if(vsent.count()!=ptfi->param.count())
		{
			rserror();
			return false;
		}
		tfunc temp=*ptfi;
		for(int k=0;k<vsent.count();k++)
		{
			temp.param[k].type=vsent[k].type;
		}
		temp.name_dec=temp.get_dec();
		ymemb::obtain_size_func(sh,temp);
		ptci->s_func.insert_c(temp);
		ptfi=ymatch::find_and_replace(sh,*ptci,fname,vsent);
		if(null==ptfi)
		{
			rserror(src,"p_call can't find call");
			return false;
		}
		ifn(r_ysent_proc(sh,*ptfi,env))
		{
			return false;
		}
		set_func(sh,outopnd,vsent,ptfi);
		i=right;
		return true;
	}

	static rbool proc_call(tsh& sh,const tsent& src,const tsent* pfirst,tsent& outopnd,
		tfunc& tfi,int level,int& i,tclass* ptci,tenv env)
	{
		rstr cname=ptci->name;
		rstr fname=src.vword[i].val;
		tfunc* ptfi=yfind::find_func_infer(*ptci,fname);
		if(ptfi!=null)
		{
			return proc_infer(sh,src,pfirst,outopnd,tfi,level,i,ptci,ptfi,env); 
		}
		ptfi=yfind::find_func(*ptci,fname);
		if(ptfi!=null)
		{
			if(ptfi->is_vararg&&src.vword.get(i+1).val==rsoptr(c_sbk_l)||
				src.vword.get(i+1).val==rsoptr(c_mbk_l)||
				ptfi->is_function&&src.vword.get(i+1).val==rsoptr(c_sbk_l))
			{
				return proc_vararg(sh,src,pfirst,outopnd,tfi,level,i,ptci,env);
			}
		}
		if(pfirst!=null&&ybase::get_tname(pfirst->type)==rskey(c_var)&&
			null==yfind::find_func(*ptci,fname))
		{
			return false;
			//a.test  ->   a["test"]
		}
		if(src.vword.get(i+1).val!=rsoptr(c_sbk_l))
		{
			return proc_call_n(sh,src,pfirst,outopnd,tfi,level,i,ptci,env);
		}
		int left=i+1;
		int right;
		right=ybase::find_symm_sbk(sh,src.vword,left);
		if(right>=src.vword.count())
		{
			rserror(src,"call miss )");
			return false;
		}
		rbuf<tsent> vsent;
		ybase::split_param(sh,vsent,src.vword.sub(left+1,right),src);
		for(int j=0;j<vsent.count();j++)
		{
			ifn(proc_exp(sh,vsent[j],tfi,level,env))
			{
				return false;
			}
		}
		if(ptfi!=null&&ptfi->is_dynamic)
		{
			ifn(set_vsent(sh,src.vword[i].pos,vsent,tfi))
			{
				return false;
			}
			if(pfirst!=null)
			{
				vsent.push_front(*pfirst);
			}
			set_func(sh,outopnd,vsent,ptfi);
			i=right;
			return true;
		}
		if(pfirst!=null)
		{
			vsent.push_front(*pfirst);//插入DOT前的对象
		}
		ptfi=ymatch::find_and_replace(sh,*ptci,fname,vsent);
		if(null==ptfi)
		{
			if(pfirst==null)
			{
				rserror(src,"p_call can't find call");
				return false;
			}
			vsent.pop_front();
			ptfi=ymatch::find_and_replace(sh,*ptci,fname,vsent);
			if(ptfi==null||!ptfi->is_friend)
			{
				rserror(src,"p_call can't find call");
				return false;
			}
		}
		set_func(sh,outopnd,vsent,ptfi);
		i=right;
		return true;
	}

	static rbool proc_exp_all(tsh& sh,tfunc& tfi,tenv env)
	{
		for(int i=0;i<tfi.vsent.count();++i)
		{
			if(sh.key.is_asm_ins(tfi.vsent[i].vword.get_bottom().val))
			{
				continue;
			}
			if(ybase::is_tag<tword>(tfi.vsent[i].vword))
			{
				continue;
			}
			ifn(yexp::proc_exp(sh,tfi.vsent[i],tfi,0,env))
			{
				return false;
			}
		}
		return true;
	}
	
	//表达式标准化，并设置dst.type
	static rbool proc_exp(tsh& sh,tsent& src,tfunc& tfi,int level,tenv env)
	{
		tclass& tci=*tfi.ptci;
		if(level>c_rs_deep)
		{
			rserror(src,"expression too deep");
			return false;
		}
		level++;
		rbuf<rstr> soptr;
		rbuf<tsent> sopnd;
		soptr.push(rsoptr(c_sharp_sharp));
		src.vword.push(tword(rsoptr(c_sharp_sharp)));
		for(int i=0;i<src.vword.count();++i)
		{
			if(src.vword[i].val==rsoptr(c_sharp_sharp)&&
				soptr.get_top()==rsoptr(c_sharp_sharp))
			{
				break;
			}
			if(src.vword[i].is_const())
			{
				tsent outopnd;
				outopnd.pos=src.pos;
				outopnd.vword.push(src.vword[i]);
				outopnd.type=ybase::get_ctype(sh,src.vword[i]);
				sopnd.push(r_move(outopnd));
			}
			elif(sh.key.is_asm_reg(src.vword[i].val))
			{
				tsent outopnd;
				outopnd.pos=src.pos;
				outopnd.vword.push(src.vword[i]);
				outopnd.type=rskey(c_int);
				sopnd.push(r_move(outopnd));
			}
			elif(src.vword[i].val==rsoptr(c_sbk_l))
			{
				//todo 小括号重载有点复杂 
				int right=ybase::find_symm_sbk(sh,src.vword,i);
				if(right>=src.vword.count())
				{
					rserror(src,"miss )");
					return false;
				}
				tsent outopnd=src.sub(i+1,right);
				ifn(proc_exp(sh,outopnd,tfi,level,env))
				{
					return false;
				}
				sopnd.push(r_move(outopnd));
				i=right;
			}
			elif(src.vword.get(i+1)==rsoptr(c_mbk_l)&&
				yfind::is_class_t(sh,src.vword[i].val))
			{
				//[]函数指针调用
				tsent outopnd;
				outopnd.pos=src.pos;
				ifn(proc_point_call(sh,src,outopnd,tfi,level,i,env))
				{
					return false;
				}
				sopnd.push(r_move(outopnd));
			}
			elif(src.vword[i].val==rsoptr(c_mbk_l))
			{
				int right=ybase::find_symm_mbk(sh,src.vword,i);
				if(right>=src.vword.count())
				{
					rserror(src,"miss ]");
					return false;
				}
				tsent outopnd;
				outopnd.pos=src.pos;
				if(sopnd.empty()||
					src.vword.get(i+1).val==rsoptr(c_addr)&&
					yfind::is_class(sh,src.vword.get(i+3).val))
				{
					ifn(proc_lisp(sh,src.sub(
						i,right+1),outopnd,tfi,level,env))
					{
						return false;
					}
				}
				else
				{
					ifn(proc_mbk(sh,sopnd.pop(),src.sub(i+1,right),
						outopnd,tfi,level,env))
					{
						return false;
					}
				}
				sopnd.push(r_move(outopnd));
				i=right;
			}
			elif(src.vword[i].val==rsoptr(c_dot))
			{
				if(sopnd.empty())
				{
					rserror(src,". miss opnd");
					return false;
				}
				i++;
				rstr name=src.vword.get(i).val;
				tsent first=sopnd.pop();
				tclass* ptci=yfind::find_class_t(sh,first.type);
				if(null==ptci)
				{
					rserror(src,". can't find class name");
					return false;
				}
				tsent outopnd;
				outopnd.pos=src.pos;
				//数据成员和函数成员不可以同名
				tdata* ptdi=yfind::find_data_member(*ptci,name);
				if(ptdi!=null)
				{
					outopnd.type=ptdi->type;
					outopnd.vword+=tword(rsoptr(c_mbk_l));
					outopnd.vword+=tword(rsoptr(c_dot));
					outopnd.vword+=rsoptr(c_comma);
					outopnd.vword+=first.vword;
					outopnd.vword+=rsoptr(c_comma);
					outopnd.vword+=tword(name);
					outopnd.vword+=tword(rsoptr(c_mbk_r));
					sopnd.push(r_move(outopnd));
					continue;
				}
				if(src.vword.get(i+1)==rsoptr(c_tbk_l)&&
					yfind::find_ftl(*ptci,name)!=null)
				{
					ifn(proc_ftl(sh,src,&first,
						outopnd,tfi,level,i,ptci,env))
					{
						return false;
					}
				}
				else
				{
					ifn(proc_call(sh,src,&first,
						outopnd,tfi,level,i,ptci,env))
					{
						return false;
					}
				}
				sopnd.push(r_move(outopnd));
			}
			elif(src.vword[i].val==rsoptr(c_arrow_r))
			{
				if(sopnd.empty())
				{
					rserror(src,". miss opnd");
					return false;
				}
				tsent first=sopnd.pop();
				first.vword.push_front(tword(rsoptr(c_mul)));
				ifn(proc_exp(sh,first,tfi,level,env))
				{
					return false;
				}
				sopnd.push(r_move(first));
				src.vword[i].val=rsoptr(c_dot);
				i--;
			}
			elif(sh.optr.is_optr(src.vword[i].val))
			{
				if(soptr.empty())
				{
					rserror(src,"exp miss optr");
					return false;
				}
				rstr cur=src.vword[i].val;
				ifn(sh.optr.is_precede(soptr.top(),cur))
				{
					soptr.push(cur);
					continue;
				}
				rstr theta=soptr.pop();
				if(sopnd.empty())
				{
					rserror(src,"exp miss opnd");
					return false;
				}
				rbuf<tsent> vsent;
				vsent.push(sopnd.pop());
				tclass* ptci=yfind::find_class_t(sh,vsent[0].type);
				tfunc* ptfi;
				tsent outopnd;
				outopnd.pos=src.pos;
				if(ptci!=null)
				{
					ptfi=ymatch::find_and_replace(sh,
						*ptci,theta,vsent);
					if(ptfi!=null)
					{
						set_func(sh,outopnd,
							vsent,ptfi);
						sopnd.push(r_move(outopnd));
						i--;
						continue;
					}
				}
				if(sopnd.empty())
				{
					rserror(src,"exp miss opnd");
					return false;
				}
				vsent.push_front(sopnd.pop());
				ptci=yfind::find_class_t(sh,vsent[0].type);
				if(ptci!=null)
				{
					ptfi=ymatch::find_and_replace(sh,
						*ptci,theta,vsent);
					if(ptfi!=null)
					{
						set_func(sh,outopnd,
							vsent,ptfi);
						sopnd.push(r_move(outopnd));
						i--;
						continue;
					}
				}
				ptci=yfind::find_class_t(sh,vsent[1].type);
				if(ptci!=null)
				{
					ptfi=ymatch::find_and_replace(sh,
						*ptci,theta,vsent);
					if(ptfi!=null)
					{
						set_func(sh,outopnd,
							vsent,ptfi);
						sopnd.push(r_move(outopnd));
						i--;
						continue;
					}
				}
				rserror(src,"exp miss opnd 2");
				return false;
			}
			elif(yfind::is_class(sh,src.vword[i].val))
			{
				tsent outopnd;
				outopnd.pos=src.pos;
				if(src.vword.get(i+1).val==rsoptr(c_sbk_l))
				{
					//临时变量
					ifn(proc_temp_var(sh,src,outopnd,tfi,level,i,env))
					{
						return false;
					}
				}
				else
				{
					//类名直接调用
					ifn(proc_class_call(sh,src,outopnd,tfi,level,i,env))
					{
						return false;
					}
				}
				sopnd.push(r_move(outopnd));
			}
			else
			{
				rstr name=src.vword[i].val;
				tsent outopnd;
				outopnd.pos=src.pos;
				tdata* ptdi=yfind::find_local(tfi,name);
				if(ptdi!=null)
				{
					outopnd.vword+=src.vword[i];
					outopnd.type=ptdi->type;
					sopnd.push(r_move(outopnd));
					continue;
				}
				ifn(tci.is_friend)
				{
					ptdi=yfind::find_data_member(tci,name);
					if(ptdi!=null)
					{
						//类成员变量增加this
						//如 m_a -> this.m_a 
						outopnd.vword+=tword(rsoptr(c_mbk_l));
						outopnd.vword+=tword(rsoptr(c_dot));
						outopnd.vword+=rsoptr(c_comma);
						outopnd.vword+=tword(rskey(c_this));
						outopnd.vword+=rsoptr(c_comma);
						outopnd.vword+=src.vword[i];
						outopnd.vword+=tword(rsoptr(c_mbk_r));
						outopnd.type=ptdi->type;
						sopnd.push(r_move(outopnd));
						continue;
					}
				}
				ptdi=yfind::find_data_member(*sh.pmain,name);
				if(ptdi!=null)
				{
					//全局变量
					outopnd.vword+=tword(rsoptr(c_mbk_l));
					outopnd.vword+=tword(rsoptr(c_dot));
					outopnd.vword+=rsoptr(c_comma);
					outopnd.vword+=tword(rskey(c_pmain));
					outopnd.vword+=rsoptr(c_comma);
					outopnd.vword+=src.vword[i];
					outopnd.vword+=tword(rsoptr(c_mbk_r));
					outopnd.type=ptdi->type;
					sopnd.push(r_move(outopnd));
					continue;
				}
				if(env.ptfi!=null)
				{
					ptdi=yfind::find_local(*env.ptfi,name);
					if(ptdi!=null)
					{
						//环境变量
						outopnd.vword+=tword(rsoptr(c_mbk_l));
						outopnd.vword+=tword(rsoptr(c_dot));
						outopnd.vword+=rsoptr(c_comma);
						outopnd.vword+=tword(rskey(c_penv));
						outopnd.vword+=rsoptr(c_comma);
						outopnd.vword+=src.vword[i];
						outopnd.vword+=tword(rsoptr(c_mbk_r));
						outopnd.type=ptdi->type;
						sopnd.push(r_move(outopnd));
						continue;
					}
				}
				ifn(proc_func_call(sh,src,outopnd,tfi,level,i,env))
				{
					rserror();
					return false;
				}
				sopnd.push(r_move(outopnd));
			}
		}
		if(sopnd.count()!=1)
		{
			ybase::print_vword(src.vword);
			rserror(src,"expression error");
			return false;
		}
		src=r_move(sopnd[0]);
		if(src.vword.empty()||src.type.empty())
		{
			rserror(src,"expression error");
			return false;
		}
		return true;
	}

	static void set_func(const tsh& sh,tsent& outopnd,const rbuf<tsent>& vsent,const tfunc* ptfi)
	{
		const tclass* ptci=ptfi->ptci;
		outopnd.type=ptfi->retval.type;
		outopnd.vword+=tword(rsoptr(c_mbk_l));
		outopnd.vword+=tword(ptci->name);
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=tword(ptfi->name_dec);
		outopnd.vword+=rsoptr(c_comma);
		outopnd.vword+=tword(rsoptr(c_mbk_l));
		for(int i=0;i<vsent.count();i++)
		{
			if(i!=0)
			{
				outopnd.vword+=rsoptr(c_comma);
			}
			outopnd.vword+=r_move(vsent[i].vword);
		}
		outopnd.vword+=tword(rsoptr(c_mbk_r));
		outopnd.vword+=tword(rsoptr(c_mbk_r));
	}
};
