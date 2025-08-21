#include "netprog2.h"
#include <time.h>
#define MAXLINE 1024
#define MAX_BOOKS 10 // LIST 에 저장할 최대 책의 수

typedef struct {
    char id[MAXLINE];
    char pw[MAXLINE];
} User;

typedef struct {
    char title[MAXLINE];
    char author[MAXLINE];
    int is_available; // 1이면 대출 가능, 0이면 대출중
} Book;

Book books[MAX_BOOKS];
int book_count = 0;
int user_books[10];
char book_dates[10][20];

void save_user_to_file(const char *id ,const char *pw) {
    FILE *fp = fopen("users.txt", "a");
    if (fp) {
        fprintf(fp, "%s %s\n", id, pw);
        fclose(fp);
    }
}

// 회원 정보 확인
int is_valid_user(const char *id, const char *pw) {
    FILE *fp = fopen("users.txt", "r");
    char file_id[MAXLINE], file_pw[MAXLINE];
    if (!fp) return 0;

    while (fscanf(fp, "%s %s", file_id, file_pw) != EOF) {
        if (strcmp(file_id, id) == 0 && strcmp(file_pw, pw) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// 책 목록을 불러오는 함수
void load_books_from_file() {
    FILE *fp = fopen("books.txt", "r");
    if (!fp)
        errquit("books.txt 열기 실패");
    char line[MAXLINE];
    book_count = 0;

    while (fgets(line, sizeof(line), fp) != NULL && book_count < MAX_BOOKS) {
        line[strcspn(line, "\n")] = 0;
        char *title = strtok(line, "|"); char *author = strtok(NULL, "|"); char *avail = strtok(NULL, "|");

        if(!title||!author||!avail) continue;

        strncpy(books[book_count].title, title, MAXLINE);
        strncpy(books[book_count].author, author, MAXLINE);
        books[book_count].is_available = atoi(avail);

        book_count++;
    }
    fclose(fp);
}

void save_books_to_file() {
    FILE *fp = fopen("books.txt", "w");
    if (!fp) errquit ("books.txt 저장 실패");
    int i;
    for (i = 0; i < book_count; i++) {
        fprintf(fp, "%s|%s|%d\n", books[i].title, books[i].author, books[i].is_available);
    }
    fclose(fp);
}

// 오늘 날짜 구하는 함수
char *get_today_date() {
    static char date[11];
    time_t t = time(NULL);
    struct tm *tm_now = localtime(&t);
    snprintf(date, sizeof(date), "%04d-%02d-%02d", tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday);
    return date;
}

// 책을 대출할 때 텍스트 파일에 저장하는 함수
void save_borrow_log(const char *userId, int bookNum) {
    FILE *fp = fopen("borrow_log.txt", "a");
    if (fp) {
        fprintf(fp, "%s|%d|%s\n", userId, bookNum, get_today_date());
        fclose(fp);
    }
}

// 연체 여부 확인 함수
int is_overdue(const char *dateStr) {
    struct tm tm = {0};
    strptime(dateStr, "%Y-%m-%d", &tm);
    time_t borrow_time = mktime(&tm);
    time_t now = time(NULL);

    double diff = difftime(now, borrow_time);
    return (diff / (60*60*24)) > 7;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in servaddr, cliaddr;
    int listen_sock, client_sock, addrlen = sizeof(cliaddr);
    char buf[MAXLINE+1], input_id[MAXLINE], input_pw[MAXLINE];
    unsigned short client_port;

    if (argc != 2) {
        printf("사용법: %s port\n", argv[0]);
        exit(0);
    }

    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        errquit("socket fail");

    bzero((char*)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listen_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        errquit("bind fail");

    listen(listen_sock, 5);

    while(1) {
        client_sock = accept(listen_sock, (struct sockaddr*)&cliaddr, &addrlen);
        if (client_sock < 0)
            continue;

        // 클라이언트의 포트번호
        client_port = ntohs(cliaddr.sin_port);
				
				// 클라이언트 접속 정보 출력 (IP 주소는 제외하고 포트 번호만 출력)
        printf("[Client %d] 도서 대출 서버에 접속\n", client_port);

        if (fork() == 0) { // 자식 프로세스
            close(listen_sock);

            const char *welcome =
                "\n────────────────────────────────────────────────────────────────────────\n"
                "도서 대출 서버에 오신 것을 환영합니다!\n"
                "회원이면 LOGIN 비회원이면 SIGNUP 을 통해 도서 대출 서버에 접속해주세요.\n"
                "────────────────────────────────────────────────────────────────────────\n";
            write(client_sock, welcome, strlen(welcome));

            while(1) {
                int nbyte = read(client_sock, buf, MAXLINE);
                if (nbyte <= 0) break;
                buf[nbyte] = 0;

                // 줄 바꿈 문자 제거 (사용자가 입력한 명령어 끝에 '\n'이 있을 수 있음)
                buf[strcspn(buf, "\n")] = 0;

                // 클라이언트 명령어 출력 (줄 바꿈 문자 제거된 상태로 출력)
                printf("[Client %d] %s 서비스 요청\n", client_port, buf);

                if (strcmp(buf, "SIGNUP") == 0) {

                    const char *ask_id = "아이디를 입력하세요: ";
                    write(client_sock, ask_id, strlen(ask_id));
                    nbyte = read(client_sock, buf, MAXLINE);
                    if (nbyte <= 0) break;
                    buf[nbyte] = 0;
                    buf[strcspn(buf, "\n")] = 0;
                    strncpy(input_id, buf, MAXLINE);

                    const char *ask_pw = "비밀번호를 입력하세요: ";
                    write(client_sock, ask_pw, strlen(ask_pw));
                    nbyte = read(client_sock, buf, MAXLINE);
                    if (nbyte <= 0) break;
                    buf[nbyte] = 0;
                    buf[strcspn(buf, "\n")] = 0;
                    strncpy(input_pw, buf, MAXLINE);

                    save_user_to_file(input_id, input_pw);
                    const char *success_msg = "회원가입 성공!\n";
                    write(client_sock, success_msg, strlen(success_msg));
                    continue;
                }
                
                if (strcmp(buf, "LOGIN") == 0) {
                    const char *ask_id = "아이디를 입력하세요: ";
                    write(client_sock, ask_id, strlen(ask_id));
                    nbyte = read(client_sock, buf, MAXLINE);
                    if (nbyte <= 0) break;
                    buf[nbyte] = 0;
                    buf[strcspn(buf, "\n")] = 0;
                    strncpy(input_id, buf, MAXLINE);

                    const char *ask_pw = "비밀번호를 입력하세요: ";
                    write(client_sock, ask_pw, strlen(ask_pw));
                    nbyte = read(client_sock, buf, MAXLINE);
                    if (nbyte <= 0) break;
                    buf[nbyte] = 0;
                    buf[strcspn(buf, "\n")] = 0;
                    char input_pw[MAXLINE];
                    strncpy(input_pw, buf, MAXLINE);

                    if (is_valid_user(input_id, input_pw)) {
                        char full_msg[1024];
                        snprintf(full_msg, sizeof(full_msg),
                                "\n로그인 성공! [%s]님, 환영합니다!\n"
                                "──────────────────────────────────────────────────────────────\n"
                                "LIST : 책 목록, BORROW : 책 대출, RETURN : 책 반납\n"
                                "──────────────────────────────────────────────────────────────\n", input_id);
                        write(client_sock, full_msg, strlen(full_msg));
                        } else {
                            const char *fail_msg = "로그인 실패! 존재하지 않거나 잘못된 정보입니다.\n";
                            write(client_sock, fail_msg, strlen(fail_msg));
                        }
                        continue;
                    }


                if (strcmp(buf, "LIST") == 0) {
                    load_books_from_file();
                    char list_msg[2048] =
                        "──────────────────────────────────────────────────────────────\n";
                    int i;
                    for (i = 0; i < book_count; i++) {
                        char line[256];
                        snprintf(line, sizeof(line), "[%d] %s - %s - %s\n", i+1, books[i].title, books[i].author, books[i].is_available ? "대출 가능" : "대출중");
                        strncat(list_msg, line, sizeof(list_msg) - strlen(list_msg) - 1);
                    }
                    strncat(list_msg,
                            "──────────────────────────────────────────────────────────────\n", sizeof(list_msg) - strlen(list_msg) - 1);
            write(client_sock, list_msg, strlen(list_msg));
        continue;
                }

                if (strcmp(buf, "BORROW") == 0) {
                    const char *ask_num = "대출할 책의 번호를 입력해주세요 : ";
                    write(client_sock, ask_num, strlen(ask_num));

                    nbyte = read(client_sock, buf, MAXLINE);
                    if (nbyte <= 0) break;
                    buf[nbyte] = 0;
                    buf[strcspn(buf, "\n")] = 0;
                    int book_num = atoi(buf);

                    if (book_num < 1 || book_num > book_count) {
                        const char *invalid = "잘못된 번호입니다. 1~10 사이의 숫자를 입력해주세요.\n";
                        write(client_sock, invalid, strlen(invalid));
                        continue;
                    }

                    int idx = book_num - 1;
                    if (books[idx].is_available == 0) {
                        const char *msg = "해당 책은 현재 대출 중이라서 빌릴 수 없습니다.\n";
                        write(client_sock, msg, strlen(msg));
                        continue;
                    }
                    
                    books[idx].is_available = 0;
                    save_books_to_file();
                    save_borrow_log(input_id, book_num);

                    char borrow_msg[256];
                    snprintf(borrow_msg, sizeof(borrow_msg),
                            "[%s]님, [%s] 책이 대출 처리 되셨습니다. 대출 기한은 7일입니다.\n",
                            input_id, books[idx].title);
                    write(client_sock, borrow_msg, strlen(borrow_msg));
                    continue;
                }

                if (strcmp(buf, "RETURN") == 0) {
                    FILE *fp = fopen("borrow_log.txt", "r");
                    if (!fp) {
                        const char *err = "대출 기록을 불러올 수 없습니다.\n";
                        write(client_sock, err, strlen(err));
                        continue;
                    }

                    char line[128], uid[50], date[20];
                    int book_num, count = 0;

                    // 대출 리스트 로딩
                    while (fgets(line, sizeof(line), fp)) {
                        line[strcspn(line, "\n")] = 0;
                        char *token = strtok(line, "|");
                        if (!token) continue; strncpy(uid, token, sizeof(uid));
                        token = strtok(NULL, "|");
                        if (!token) continue; book_num = atoi(token);
                        token = strtok(NULL, "|");
                        if (!token) continue; strncpy(date, token, sizeof(date));

                        if (strcmp(uid, input_id) == 0) {
                            user_books[count] = book_num;
                            strncpy(book_dates[count], date, sizeof(book_dates[count]));
                            count++;
                        }
                     }
                    fclose(fp);

                    if (count == 0) {
                        const char *none = "현재 대출 중인 책이 없습니다.\n";
                        write(client_sock, none, strlen(none));
                        continue;
                    }
                    
                    char list_msg[2048] =
                        "─────────────────────────────────────────────────────────────\n";
                    snprintf(line, sizeof(line), "[%s] 님의 도서 대출 리스트\n", input_id);
                    strncat(list_msg, line, sizeof(list_msg) - strlen(list_msg) -1);

                    int i;
                    for (i = 0; i < count; i++) {
                        int idx = user_books[i] - 1;
                        snprintf(line, sizeof(line), "[%d] %s - %s - %s\n", i+1, books[idx].title, books[idx].author, book_dates[i]);
                        strncat(list_msg, line, sizeof(list_msg) - strlen(list_msg) - 1);
                    }
                    strncat(list_msg,
                            "─────────────────────────────────────────────────────────────\n"
                            "반납할 책의 번호를 입력해주세요 : ",
                            sizeof(list_msg) - strlen(list_msg) - 1);
                    write(client_sock, list_msg, strlen(list_msg));

                    nbyte = read(client_sock, buf, MAXLINE);
                    if (nbyte <= 0) break;
                    buf[nbyte] = 0;
                    buf[strcspn(buf, "\n")] = 0;
                    int sel = atoi(buf);

                    if (sel < 1 || sel > count) {
                        const char *err = "잘못된 번호입니다.\n";
                        write(client_sock, err, strlen(err));
                        continue;
                    }

                    int returnBookNum = user_books[sel - 1];
                    int idx = returnBookNum - 1;
                    books[idx].is_available = 1;
                    save_books_to_file();

                    FILE *rfp = fopen("borrow_log.txt", "r");
                    FILE *wfp = fopen("temp_log.txt", "w");
                    while (fgets(line, sizeof(line), rfp)) {
                        char tmp_uid[50], tmp_date[20];
                        int tmp_num;

                        sscanf(line, "%[^|]|%d|%s", tmp_uid, &tmp_num, tmp_date);
                        if (!(strcmp(tmp_uid, input_id) == 0 && tmp_num == returnBookNum)) {
                                fputs(line, wfp);
                        }
                    }
                    fclose(rfp);
                    fclose(wfp);
                    rename("temp_log.txt", "borrow_log.txt");

                     // 연체 여부 판단
                    int overdue = is_overdue(book_dates[sel - 1]);
                    char result_msg[256];
                    snprintf(result_msg, sizeof(result_msg),
                            "[%s]님, [%s] 책이 정상적으로 반납 처리되었습니다.\n%s"
                            , input_id, books[idx].title,
                            overdue ? "⚠ 연체되었습니다. 대출 기한은 7일입니다.\n" : "");
                    write(client_sock, result_msg, strlen(result_msg));
                    continue;
                }

                write(client_sock, buf, nbyte);
            }

            // 클라이언트 연결 종료 시 포트 번호와 함께 출력
            printf("[Client %d] 연결 종료\n", client_port);
            close(client_sock);
            exit(0);
        } else {
            close(client_sock);
        }
    }
    close(listen_sock);
    return 0;
}
