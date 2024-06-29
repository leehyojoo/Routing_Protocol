#include <stdio.h>
#include <stdlib.h>
#define MAXLINE 1000

//  Struct 노드(Node) - 목적지, 거리, 다음 노드
typedef struct Node_ {
    char destination;
    int distance;
    char next;
} Node;

void process_network_changes(FILE* topology, FILE* changesfile, FILE* outputfile, FILE* messagesfile);
void initialize_network_from_topology(FILE* topology, Node*** network, int node_cnt);
int apply_changes(FILE* changesfile, Node*** network, int node_cnt, int change);
void free_network_memory(Node** network, int node_cnt);
void initnetwork(FILE* topology, Node*** network, int node_cnt);
int change_cnt_network(Node*** network, int node_cnt);
void sender_to_reciever(FILE* outputfile, FILE* messagesfile, Node** network);
void set_infinite_distance(Node*** network, int node_1, int node_2);
int change_network(FILE* changesfile, Node*** network, int node_cnt, int change);
void net_print(FILE* outputfile, Node** network, int node_cnt);

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: distvec topologyfile messagesfile changesfile\n");
        exit(0);
    }

    FILE* topology = fopen(argv[1], "r");
    FILE* messagesfile = fopen(argv[2], "r");
    FILE* changesfile = fopen(argv[3], "r");
    FILE* outputfile = fopen("output_dv.txt", "w");

    // 파일 열기 오류 처리
    if (!topology || !messagesfile || !changesfile || !outputfile) {
        printf("Error: open input file\n");
        if (topology) fclose(topology);
        if (messagesfile) fclose(messagesfile);
        if (changesfile) fclose(changesfile);
        if (outputfile) fclose(outputfile);
        exit(0);
    }

    // 네트워크 변경 사항 처리 및 결과 출력
    process_network_changes(topology, changesfile, outputfile, messagesfile);
    printf("Complete. Output file written to output_dv.txt.\n");

    fclose(outputfile);
    fclose(changesfile);
    fclose(messagesfile);
    fclose(topology);

    return 0;
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

// 네트워크 변경 사항 처리 및 결과 출력 함수
void process_network_changes(FILE* topology, FILE* changesfile, FILE* outputfile, FILE* messagesfile) {
    Node** network;
    int node_cnt;
    int change = 0;

    // 초기 파일 위치로 이동하여 노드 수를 읽음
    rewind(topology);
    fscanf(topology, "%d\n", &node_cnt);

    // 무한 반복하여 모든 변경 사항 처리
    while (1) {
        // 네트워크 초기화
        initialize_network_from_topology(topology, &network, node_cnt);

        // 변경 사항 적용
        if (change > 0 && apply_changes(changesfile, &network, node_cnt, change) != 0) {
            break;
        }

        // 네트워크 정보 교환 (Distance Vector 알고리즘)
        while (change_cnt_network(&network, node_cnt) > 0) { }

        // 현재 상태 출력 및 메시지 전달
        net_print(outputfile, network, node_cnt);
        sender_to_reciever(outputfile, messagesfile, network);

        // 다음 변경 사항으로 진행
        change++;
    }

    // 메모리 해제
    free_network_memory(network, node_cnt);
}

// 네트워크를 토폴로지 파일로부터 초기화하는 함수
void initialize_network_from_topology(FILE* topology, Node*** network, int node_cnt) {
    rewind(topology);
    fscanf(topology, "%d\n", &node_cnt);
    initnetwork(topology, network, node_cnt);
}

// 변경 사항을 적용하는 함수
int apply_changes(FILE* changesfile, Node*** network, int node_cnt, int change) {
    return change_network(changesfile, network, node_cnt, change);
}

// 네트워크 메모리를 해제하는 함수
void free_network_memory(Node** network, int node_cnt) {
    for (int i = 0; i < node_cnt; i++) {
        free(network[i]);
    }
    free(network);
}

// 네트워크 초기화 함수
void initnetwork(FILE* topology, Node*** network, int node_cnt) {
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

int change_cnt_network(Node*** network, int node_cnt) {
    int change_cnt = 0;  // 변경된 경로 수

    // 각 노드에 대해 반복
    for (int current_node = 0; current_node < node_cnt; current_node++) {
        int adjacent_count = 0;             // 인접한 노드 수
        int adjacent_nodes[node_cnt];       // 인접한 노드 목록

        // 인접한 노드 탐색
        for (int i = 0; i < node_cnt; i++) {
            // 인접 노드는 자기 자신이 아니고, 다음 홉이 존재해야 함
            if ((*network)[current_node][i].next != '-' && (*network)[current_node][i].next != (char)('0' + current_node)) {
                int is_already_added = 0;    // 노드가 이미 추가되었는지 여부
                // 이미 추가되지 않은 경우에만 추가
                for (int j = 0; j < adjacent_count; j++) {
                    if ((int)((*network)[current_node][i].next - '0') == adjacent_nodes[j]) {
                        is_already_added = 1;  // 이미 추가됨
                        break;
                    }
                }
                if (!is_already_added) {
                    adjacent_nodes[adjacent_count++] = (*network)[current_node][i].next - '0'; // 인접 노드 추가
                }
            }
        }

        // 현재 노드 업데이트
        for (int i = 0; i < adjacent_count; i++) {
            int adjacent_node = adjacent_nodes[i];   // 현재 인접한 노드

            // 목적지 노드에 대해 반복
            for (int destination = 0; destination < node_cnt; destination++) {
                // 현재 노드나 인접한 노드와 목적지가 같은 경우 무시
                if (current_node == destination || adjacent_node == destination) {
                    continue;
                }

                // 무한대 거리인 경우에는 업데이트하지 않음
                if ((*network)[current_node][destination].distance == -999 || (*network)[adjacent_node][destination].distance == -999 || (*network)[current_node][adjacent_node].distance == -999) {
                    continue;
                }
                // 새로운 경로가 더 짧으면 업데이트
                else if ((*network)[current_node][destination].distance > (*network)[adjacent_node][destination].distance + (*network)[current_node][adjacent_node].distance) {
                    (*network)[current_node][destination].distance = (*network)[adjacent_node][destination].distance + (*network)[current_node][adjacent_node].distance;
                    (*network)[current_node][destination].next = (*network)[current_node][adjacent_node].next; // 다음 홉 업데이트
                    change_cnt++;   // 변경된 경로 수 증가
                }
            }
        }
    }

    return change_cnt;  // 변경된 경로 수 반환
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