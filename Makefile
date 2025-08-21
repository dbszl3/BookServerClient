CC = gcc
TARGETS = book_client book_server
OBJS = netprog2.o book_client.o book_server.o
all: $(TARGETS)
book_client: book_client.o netprog2.o
    $(CC) -o book_client book_client.o netprog2.o
book_client.o: book_client.c netprog2.h
    $(CC) -c book_client.c
book_server: book_server.o netprog2.o
    $(CC) -o book_server book_server.o netprog2.o
book_server.o: book_server.c netprog2.h
    $(CC) -c book_server.c
netprog2.o: netprog2.c netprog2.h
    $(CC) -c netprog2.c
clean:
    rm -f $(TARGETS) $(OBJS)
reset:
    @echo "초기화 중: 사용자, 대출 로그, 책 상태..."
    > users.txt // 각 파일의 내용을 완전히 비우고 파일은 그대로 남겨두는 명령
    > borrow_log.txt
    @awk -F'|' 'BEGIN {OFS="|"} {if (NF==3) print $$1, $$2, 1; else print $$0}' books.txt > tmp_books.txt && mv tmp_books.txt books.txt
    @echo "> books.txt"
    @echo "초기화 완료!"

// make reset     
// books.txt 는 파일을 awk 명령어를 통해 한 줄씩 읽으면서 처리하는데 
// '|' 를 필드 구분자로 설정하고 대출 상태를 모두 1 로 초기화
// 위에서 처리한 결과를 tmp_books.txt 라는 임시 파일에 저장 후
// 처리된 임시 파일을 books.txt 로 덮어쓰기 해서 실제 초기화 완료
