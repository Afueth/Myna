import { runClang } from '@yowasp/clang';
import fs from 'node:fs';
const src = `#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
int main(int c, char **v){ DIR *d = opendir(v[1]); if(!d){printf("opendir fail %s\n", strerror(errno));return 1;}
 struct dirent *e; int n=0; errno=0; while((e=readdir(d))) {printf("%s\n", e->d_name); n++;} printf("n=%d errno=%s\n", n, strerror(errno)); return 0; }`;
const r = await runClang(['clang','--target=wasm32-wasip1','ls.c','-o','ls.wasm'], {'ls.c': src});
fs.writeFileSync(process.argv[2], r['ls.wasm']);
