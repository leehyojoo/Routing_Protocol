#include <stdio.h>
#include <stdlib.h>
#define MAXLINE 1000

typedef struct Node_ {
    char destination;
    int distance;
    char next;
} Node;

void init_network(FILE* topology, Node*** network, int node_cnt);
void update_shortest_paths(Node** network, int start, int current, bool* visited, int node_cnt);
void run_dijkstra(Node**network, int node_cnt);
int smallest_index(Node* network_, bool* visited, int node_cnt);
void sender_to_reciever(FILE* outputfile, FILE* messagesfile, Node** network);
void set_infinite_distance(Node*** network, int node_1, int node_2);
int change_network(FILE* changesfile, Node*** network, int node_cnt, int change);
void net_print(FILE* outputfile, Node** network, int node_cnt);

// 파일 열기 및 오류 처리
void open_files(FILE** topology, FILE** messagesfile, FILE** changesfile, FILE** outputfile, int argc, char** argv) {
    if (argc != 4) {
        printf("usage: linkstate topologyfile messagesfile changesfile\n");
        exit(0);
    }

    *topology = fopen(argv[1], "r");
    *messagesfile = fopen(argv[2], "r");
    *changesfile = fopen(argv[3], "r");
    *outputfile = fopen("output_ls.txt", "w");

    if (!*topology || !*messagesfile || !*changesfile || !*outputfile) {
        printf("Error: open input file\n");
        if (*topology) fclose(*topology);
        if (*messagesfile) fclose(*messagesfile);
        if (*changesfile) fclose(*changesfile);
        if (*outputfile) fclose(*outputfile);
        exit(0);
    }
}

// 파일 닫기
void close_files(FILE* topology, FILE* messagesfile, FILE* changesfile, FILE* outputfile) {
    fclose(outputfile);
    fclose(changesfile);
    fclose(messagesfile);
    fclose(topology);
}

void net_print(FILE* outputfile, Node** network, int node_cnt) {
    for (int i = 0; i < node_cnt; i++) {
        for (int j = 0; j < node_cnt; j++) {
            // 거리가 유효한 경우에만 출력
            if (network[i][j].distance != 999) {
                fprintf(outputfile, "%c %c %d\n", network[i][j].destination, network[i][j].next, network[i][j].distance);
            }
        }
        fprintf(outputfile, "\n"); // 각 행의 끝에 개행 문자 출력
    }
}

int main(int argc, char **argv) {
    FILE *topology, *messagesfile, *changesfile, *outputfile;
    Node** network;
    int node_cnt;
    int change = 0;

    open_files(&topology, &messagesfile, &changesfile, &outputfile, argc, argv);

    while (1) {
        rewind(topology);
        fscanf(topology, "%d\n", &node_cnt);
        init_network(topology, &network, node_cnt);
        if (change != 0) {
            if (change_network(changesfile, &network, node_cnt, change) != 0) {
                break;
            }
        }
        run_dijkstra(network, node_cnt);
        net_print(outputfile, network, node_cnt);
        sender_to_reciever(outputfile, messagesfile, network);
        change++;
    }

    printf("Complete. Output file written to output_ls.txt.\n");

    for (int i = 0; i < node_cnt; i++) {
        free(network[i]);
    }
    free(network);

    close_files(topology, messagesfile, changesfile, outputfile);

    return 0;
}

void init_network(FILE* topology, Node*** network, int node_cnt) {
    // 네트워크 메모리 할당
    *network = (Node**)malloc(sizeof(Node*) * node_cnt);
    for (int i = 0; i < node_cnt; i++) {
        (*network)[i] = (Node*)malloc(sizeof(Node) * node_cnt);
    }

     // 각 노드 초기화
    for (int i = 0; i < node_cnt; i++) {
        for (int j = 0; j < node_cnt; j++) {
            (*network)[i][j].destination = (char)('0' + j); // 목적지 노드 설정

            if (i == j) {
                // 자기 자신인 경우
                (*network)[i][j].distance = 0;
                (*network)[i][j].next = (char)('0' + i);
            } else {
                // 다른 노드인 경우
                (*network)[i][j].distance = 999; // 무한대 의미
                (*network)[i][j].next = '-';     // 연결 없음
            }
        }
    }

     // 토폴로지 파일에서 네트워크 정보 읽기
    int src, dst, distance;
    while (fscanf(topology, "%d %d %d\n", &src, &dst, &distance) != EOF) {
        (*network)[src][dst].distance = distance;        // 거리 설정
        (*network)[src][dst].next = (char)('0' + dst);   // 다음 홉 설정
        (*network)[dst][src].distance = distance;        // 양방향 거리 설정
        (*network)[dst][src].next = (char)('0' + src);   // 양방향 다음 홉 설정
    }
}

// 선택된 노드를 거쳐 갈 때 더 짧은 경로가 있는지 확인하여 업데이트
void update_shortest_paths(Node** network, int start, int current, bool* visited, int node_cnt) {
    for (int dest = 0; dest < node_cnt; dest++) {
        // 방문하지 않은 노드에 대해서만 처리
        if (!visited[dest]) {
            // 현재 노드를 거쳐 목적지로 가는 새로운 거리 계산
            int new_distance = network[start][current].distance + network[current][dest].distance;
            // 새로운 거리가 기존 거리보다 짧으면 업데이트
            if (new_distance < network[start][dest].distance) {
                network[start][dest].distance = new_distance;
                // 이전 노드를 찾아 업데이트
                int temp = current;
                while (network[start][temp].destination != network[start][temp].next) {
                    temp = (int)(network[start][temp].next - '0');
                }
                network[start][dest].next = (char)('0' + temp);
            }
        }
    }
}

// 시작 노드부터 모든 노드까지의 최단 경로 계산
void run_dijkstra(Node** network, int node_cnt) {
    // 모든 노드에 대해 반복
    for (int start = 0; start < node_cnt; start++) {
        bool visited[node_cnt] = { false }; // 방문한 노드를 기록하는 배열 초기화
        visited[start] = true; // 시작 노드를 방문한 것으로 표시

        // 현재 노드부터 모든 노드까지의 최단 경로를 계산
        for (int i = 0; i < node_cnt - 1; i++) {
            int current = smallest_index(network[start], visited, node_cnt); // 현재 노드 선택
            if (current < 0 || current >= node_cnt) continue; // 유효하지 않은 노드인 경우 건너뜀
            visited[current] = true; // 현재 노드를 방문한 것으로 표시

            // 최단 경로 업데이트 함수 호출
            update_shortest_paths(network, start, current, visited, node_cnt);
        }
    }
}

// 방문하지 않은 노드 중에서 가장 작은 거리를 가진 노드의 인덱스 반환
int smallest_index(Node* network_, bool* visited, int node_cnt) {
    int min_distance = 999; // 초기 최소 거리를 무한대로 설정
    int smallest_index = -1; // 초기 인덱스를 -1로 설정

    // 모든 노드를 확인하여 가장 작은 거리를 가진 노드의 인덱스 찾기
    for (int i = 0; i < node_cnt; i++) {
        if (network_[i].distance < min_distance && !visited[i]) {
            min_distance = network_[i].distance;
            smallest_index = i;
        }
    }

    return smallest_index;
}

void sender_to_reciever(FILE* outputfile, FILE* messagesfile, Node** network) {
    int sender, receiver;                       // 송신자와 수신자
    char message[MAXLINE];                      // 메시지 버퍼
    rewind(messagesfile);                       // 메시지 파일 포인터를 시작으로 이동

    for (;;) {
        // 송신자와 수신자 정보를 읽음
        if (fscanf(messagesfile, "%d %d ", &sender, &receiver) != 2) {
            break; // 파일 끝에 도달하면 종료
        }
        fgets(message, MAXLINE, messagesfile);  // 메시지 읽음
        fprintf(outputfile, "from %d to %d cost ", sender, receiver); // 출력 파일에 송신자와 수신자 정보 기록

        // 목적지까지의 거리가 유효한 경우
        if (network[sender][receiver].distance != 999) {
            fprintf(outputfile, "%d hops ", network[sender][receiver].distance); // 목적지까지의 홉 수 출력

            // 경로를 따라 송신자부터 수신자까지의 노드를 출력
            while (1) {
                fprintf(outputfile, "%d ", sender); // 현재 노드 출력
                // 다음 노드가 목적지라면 반복 종료
                if (receiver == ((int)(network[sender][receiver].next) - '0')) {
                    break;
                }
                sender = (int)(network[sender][receiver].next) - '0'; // 다음 노드로 이동
            }
        } else {
            fprintf(outputfile, "infinite hops unreachable "); // 목적지에 도달할 수 없는 경우 메시지 출력
        }
        fprintf(outputfile, "message %s", message); // 메시지 출력
    }
    fprintf(outputfile, "\n"); // 파일 끝에 개행 문자 추가
}

// 두 노드 사이의 거리를 무한대로 설정하는 함수
void set_infinite_distance(Node*** network, int node_1, int node_2) {
    (*network)[node_1][node_2].distance = 999;
    (*network)[node_1][node_2].next = '-';
    (*network)[node_2][node_1].distance = 999;
    (*network)[node_2][node_1].next = '-';
}

// 네트워크 변경 및 거리 업데이트 함수
int change_network(FILE* changesfile, Node*** network, int node_cnt, int change) {
    int source, destination, distance;

    rewind(changesfile);
    for (int i = 0; i < change; i++) {
        if (fscanf(changesfile, "%d %d %d\n", &source, &destination, &distance) == 3) {
            if (distance == -999) {
                // 노드 간의 거리를 무한대로 설정하고 다음 홉을 '-'로 지정
                set_infinite_distance(network, source, destination);
            } else if ((*network)[source][destination].distance > distance) {
                // 새로운 거리가 현재 거리보다 짧은 경우 거리를 업데이트
                (*network)[source][destination].distance = distance;
                (*network)[source][destination].next = (char)('0' + destination);
                (*network)[destination][source].distance = distance;
                (*network)[destination][source].next = (char)('0' + source);
            }
        } else {
            return -1; // 파일 읽기 오류 발생 시 -1 반환
        }
    }

    // 노드 간의 경로를 확인하고 특정 조건에 따라 거리를 무한대로 설정
    for (int i = 0; i < node_cnt; i++) {
        for (int j = 0; j < node_cnt; j++) {
            if (((*network)[i][j].destination - '0') == 1 && ((*network)[i][j].next - '0') == 2) {
                set_infinite_distance(network, i, j);
            }
            if (((*network)[i][j].destination - '0') == 2 && ((*network)[i][j].next - '0') == 1) {
                set_infinite_distance(network, i, j);
            }
        }
    }

    return 0; // 함수 실행 완료 시 0 반환
}