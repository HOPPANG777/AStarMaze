#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include <math.h>

#pragma warning(disable : 6031)

// MAZE_SIZE: 미로의 크기를 정의 (홀수 값 권장: DFS 미로 생성에 유리함)
// 여기서는 31로 설정 (인덱스 0 ~ 30)
#define MAZE_SIZE 29

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
//==============================================================================
// Node 구조체 - A* 알고리즘에서 사용할 노드 정보
//==============================================================================
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

static Node* openList = NULL;
static int openCount = 0;

//==============================================================================
// 커서 숨기기, 위치 이동
//==============================================================================
void HideCursor()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ci;
	GetConsoleCursorInfo(hOut, &ci);
	ci.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &ci);
}

void MoveCursorToTopLeft()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { 0, 0 };
	SetConsoleCursorPosition(hOut, pos);
}

//==============================================================================
// 시각화
//==============================================================================
void visualizeMaze(int maze[MAZE_SIZE][MAZE_SIZE], bool closed[MAZE_SIZE][MAZE_SIZE], int x, int y)
{
	MoveCursorToTopLeft();
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			if (i == x && j == y) printf("□ ");
			else if (closed[i][j]) printf("· ");
			else if (maze[i][j] == 1) printf("■ ");
			else printf("  ");
		}
		printf("\n");
	}
	Sleep(10);
}


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

//==============================================================================
// 최소 힙 (open list)
//==============================================================================
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
	Node minNode = openList[0];
	openList[0] = openList[--openCount];
	int i = 0;

	// 버블다운: 현재 노드와 자식 노드들을 비교하여 힙 성질을 유지합니다.
	while (true)
	{
		int left = 2 * i + 1;	// 왼쪽 자식 인덱스
		int right = 2 * i + 2;	// 오른쪽 자식 인덱스
		int smallest = i;		// 현재 노드가 최소라고 가정

		// 왼쪽 자식이 존재하고, 그 f 값이 현재 노드보다 작으면 smallest를 업데이트
		if (left < openCount && openList[left].f < openList[smallest].f) smallest = left;
		
		// 오른쪽 자식도 같은 방식으로 비교
		if (right < openCount && openList[right].f < openList[smallest].f) smallest = right;
		
		// 만약 자식 중 최소가 현재 노드와 다르다면 교환
		if (smallest != i)
		{
			Node tmp = openList[i];
			openList[i] = openList[smallest];
			openList[smallest] = tmp;
			i = smallest; // 현재 노드를 교환한 자식 노드의 인덱스로 업데이트
		}
		else break; // 현재 노드가 자식보다 작으면 최소 힙 성질이 유지되므로 종료
	}

	// 최소값을 가진 노드를 반환합니다.
	return minNode;
}

// 함수: heuristic
// 현재 좌표 (x, y)에서 목표 (goalX, goalY)까지의 
// 맨해튼 거리를 계산하여 반환합니다.
//==============================================
// 유틸리티 함수
//==============================================
int heuristic(int x, int y, int goalX, int goalY)
{
	return (x >= goalX ? x - goalX : goalX - x) + 
		   (y >= goalY ? y - goalY : goalY - y);
} 

void initializeArrays
(
	int maze[MAZE_SIZE][MAZE_SIZE], bool closed[MAZE_SIZE][MAZE_SIZE], 
	int gCost[MAZE_SIZE][MAZE_SIZE], 
	int parentX[MAZE_SIZE][MAZE_SIZE], int parentY[MAZE_SIZE][MAZE_SIZE]
)
{
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			maze[i][j] = 0;
			closed[i][j] = false;
			gCost[i][j] = INF;
			parentX[i][j] = -1;
			parentY[i][j] = -1;
		}
	}
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
//==============================================================================
// A* 알고리즘
//==============================================================================
bool aStar
(
	int maze[MAZE_SIZE][MAZE_SIZE], 
	int startX, int startY, 
	int goalX, int goalY, 
	int parentX[MAZE_SIZE][MAZE_SIZE], int parentY[MAZE_SIZE][MAZE_SIZE], 
	int gCost[MAZE_SIZE][MAZE_SIZE], bool closed[MAZE_SIZE][MAZE_SIZE]
)
{
	openCount = 0;
	gCost[startX][startY] = 0;
	int h = heuristic(startX, startY, goalX, goalY);
	Node startNode;
	startNode.x = startX;
	startNode.y = startY;
	startNode.g = 0;
	startNode.h = h;
	startNode.f = h;
	pushOpen(startNode);

	int dx[4] = { -1, 1, 0, 0 };
	int dy[4] = { 0, 0, -1, 1 };

	while (openCount > 0)
	{
		Node current = popMin();
		int x = current.x, y = current.y;
		visualizeMaze(maze, closed, x, y);

		if (closed[x][y]) continue;
		closed[x][y] = true;
		if (x == goalX && y == goalY) return true;

		for (int i = 0; i < 4; i++)
		{
			int nx = x + dx[i];
			int ny = y + dy[i];
			if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
			if (maze[nx][ny] == 1 || closed[nx][ny]) continue;
			int tentativeG = current.g + 1;
			if (tentativeG < gCost[nx][ny])
			{
				gCost[nx][ny] = tentativeG;
				parentX[nx][ny] = x;
				parentY[nx][ny] = y;
				int hCost = heuristic(nx, ny, goalX, goalY);
				Node neighbor;
				neighbor.x = nx;
				neighbor.y = ny;
				neighbor.g = tentativeG;
				neighbor.h = hCost;
				neighbor.f = tentativeG + hCost;
				pushOpen(neighbor);
			}
		}
	}
	return false;
}

//=========================================================
// 경로 출력
//=========================================================
void printMazeWithPath
(
	int maze[MAZE_SIZE][MAZE_SIZE], 
	int pathX[], int pathY[], int pathLength, 
	int startX, int startY, 
	int goalX, int goalY
)
{
	char display[MAZE_SIZE][MAZE_SIZE];
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			display[i][j] = (maze[i][j] == 1) ? '#' : ' ';
		}
	}
	for (int i = 0; i < pathLength; i++)
	{
		display[pathX[i]][pathY[i]] = '.';
	}
	display[startX][startY] = 'S';
	display[goalX][goalY] = 'E';

	printf("\n최종 경로:\n");
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
		{
			printf("%c ", display[i][j]);
		}
		printf("\n");
	}
}

//==============================================================================
// 메인 함수
//==============================================================================
int main()
{
	HideCursor();
	srand((unsigned int)time(NULL));

	int maze[MAZE_SIZE][MAZE_SIZE];
	bool closed[MAZE_SIZE][MAZE_SIZE];
	int gCost[MAZE_SIZE][MAZE_SIZE];
	int parentX[MAZE_SIZE][MAZE_SIZE];
	int parentY[MAZE_SIZE][MAZE_SIZE];
	openList = (Node*)malloc(sizeof(Node) * MAZE_SIZE * MAZE_SIZE);

	initializeArrays(maze, closed, gCost, parentX, parentY);

	int startX = 1, startY = 1;
	int goalX = MAZE_SIZE - 2, goalY = MAZE_SIZE - 2;

	if (!aStar(maze, startX, startY, goalX, goalY, parentX, parentY, gCost, closed))
	{
		printf("\n경로를 찾을 수 없습니다.\n");
		free(openList);
		system("pause");
		return 1;
	}

	int* pathX = (int*)malloc(sizeof(int) * MAZE_SIZE * MAZE_SIZE);
	int* pathY = (int*)malloc(sizeof(int) * MAZE_SIZE * MAZE_SIZE);
	int pathLength = 0;
	int cx = goalX, cy = goalY;

	while (cx != startX && cy != startY)
	{
		pathX[pathLength] = cx;
		pathY[pathLength] = cy;
		pathLength++;
		int tempX = parentX[cx][cy];
		int tempY = parentY[cx][cy];
		cx = tempX;
		cy = tempY;
	}
	pathX[pathLength] = startX;
	pathY[pathLength] = startY;
	pathLength++;

	printf("\n찾은 경로 (총 %d 스텝):\n", pathLength);
	for (int i = pathLength - 1; i >= 0; i--) printf("(%d, %d) ", pathX[i], pathY[i]);
	printf("\n");

	printMazeWithPath(maze, pathX, pathY, pathLength, startX, startY, goalX, goalY);

	free(pathX);
	free(pathY);
	free(openList);

	system("pause");
	return 0;
}
