#include <stdio.h>
#include <unistd.h>

#ifdef _WIN32
#include <string.h>
#else
#include <memory.h>
#endif

int usage()
{
	char *str = "\
Usage: hzy_zip [-u] -f <in_file> -o <out_file>\n\
Options:\n\
	-u: unzip\n\
	-f: set <in_file>\n\
	-o: set <out_file>\n\
";
	printf("%s", str);
	return 0;
}

#define T_FILE 1

#define A_ZIP 0
#define A_UNZIP 1

struct options {
	char type;
	char action;
	char *filename;
	char *foutname;
};


struct zip_unit {
	unsigned int num: 6;
	unsigned int is_short: 1; //undefined
	unsigned int is_zip: 1;
};

#define BUFFER_SIZE 1024

typedef union {
	struct zip_unit flag;
	char byte;
}zunit;

int zip(char *buffer, int len, char *zbuffer)
{
	int i = 0, j = 0;
	int c_num;
	char byte;
	zunit zu;
	while(i < len)
	{
		c_num = 1;
		while(i < len - 1 && c_num < (1<<6) && buffer[i] == buffer[i+1])
		{
			i++;
			c_num++;
		}
		
		if(i == len - 1 && c_num < (1<<6) - 1 && buffer[i] == buffer[i-1])
		{
			c_num++;
		}
		byte = buffer[i];
		zu.byte = byte;
		//printf("%c %d\n", byte, c_num);
		if(c_num > 1 || zu.flag.is_zip)
		{
			zu.byte = 0;
			zu.flag.is_zip = 1;
			zu.flag.is_short = 0;
			zu.flag.num = c_num;
			zbuffer[j++] = zu.byte;
			zbuffer[j] = byte;
		}
		else
		{
			zbuffer[j] = byte;
		}
		j++;
		i++;
	}
	return j;
}

int unzip(char *zbuffer, int zlen, char *buffer, char *last)
{
	int i = 0, j = 0;
	int c_num;
	zunit zu;
	
	if(*last)
	{
		zu.byte = *last;
		//printf("%c %d\n", zbuffer[0], zu.flag.num);
		for(c_num=zu.flag.num;c_num > 0;c_num--)
			buffer[j++] = zbuffer[0];
		i++;
	}
	*last = 0;
	
	while(i < zlen)
	{
		zu.byte = zbuffer[i];
		if(!zu.flag.is_zip)
		{
			//printf("%c\n", zu.byte);
			buffer[j++] = zu.byte;
			i++;
		}else if(i == zlen - 1)
		{
			*last = zu.byte;
			i++;
		}
		else
		{
			//printf("%c %d\n", zbuffer[i+1], zu.flag.num);
			for(c_num=zu.flag.num;c_num > 0;c_num--)
				buffer[j++] = zbuffer[i+1];
			i += 2;
		}
	}
	return j;
}


int fzip(char *fname, char *fzipname)
{
	char buffer[BUFFER_SIZE];
	char zbuffer[BUFFER_SIZE];
	int len;
	long inlen = 0, outlen = 0;
	FILE *f, *fzip;
	if((f = fopen(fname, "rb")) == NULL)
	{
		printf("open file %s failed!\n", fname);
		return -1;
	}
	if((fzip = fopen(fzipname, "wb")) == NULL)
	{
		printf("open file %s failed!\n", fzipname);
		return -1;
	}
	
	while((len = fread(buffer, sizeof(char), BUFFER_SIZE, f)) > 0)
	{
		inlen += len;
		len = zip(buffer, len, zbuffer);
		outlen += len;
		fwrite(zbuffer, sizeof(char), len, fzip);
	}
	
	fclose(f);
	fclose(fzip);
	printf("zip %ld -> %ld bytes\n", inlen, outlen);
	return 0;
}

int funzip(char *fzipname, char *fname)
{
	char zbuffer[BUFFER_SIZE/32];
	char buffer[BUFFER_SIZE];
	char last = 0;
	int len;
	long inlen=0, outlen=0;
	FILE *f, *fzip;
	if((fzip = fopen(fzipname, "rb")) == NULL)
	{
		printf("open file %s failed!\n", fzipname);
		return -1;
	}
	if((f = fopen(fname, "wb")) == NULL)
	{
		printf("open file %s failed!\n", fname);
		return -1;
	}
	
	while((len = fread(zbuffer, sizeof(char), BUFFER_SIZE/32, fzip)) > 0)
	{
		inlen += len;
		len = unzip(zbuffer, len, buffer, &last);
		outlen += len;
		fwrite(buffer, sizeof(char), len, f);
	}
	
	fclose(f);
	fclose(fzip);
	printf("unzip %ld -> %ld bytes\n", inlen, outlen);
	return 0;
}




int proc_args(int argc, char *argv[], struct options *opt)
{
	char ch;
	memset(opt, 0, sizeof(opt));
	while((ch = getopt(argc, argv, "o:f:u")) != -1)
	{
		switch(ch)
		{
			case 'f':
				opt->type = T_FILE;
				opt->filename = optarg;
				break;
			case 'o':
				opt->foutname = optarg;
				break;
			case 'u':
				opt->action = A_UNZIP;
				break;
			default:
				printf("Unsupport option: %c\n", ch);
				return 0;
		}
	}
	
	return 0;
}


int main(int argc, char *argv[])
{
	struct options opt;
	proc_args(argc, argv, &opt);
	
	if(opt.type == T_FILE && opt.filename && opt.foutname)
	{
		if(opt.action == A_ZIP)
		{
			printf("zip %s --> %s\n", opt.filename, opt.foutname);
			fzip(opt.filename, opt.foutname);
			return 0;
		}
		else
		{
			printf("unzip %s --> %s\n", opt.filename, opt.foutname);
			funzip(opt.filename, opt.foutname);
			return 0;
		}
	}
	
	usage();
	
	return 0;
}