/*
本程序适用于MAC和Linux下，本程序默认通过读取rom.conf和ads.conf文件来配置
目录:
ads    # 放置广告包, 如ads/gg, ads/nc
base   # 放置机型底包, 如base/zte/n986, base/xiaomi/2a
over   # 放在里面的文件都会copy到system里面, 一般放sb
rom.conf # 内容: zte_n986 = true (代表base/zte/n986)
ads.conf # 内容: gg = true (代表ads/gg)
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statvfs.h> // mac vfs头文件
//#include <sys/vfs.h> // Linux vfs头文件

#include "simg2img/simg2img.h" // Simg2img
#include "make_ext4fs/make_ext4fs_main.h" // Make_Ext4fs

// 全局变量
char filename_tmp[200]={0};

// 定义函数原型
int usage();                                                /* 该程序使用方法 */
int error_put(char *str);                                   /* 输出错误到屏幕 */
char *dir_file(char *dir, char *file);                      /* 合并字符串 */
char *DelSpace(char *in);                                   /* 删除配置文件的空格 */
int del_file(char *filename);                               /* 删除文件函数 */
int copy_file(char *filename, char *out_file);              /* 复制文件函数 */
int normal(char *rom_path, char *ads_path, char *out);      /* 默认打包函数 */
int build_rom(char *rom, char *ads, char *out);             /* 编译rom函数 */

int main(int argc, char **argv)
{
	char ads_tmp[100]={0}, rom_tmp[100]={0}, rk[50]={0}, rv[50]={0}, ak[50]={0}, av[50]={0};
	char *ads_key=0, *ads_val=0, *rom_key=0, *rom_val=0, *out_dir=0, *ads_config=0, *rom_config=0;
	argc-=1;argv+=1;

	// 默认的配置文件和输出目录
	ads_config="ads.conf";
	rom_config="rom.conf";
	out_dir="out";

	// 从命令行的参数中获取配置文件信息
	while(argc > 0)
	{
		char *arg = argv[0];
		char *val = argv[1];

		if(argc < 2) return usage();
		argc -= 2;
		argv += 2;

		if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
			out_dir = val;
		} else if(!strcmp(arg, "--adsconfig")) {
			ads_config = val;
		} else if(!strcmp(arg, "--romconfig")) {
			rom_config = val;
		} else {
			return usage();
		}
	}

	// 判断广告包目录和rom包目录是否存在
	if(!access(ads_config, 0) && !access(rom_config, 0)) {
		FILE* ads_info = fopen(ads_config, "r");
		FILE* rom_info = fopen(rom_config, "r");

		if(ads_info == NULL) error_put("Can't Open Configure file.");
		if(rom_info == NULL) error_put("Can't Open Configure file.");

		// 获取配置文件里的信息
		while(fgets(rom_tmp, 100, rom_info))
		{
			if(rom_tmp[0] == '#' ) continue;
			if(rom_tmp[0] == '\n') continue;

			if(strstr(rom_tmp,"=")) sscanf(rom_tmp, "%[0-9a-zA-Z_\t ]=%s", rk, rv);

			rom_key = DelSpace(rk);
			rom_val = DelSpace(rv);

			if(strcmp(rom_val, "true")) continue;

			while(fgets(ads_tmp, 100, ads_info))
			{
				if(ads_tmp[0] == '#' ) continue;
				if(ads_tmp[0] == '\n') continue;

				if(strstr(ads_tmp,"=")) sscanf(ads_tmp, "%[0-9a-zA-Z_\t ]=%s", ak, av);

				ads_key = DelSpace(ak);
				ads_val = DelSpace(av);

				if(strcmp(ads_val, "true")) continue;
				
				// 开始编译rom
				if(build_rom(rom_key, ads_key, out_dir)) {
					fprintf(stderr, "\nFailed to build Rom.\n"
							"Wait 5s to continue\n");
					sleep(5);
				}
			}

			fseek(ads_info,0,0);
		}

		fclose(ads_info);
		fclose(rom_info);
	} else error_put("Configure not found.");

	return 0;
}

int usage()
{
	fprintf(stderr, "Usage: amrom (Ud3v0id@gmail.com)\n"
			"\t--adsconfig <filename>\n"
			"\t--romconfig <filename>\n"
			"\t-o|--output <directory>\n"
			);
	return 1;
}

// 输出传递进来的字符串并退出程序
int error_put(char *str)
{
	fprintf(stderr, "\nError: %s\n",str);
	return -1;
}

// 合并两个字符串
char *dir_file(char *dir, char *file)
{
	char *p = filename_tmp;
	char *out = p;

	for(p--;p++ && *dir != '\0';dir++) *p=*dir;
	for(*p='/';p++ && *file != '\0';file++) *p=*file;
	*p='\0';

	return out;
}

// 删除配置文件获取后多余的空格
char *DelSpace(char *in)
{
	char *out = 0;
	char *p = in;

	// 去掉开头空格字符
	while((*p == ' ')||(*p == '\t')) p++;

	out = p;

	// 遇到结尾空格或者换行符退出
	while(1)
	{
		if(*p == ' ')  break;
		if(*p == '\n') break;
		if(*p == '\0') break;
		if(*p == '\t') break;

		p++;
	}

	*p = '\0';

	return out;
}

// 删除文件函数: 通过递归来删除非空文件夹里的文件
int del_file(char *filename)
{
	char dir_name[250]={0};
	struct stat st;
	stat(filename, &st);

	// 判断文件或文件夹是否存在
	if(access(filename, 0)) return 0;

	// 如果不是文件夹则直接unlink文件
	if(S_IFDIR & st.st_mode) {
		DIR *dir = opendir(filename);
		struct dirent *ptr;

		// 循环读取文件夹内的文件名
		while((ptr = readdir(dir)) != NULL)	{
			if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))  continue;

			sprintf(dir_name, "%s/%s", filename, ptr->d_name);

			// 如果rmdir失败则递归
			if(rmdir(dir_name))	del_file(dir_name);
			rmdir(dir_name);
		}
		closedir(dir);
	} else return unlink(filename);
	
	if(!access(filename, 0)) rmdir(filename);
	return 0;
}

// 复制文件函数: 通过递归来复制文件夹内的文件
int copy_file(char *filename, char *out_file)
{
	int in_filename, out_filename;
	char *p = 0, *f = 0;
	char  buf[4096];
	char in_name[250]={0}, out_name[250]={0};
	struct stat st;
	stat(filename, &st);

	// 判断输入文件是否存在
	if(access(filename, 0)) return error_put("Copy file not found!");

	// 如果是文件夹则创建输出目录相应文件夹并递归
	if(S_IFDIR & st.st_mode) {
		DIR *dir = opendir(filename);
		struct dirent *ptr;

		while((ptr = readdir(dir)) != NULL) {
			if(!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))  continue;

			sprintf(in_name, "%s/%s", filename, ptr->d_name);
			sprintf(out_name,"%s/%s", out_file, ptr->d_name);
			stat(in_name, &st);

			if(S_IFDIR & st.st_mode) mkdir(out_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			copy_file(in_name,out_name);
		}
	} else {
		// 判断输出文件是否为文件夹
		stat(out_file, &st);
		if(S_IFDIR & st.st_mode) {
			f = filename;
			// 获取要复制的文件名
			for(;*f != '\0';f++) if(*f == '/') p=f;
			if(p == 0) p = filename; else p++;

			sprintf(out_name, "%s%s", out_file, p);

			// 递归来处理文件
			return copy_file(filename, out_name);
		}

		// 删除输出文件
		if(!access(out_file, 0)) unlink(out_file);

		// 打开输入文件和创建输出文件
		if((in_filename = open(filename, O_RDONLY)) == -1) return error_put("Can't Open in_file");
		if((out_filename = creat(out_file, 0644)) == -1) return error_put("Can't Create out_file");

		// 循环读取输入文件的内容到buf内并输出到输出文件
		for(int n_chars = 0;(n_chars = read(in_filename, buf, 4096)) > 0;) if(write(out_filename, buf, n_chars) != n_chars) return error_put("Copy write error.");

		if(close(in_filename) == -1 || close(out_filename) == -1) return error_put("Copy closeing files error.");
	}

	return 0;
}

// 默认打包格式
int normal(char *rom_path, char *ads_path, char *out)
{
	unsigned int fs_total=0, fs_free=0, i=0;
	char build_cmd[512]={0}, work_dir[50]={0};

	// 如果存在build.sh，就执行它并且不执行程序后续部分
	fprintf(stderr, "Exec %s.\n", dir_file(rom_path, "build.sh"));
	if(!access(filename_tmp, 0)) return system(filename_tmp);

	// 创建输出目录
	fprintf(stderr, "Clear Out Directory.\n");
	for(del_file(out); i < strlen(out); i++) if(out[i] == '/') {out[i] = 0;mkdir(out, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);out[i] ='/';}

	// 复制boot/userdata/contexts/到输出目录
	fprintf(stderr, "Copy Base misc to Out.\n");
	if( access(dir_file(rom_path, "system.img"),    0)) return error_put("system.img not found");
	if(!access(dir_file(rom_path, "boot.img"),      0)) copy_file(filename_tmp, out);
	if(!access(dir_file(rom_path, "userdata.img"),  0)) copy_file(filename_tmp, out);
	if(!access(dir_file(rom_path, "file_contexts"), 0)) copy_file(filename_tmp, "file_contexts");

	// Simg2img 解压system.img
	fprintf(stderr, "Simg2img to %s.\n", dir_file(rom_path, "system.img"));
	if(simg2img(filename_tmp, "system.raw") == 250) copy_file(filename_tmp, "system.raw");

	// 挂载system.raw
	// MAC下不支持挂载ext4格式, 通过安装fuse-ext2模块来挂载, Linux可以通过mount()来挂载
	getcwd(work_dir, sizeof(work_dir));
	fprintf(stderr, "Mount %s/system.raw\n", work_dir);
	mkdir(dir_file(out, "system"), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	sprintf(build_cmd,"mount -t fuse-ext2 %s/system.raw %s", work_dir, filename_tmp);
	if(system(build_cmd)) return error_put("Mount system Failed");

	// 获取挂载目录的大小
	// 注意: Linux的结构体和Mac不同，需要修改
	struct statvfs fs;
	if(statvfs(filename_tmp, &fs)) return error_put("get system fs failed.");
	fs_total = fs.f_blocks*4;
	fs_free = fs.f_bfree*4;

	// 添加广告包和over目录
	fprintf(stderr, "Add ads & over.\n");
	copy_file(ads_path, filename_tmp);
	copy_file("over", filename_tmp);

	// 如果存在over.sh就执行它
	fprintf(stderr, "Exec %s.\n", dir_file(rom_path, "over.sh"));
	if(!access(filename_tmp, 0)) system(filename_tmp);

	// 判断是否需要自动排桌面
	fprintf(stderr, "Reorganize Launcher.\n");
	/* TODO: 酷派一级桌面db为compound.db, 二级桌面db为launcher3.db (system/lib/techno)
			 普通机型的db为launcher.db
			 通过安装的广告包的配置文件来替换掉db里的旧广告包排列
			 缺点：无法自定x,y轴，只能在固定的旧广告包上进行替换数据*/

	// 调用make_ext4fs函数打包system
	fprintf(stderr, "Build Rom...\n");
	sprintf(build_cmd , "%uk", fs_total);
	if(!access("file_contexts", 0)) {if(call_make_ext4fs(build_cmd, 1, "system.img", dir_file(out, "system"))) return error_put("make_ext4fs failed");}
	else {if(call_make_ext4fs(build_cmd, 0, "system.img", dir_file(out, "system"))) return error_put("make_ext4fs failed");}

	// 卸载system并清理文件
	if(unmount(filename_tmp, MNT_FORCE)) return error_put("Unmount failed.");
	del_file("system.raw");del_file(filename_tmp);del_file("file_contexts");
	if(rename("system.img",dir_file(out,"system.img"))) return error_put("Move system.img failed.");

	return 0;
}

int build_rom(char *rom, char *ads, char *out)
{
	char rom_ma9r[20]={0},rom_devices[20]={0}, rom_path[20]={0}, ads_path[20]={0}, out_path[20]={0};
	// 分割rom字符串
	if(strstr(rom,"_")) sscanf(rom, "%[0-9a-zA-Z]_%s", rom_ma9r, rom_devices);

	// 默认配置
	sprintf(ads_path, "ads/%s", ads);
	sprintf(rom_path, "base/%s/%s", rom_ma9r, rom_devices);
	sprintf(out_path, "%s/%s/%s/%s/", out, rom_ma9r, rom_devices, ads);

	// 判断广告包目录和rom目录是否存在
	if(!strcmp(rom_ma9r, "") || !strcmp(rom_devices, "")) return error_put("get rom info failed");
	if(access(ads_path, 0)) return error_put("ads folder not found");
	if(access(rom_path, 0)) return error_put("rom folder not found");

	// 不同打包方法可以添加不同函数来实现
	//if(!strcmp(rom_ma9r, "samsung")) return samsung();

	// 默认打包方式
	return normal(rom_path, ads_path, out_path);
}
