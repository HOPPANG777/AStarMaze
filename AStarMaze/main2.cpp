#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include <math.h>

#pragma warning(disable : 6031)

#define MAZE_SIZE 29
#define INF 1000000

//==============================================================================
// Node 구조체 - A* 알고리즘에서 사용할 노드 정보
//==============================================================================
typedef struct
{
	int x, y; // 좌표
	int g, h, f; // g: 실제비용, h: 휴리스틱, f = g + h
} Node;

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

//==============================================================================
// 최소 힙 (open list)
//==============================================================================
void pushOpen(Node node)
{
	int i = openCount++;
	openList[i] = node;
	while (i > 0)
	{
		int parent = (i - 1) / 2;
		if (openList[i].f < openList[parent].f)
		{
			Node tmp = openList[i];
			openList[i] = openList[parent];
			openList[parent] = tmp;
			i = parent;
		}
		else break;
	}
}

Node popMin()
{
	Node minNode = openList[0];
	openList[0] = openList[--openCount];
	int i = 0;
	while (true)
	{
		int left = 2 * i + 1;
		int right = 2 * i + 2;
		int smallest = i;
		if (left < openCount && openList[left].f < openList[smallest].f) smallest = left;
		if (right < openCount && openList[right].f < openList[smallest].f) smallest = right;
		if (smallest != i)
		{
			Node tmp = openList[i];
			openList[i] = openList[smallest];
			openList[smallest] = tmp;
			i = smallest;
		}
		else break;
	}
	return minNode;
}

//==============================================================================
// 유틸리티 함수
//==============================================================================
int heuristic(int x, int y, int goalX, int goalY)
{
	return abs(x - goalX) + abs(y - goalY);
}

void initializeArrays(int maze[MAZE_SIZE][MAZE_SIZE], bool closed[MAZE_SIZE][MAZE_SIZE], int gCost[MAZE_SIZE][MAZE_SIZE], int parentX[MAZE_SIZE][MAZE_SIZE], int parentY[MAZE_SIZE][MAZE_SIZE])
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

//==============================================================================
// A* 알고리즘
//==============================================================================
bool aStar(int maze[MAZE_SIZE][MAZE_SIZE], int startX, int startY, int goalX, int goalY, int parentX[MAZE_SIZE][MAZE_SIZE], int parentY[MAZE_SIZE][MAZE_SIZE], int gCost[MAZE_SIZE][MAZE_SIZE], bool closed[MAZE_SIZE][MAZE_SIZE])
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

//==============================================================================
// 경로 출력
//==============================================================================
void printMazeWithPath(int maze[MAZE_SIZE][MAZE_SIZE], int pathX[], int pathY[], int pathLength, int startX, int startY, int goalX, int goalY)
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
		display[pathX[i]][pathY[i]] = '.';
	display[startX][startY] = 'S';
	display[goalX][goalY] = 'E';

	printf("\n최종 경로:\n");
	for (int i = 0; i < MAZE_SIZE; i++)
	{
		for (int j = 0; j < MAZE_SIZE; j++)
			printf("%c ", display[i][j]);
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

	while (!(cx == startX && cy == startY))
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
	pathY[pathLength++] = startY;

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
