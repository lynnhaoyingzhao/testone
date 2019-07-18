#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "common.h"

void show_userinfo(MSG *msg)
{
    puts("======================================================================================");
    printf("%s.\n",msg->recvmsg);
    return;
}

void do_user_query(int sockfd,MSG *msg)
{
    msg->msgtype = USER_QUERY;

    send(sockfd, msg, sizeof(MSG), 0);
    recv(sockfd, msg, sizeof(MSG), 0);
    printf("工号\t用户类型\t 姓名\t密码\t年龄\t电话\t地址\t职位\t入职年月\t等级\t 工资\n");
    show_userinfo(msg);
}

void do_user_modify(int sockfd,MSG *msg)
{
    int n = 0;
    msg->msgtype = USER_MODIFY;

    printf("请输入您要修改的工号：");
    scanf("%d", &msg->info.no);
    getchar();

    printf("***********请输入要修改的选项(其他信息亲请联系管理员)*********\n");
    printf("***********	1：家庭住址   2：电话  3：密码  4：退出***********\n");
    printf("**************************************************************\n");
    printf("选择（数字）>>");
    scanf("%d",&n);
    getchar();

    switch(n)
    {
        case 1:
            printf("家庭住址：");
            msg->recvmsg[0] = 'D';
            scanf("%s",msg->info.addr);getchar();
            break;
        case 2:
            printf("电话：");
            msg->recvmsg[0] = 'P';
            scanf("%s",msg->info.phone);getchar();
            break;
        case 3:
            printf("新密码：");
            msg->recvmsg[0] = 'M';
            scanf("%6s",msg->info.passwd);getchar();
            break;
        case 4:
            return ;
    }

    send(sockfd, msg, sizeof(MSG), 0);
    recv(sockfd, msg, sizeof(MSG), 0);
    printf("%s",msg->recvmsg);

    printf("修改结束\n");
}

void user_menu(int sockfd,MSG *msg)
{
    int n;
    while(1)
    {
        printf("*************************************************************\n");
        printf("*************  1：查询  	2：修改		3：退出	 *************\n");
        printf("*************************************************************\n");
        printf("选择（数字）>>");
        scanf("%d",&n);
        getchar();

        switch(n)
        {
            case 1:
                do_user_query(sockfd,msg);
                break;
            case 2:
                do_user_modify(sockfd,msg);
                break;
            case 3:
                msg->msgtype = QUIT;
                send(sockfd, msg, sizeof(MSG), 0);
                close(sockfd);
                exit(0);
            default:
                printf("输入有误\n");
                break;
        }
    }
}

void do_admin_deluser(int sockfd,MSG *msg)
{
    msg->msgtype = ADMIN_DELUSER;

    printf("请输入要删除的用户工号：");
    scanf("%d",&msg->info.no);
    getchar();
    printf("请输入要删除的用户名：");
    scanf("%s",msg->info.name);
    getchar();

    send(sockfd, msg, sizeof(MSG), 0);
    recv(sockfd, msg, sizeof(MSG), 0);

    if(strncmp(msg->recvmsg,"OK",2)==0)
        printf("删除成功\n");
    else
        printf("%s",msg->recvmsg);

    printf("删除工号为：%d 的用户.\n",msg->info.no);
}

void do_admin_adduser(int sockfd,MSG *msg)
{
    msg->msgtype  = ADMIN_ADDUSER;
    msg->usertype = ADMIN;
    memset(&msg->info,0,sizeof(staff_info_t));
    char temp;

    while(1){
        printf("***************热烈欢迎新员工***************.\n");
        printf("请输入工号：");
        scanf("%d",&msg->info.no);
        getchar();
        printf("您输入的工号是：%d\n",msg->info.no);
        printf("工号信息一旦录入无法更改，请确认您所输入的是否正确！(Y/N)");
        scanf("%c",&temp);
        getchar();
        if(temp == 'N' || temp == 'n'){
            printf("请重新添加用户：");
            break;
        }

        printf("请输入用户名：");
        scanf("%s",msg->info.name);
        getchar();

        printf("请输入用户密码：");
        scanf("%6s",msg->info.passwd);
        getchar();

        printf("请输入年龄：");
        scanf("%d",&msg->info.age);
        getchar();

        printf("请输入电话：");
        scanf("%s",msg->info.phone);
        getchar();

        printf("请输入家庭住址：");
        scanf("%s",msg->info.addr);
        getchar();

        printf("请输入职位：");
        scanf("%s",msg->info.work);
        getchar();

        printf("请收入入职日期(格式：0000.00.00)：");
        scanf("%s",msg->info.date);
        getchar();

        printf("请输入评级(1~5,5为最高，新员工为 1)：");
        scanf("%d",&msg->info.level);
        getchar();

        printf("请输入工资：");
        scanf("%lf",&msg->info.salary);
        getchar();

        printf("是否为管理员：(Y/N)");
        scanf("%c",&temp);
        getchar();
        if(temp == 'Y' || temp == 'y')
            msg->info.usertype = ADMIN;
        else if(temp == 'N' || temp == 'n')
            msg->info.usertype = USER;
        printf("msg->info.usertype:%d\n",msg->info.usertype);

        send(sockfd, msg, sizeof(MSG), 0);
        recv(sockfd, msg, sizeof(MSG), 0);

        if(strncmp(msg->recvmsg,"OK",2) == 0)
            printf("添加成功\n");
        else
            printf("%s",msg->recvmsg);

        printf("是否继续添加员工:(Y/N)");
        scanf("%c",&temp);
        getchar();
        if(temp == 'N' || temp == 'n')
            break;
    }

}

void do_admin_modification(int sockfd,MSG *msg)
{
    int n = 0;
    msg->msgtype = ADMIN_MODIFY;
    memset(&msg->info,0,sizeof(staff_info_t));

    send(sockfd, msg, sizeof(MSG), 0);
    recv(sockfd, msg, sizeof(MSG), 0);

    printf("请输入您要修改只认的工号：");
    scanf("%d", &msg->info.no);
    getchar();

    printf("*******************请输入要修改的选项********************\n");
    printf("******	1：姓名	  2：年龄	3：家庭住址   4：电话  ******\n");
    printf("******	5：职位	   6：工资  7：入职年月   8：评级  ******\n");
    printf("******	9：密码	 10：退出				  *******\n");
    printf("*************************************************************\n");
    printf("请输入您的选择（数字）>>");
    scanf("%d",&n);
    getchar();

    switch(n)
    {
        case 1:
            printf("请输入用户名：");
            msg->recvmsg[0] = 'N';
            scanf("%s",msg->info.name);getchar();
            break;
        case 2:
            printf("请输入年龄：");
            msg->recvmsg[0] = 'A';
            scanf("%d",&msg->info.age);getchar();
            break;
        case 3:
            printf("请输入家庭住址：");
            msg->recvmsg[0] = 'D';
            scanf("%s",msg->info.addr);getchar();
            break;
        case 4:
            printf("请输入电话：");
            msg->recvmsg[0] = 'P';
            scanf("%s",msg->info.phone);getchar();
            break;
        case 5:
            printf("请输入职位：");
            msg->recvmsg[0] = 'W';
            scanf("%s",msg->info.work);getchar();
            break;
        case 6:
            printf("请输入工资：");
            msg->recvmsg[0] = 'S';
            scanf("%lf",&msg->info.salary);getchar();
            break;
        case 7:
            printf("请输入入职日期(格式：0000.00.00.)：");
            msg->recvmsg[0] = 'T';
            scanf("%s",msg->info.date);getchar();
            break;
        case 8:
            printf("请输入评级(1~5,5为最高)：");
            msg->recvmsg[0] = 'L';
            scanf("%d",&msg->info.level);getchar();
            break;
        case 9:
            printf("请输入新密码：(6位数字)");
            msg->recvmsg[0] = 'M';
            scanf("%6s",msg->info.passwd);getchar();
            break;
        case 10:
            return ;
    }

    send(sockfd, msg, sizeof(MSG), 0);
    recv(sockfd, msg, sizeof(MSG), 0);
    printf("%s",msg->recvmsg);

    printf("修改结束.\n");

}

void do_admin_query(int sockfd,MSG *msg)
{
    int n;
    msg->msgtype = ADMIN_QUERY;

    while(1)
    {
        memset(&msg->info,0,sizeof(staff_info_t));
        if(msg->usertype == ADMIN)
        {
            printf("*************************************************************\n");
            printf("******* 1：按人名查找  	2：查找所有 	3：退出	 *******\n");
            printf("*************************************************************\n");
            printf("请输入您的选择（数字）>>");
            scanf("%d",&n);
            getchar();

            switch(n)
            {
                case 1:
                    msg->flags = 1;//按人名查找
                    break;
                case 2:
                    msg->flags = 0;//默认查找所有员工
                    break;
                case 3:
                    return;
            }
        }

        if(msg->flags == 1)
        {
            printf("请输入您要查找的用户名：");
            scanf("%s",msg->info.name);
            getchar();

            send(sockfd, msg, sizeof(MSG), 0);
            recv(sockfd, msg, sizeof(MSG), 0);
            printf("工号\t用户类型\t 姓名\t密码\t年龄\t电话\t地址\t职位\t入职年月\t等级\t 工资\n");
            show_userinfo(msg);

        }else{
            send(sockfd, msg, sizeof(MSG), 0);
            printf("工号\t用户类型\t 姓名\t密码\t年龄\t电话\t地址\t职位\t入职年月\t等级\t 工资\n");
            while (1)
            {
                recv(sockfd, msg, sizeof(MSG), 0);
                if(strncmp(msg->recvmsg , "over*",5) ==0)
                    break;
                show_userinfo(msg);
            }
        }
    }
    printf("查找结束\n");
}

void do_admin_history (int sockfd,MSG *msg)
{
    msg->msgtype = ADMIN_HISTORY;
    send(sockfd, msg, sizeof(MSG), 0);

    while(1){
        recv(sockfd, msg, sizeof(MSG), 0);
        if(strncmp(msg->recvmsg ,"over*",5) ==0)
            break;
        printf("msg->recvmsg: %s",msg->recvmsg);
    }
    printf("查询结束\n");
}

void admin_menu(int sockfd,MSG *msg)
{
    int n;

    while(1)
    {
        printf("*************************************************************\n");
        printf("* 1：查询  2：修改 3：添加用户  4：删除用户  5：查询历史记录*\n");
        printf("* 6：退出													*\n");
        printf("*************************************************************\n");
        printf("请输入您的选择（数字）>>");
        scanf("%d",&n);
        getchar();

        switch(n)
        {
            case 1:
                do_admin_query(sockfd,msg);
                break;
            case 2:
                do_admin_modification(sockfd,msg);
                break;
            case 3:
                do_admin_adduser(sockfd,msg);
                break;
            case 4:
                do_admin_deluser(sockfd,msg);
                break;
            case 5:
                do_admin_history(sockfd,msg);
                break;
            case 6:
                msg->msgtype = QUIT;
                send(sockfd, msg, sizeof(MSG), 0);
                close(sockfd);
                exit(0);
            default:
                printf("输入有误\n");
        }
    }
}

int admin_or_user_login(int sockfd,MSG *msg)
{
    memset(msg->username, 0, NAMELEN);
    printf("用户名：");
    scanf("%s",msg->username);
    getchar();

    memset(msg->passwd, 0, DATALEN);
    printf("密码：");
    scanf("%s",msg->passwd);
    getchar();

    send(sockfd, msg, sizeof(MSG), 0);
    recv(sockfd, msg, sizeof(MSG), 0);
    printf("msg->recvmsg :%s\n",msg->recvmsg);

    if(strncmp(msg->recvmsg, "OK", 2) == 0)
    {
        if(msg->usertype == ADMIN)
        {
            printf("管理员登陆\n");
            admin_menu(sockfd,msg);
        }
        else if(msg->usertype == USER)
        {
            printf("用户登陆\n");
            user_menu(sockfd,msg);
        }
    }
    else
    {
        printf("登陆失败！%s\n", msg->recvmsg);
        return -1;
    }
    return 0;
}

void do_login(int sockfd){
    int n;
    MSG msg;

    while(1){
        printf("*************************************************************\n");
        printf("********  1：管理员模式    2：普通用户模式    3：退出********\n");
        printf("*************************************************************\n");
        printf("选择（数字）>>");
        scanf("%d",&n);
        getchar();

        switch(n)
        {
            case 1:
                msg.msgtype  = ADMIN_LOGIN;
                msg.usertype = ADMIN;
                break;
            case 2:
                msg.msgtype =  USER_LOGIN;
                msg.usertype = USER;
                break;
            case 3:
                msg.msgtype = QUIT;
                close(sockfd);
            default:
                printf("输入有误\n");
        }
        admin_or_user_login(sockfd,&msg);
    }
}

int main(int argc, const char *argv[])
{
    int sockfd, acceptfd;
    ssize_t recvbytes, sendbytes;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen = sizeof(serveraddr);
    socklen_t cli_len = sizeof(clientaddr);

    sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("socket failed.\n");
        return -1;
    }

    memset(&serveraddr,0,sizeof(serveraddr));
    memset(&clientaddr,0,sizeof(clientaddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[2]));
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(sockfd,(const struct sockaddr *)&serveraddr,addrlen) < 0){
        perror("connect failed.\n");
        return -1;
    }

    do_login(sockfd);

    close(sockfd);
    return 0;
}







