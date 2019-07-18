#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>

#include "common.h"

sqlite3 *db;
int flags = 0;

void login(int acceptfd,MSG *msg);
void user_query(int acceptfd,MSG *msg);
void user_modify(int acceptfd,MSG *msg);
void root_query(int acceptfd,MSG *msg);
void root_modify(int acceptfd,MSG *msg);
void root_adduser(int acceptfd,MSG *msg);
void root_deluser(int acceptfd,MSG *msg);
void root_history(int acceptfd,MSG *msg);
void quit(int acceptfd,MSG *msg);

void history_init(MSG *msg,char *buf)
{
    int nrow,ncolumn;
    char *errmsg, **resultp;
    char sqlhistory[DATALEN] = {0};
    char timedata[DATALEN] = {0};

    sprintf(sqlhistory,"insert into historyinfo values ('%s','%s','%s');",timedata,msg->username,buf);
    if(sqlite3_exec(db,sqlhistory,NULL,NULL,&errmsg)!= SQLITE_OK){
        printf("%s.\n",errmsg);
        printf("insert historyinfo failed.\n");
    }else{
        printf("insert historyinfo success.\n");
    }
}

int history_callback(void *arg, int ncolumn, char **f_value, char **f_name)
{
    int i = 0;
    MSG *msg= (MSG *)arg;
    int acceptfd = msg->flags;

    if(flags == 0){
        for(i = 0; i < ncolumn; i++){
            printf("%-11s", f_name[i]);
        }
        putchar(10);
        flags = 1;
    }

    for(i = 0; i < ncolumn; i+=3)
    {
        printf("%s-%s-%s",f_value[i],f_value[i+1],f_value[i+2]);
        sprintf(msg->recvmsg,"%s---%s---%s.\n",f_value[i],f_value[i+1],f_value[i+2]);
        send(acceptfd,msg,sizeof(MSG),0);
        usleep(1000);
    }
    puts("");
    return 0;
}

int process_client_request(int acceptfd,MSG *msg)
{
    switch (msg->msgtype)
    {
        case USER_LOGIN:
        case ADMIN_LOGIN:
            login(acceptfd,msg);
            break;
        case USER_MODIFY:
            user_modify(acceptfd,msg);
            break;
        case USER_QUERY:
            user_query(acceptfd,msg);
            break;
        case ADMIN_MODIFY:
            root_modify(acceptfd,msg);
            break;
        case ADMIN_ADDUSER:
            root_adduser(acceptfd,msg);
            break;
        case ADMIN_DELUSER:
            root_deluser(acceptfd,msg);
            break;
        case ADMIN_QUERY:
            root_query(acceptfd,msg);
            break;
        case ADMIN_HISTORY:
            root_history(acceptfd,msg);
            break;
        case QUIT:
            quit(acceptfd,msg);
            break;
        default:
            break;
    }
}

int main(int argc, const char *argv[])
{
    int sockfd,acceptfd;
    ssize_t recvbytes;
    struct sockaddr_in serveraddr,clientaddr;
    socklen_t addrlen = sizeof(serveraddr);
    socklen_t cli_len = sizeof(clientaddr);

    MSG msg;
    char *errmsg;

    if(sqlite3_open(STAFF_DATABASE,&db) != SQLITE_OK){
        printf("%s.\n",sqlite3_errmsg(db));
    }else{
        printf("the sqlite open success.\n");
    }

    if(sqlite3_exec(db,"create table usrinfo(staffno integer,usertype integer,name text,passwd text,age integer,phone text,addr text,work text,date text,level integer,salary REAL);",NULL,NULL,&errmsg)!= SQLITE_OK){
        printf("%s.\n",errmsg);
    }else{
        printf("create usrinfo table success.\n");
    }

    if(sqlite3_exec(db,"create table historyinfo(time text,name text,words text);",NULL,NULL,&errmsg)!= SQLITE_OK){
        printf("%s.\n",errmsg);
    }else{
        printf("create historyinfo table success.\n");
    }

    sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("socket failed.\n");
        exit(-1);
    }
    printf("sockfd :%d.\n",sockfd);

    memset(&serveraddr,0,sizeof(serveraddr));
    memset(&clientaddr,0,sizeof(clientaddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(atoi(argv[2]));
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(bind(sockfd, (const struct sockaddr *)&serveraddr,addrlen) < 0){
        printf("bind failed.\n");
        return -1;
    }

    if(listen(sockfd,10) <0){
        printf("listen failed.\n");
        return -1;
    }

    fd_set readfds,tempfds;
    FD_ZERO(&readfds);
    FD_ZERO(&tempfds);
    FD_SET(sockfd,&readfds);
    int nfds = sockfd;
    int retval;
    int i = 0;

    while(1){
        tempfds = readfds;
        retval =select(nfds + 1, &tempfds, NULL,NULL,NULL);
        for(i = 0;i < nfds + 1; i ++){
            if(FD_ISSET(i,&tempfds)){
                if(i == sockfd){
                    acceptfd = accept(sockfd,(struct sockaddr *)&clientaddr,&cli_len);
                    if(acceptfd == -1){
                        printf("acceptfd failed.\n");
                        return -1;
                    }
                    printf("ip : %s.\n",inet_ntoa(clientaddr.sin_addr));
                    FD_SET(acceptfd,&readfds);
                    nfds = nfds > acceptfd ? nfds : acceptfd;
                }else{
                    recvbytes = recv(i,&msg,sizeof(msg),0);
                    printf("msg.type :%#x.\n",msg.msgtype);
                    if(recvbytes == -1){
                        printf("recv failed.\n");
                        continue;
                    }else if(recvbytes == 0){
                        printf("peer shutdown.\n");
                        close(i);
                        FD_CLR(i, &readfds);
                    }else{
                        process_client_request(i,&msg);
                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

void login(int acceptfd,MSG *msg){
    char sql[DATALEN] = {0};
    char *errmsg;
    char **result;
    int nrow,ncolumn;

    msg->info.usertype =  msg->usertype;
    strcpy(msg->info.name,msg->username);
    strcpy(msg->info.passwd,msg->passwd);

    printf("usrtype: %#x-----usrname: %s---passwd: %s.\n",msg->info.usertype,msg->info.name,msg->info.passwd);
    sprintf(sql,"select * from usrinfo where usertype=%d and name='%s' and passwd='%s';",msg->info.usertype,msg->info.name,msg->info.passwd);
    if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
        printf("---****----%s.\n",errmsg);
    }else{
        if(nrow == 0){
            strcpy(msg->recvmsg,"name or passwd failed.\n");
            send(acceptfd,msg,sizeof(MSG),0);
        }else{
            strcpy(msg->recvmsg,"OK");
            send(acceptfd,msg,sizeof(MSG),0);
        }
    }
}

void user_query(int acceptfd,MSG *msg){
    int i = 0,j = 0;
    char sql[DATALEN] = {0};
    char **resultp;
    int nrow,ncolumn;
    char *errmsg;

    sprintf(sql,"select * from usrinfo where name='%s';",msg->username);
    if(sqlite3_get_table(db, sql, &resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
        printf("%s.\n",errmsg);
    }else{
        printf("searching.....\n");
        for(i = 0; i < ncolumn; i ++){
            printf("%-8s ",resultp[i]);
        }
        puts("");
        puts("======================================================================================");

        int index = ncolumn;
        for(i = 0; i < nrow; i ++){
            printf("%s    %s     %s     %s     %s     %s     %s     %s     %s     %s     %s.\n",resultp[index+ncolumn-11],resultp[index+ncolumn-10],\
                resultp[index+ncolumn-9],resultp[index+ncolumn-8],resultp[index+ncolumn-7],resultp[index+ncolumn-6],resultp[index+ncolumn-5],\
                resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);

            sprintf(msg->recvmsg,"%s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s;",resultp[index+ncolumn-11],resultp[index+ncolumn-10],\
                resultp[index+ncolumn-9],resultp[index+ncolumn-8],resultp[index+ncolumn-7],resultp[index+ncolumn-6],resultp[index+ncolumn-5],\
                resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);
            send(acceptfd,msg,sizeof(MSG),0);

            usleep(1000);
            puts("======================================================================================");
            index += ncolumn;
        }

        sqlite3_free_table(resultp);
        printf("sqlite3_get_table successfully.\n");
    }

}

void user_modify(int acceptfd,MSG *msg){
    int nrow,ncolumn;
    char *errmsg, **resultp;
    char sql[DATALEN] = {0};
    char historybuf[DATALEN] = {0};

    switch (msg->recvmsg[0])
    {
        case 'P':
            sprintf(sql,"update usrinfo set phone='%s' where staffno=%d;",msg->info.phone,msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的电话为%s",msg->username,msg->info.no,msg->info.phone);
            break;
        case 'D':
            sprintf(sql,"update usrinfo set addr='%s' where staffno=%d;",msg->info.addr, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的家庭住址为%s",msg->username,msg->info.no,msg->info.addr);
            break;
        case 'M':
            sprintf(sql,"update usrinfo set passwd='%s' where staffno=%d;",msg->info.passwd, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的密码为%s",msg->username,msg->info.no,msg->info.passwd);
            break;
    }
    printf("msgtype :%#x--usrtype: %#x--usrname: %s-passwd: %s.\n",msg->msgtype,msg->info.usertype,msg->info.name,msg->info.passwd);
    printf("msg->info.no :%d\t msg->info.addr %s\t msg->info.phone: %s.\n",msg->info.no,msg->info.addr,msg->info.phone);

    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
        printf("%s.\n",errmsg);
        sprintf(msg->recvmsg,"数据库修改失败！%s\n", errmsg);
    }else{
        printf("the database is updated successfully.\n");
        sprintf(msg->recvmsg, "数据库修改成功!\n");
        history_init(msg,historybuf);
    }

    send(acceptfd,msg,sizeof(MSG),0);

    printf("------%s.\n",historybuf);
}

void root_query(int acceptfd,MSG *msg){
    int i = 0,j = 0;
    char sql[DATALEN] = {0};
    char **resultp;
    int nrow,ncolumn;
    char *errmsg;

    if(msg->flags == 1){
        sprintf(sql,"select * from usrinfo where name='%s';",msg->info.name);
    }else{
        sprintf(sql,"select * from usrinfo;");
    }

    if(sqlite3_get_table(db, sql, &resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
        printf("%s.\n",errmsg);
    }else{
        printf("searching.....\n");
        printf("ncolumn :%d\tnrow :%d.\n",ncolumn,nrow);

        for(i = 0; i < ncolumn; i ++){
            printf("%-8s ",resultp[i]);
        }
        puts("");
        puts("=============================================================");

        int index = ncolumn;
        for(i = 0; i < nrow; i ++){
            printf("%s    %s     %s     %s     %s     %s     %s     %s     %s     %s     %s.\n",resultp[index+ncolumn-11],resultp[index+ncolumn-10],\
                resultp[index+ncolumn-9],resultp[index+ncolumn-8],resultp[index+ncolumn-7],resultp[index+ncolumn-6],resultp[index+ncolumn-5],\
                resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);

            sprintf(msg->recvmsg,"%s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s;",resultp[index+ncolumn-11],resultp[index+ncolumn-10],\
                resultp[index+ncolumn-9],resultp[index+ncolumn-8],resultp[index+ncolumn-7],resultp[index+ncolumn-6],resultp[index+ncolumn-5],\
                resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);
            send(acceptfd,msg,sizeof(MSG),0);
            //}
            usleep(1000);
            puts("=============================================================");
            index += ncolumn;
        }

        if(msg->flags != 1){
            strcpy(msg->recvmsg,"over*");
            send(acceptfd,msg,sizeof(MSG),0);
        }

        sqlite3_free_table(resultp);
        printf("sqlite3_get_table successfully.\n");
    }

}

void root_modify(int acceptfd,MSG *msg){
    int nrow,ncolumn;
    char *errmsg, **resultp;
    char sql[DATALEN] = {0};
    char historybuf[DATALEN] = {0};

    switch (msg->recvmsg[0])
    {
        case 'N':
            sprintf(sql,"update usrinfo set name='%s' where staffno=%d;",msg->info.name, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的用户名为%s",msg->username,msg->info.no,msg->info.name);
            break;
        case 'A':
            sprintf(sql,"update usrinfo set age=%d where staffno=%d;",msg->info.age, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的年龄为%d",msg->username,msg->info.no,msg->info.age);
            break;
        case 'P':
            sprintf(sql,"update usrinfo set phone='%s' where staffno=%d;",msg->info.phone,msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的电话为%s",msg->username,msg->info.no,msg->info.phone);
            break;
        case 'D':
            sprintf(sql,"update usrinfo set addr='%s' where staffno=%d;",msg->info.addr, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的家庭住址为%s",msg->username,msg->info.no,msg->info.addr);
            break;
        case 'W':
            sprintf(sql,"update usrinfo set work='%s' where staffno=%d;",msg->info.work, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的职位为%s",msg->username,msg->info.no,msg->info.work);
            break;
        case 'T':
            sprintf(sql,"update usrinfo set date='%s' where staffno=%d;",msg->info.date, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的入职日期为%s",msg->username,msg->info.no,msg->info.date);
            break;
        case 'L':
            sprintf(sql,"update usrinfo set level=%d where staffno=%d;",msg->info.level, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的评级为%d",msg->username,msg->info.no,msg->info.level);
            break;
        case 'S':
            sprintf(sql,"update usrinfo set salary=%.2f where staffno=%d;",msg->info.salary, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的工资为%.2f",msg->username,msg->info.no,msg->info.salary);
            break;
        case 'M':
            sprintf(sql,"update usrinfo set passwd='%s' where staffno=%d;",msg->info.passwd, msg->info.no);
            sprintf(historybuf,"%s修改工号为%d的密码为%s",msg->username,msg->info.no,msg->info.passwd);
            break;
    }

    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
        printf("%s.\n",errmsg);
        sprintf(msg->recvmsg,"数据库修改失败！%s", errmsg);
    }else{
        printf("the database is updated successfully.\n");
        sprintf(msg->recvmsg, "数据库修改成功!");
        history_init(msg,historybuf);
    }

    send(acceptfd,msg,sizeof(MSG),0);

    printf("------%s.\n",historybuf);
}

void root_adduser(int acceptfd,MSG *msg){
    char sql[DATALEN] = {0};
    char buf[DATALEN] = {0};
    char *errmsg;

    printf("%d\t %d\t %s\t %s\t %d\n %s\t %s\t %s\t %s\t %d\t %f.\n",msg->info.no,msg->info.usertype,msg->info.name,msg->info.passwd,\
        msg->info.age,msg->info.phone,msg->info.addr,msg->info.work,\
        msg->info.date,msg->info.level,msg->info.salary);

    sprintf(sql,"insert into usrinfo values(%d,%d,'%s','%s',%d,'%s','%s','%s','%s',%d,%f);",\
        msg->info.no,msg->info.usertype,msg->info.name,msg->info.passwd,\
        msg->info.age,msg->info.phone,msg->info.addr,msg->info.work,\
        msg->info.date,msg->info.level,msg->info.salary);

    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!= SQLITE_OK){
        printf("----------%s.\n",errmsg);
        strcpy(msg->recvmsg,"failed");
        send(acceptfd,msg,sizeof(MSG),0);
        return;
    }else{
        strcpy(msg->recvmsg,"OK");
        send(acceptfd,msg,sizeof(msg),0);
        printf("%s register success.\n",msg->info.name);
    }

    sprintf(buf,"管理员%s添加了%s用户",msg->username,msg->info.name);
    history_init(msg,buf);
}

void root_deluser(int acceptfd,MSG *msg){
    char sql[DATALEN] = {0};
    char buf[DATALEN] = {0};
    char *errmsg;

    printf("msg->info.no :%d\t msg->info.name: %s.\n",msg->info.no,msg->info.name);

    sprintf(sql,"delete from usrinfo where staffno=%d and name='%s';",msg->info.no,msg->info.name);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!= SQLITE_OK){
        printf("----------%s.\n",errmsg);
        strcpy(msg->recvmsg,"failed");
        send(acceptfd,msg,sizeof(MSG),0);
        return;
    }else{
        strcpy(msg->recvmsg,"OK");
        send(acceptfd,msg,sizeof(msg),0);
        printf("%s deluser %s success.\n",msg->info.name,msg->info.name);
    }

    sprintf(buf,"管理员%s删除了%s用户",msg->username,msg->info.name);
    history_init(msg,buf);
}

void root_history(int acceptfd,MSG *msg){
    char sql[DATALEN] = {0};
    char *errmsg;
    msg->flags = acceptfd;

    sprintf(sql,"select * from historyinfo;");
    if(sqlite3_exec(db,sql,history_callback,(void *)msg,&errmsg) != SQLITE_OK){
        printf("%s.\n",errmsg);
    }else{
        printf("query history record done.\n");
    }

    strcpy(msg->recvmsg,"over*");
    send(acceptfd,msg,sizeof(MSG),0);

    flags = 0;
}

void quit(int acceptfd,MSG *msg){
}









