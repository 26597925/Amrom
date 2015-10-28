/*
                   _ooOoo_
                  o8888888o
                  88" . "88
                  (| -_- |)
                  O\  =  /O
               ____/`---'\____
             .'  \\|     |//  `.
            /  \\|||  :  |||//  \
           /  _||||| -:- |||||-  \
           |   | \\\  -  /// |   |
           | \_|  ''\---/''  |   |
           \  .-\__  `-`  ___/-. /
         ___`. .'  /--.--\  `. . __
      ."" '<  `.___\_<|>_/___.'  >'"".
     | | :  `- \`.;`\ _ /`;.`/ - ` : | |
     \  \ `-.   \_ __\ /__ _/   .-` /  /
======`-.____`-.___\_____/___.-`____.-'======
                   `=---='
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
           佛祖保佑       永无BUG
 佛曰:
        写字楼里写字间，写字间里程序员；
        程序人员写程序，又拿程序换酒钱。
        酒醒只在网上坐，酒醉还来网下眠；
        酒醉酒醒日复日，网上网下年复年。
        但愿老死电脑间，不愿鞠躬老板前；
        奔驰宝马贵者趣，公交自行程序员。
        别人笑我忒疯癫，我笑自己命太贱；
        不见满街漂亮妹，哪个归得程序员？
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statvfs.h>

#include "simg2img/simg2img.h" // Simg2img
#include "make_ext4fs/make_ext4fs_main.h" // Make_Ext4fs

// 全局变量
char filename_tmp[200]={0};
char *app_config=0;

// 定义函数原型
int usage();                                                               /* 该程序使用方法 */
int error_put(char *str);                                                  /* 输出错误到屏幕 */
char *dir_file(char *dir, char *file);                                     /* 合并字符串 */
char *DelSpace(char *in);                                                  /* 删除配置文件的空格 */
int del_file(char *filename);                                              /* 删除文件函数 */
int copy_file(char *filename, char *out_file);                             /* 复制文件函数 */
int launcher(char *filename, char *sqldata, int sql_start, int sql_end);   /* 自动排列桌面 */
int normal(char *rom_path, char *ads_path, char *out);                     /* 默认打包函数 */
int build_rom(char *rom, char *ads, char *out);                            /* 编译rom函数 */

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

	// 判断是否有权限执行
	#if defined(__linux__)
	if(getuid() != 0) return error_put("only root can do that.");
	#endif

	// 判断广告包目录和rom包目录是否存在
	if(!access(ads_config, 0) && !access(rom_config, 0)) {
		FILE* ads_info = fopen(ads_config, "r");
		FILE* rom_info = fopen(rom_config, "r");

		if(ads_info == NULL) return error_put("Can't Open Configure file.");
		if(rom_info == NULL) return error_put("Can't Open Configure file.");

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
					fprintf(stderr, "\n[*] Failed to build Rom.\n"
							"[*] Wait 5s to continue\n\n");
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

			sprintf(out_name, "%s/%s", out_file, p);

			// 递归来处理文件
			return copy_file(filename, out_name);
		}

		// 删除输出文件
		if(!access(out_file, 0)) unlink(out_file);

		// 打开输入文件和创建输出文件
		if((in_filename = open(filename, O_RDONLY)) == -1) return error_put("Can't Open in_file");
		//if((out_filename = creat(out_file, 0644)) == -1) return error_put("Can't Creat out_file");
		if((out_filename = creat(out_file, 0644)) == -1) {
			sprintf(out_name, "Can't creat %s", out_file);
			return error_put(out_name);
		}

		// 循环读取输入文件的内容到buf内并输出到输出文件
		for(int n_chars = 0;(n_chars = read(in_filename, buf, 4096)) > 0;) if(write(out_filename, buf, n_chars) != n_chars) return error_put("Copy write error.");

		if(close(in_filename) == -1 || close(out_filename) == -1) return error_put("Copy closeing files error.");
	}

	return 0;
}

// 自动排列桌面
int launcher(char *filename, char *sqldata, int sql_start, int sql_end)
{
	sqlite3 *db = 0;
	char *err_msg = 0;
	char sql[512]={0}, app_tmp[256]={0}, app_package[100]={0}, app_class[100]={0}, app_title[100]={0};

	// 打开app配置文件
	FILE* app_info = fopen(app_config, "r");
	if(app_info == NULL) return error_put("Can't Open Configure file.");

	// 打开databases
	if (SQLITE_OK != sqlite3_open(filename, &db)) return error_put("Can't open the databases.");
	
	// 循环读取广告包配置文件获取包名和类名
	while(fgets(app_tmp, 256, app_info)) {
		// '#'开头的都跳过
		if(app_tmp[0] == '#' )  continue;
		if(app_tmp[0] == '\n' ) continue;

		// 分割字符串
		if(strstr(app_tmp,"_")) sscanf(app_tmp, "%[0-9a-zA-Z./]_%[0-9a-zA-Z./]_%s", app_package, app_class, app_title); else continue;

		if(sql_start <= sql_end){
			// 替换标题
			sprintf(sql, "update %s set title='%s' where _id=%d", sqldata, app_title, sql_start);
			if(SQLITE_OK != sqlite3_exec(db, sql, 0, 0, &err_msg)) break;
			// 替换app桌面应用入口
			sprintf(sql, "update %s set intent='#Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=%s/%s;end' where _id=%d",
					sqldata, app_package, app_class, sql_start);
			if(SQLITE_OK != sqlite3_exec(db, sql, 0, 0, &err_msg)) break;
			// 替换图标
			sprintf(sql, "update %s set iconPackage='%s' where _id=%d", sqldata, app_package, sql_start);
			if(SQLITE_OK != sqlite3_exec(db, sql, 0, 0, &err_msg)) break;

			sql_start++;
		}
	}

	// 删除多余的表格
	while( sql_start <= sql_end) {
		sprintf(sql, "delete from %s where _id=%d", sqldata, sql_start);
		if(SQLITE_OK != sqlite3_exec(db, sql, 0, 0, &err_msg)) break;
		sql_start++;
	}

	fclose(app_info);
	if (SQLITE_OK != sqlite3_close(db)) return error_put("Close databases failed.");
	return 0;
}

// 默认打包格式
int normal(char *rom_path, char *ads_path, char *out)
{
	unsigned int fs_total=0, fs_free=0, i=0;
	char build_cmd[512]={0}, work_dir[50]={0}, sql_start[10]={0}, sql_end[10]={0}, free_size[20]={0};

	// 如果存在build.sh，就执行它并且不执行程序后续部分
	fprintf(stderr, "Exec %s.\n", dir_file(rom_path, "build.sh"));
	if(!access(filename_tmp, 0)) return system(filename_tmp);

	// 创建输出目录
	fprintf(stderr, "Clear Out Directory.\n");
	for(del_file(out); i < strlen(out); i++) if(out[i] == '/') {out[i] = 0;mkdir(out, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);out[i] ='/';}

	// 复制boot/userdata/contexts/到输出目录
	fprintf(stderr, "Copy Base misc to Out.\n");
	if( access(dir_file(rom_path, "system.img"),    0)) if(access(dir_file(rom_path, "system.img.ext4"), 0)) return error_put("system.img or system.img.ext4 not found");
	if(!access(dir_file(rom_path, "settings"),      0)) copy_file(filename_tmp, out);
	if(!access(dir_file(rom_path, "boot.img"),      0)) copy_file(filename_tmp, out);
	if(!access(dir_file(rom_path, "userdata.img"),  0)) copy_file(filename_tmp, out);
	if(!access(dir_file(rom_path, "file_contexts"), 0)) copy_file(filename_tmp, "file_contexts");
	if(!access(dir_file(rom_path, "ads_5.0"),       0)) sprintf(ads_path, "%s_5.0", ads_path);
	if(!access(dir_file(rom_path, "unlock.apk"),    0)) {sprintf(build_cmd, "%s../", out);copy_file(filename_tmp, build_cmd);}
	if(!access(dir_file(rom_path, "unlock.tar"),    0)) {sprintf(build_cmd, "%s../", out);copy_file(filename_tmp, build_cmd);}

	// Simg2img 解压system.img
	if(!access(dir_file(rom_path, "system.img"), 0)) sprintf(build_cmd, "%s/%s", rom_path, "system.img");
	if(!access(dir_file(rom_path, "system.img.ext4"), 0)) sprintf(build_cmd, "%s/%s", rom_path, "system.img.ext4");
	fprintf(stderr, "Simg2img %s.\n", build_cmd);
	if(simg2img(build_cmd, "system.raw") == 250) copy_file(build_cmd, "system.raw");

	// 挂载system.raw
	getcwd(work_dir, sizeof(work_dir));
	fprintf(stderr, "Mount %s/system.raw\n", work_dir);
	mkdir(dir_file(out, "system"), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	#if defined(__linux__)
	sprintf(build_cmd,"mount -t ext4 -o loop %s/system.raw %s > /dev/null 2>&1", work_dir, filename_tmp);
	if(system(build_cmd)) return error_put("Mount system Failed");
	#elif defined(__APPLE__) && defined(__MACH__)
	sprintf(build_cmd,"fuse-ext2 -o force %s/system.raw %s > /dev/null 2>&1", work_dir, filename_tmp);
	if(system(build_cmd)) return error_put("Mount system Failed");
	#endif

	// 拷贝rom
	if(!access(dir_file(rom_path, "base"), 0)) copy_file(filename_tmp, out);

	// 判断是否为system
	for(i=0;i<3;i++) if(sleep(3) && !access(dir_file(out, "system/build.prop"), 0)) break;
	if(access(dir_file(out, "system/build.prop"), 0)) return error_put("build.prop not found!");

	// 获取挂载目录的大小, 默认的block大小为4096
	// 注意: Linux的结构体和Mac不同，需要修改
	struct statvfs fs;
	if(statvfs(dir_file(out, "system"), &fs)) return error_put("get system fs failed.");
	fs_total = fs.f_blocks*4;
	fs_free = fs.f_bfree*4;

	// 添加广告包和over目录
	fprintf(stderr, "Add ads & over.\n");
	// 添加广告包
	if(!access(dir_file(rom_path, ads_path), 0)) sprintf(build_cmd, "%s/%s", rom_path, ads_path);else sprintf(build_cmd, "%s", ads_path);
	copy_file(build_cmd, dir_file(out, "system"));
	// 添加over目录
	if(!access(dir_file(rom_path, "over"), 0)) sprintf(build_cmd, "%s/%s", rom_path, "over");else sprintf(build_cmd, "%s", "over");
	copy_file(build_cmd, dir_file(out, "system"));

	// 判断是否需要排列桌面(sqlite databases)
	if(!access(dir_file(rom_path, "sql.conf"), 0)) {

		fprintf(stderr, "Reorganize Launcher.\n");
		// 获取广告起始位置和结束位置
		FILE* sql_info = fopen(filename_tmp, "r");
		if(sql_info == NULL) return error_put("Can't Open Configure file.");

		while(fgets(build_cmd, 100, sql_info)) {
			// 不读取'#'开头的注释
			if(build_cmd[0] == '#' )  continue;
			if(build_cmd[0] == '\n' ) continue;

			if(strstr(build_cmd,"_")) sscanf(build_cmd, "%[0-9]_%[0-9]", sql_start, sql_end); else continue;
		}

		// 自动排列桌面
		if(!access(dir_file(rom_path, "launcher.db"), 0) && !copy_file(filename_tmp, "./")) { // 普通桌面
			launcher("launcher.db", "favorites", atoi(sql_start), atoi(sql_end));
			copy_file("launcher.db", dir_file(out, "system/etc"));}
		if(!access(dir_file(rom_path, "compound.db"), 0) && !copy_file(filename_tmp, "./")) { // 酷派一级桌面
			launcher("compound.db", "compoundworkspace", atoi(sql_start), atoi(sql_end));
			copy_file("compound.db", dir_file(out, "system/lib/uitechno"));}
		if(!access(dir_file(rom_path, "launcher3.db"), 0) && !copy_file(filename_tmp, "./")) { // 酷派二级桌面
			launcher("launcher3.db", "menu", atoi(sql_start), atoi(sql_end));
			copy_file("launcher3.db", dir_file(out, "system/lib/uitechno"));}

		fclose(sql_info);
	}

	// TODO: 判断是否需要排列桌面(apktool tools)

	// 如果存在over.sh就执行它
	fprintf(stderr, "Exec %s.\n", dir_file(rom_path, "over.sh"));
	if(!access(filename_tmp, 0)) system(filename_tmp);


	// 判断三星的cache是否够空间刷进去
	if(statvfs(dir_file(out, "system"), &fs)) return error_put("get system fs failed.");
	if(!access(dir_file(rom_path, "free_size"), 0)) {
		// 读取文本内容
		FILE* free_info = fopen(filename_tmp, "r");
		if(free_info == NULL) return error_put("Can't Open Configure file.");

		while(fgets(build_cmd, 100, free_info)) {
			// 不读取'#'开头的注释
			if(build_cmd[0] == '#' )  continue;
			if(build_cmd[0] == '\n' ) continue;

			sscanf(build_cmd, "%[0-9]", free_size);
		}

		// 剩余空间不足时报错
		if(atoi(free_size) > fs.f_bfree*4) {
			fprintf(stderr, "[*] Free space less %dk.\n", atoi(free_size));
			fclose(free_info);
			return error_put("Build failed.");
		}

		fclose(free_info);
	}
	
	// 调用make_ext4fs函数打包system
	fprintf(stderr, "Build Rom (Size: %uk)...\n",fs_total + 10240);
	sprintf(build_cmd , "%uk", fs_total + 10240); // 打包后的大小会缩小，增加10m给他打包
	if(!access("file_contexts", 0)) {if(call_make_ext4fs(build_cmd, 1, "system.img", dir_file(out, "system"))) return error_put("make_ext4fs failed");}
	else {if(call_make_ext4fs(build_cmd, 0, "system.img", dir_file(out, "system"))) return error_put("make_ext4fs failed");}

	// 卸载system并清理文件
	#if defined(__linux__)
	if(umount2(filename_tmp, MNT_FORCE)) return error_put("unmount failed.");
	#elif defined(__APPLE__) && defined(__MACH__)
	if(unmount(filename_tmp, MNT_FORCE)) return error_put("unmount failed.");
	#endif
	del_file("launcher.db");del_file("compound.db");del_file("launcher3.db");
	del_file("system.raw");del_file(filename_tmp);del_file("file_contexts");
	if(!access(dir_file(rom_path, "system.img"), 0)) if(rename("system.img",dir_file(out,"system.img"))) return error_put("Move system.img failed.");
	if(!access(dir_file(rom_path, "system.img.ext4"), 0)) if(rename("system.img",dir_file(out,"system.img.ext4"))) return error_put("Move system.img.ext4 failed.");

	// 三星tar文件打包
	if(!access(dir_file(out,"system.img.ext4"), 0)) {
		sprintf(build_cmd, "gtar -H ustar -C %s -c `echo -n $(ls %s)` > Samsung.tar", out, out);
		if(system(build_cmd)) return error_put("Create Samsung.tar Failed.");
		del_file(out);out[strlen(out)-1]='.';sprintf(build_cmd, "%star", out);del_file(out);
		if(rename("Samsung.tar", build_cmd)) return error_put("Move Samsung.tar failed.");
	}

	fprintf(stderr, "Build Done!\n\n");
	return 0;
}

int build_rom(char *rom, char *ads, char *out)
{
	char rom_ma9r[20]={0}, rom_devices[20]={0}, rom_path[50]={0}, ads_path[20]={0}, out_path[50]={0};
	// 分割rom字符串
	if(strstr(rom,"_")) sscanf(rom, "%[0-9a-zA-Z]_%s", rom_ma9r, rom_devices);

	// 输出Build信息
	fprintf(stderr, "Manufacturer:%s Devices:%s Ads:%s\n", rom_ma9r, rom_devices, ads);

	// 默认配置
	sprintf(ads_path, "ads/%s", ads);
	sprintf(rom_path, "base/%s/%s", rom_ma9r, rom_devices);
	sprintf(out_path, "%s/%s/%s/%s/", out, rom_ma9r, rom_devices, ads);

	// 机型配置
	sprintf(filename_tmp, "%s_%s", ads_path, rom_ma9r);
	if(!access(filename_tmp, 0)) sprintf(ads_path, "ads/%s_%s", ads, rom_ma9r);

	// 判断广告包目录和rom目录是否存在
	if(!strcmp(rom_ma9r, "") || !strcmp(rom_devices, "")) return error_put("get rom info failed");
	if(access(ads_path, 0)) return error_put("ads folder not found");
	if(access(rom_path, 0)) return error_put("rom folder not found");

	// 定义广告包里的配置文件
	if(!strcmp(ads, "gg")) app_config="ads/gg.conf";
	if(!strcmp(ads, "jj")) app_config="ads/jj.conf";
	if(!strcmp(ads, "nc")) app_config="ads/nc.conf";
	if(!strcmp(ads, "zl")) app_config="ads/zl.conf";
	if(!strcmp(ads, "ql")) app_config="ads/ql.conf";

	// 不同打包方法可以添加不同函数来实现
	//if(!strcmp(rom_ma9r, "samsung")) return samsung();

	// 默认打包方式
	return normal(rom_path, ads_path, out_path);
}
