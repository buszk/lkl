/*
 @Author	: ouadimjamal@gmail.com
 @date		: December 2015

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.  No representations are made about the suitability of this
software for any purpose.  It is provided "as is" without express or
implied warranty.
*/

#include "pmparser.h"

/**
 * gobal variables
 */
procmaps_struct* g_last_head=NULL;
procmaps_struct* g_current=NULL;


procmaps_struct* pmparser_parse(int pid){
	char maps_path[500];
	if(pid>=0 ){
		sprintf(maps_path,"/proc/%d/maps",pid);
	}else{
		sprintf(maps_path,"/proc/self/maps");
	}
	FILE* file=fopen(maps_path,"r");
	if(!file){
		fprintf(stderr,"pmparser : cannot open the memory maps, %s\n",strerror(errno));
		return NULL;
	}
	int ind=0;char buf[3000];
	char c;
	procmaps_struct* list_maps=NULL;
	procmaps_struct* tmp;
	procmaps_struct* current_node=list_maps;
	char addr1[20],addr2[20], perm[8], offset[20], dev[10],inode[30],pathname[600];
	while(1){
		if( (c=fgetc(file))==EOF ) break;
		if (fgets(buf+1,259,file)) {}
		buf[0]=c;
		//allocate a node
		tmp=(procmaps_struct*)malloc(sizeof(procmaps_struct));
		//fill the node
		_pmparser_split_line(buf,addr1,addr2,perm,offset, dev,inode,pathname);
		//printf("#%s",buf);
		// fprintf(stderr, "%s-%s %s %s %s %s\t%s\n",addr1,addr2,perm,offset,dev,inode,pathname);
		//addr_start & addr_end
		sscanf(addr1,"%lx",(long unsigned *)&tmp->addr_start );
		sscanf(addr2,"%lx",(long unsigned *)&tmp->addr_end );
		//size
		tmp->length=(unsigned long)(tmp->addr_end)-(unsigned long)(tmp->addr_start);
		//perm
		strcpy(tmp->perm,perm);
		tmp->is_r=(perm[0]=='r');
		tmp->is_w=(perm[1]=='w');
		tmp->is_x=(perm[2]=='x');
		tmp->is_p=(perm[3]=='p');

		//offset
		sscanf(offset,"%lx",&tmp->offset );
		//device
		strcpy(tmp->dev,dev);
		//inode
		tmp->inode=atoi(inode);
		//pathname
		strcpy(tmp->pathname,pathname);
		tmp->next=NULL;
		//attach the node
		if(ind==0){
			list_maps=tmp;
			list_maps->next=NULL;
			current_node=list_maps;
		}
		current_node->next=tmp;
		current_node=tmp;
		ind++;
		//printf("%s",buf);
	}


	g_last_head=list_maps;
	return list_maps;
}


procmaps_struct* pmparser_next(){
	if(g_last_head==NULL) return NULL;
	if(g_current==NULL){
		g_current=g_last_head;
	}else
		g_current=g_current->next;

	return g_current;
}



void pmparser_free(procmaps_struct* maps_list){
	if(maps_list==NULL) return ;
	procmaps_struct* act=maps_list;
	procmaps_struct* nxt=act->next;
	while(act!=NULL){
		free(act);
		act=nxt;
		if(nxt!=NULL)
			nxt=nxt->next;
	}

}


void _pmparser_split_line(
		char*buf,char*addr1,char*addr2,
		char*perm,char* offset,char* device,char*inode,
		char* pathname){
	//
	int orig=0;
	int i=0;
	//addr1
	while(buf[i]!='-'){
		addr1[i-orig]=buf[i];
		i++;
	}
	addr1[i]='\0';
	i++;
	//addr2
	orig=i;
	while(buf[i]!='\t' && buf[i]!=' '){
		addr2[i-orig]=buf[i];
		i++;
	}
	addr2[i-orig]='\0';

	//perm
	while(buf[i]=='\t' || buf[i]==' ')
		i++;
	orig=i;
	while(buf[i]!='\t' && buf[i]!=' '){
		perm[i-orig]=buf[i];
		i++;
	}
	perm[i-orig]='\0';
	//offset
	while(buf[i]=='\t' || buf[i]==' ')
		i++;
	orig=i;
	while(buf[i]!='\t' && buf[i]!=' '){
		offset[i-orig]=buf[i];
		i++;
	}
	offset[i-orig]='\0';
	//dev
	while(buf[i]=='\t' || buf[i]==' ')
		i++;
	orig=i;
	while(buf[i]!='\t' && buf[i]!=' '){
		device[i-orig]=buf[i];
		i++;
	}
	device[i-orig]='\0';
	//inode
	while(buf[i]=='\t' || buf[i]==' ')
		i++;
	orig=i;
	while(buf[i]!='\t' && buf[i]!=' '){
		inode[i-orig]=buf[i];
		i++;
	}
	inode[i-orig]='\0';
	//pathname
	pathname[0]='\0';
	while(buf[i]=='\t' || buf[i]==' ')
		i++;
	orig=i;
	while(buf[i]!='\t' && buf[i]!=' ' && buf[i]!='\n'){
		pathname[i-orig]=buf[i];
		i++;
	}
	pathname[i-orig]='\0';

}

void pmparser_print(procmaps_struct* map, int order){

	procmaps_struct* tmp=map;
	int id=0;
	if(order<0) order=-1;
	while(tmp!=NULL){
		//(unsigned long) tmp->addr_start;
		if(order==id || order==-1){
			printf("Backed by:\t%s\n",strlen(tmp->pathname)==0?"[anonym*]":tmp->pathname);
			printf("Range:\t\t%p-%p\n",tmp->addr_start,tmp->addr_end);
			printf("Length:\t\t%ld\n",tmp->length);
			printf("Offset:\t\t%ld\n",tmp->offset);
			printf("Permissions:\t%s\n",tmp->perm);
			printf("Inode:\t\t%d\n",tmp->inode);
			printf("Device:\t\t%s\n",tmp->dev);
		}
		if(order!=-1 && id>order)
			tmp=NULL;
		else if(order==-1){
			printf("#################################\n");
			tmp=tmp->next;
		}else tmp=tmp->next;

		id++;
	}
}

extern char *strcasestr(const char *haystack, const char *needle);
void fill_kasan_meta(struct lkl_kasan_meta* to, const char *binary_name) {
    static int add =0;
    struct procmaps_struct *head, *pt;
    pid_t pid = getpid();
    printf("my pid: %d\n",pid);

    head = pmparser_parse(pid);
    //pmparser_print(head, -1);
    
    pt = head;
    to->global_base = 0;
    to->global_size = 0;
    while(pt != NULL) {
        if(strncmp(pt->pathname,"[stack]",10) == 0) {
            to->stack_base = (unsigned long)pt->addr_start;
            to->stack_size = (unsigned long)pt->length; 
        }
        if(strcasestr(pt->pathname, binary_name)) {
            if (to->global_base == 0) {
                to->global_base = (unsigned long)pt->addr_start;
                to->global_size = (unsigned long)pt->length; 
            }
            else if ((unsigned long)pt->addr_start > to->global_base) {
                to->global_size = (unsigned long)pt->addr_start - to->global_base + (unsigned long)pt->length;
            }
            else {
                abort();
            }
            add =1;
        }
        else if (add)
        {
            add =0;
            to->global_size = (unsigned long)pt->addr_start - to->global_base + (unsigned long)pt->length;
        }
        
        pt = pt->next;
    }
    return;

}