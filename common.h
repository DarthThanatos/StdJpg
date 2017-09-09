#pragma once
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "math.h"

double min(double a, double b) {
    return a<b ? a : b;
}

double max(double a, double b){
	return a>b ? a:b;
}

typedef struct _DQT_node {
	unsigned char precision;
	unsigned char id;
	int QTable[8][8];
	int QTable_index;
	struct _DQT_node* next;
} DQT;

typedef struct _DQT_list {
	DQT *head;
	DQT *tail;
} DQT_List;

typedef struct _Huffman_tree_node {
	struct _Huffman_tree_node* zero;
	struct _Huffman_tree_node* one;
	int value;
} Huffman_tree_node;

typedef struct _Huffman_node {
	unsigned char huffman_class;
	unsigned char id;
	unsigned int general_size;
	int table_sizes[16];
	int *code_lists[16];
	Huffman_tree_node* root;
	struct _Huffman_node* next;
}Huffman;

typedef struct _Huffman_list {
	Huffman *head;
	Huffman *tail;
} Huffman_List;

typedef struct _comp {
	int id;
	int horizontal_sampling;
	int vertical_sampling;
	int quantization_selector, ac, dc;
	DQT *quantization_table;
	Huffman_tree_node* ac_tree;
	Huffman_tree_node* dc_tree;
	int block_part_parsed;
}Component;

typedef struct _c0_comp_ {
	int precision;
	unsigned short max_Y;
	unsigned short max_X;
	int **image;
	int H_max, V_max;
	int number_of_components;
	Component *components;
}C0_components;

typedef struct _scan_desc {
	int number_of_components;
	int *components_permutation;
	char ss, se, ah_ai;
}ScanDescription;

typedef struct _MCU_node {
	int x, y; //left-top corner
	double YValues[4][64];
	double inversedYValues[4][8][8]; //after IDCT
	double CrValues[2][64];
	double inversedCrValues[2][8][8];
	double *current_table;
	int current_pos;
	struct _MCU_node* next;
}MCUnode;

typedef struct _MCU_list {
	MCUnode *head;
	MCUnode *tail;
	struct _MCU_list* next;
}MCU_list;
