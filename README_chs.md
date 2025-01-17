### Rush语言 = Python的语法 + C++的高效 + Lisp的动态能力


##### 为什么需要它？

理由一：Rush支持类似Python的无花括号语法，并且兼容60%的C++语法，Rush自身可用标准C++编译器编译或者自我编译（自举）。

理由二：Rush既可编译运行又可解释运行，它以静态类型为主，没有GC，并且支持翻译为C++，可达到与C++接近的性能。

理由三：它以Lisp作为中间层，支持mixin、宏、元编程以及各种动态特性，并且同时支持call by name、call by need和call by value。

理由四：Rush的设计目标是简洁、快速、稳定，完全开源，它的源码结构比lua简单得多，但实现的功能不比lua少。

##### Rush的整体设计：（红色部分不属于Rush）

![github](https://github.com/roundsheep/rush/blob/gh-pages/doc/rush.png "github")

##### Rush编码风格1：（类似《算法导论》的伪代码）

```cpp
define ← =

void insertion_sort(rstr& a):
	for j ← 1; j<a.count; j ← j+1
		key ← a[j]
		i ← j-1
		while i>=0 && a[i]>key
			a[i+1] ← a[i]
			i ← i-1
		a[i+1] ← key
```

##### Rush编码风格2：（类似python的无花括号风格）

```cpp
bool next_permutation<T>(rbuf<T>& v):
	if v.count<=1  
		return false  
	next=v.count-1  
	for  
		temp=next  
		next--  
		if v[next]<v[temp]  
			mid=v.count-1  
			for !(v[next]<v[mid])  
				mid--  
			swap<T>(v[next],v[mid])  
			reverse<T>(v,temp)  
			return true  
		if next==0  
			reverse<T>(v,0)  
			return false
```

##### Rush编码风格3：（Lisp风格，S表达式用逗号分隔）

```cpp
void main():
	int a
	int b
	[int,=,[a,1]]
	[int,=,[b,2]]
	[rf,print,[[int,+,[a,b]]]]
```
	
##### Rush编码风格4：（这是标准C++语法，本段代码可用VC++、G++或Rush进行编译）

```cpp
static rbool proc_inherit(tsh& sh,tclass& tci,int level=0)
{
	if(level++>c_rs_deep)
		return false;
	if(tci.vfather.empty())
		return true;
	rbuf<tword> v;
	for(int i=0;i<tci.vfather.count();i++)
	{
		rstr cname=tci.vfather[i].vword.get(0).val;
		tclass* ptci=zfind::class_search(sh,cname);
		if(ptci==null)
		{
			ptci=zfind::classtl_search(sh,cname);
			if(ptci==null)
				return false;
		}
		if(!proc_inherit(sh,*ptci,level))
			return false;
		v+=ptci->vword;
	}
	v+=tci.vword;
	tci.vword=v;
	return true;
}
```

##### Rush编码风格5：（完全类型推导，结合静态类型的效率和动态类型的灵活）

```cpp
main():
	printl func(1,2)
	printl func(3.4,9.9)
	printl func('abc','efg')

func(a,b):
	c=a+b
	return c
```

##### Rush编码风格6：（汇编与类JS混合，兼容60%的JS语法，function中为动态类型）

```javascript
define var

void main(){
	mov eax,1
	add eax,2
	js_main(eax)
}
	
function js_main(num){
	putsl(num);
	
	var f=function(a){
		return function(b){
			return a+b;
		};
	};
	
	putsl(f(3)(4));
	
	var k=function(n,h){
		if(n<=1){
			return 1;
		}
		return h(n,k(n-1,h));
	};
	
	putsl(k(10,function(a,b){a+b}));
	putsl(k(10,function(a,b){a*b}));
}
```

##### Rush编码风格7：（Lisp风格，用中括号代替小括号）

```javascript
define lambda function

function test(){
	[= f [lambda [n]
		[cond [== n 0]
			0
			[+ n [f [- n 1]]]]
	]]
	[putsl [f 10]]
}
```

<br/>
##### 性能测试数据：

（Intel i5 3.3GHZ，使用命令 xxx -nasm ..\src\rush.cxx 自举解析约25000行C++生成250000行汇编，可以看到Rush与C++的比分是1/3，这是由于Rush本身使用了move语义和短字符串优化，但Rush本身又不支持这种优化，因此有理由相信真实的Rush运行性能可达到C++的1/1.5，与C++已经相当接近。同时可以看到Rush的编译速度非常快，不使用nasm的情况下可5秒完成自举）

|EXE name|EXE size|time consuming|PK C++|backend|
|:-------|:-------|:-------------|:-----|:------|
|rush|673 KB|4813 ms|1 -- 1|vs -O2 (this is C++'s speed)|
|rush_mingw|1049 KB|6203 ms|1 -- 1.3|g++ -O2|
|rush_vs|774 KB|14453 ms|1 -- 3|lisp to c++ with vs -O2|
|rush_nasm|762 KB|53579 ms|1 -- 11.1|lisp to nasm|
|rush_gpp|1679 KB|79000 ms|1 -- 16.4|lisp to c++ with g++ -O0|

<br/>
##### Rush支持多种运行方式，方法如下：

##### JIT：

1. cd到bin目录
2. 命令行敲入 rush -jit ..\src\example\test\1.rs

##### JIT（64位）：

1. cd到bin目录
2. 命令行敲入 rush64 -jit ..\src\example\test\1.rs

##### 解释运行：

1. cd到bin目录
2. 命令行敲入 rush ..\src\example\test\1.rs

##### 编译运行（以NASM为后端）：

1. cd到bin目录
2. 命令行敲入 rnasm ..\src\example\test\1.rs

##### 编译运行（以NASM为后端，64位）：

1. cd到bin目录
2. 命令行敲入 rnasm64 ..\src\example\test\1.rs

##### 编译运行（翻译为C++并以G++为后端）：

1. cd到bin目录
2. 命令行敲入 gpp ..\src\example\test\1.rs

##### 翻译为64位C++：

1. cd到bin目录
2. 命令行敲入 rush64 -gpp ..\src\example\64bit_test.rs
3. 将生成的src\example\test\64bit_test.cpp导入Visual Studio（或者使用64位的G++）
4. 选择x64
5. 按F7

##### 编译运行（翻译为ASM再翻译成C++，处于测试阶段）：

1. cd到bin目录
2. 命令行输入 rcpp ..\src\example\test\53.rs

##### JS或Lisp解释运行（处于测试阶段）：

1. cd到bin目录
2. 命令行输入 rush -jit ..\src\example\dynamic\js.rs

##### HTML5运行（翻译为JS，不需要emscripten，目前仅支持chrome，处于测试阶段）：

1. cd到bin目录
2. 命令行输入 rush -js ..\src\example\test\53.rs
3. 双击 ..\src\example\test\53.html

##### Mac OS 或 Ubuntu 解释运行（处于测试阶段）：

1. 确保安装了clang（3.5或以上）或g++（4.8或以上）
2. cd到bin目录
3. 命令行输入 g++ ../src/rush.cxx -o rush -w -m32 -std=c++11
4. 命令行敲入 ./rush ../src/example/test/50.rs

##### IOS：

1. cd到bin目录
2. 命令行敲入 rush -gpp ..\src\example\test\1.rs
3. 将生成的src\example\test\1.cpp和ext\mingw\gpp.h两个文件导入xcode
4. 根据需要修改main函数，注释掉windows.h头文件

##### Android：（绿色集成包，不依赖其他环境）

1. 确保编译环境为64位windows
2. 下载一键安卓工具包并解压到一个无空格无中文的路径（1.1G） http://pan.baidu.com/s/1c0oc3Ws
3. 点击create_proj.bat
4. 输入工程名如test，等待命令窗口结束
5. 点击proj\test\build_cpp.bat
6. 等待几分钟命令窗口出现“请按任意键继续”
7. 按回车键并等待命令窗口结束
8. 成功后会得到proj\test\proj.android\bin\xxx.apk
9. 根据需要将Rush翻译得到的CPP文件包含进proj\test\Classes\HelloWorldScene.cpp

##### 第三方IDE编辑代码：

1. 运行ext\ide\SciTE.exe
2. 点击File->Open
3. 选择src\example\test\1.rs，点击“打开”
4. 按F5运行程序（或者F7生成EXE）

<br/>
Visual Assist智能补全请看视频演示：

http://www.tudou.com/programs/view/40Ez3FuqE10/

<br/>

##### 重新编译Rush源码：

1. 确保安装了 VS2012 update4 或者 VS2013
2. 打开src\proj\rush.sln
3. 选择Release模式（因为JIT不支持Debug）
4. 按F7，成功后会生成bin\rush.exe

##### 自举（以NASM或G++为后端）：

1. 双击self_test.bat
2. 稍等几分钟后会生成bin\rush_nasm.exe和bin\rush_gpp.exe（实际上Rush完成自举只需要5秒，瓶颈在NASM，据说Chez Scheme自举也是5秒）
3. 注意自举后仅NASM模式和GPP模式可用

##### 调试：

1. cd到bin目录
2. 命令行敲入 rush -gpp ..\src\example\test\1.rs
3. 使用Visual Studio或gdb调试src\example\test\1.cpp

##### 自动测试example下所有例子：

1. 双击bin\example_test.bat

##### 平台支持：

|-|Win32|Win64|Linux32|Linux64(or Mac OS)|
|:-------|:-------|:-------------|:-----|:------|
|C++ -> Lisp|stable|stable|alpha|alpha|
|JS -> Lisp|alpha|todo|todo|todo|
|Python -> Lisp|only syntax|only syntax|only syntax|todo|
|Lisp -> Interpret|alpha|todo|todo|todo|
|Lisp -> ASM|stable|stable|beta|todo|
|Lisp -> C++|stable|stable|beta|todo|
|ASM -> NASM|stable|relative stable|todo|todo|
|ASM -> JIT|stable|relative stable|todo|todo|
|ASM -> Interpret|stable|todo|alpha|todo|
|ASM -> C++|alpha|todo|todo|todo|
|ASM -> JS|alpha|todo|alpha|todo|

##### 语言特性：

|Backend|Lisp Macro|C Macro|Closure|Eval|Reflect|Dynamic Type|GC|Template|Operator Overload|
|:-------|:-------|:-------|:--------|:-----|:------|:-------|:--------|:-----|:------|
|ASM -> NASM|no|yes|only static|no|no|no|no|yes|yes|
|ASM -> JIT|yes|yes|only static|yes|yes|no|no|yes|yes|
|ASM -> Interpret|no|yes|only static|no|yes|no|no|yes|yes|
|Lisp -> Interpret|yes|yes|yes|yes|yes|yes|yes|no|no|
|Lisp -> C++|no|yes|no|no|no|no|no|yes|yes|

<br/>


QQ交流群：34269848   

E-mail：287848066@qq.com
