#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "solution.h"
#include "utility.h"

static int printSolution(int* color, int V, int fd);
 
int isSafe (int v, int** graph, int V, int* color, int c){
    int i;
    for (i = 0; i < V; i++)
        if (graph[v][i] && c == color[i])
            return 0;
    return 1;
}

static int graphColoringUtil(int** graph, int V, int m, int* color, int v)
{
    if (v == V)
        return 1;
 
    int c;
    for (c=1;c<= m;c++)
    {
        if(isSafe(v, graph, V, color, c))
        {
           color[v] = c;
 
          	if (graphColoringUtil(graph, V, m, color, v+1) == 1)
             return 1;
 
           color[v] = 0;
        }
    }
 
    return 0;
}

static int graphColoring(int** graph, int V, int m, int fd)
{
    int *color = malloc(sizeof(int)*V);
    if(color == NULL) return 0;

    int i;
    for (i = 0; i < V; i++)
       color[i] = 0;
 
    if (graphColoringUtil(graph, V, m, color, 0) == 0){
    	free(color);
    	return 0;
    }
 
    if(printSolution(color, V, fd)==0) { free(color); return 0; }
    
    free(color);
    return 1;
}
 
static int printSolution(int* color, int V, int fd)
{
	char msg[3];
	int i;

	int rt = read(fd, msg, 3);
	if(rt < 0) exit_on_error("printSolution:read");
    if(rt == 0) return 0;
    for(i=0;i<V;++i){
      int rt = sprintf(msg, "%d ", color[i]);
      rt = write(fd, msg, (rt<3 ? rt: 3));
      if(rt < 0) exit_on_error("printSolution:write");
      if(rt == 0) return 0;
    }
    
    rt = write(fd, "KK", 3);
	if(rt < 0) exit_on_error("printSolution:write:OK");
	if(rt == 0) return 0;
		
    rt = read(fd, msg, 3);
	if(rt < 0) exit_on_error("printSolution:read");
    if(rt == 0) return 0;
    
    
    return 1;
}
 
int findColoring(int in, int fd)
{
    FILE* input = fdopen(in, "r");
    if(input == NULL) return 0;

    int V;

    fscanf(input, "%d", &V);

    int** graph = malloc(sizeof(int*)*V);
    if(graph == NULL) return 0;
    
    int i, j;
    
    for(i=0;i<V;++i){
    	graph[i] = malloc(sizeof(int)*V);
    	if(graph[i] == NULL) { free(graph); return 0; }
    }

    for(i=0;i<V;++i)
    	for(j=0;j<V;++j)
    		fscanf(input, "%d", &graph[i][j]);

    int m = 5;
    if(graphColoring(graph, V, m, fd)!=1) { 
		for(i=0;i<V;++i)
    		free(graph[i]);
    	free(graph);
    return 0;
	}

    for(i=0;i<V;++i)
    	free(graph[i]);
    free(graph);

    return 1;
}
