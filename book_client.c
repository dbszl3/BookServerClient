#include "netprog2.h"
#include <termios.h>
#include <unistd.h>
#define MAXLINE 1024

// 비밀번호 입력 시 *로 가리고 백스페이스도 처리하는 함수
void get_password_input(char *pw, int max_len) {
    struct termios oldt, newt;
    int i = 0;
    char ch;

    // 기존 터미널 설정 저장 및 비표시 + 비정규 입력 설정
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (i < max_len - 1 && read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n') {
        if (ch == 127 || ch == '\b') {  // 백스페이스 입력
            if (i > 0) {
                i--;
                printf("\b \b"); // 커서 뒤로 이동 후 공백 출력, 다시 뒤로 이동
                fflush(stdout);
            }
        } else {
            pw[i++] = ch;
            printf("*");
            fflush(stdout);
        }
    }
    pw[i] = '\0';
    printf("\n");

    // 원래 터미널 설정 복원
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in servaddr, cliaddr;
    int s, nbyte, addrlen = sizeof(cliaddr);
    char buf[MAXLINE+1];

    if (argc != 3) {
        printf("사용법: %s ip_addr port\n", argv[0]);
        exit(0);
    }

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        errquit("socket fail");

    bzero((char*)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(s, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        errquit("connect fail");

    getsockname(s, (struct sockaddr*)&cliaddr, &addrlen);
    char IPaddr[20];
    inet_ntop(AF_INET, &cliaddr.sin_addr, IPaddr, ntohs(cliaddr.sin_port));

    // 서버 환영 메시지 출력
    while (1) {
        if ((nbyte = read(s, buf, MAXLINE)) <= 0)
            break;
        buf[nbyte] = 0;
        printf("%s", buf);
        fflush(stdout);
        if (nbyte < MAXLINE) break;
    }

    while (1) {
        printf("\n원하시는 서비스를 선택하세요 (EXIT 입력 시 종료) : ");
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        buf[strcspn(buf, "\n")] = 0;
        if (strstr(buf, "EXIT") != NULL)
            break;

        nbyte = strlen(buf);
        if (write(s, buf, nbyte) < 0)
            errquit("write fail");

        while (1) {
            nbyte = read(s, buf, MAXLINE);
            if (nbyte <= 0)
                break;

            buf[nbyte] = 0;
            printf("%s", buf);
            fflush(stdout);

            if (strstr(buf, "아이디를 입력하세요") != NULL ||
                strstr(buf, "비밀번호를 입력하세요") != NULL ||
                strstr(buf, "대출할 책의 번호를 입력해주세요") != NULL ||
                strstr(buf, "반납할 책의 번호를 입력해주세요") != NULL) {

                if (strstr(buf, "비밀번호를 입력하세요") != NULL) {
                    char pw[MAXLINE];
                    get_password_input(pw, MAXLINE);
                    nbyte = strlen(pw);
                    if (write(s, pw, nbyte) < 0)
                        errquit("write fail");
                } else {
                    if (fgets(buf, sizeof(buf), stdin) == NULL)
                        break;
                    buf[strcspn(buf, "\n")] = 0;
                    nbyte = strlen(buf);
                    if (write(s, buf, nbyte) < 0)
                        errquit("write fail");
                }
                continue;
            }

            if (nbyte < MAXLINE) break;
        }
    }

    close(s);
    return 0;
}
