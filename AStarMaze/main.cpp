#define _CRT_SECURE_NO_WARNINGS	// scanf경고 무시

// 언어의 표준 헤더
// std -> Standard Input Output(표준입출력) 헤더
#include <stdio.h>
// C언어 유틸리티 추가 Ex) system("pause"), system("cls")
#include <stdlib.h>		
// bool 추가해주는 헤더
#include <stdbool.h>	
// C언어 수학 관련 헤더
#include <math.h>		
// 난수 생성을 위한 헤더
#include <time.h>  
// 동적 메모리 할당을 위한 헤더
#include <malloc.h>
// Sleep, system("cls") 등 사용
#include <windows.h>  

#pragma warning(disable : 6031)	// scanf반환값 경고 무시

// MAZE_SIZE: 미로의 크기를 정의 (홀수 값 권장: DFS 미로 생성에 유리함)
// 여기서는 31로 설정 (인덱스 0 ~ 30)
#define MAZE_SIZE 15

// INF: 초기 이동 비용을 나타내는 매우 큰 값. 
// 실제 경로 비용은 이 값보다 항상 작게 되어야 합니다.
#define INF 1000000

/*
 * Node 구조체
 * A* 알고리즘에서 각 셀(또는 노드)의 정보를 저장하는 구조체입니다.
 * - x, y: 이 노드의 좌표 (미로상의 위치)
 * - g: 시작점으로부터 이 노드까지 도달하는 데 실제로 소모된 비용.
 *      예를 들어, 각 칸 이동 비용이 1이면, 시작점에서 해당 노드까지의 이동 횟수를 의미합니다.
 * - h: 현재 노드에서 목표 노드까지 도달하기 위한 추정 비용 (휴리스틱 값).
 *      보통 맨해튼 거리나 유클리드 거리를 사용하며, 이 값은 실제 비용을 절대 초과해서는 안 됩니다.
 * - f: 총 비용으로 f = g + h.
 *      A* 알고리즘에서는 f 값이 낮은 노드를 우선적으로 탐색합니다.
 */
typedef struct
{
	int x;
	int y;
	int g;  // 시작점에서 현재 노드까지의 실제 이동 비용
	int h;  // 현재 노드에서 목표까지의 추정 비용 (휴리스틱)
	int f;  // 총 비용: f = g + h
} Node;

/*
 * 열린 리스트(openList)는 아직 탐색되지 않은 노드들을 저장합니다.
 * 이 예제에서는 최소 힙(min-heap)을 사용하여 관리하며,
 * f 값(총 비용)이 가장 낮은 노드를 빠르게 선택할 수 있도록 합니다.
 * openList 배열의 크기는 최대 MAZE_SIZE * MAZE_SIZE이며, openCount는 현재 저장된 노드의 개수를 나타냅니다.
 */

Node* openList = (Node*)malloc(sizeof(Node) * MAZE_SIZE * MAZE_SIZE);
static int openCount = 0;

void MoveCursorToTopLeft()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { 0, 0 };
	SetConsoleCursorPosition(hOut, pos);
}


void visualizeMaze(int maze[MAZE_SIZE][MAZE_SIZE], bool closed[MAZE_SIZE][MAZE_SIZE], int x, int y)
{
	//system("cls"); // 콘솔 클리어

	MoveCursorToTopLeft();

	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			if (i == x && j == y) printf("□ "); // 현재 탐색 중인 노드
			else if (closed[i][j]) printf("· "); // 방문한 셀
			else if (maze[i][j] == 1) printf("■ "); // 벽
			else printf("  "); // 통로
		}
		printf("\n");
	}

	Sleep(20); // 약간의 지연 (20ms)
}

void HideCursor()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hOut, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &cursorInfo);
}

// ----------------------------------------------------------------------
// 최소 힙(min-heap) 관련 함수 (열린 리스트 관리)
// ----------------------------------------------------------------------

/*
 * 함수: pushOpen
 * 목적:
 *   새로운 노드를 최소 힙에 삽입하여 열린 리스트에 추가합니다.
 * 동작:
 *   1. 노드를 배열의 마지막 인덱스에 추가합니다.
 *   2. 현재 노드와 부모 노드를 비교하면서 버블업(bubble-up) 과정을 수행합니다.
 *      - 부모의 f 값보다 현재 노드의 f 값이 작으면 두 노드를 교환합니다.
 *      - 이 과정을 현재 노드가 루트에 도달하거나 부모의 f 값이 작을 때까지 반복합니다.
 */
void pushOpen(Node node)
{
	// 배열의 마지막 위치에 노드를 삽입하고 openCount를 1 증가시킵니다.
	int i = openCount++;
	openList[i] = node;

	// 버블업: 현재 노드와 부모 노드를 비교하여 최소 힙의 성질을 유지합니다.
	while (i > 0)
	{
		int parent = (i - 1) / 2;  // 현재 노드의 부모 인덱스 계산
		if (openList[i].f < openList[parent].f)  // 만약 현재 노드의 f 값이 부모보다 작다면
		{
			// 현재 노드와 부모 노드를 교환
			Node tmp = openList[i];
			openList[i] = openList[parent];
			openList[parent] = tmp;
			i = parent;  // 인덱스를 부모 인덱스로 업데이트하여 버블업 계속 수행
		}
		else
		{
			break;  // 부모의 f 값이 현재 노드보다 작으면 힙 성질이 유지되므로 종료
		}
	}
}

/*
 * 함수: popMin
 * 목적:
 *   최소 힙에서 가장 작은 f 값을 가진 노드(루트 노드)를 제거하고 반환합니다.
 * 동작:
 *   1. 루트 노드(인덱스 0)를 저장합니다.
 *   2. 힙의 마지막 노드를 루트로 옮깁니다.
 *   3. 버블다운(Heapify-down) 과정을 수행하여 최소 힙의 성질을 복원합니다.
 *      - 현재 노드와 자식 노드들 중에서 f 값이 가장 작은 노드를 찾습니다.
 *      - 만약 자식 중 하나가 현재 노드보다 작으면, 해당 자식과 교환합니다.
 *      - 이 과정을 자식 노드가 없거나, 현재 노드가 최소일 때까지 반복합니다.
 */
Node popMin()
{
	// 루트 노드(최소값)를 minNode에 저장합니다.
	Node minNode = openList[0];
	// 마지막 노드를 루트로 옮기고, openCount를 1 감소시킵니다.
	openList[0] = openList[--openCount];
	int i = 0;

	// 버블다운: 현재 노드와 자식 노드들을 비교하여 힙 성질을 유지합니다.
	while (true)
	{
		int left = 2 * i + 1;   // 왼쪽 자식 인덱스
		int right = 2 * i + 2;  // 오른쪽 자식 인덱스
		int smallest = i;       // 현재 노드가 최소라고 가정

		// 왼쪽 자식이 존재하고, 그 f 값이 현재 노드보다 작으면 smallest를 업데이트
		if (left < openCount && openList[left].f < openList[smallest].f)
			smallest = left;
		// 오른쪽 자식도 같은 방식으로 비교
		if (right < openCount && openList[right].f < openList[smallest].f)
			smallest = right;

		// 만약 자식 중 최소가 현재 노드와 다르다면 교환
		if (smallest != i)
		{
			Node tmp = openList[i];
			openList[i] = openList[smallest];
			openList[smallest] = tmp;
			i = smallest;  // 현재 노드를 교환한 자식 노드의 인덱스로 업데이트
		}
		else
		{
			break;  // 현재 노드가 자식보다 작으면 최소 힙 성질이 유지되므로 종료
		}
	}

	// 최소값을 가진 노드를 반환합니다.
	return minNode;
}

// ----------------------------------------------------------------------
// A* 알고리즘의 핵심 원리 및 동작 과정 (자세한 주석)
// ----------------------------------------------------------------------
/*
 * A* 알고리즘은 시작점에서 목표점까지의 최단 경로를 찾기 위해 다음과 같은 요소들을 사용합니다:
 *
 * 1. g 값 (실제 비용):
 *    - 시작점에서 현재 노드까지 실제로 이동한 비용입니다.
 *    - 각 이동의 비용은 일반적으로 1로 가정되지만, 상황에 따라 다를 수 있습니다.
 *
 * 2. h 값 (휴리스틱 비용):
 *    - 현재 노드에서 목표점까지 도달하는 데 예상되는 비용입니다.
 *    - 이 예제에서는 맨해튼 거리를 사용합니다.
 *      맨해튼 거리는 두 점 간의 x 좌표 차이와 y 좌표 차이의 절대값의 합입니다.
 *    - h 값은 실제 비용보다 크지 않아야 하며, 이를 'admissible'하다고 합니다.
 *
 * 3. f 값 (총 비용):
 *    - f = g + h로 계산되며, A* 알고리즘은 이 f 값을 기준으로 노드를 탐색합니다.
 *    - f 값이 낮을수록 시작점에서 목표점까지 도달할 가능성이 높은 경로로 간주합니다.
 *
 * A* 알고리즘의 전체 흐름:
 *
 * 1. 초기화:
 *    - 시작 노드를 생성하고, 그 g 값은 0, h 값은 휴리스틱 함수를 통해 계산합니다.
 *    - 시작 노드를 열린 리스트(우선순위 큐, 여기서는 최소 힙)에 추가합니다.
 *
 * 2. 탐색 루프:
 *    - 열린 리스트가 비어있지 않은 동안 다음을 반복합니다.
 *
 *      a. 열린 리스트에서 f 값이 가장 낮은 노드를 popMin()을 통해 꺼냅니다.
 *         이 노드는 현재까지 가장 유망한 경로를 가진 노드입니다.
 *
 *      b. 꺼낸 노드가 이미 닫힌 리스트(방문한 노드)라면, 이를 무시하고 다음 노드를 선택합니다.
 *
 *      c. 현재 노드를 닫힌 리스트에 추가합니다.
 *
 *      d. 현재 노드가 목표 노드라면, 탐색을 종료하고 경로 복원 단계로 넘어갑니다.
 *
 *      e. 현재 노드의 인접 노드(상, 하, 좌, 우)를 모두 탐색합니다.
 *         - 각 인접 노드에 대해 미로 범위를 벗어나는지, 벽인지, 또는 이미 방문한 노드인지를 확인합니다.
 *         - 인접 노드까지 이동하는 데 필요한 비용(tentativeG)은 현재 노드의 g 값에 이동 비용(여기서는 1)을 더한 값입니다.
 *
 *      f. 만약 인접 노드로 가는 새로운 경로의 비용(tentativeG)이 해당 노드에 저장된 gCost보다 작다면:
 *         - gCost를 새로운 값으로 업데이트합니다.
 *         - 인접 노드의 부모를 현재 노드로 설정합니다 (경로 복원 시 사용).
 *         - 인접 노드의 h 값을 휴리스틱 함수로 계산하고, f 값을 업데이트한 후
 *           그 노드를 열린 리스트(최소 힙)에 추가합니다.
 *
 * 3. 경로 복원:
 *    - 목표 노드에 도달한 후, 각 노드가 가지고 있는 부모 정보를 역추적하여 시작점부터 목표점까지의 최단 경로를 복원합니다.
 *
 */

 // 함수: heuristic
 // 현재 좌표 (x, y)에서 목표 (goalX, goalY)까지의 맨해튼 거리를 계산하여 반환합니다.
int heuristic(int x, int y, int goalX, int goalY)
{
	return abs(x - goalX) + abs(y - goalY);
}

// 함수: initializeArrays
// 미로, 방문 여부, 이동 비용, 부모 좌표 배열을 초기 상태로 설정합니다.
void initializeArrays
(
	int maze[MAZE_SIZE][MAZE_SIZE],
	bool closed[MAZE_SIZE][MAZE_SIZE],
	int gCost[MAZE_SIZE][MAZE_SIZE],
	int parentX[MAZE_SIZE][MAZE_SIZE],
	int parentY[MAZE_SIZE][MAZE_SIZE]
)
{
	// 모든 셀에 대해
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			maze[i][j] = 0;           // 모든 셀을 기본적으로 통로(0)로 설정
			closed[i][j] = false;       // 모든 셀을 미방문 상태로 초기화
			gCost[i][j] = INF;          // 이동 비용을 INF로 초기화 (아직 경로가 없음)
			parentX[i][j] = -1;         // 부모 좌표는 없음 (-1)
			parentY[i][j] = -1;
		}
	}
}

// 함수: aStar
// A* 알고리즘을 사용하여 미로에서 최단 경로를 탐색합니다.
// 자세한 동작 설명:
//   1. 시작 노드를 초기화합니다. 시작 노드의 g 값은 0이고, h 값은 heuristic() 함수로 계산됩니다.
//      f 값은 g + h입니다. 시작 노드를 최소 힙에 pushOpen()으로 추가합니다.
//   2. 열린 리스트(최소 힙)가 비어있지 않은 동안, 다음을 반복합니다.
//      a. popMin()을 사용하여 f 값이 최소인 노드를 선택합니다.
//      b. 선택된 노드가 이미 방문되었으면 무시(continue)합니다.
//      c. 현재 노드를 방문 처리(closed[x][y] = true)합니다.
//      d. 만약 현재 노드가 목표 좌표라면, 탐색을 성공한 것이므로 true를 반환합니다.
//      e. 현재 노드의 인접한 4방향(상, 하, 좌, 우)을 순회합니다.
//         - 각 인접 노드가 미로 범위 내에 있고, 벽이 아니며, 아직 방문하지 않은 경우에만 진행합니다.
//         - tentativeG는 현재 노드에서 인접 노드로 이동하는 비용을 나타내며, 현재의 g 값에 1을 더한 값입니다.
//         - 만약 tentativeG가 인접 노드의 기존 gCost보다 작다면, 이는 더 짧은 경로임을 의미합니다.
//           따라서, 해당 노드의 gCost를 업데이트하고, 부모를 현재 노드로 설정합니다.
//           그리고, 새로운 h 값을 계산한 후 f 값을 업데이트하여 이 노드를 열린 리스트에 추가합니다.
//   3. 만약 열린 리스트가 모두 소진되어 목표에 도달하지 못하면, false를 반환합니다.
bool aStar
(
	int maze[MAZE_SIZE][MAZE_SIZE],
	int startX,
	int startY,
	int goalX,
	int goalY,
	int parentX[MAZE_SIZE][MAZE_SIZE],
	int parentY[MAZE_SIZE][MAZE_SIZE],
	int gCost[MAZE_SIZE][MAZE_SIZE],
	bool closed[MAZE_SIZE][MAZE_SIZE]
)
{
	openCount = 0;  // 열린 리스트 초기화

	// 시작점 설정: g 값 0, 휴리스틱 h 계산, f = g + h
	gCost[startX][startY] = 0;
	int h = heuristic(startX, startY, goalX, goalY);
	Node startNode = { startX, startY, 0, h, 0 + h };
	pushOpen(startNode);

	// 4방향 이동 배열: 상, 하, 좌, 우 (한 칸씩 이동)
	int dx[4] = { -1, 1, 0, 0 };
	int dy[4] = { 0, 0, -1, 1 };

	// 열린 리스트(최소 힙)에 노드가 있는 동안 반복
	while (openCount > 0)
	{
		// 최소 힙에서 f 값이 가장 낮은 노드를 선택
		Node current = popMin();
		int x = current.x, y = current.y;
		visualizeMaze(maze, closed, x, y); // popMin 직후

		// 만약 이 노드가 이미 방문되었다면 건너뜁니다.
		if (closed[x][y]) continue;

		// 현재 노드를 방문 처리
		closed[x][y] = true;

		// 목표 노드에 도달하면 성공하였으므로 true 반환
		if (x == goalX && y == goalY) return true;

		// 현재 노드의 인접 노드(상, 하, 좌, 우)를 탐색
		for (int i = 0; i < 4; i++)
		{
			int nx = x + dx[i];  // 인접 노드의 x 좌표
			int ny = y + dy[i];  // 인접 노드의 y 좌표

			// 인접 노드가 미로 범위를 벗어나면 무시
			if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
			// 인접 노드가 벽이거나 이미 방문한 경우 무시
			if (maze[nx][ny] == 1 || closed[nx][ny]) continue;

			// tentativeG: 현재 노드를 거쳐 인접 노드에 도달하는 데 필요한 비용 (현재 비용 + 이동 비용)
			int tentativeG = current.g + 1;

			// 만약 이 경로가 기존 경로보다 비용이 적다면, 업데이트합니다.
			if (tentativeG < gCost[nx][ny])
			{
				gCost[nx][ny] = tentativeG;  // 비용 업데이트
				parentX[nx][ny] = x;         // 경로 복원을 위해 부모 좌표 저장
				parentY[nx][ny] = y;
				int hCost = heuristic(nx, ny, goalX, goalY);  // 새로운 휴리스틱 값 계산
				Node neighbor = { nx, ny, tentativeG, hCost, tentativeG + hCost };
				// 이웃 노드를 열린 리스트(최소 힙)에 추가
				pushOpen(neighbor);
			}
		}
	}
	// 모든 노드를 탐색했음에도 목표에 도달하지 못하면 false 반환
	return false;
}

// ----------------------------------------------------------------------
// 미로와 경로 출력 함수
// ----------------------------------------------------------------------

/*
 * 함수: printMazeWithPath
 * 목적:
 *   미로와 A* 알고리즘으로 탐색한 경로를 시각적으로 출력합니다.
 * 동작:
 *   1. 미로 배열을 문자 배열로 변환합니다.
 *      - 벽(1)은 'X'로, 통로(0)는 '.'로 표시합니다.
 *   2. 경로 좌표 배열에 저장된 모든 셀을 '*'로 표시합니다.
 *   3. 시작점은 'S', 목표점은 'E'로 특별히 표시합니다.
 *   4. 완성된 미로를 행렬 형태로 출력합니다.
 */
void printMazeWithPath
(
	int maze[MAZE_SIZE][MAZE_SIZE],
	int pathX[],
	int pathY[],
	int pathLength,
	int startX,
	int startY,
	int goalX,
	int goalY
)
{
	char mazeDisplay[MAZE_SIZE][MAZE_SIZE];
	// 미로 배열을 문자 배열로 변환
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			// maze[i][j]가 1이면 벽('#'), 0이면 통로(' ')
			mazeDisplay[i][j] = (maze[i][j] == 1) ? (char)'#' : ' ';
		}
	}
	// 탐색한 경로 좌표를 '.' 문자로 표시
	for (int i = 0; i < pathLength; i++)
	{
		int px = pathX[i], py = pathY[i];
		mazeDisplay[px][py] = '.';
	}
	// 시작점과 목표점을 각각 'S'와 'E'로 표시
	mazeDisplay[startX][startY] = 'S';
	mazeDisplay[goalX][goalY] = 'E';

	// 완성된 미로를 출력합니다.
	printf("\n미로 경로:\n");
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			printf("%c ", mazeDisplay[i][j]);
		}
		printf("\n");
	}
}

// ----------------------------------------------------------------------
// 미로 생성 알고리즘 (DFS 방식)
// ----------------------------------------------------------------------

/*
 * 함수: carveMaze
 * 목적:
 *   DFS(깊이 우선 탐색)를 이용하여 미로 내에서 통로(0)를 생성합니다.
 * 동작 과정:
 *   1. 현재 위치 (x, y)를 통로(0)로 설정합니다.
 *   2. 4방향(아래, 위, 오른쪽, 왼쪽)의 인덱스를 담은 배열을 생성합니다.
 *   3. 이 배열을 무작위로 섞어, 이동 방향의 순서를 랜덤하게 만듭니다.
 *   4. 각 방향에 대해, 2칸 떨어진 셀 (nx, ny)이 미로 내부 경계(1 ~ MAZE_SIZE-2) 내에 있고 아직 통로가 아니라면,
 *      a. 현재 셀과 (nx, ny) 사이의 중간 셀(벽)을 제거하여 두 셀을 연결합니다.
 *      b. (nx, ny) 위치에서 재귀적으로 carveMaze()를 호출하여 DFS 탐색을 이어갑니다.
 */
void carveMaze(int x, int y, int maze[MAZE_SIZE][MAZE_SIZE])
{
	maze[x][y] = 0;  // 현재 셀을 통로(0)로 설정

	// 4방향 인덱스를 담은 배열
	int dirs[4] = { 0, 1, 2, 3 };

	// 방향 배열을 무작위로 섞음 (Fisher-Yates shuffle)
	for (int i = 0; i < 4; i++)
	{
		int r = rand() % 4;
		int temp = dirs[i];
		dirs[i] = dirs[r];
		dirs[r] = temp;
	}

	// 2칸씩 이동하기 위한 dx, dy 배열
	// 한 칸 이동 시는 벽이 위치하고, 그 다음 칸이 실제 통로가 될 위치입니다.
	int dx[4] = { 0, 0, 2, -2 };
	int dy[4] = { 2, -2, 0, 0 };

	// 무작위로 섞인 방향 순서대로 시도
	for (int i = 0; i < 4; i++)
	{
		int nx = x + dx[dirs[i]];  // 이동 후의 x 좌표
		int ny = y + dy[dirs[i]];  // 이동 후의 y 좌표

		// (nx, ny)가 미로 내부의 경계 내에 있고 아직 통로가 아니라면
		if (nx > 0 && nx < MAZE_SIZE - 1 && ny > 0 && ny < MAZE_SIZE - 1)
		{
			if (maze[nx][ny] == 1)
			{
				// 현재 셀과 (nx, ny) 사이의 중간 셀을 제거하여 두 셀을 연결 (통로 생성)
				maze[x + dx[dirs[i]] / 2][y + dy[dirs[i]] / 2] = 0;
				// (nx, ny)에서 재귀적으로 DFS 수행
				carveMaze(nx, ny, maze);
			}
		}
	}
}

/*
 * 함수: generateMazeDFS
 * 목적:
 *   DFS 방식을 이용하여 미로 전체를 생성합니다.
 * 동작 과정:
 *   1. 미로의 모든 셀을 벽(1)으로 초기화합니다.
 *   2. (1,1) 위치에서 DFS를 시작하여 carveMaze()를 호출함으로써 통로(0)를 생성합니다.
 *   3. 시작점 (1,1)과 목표점 (MAZE_SIZE-2, MAZE_SIZE-2)은 반드시 통로(0)로 설정하여,
 *      A* 알고리즘 탐색 시에 항상 접근 가능하도록 합니다.
 */
void generateMazeDFS(int maze[MAZE_SIZE][MAZE_SIZE])
{
	// 전체 미로를 벽(1)으로 초기화합니다.
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			maze[i][j] = 1;
		}
	}
	// (1,1) 위치에서 DFS를 시작하여 통로를 생성합니다.
	carveMaze(1, 1, maze);
	// 시작점과 목표점은 항상 통로로 설정
	maze[1][1] = 0;
	maze[MAZE_SIZE - 2][MAZE_SIZE - 2] = 0;
}

int main()
{
	HideCursor();

	// 난수 생성을 위한 시드 설정: 현재 시간을 기준으로 하여 매 실행마다 다른 미로를 생성합니다.
	srand((unsigned int)time(NULL));

	// 미로 및 A* 알고리즘에 사용할 보조 배열 선언 
	int (*maze)[MAZE_SIZE] = (int (*)[MAZE_SIZE])malloc(sizeof(int[MAZE_SIZE][MAZE_SIZE]));      // 미로 정보: 0은 통로, 1은 벽
	bool (*closed)[MAZE_SIZE] = (bool (*)[MAZE_SIZE])malloc(sizeof(bool[MAZE_SIZE][MAZE_SIZE]));      // A* 알고리즘에서 방문한 셀을 기록하는 배열
	int (*gCost)[MAZE_SIZE] = (int (*)[MAZE_SIZE])malloc(sizeof(int[MAZE_SIZE][MAZE_SIZE]));      // 시작점부터 각 셀까지의 최소 이동 비용을 저장
	int (*parentX)[MAZE_SIZE] = (int (*)[MAZE_SIZE])malloc(sizeof(int[MAZE_SIZE][MAZE_SIZE]));      // 각 셀의 경로 복원을 위한 부모 노드 x 좌표 저장 배열
	int (*parentY)[MAZE_SIZE] = (int (*)[MAZE_SIZE])malloc(sizeof(int[MAZE_SIZE][MAZE_SIZE]));      // 각 셀의 경로 복원을 위한 부모 노드 y 좌표 저장 배열

	// 보조 배열들을 초기화합니다.
	initializeArrays(maze, closed, gCost, parentX, parentY);

	// DFS 방식을 이용하여 미로를 생성합니다.
	generateMazeDFS(maze);

	// A* 알고리즘에서 사용할 시작점과 목표점을 설정합니다.
	// DFS 미로 생성 시 일반적으로 (1,1)과 (MAZE_SIZE-2, MAZE_SIZE-2)를 사용합니다.
	int startX = 1, startY = 1;
	int goalX = MAZE_SIZE - 2, goalY = MAZE_SIZE - 2;


	// A* 알고리즘을 실행하여 시작점부터 목표점까지의 경로를 탐색합니다.
	if (!aStar(maze, startX, startY, goalX, goalY, parentX, parentY, gCost, closed))
	{
		printf("경로를 찾을 수 없습니다.\n");

		free(maze);
		free(closed);
		free(gCost);
		free(parentX);
		free(parentY);
		free(openList);

		maze = NULL;
		closed = NULL;
		gCost = NULL;
		parentX = NULL;
		parentY = NULL;
		openList = NULL;

		return 0;
	}


	// 경로 복원 단계:
	// 목표점에서 시작점까지 부모 정보를 따라 역추적하며 경로 좌표를 pathX, pathY 배열에 저장합니다.
	int* pathX = (int*)malloc(sizeof(int) * MAZE_SIZE * MAZE_SIZE);
	int* pathY = (int*)malloc(sizeof(int) * MAZE_SIZE * MAZE_SIZE);

	int pathLength = 0;
	int cx = goalX, cy = goalY;

	while (!(cx == startX && cy == startY))
	{
		pathX[pathLength] = cx;
		pathY[pathLength] = cy;
		pathLength++;
		// 부모 노드를 이용하여 한 단계 역추적합니다.
		int tempX = parentX[cx][cy];
		int tempY = parentY[cx][cy];
		cx = tempX;
		cy = tempY;
	}
	// 시작점도 경로에 포함시키기 위해 추가합니다.
	pathX[pathLength] = startX;
	pathY[pathLength] = startY;
	pathLength++;

	// 경로 좌표를 역순으로 출력하여 시작점부터 목표점까지의 경로를 확인합니다.
	printf("찾은 경로 (총 %d 스텝):\n", pathLength);
	for (int i = pathLength - 1; i >= 0; i--)
	{
		printf("(%d, %d) ", pathX[i], pathY[i]);
	}
	printf("\n");

	// A* 탐색으로 구한 경로가 표시된 미로를 시각적으로 출력합니다.
	printMazeWithPath(maze, pathX, pathY, pathLength, startX, startY, goalX, goalY);

	// 동적 할당한 메모리를 해제합니다.
	{
		free(maze);
		free(closed);
		free(gCost);
		free(parentX);
		free(parentY);
		free(pathX);
		free(pathY);
		free(openList);

		maze = NULL;
		closed = NULL;
		gCost = NULL;
		parentX = NULL;
		parentY = NULL;
		pathX = NULL;
		pathY = NULL;
		openList = NULL;
	}

	printf("\n프로그램을 종료합니다.\n\n");

	system("pause");  // 콘솔 창이 바로 닫히지 않도록 일시 정지

	return 0;
}