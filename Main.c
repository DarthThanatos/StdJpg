#include "common.h"

DQT_List dqt_list;
Huffman_List huffman_list;
C0_components c0Components;
ScanDescription scanDesc;
MCU_list mcu_list;

bool shouldContinue = true;

#define COMP_PERM() c0Components.components[scanDesc.components_permutation[i]]

void print_bin_rek(int number) {
	if (number / 2 != 0) print_bin_rek(number / 2);
	printf("%d", number % 2);
}

void printMCUs() {
	for (MCUnode *p = mcu_list.head->next; p != NULL; p = p->next) {
		for (int k = 0; k < 4; k++) {
			printf("MCU(%d %d) Y\n", p->x, p->y);
			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < 8; j++) {
					printf("%lf ", p->YValues[k][i * 8 + j]);
				}
				printf("\n");
			}
			getchar();
		}
		for (int k = 0; k < 2; k++) {
			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < 8; j++) {
					printf("%lf ", p->CrValues[k][i * 8 + j]);
				}
				printf("\n");
			}
			getchar();
		}
	}
}

void init_DQT_list() {
	dqt_list.head = malloc(sizeof(DQT));
	dqt_list.head->next = NULL;
	dqt_list.tail = dqt_list.head;
}

void init_Huffman_list() {
	huffman_list.head = malloc(sizeof(Huffman));
	huffman_list.head->next = NULL;
	huffman_list.tail = huffman_list.head;
}

void new_DQT_node() {
	DQT *p = malloc(sizeof(DQT));
	p->next = dqt_list.tail->next;
	dqt_list.tail->next = p;
	dqt_list.tail = p;
	p->precision = p->id = p->QTable_index = 0;
}

void new_Huffman_tree_node(Huffman_tree_node** t) {
	*t = malloc(sizeof(Huffman_tree_node));
	(*t)->zero = NULL; (*t)->one = NULL;
	(*t)->value = -1;
}

void new_Huffman_node() {
	Huffman *p = malloc(sizeof(Huffman));
	p->next = huffman_list.tail->next;
	huffman_list.tail->next = p;
	huffman_list.tail = p;
	p->huffman_class = p->id = 0;
	new_Huffman_tree_node(&p->root);
}

int decodeHuffman(Huffman_tree_node* t, unsigned short code, int code_length) {
	for (int i = code_length - 1; i >= 0; i--) {
		unsigned short copy = code;
		copy >>= i;
		if ((copy & 0x0001) == 0) {
			if (t->zero == NULL) {
				return -1;
			}
			t = t->zero;
		}
		else {
			if (t->one == NULL) {
				return -1;
			}
			t = t->one;
		}
	}
	return t->value;
}

void searchHuffmanTree(int destId, int class, unsigned short code, int code_length) {
	printf("In tree [destId = %d, class = %d] ", destId, class);
	for (Huffman *p = huffman_list.head->next; p != NULL; p = p->next) {
		if (p->huffman_class == class && p->id == destId) {
			int val = decodeHuffman(p->root, code, code_length);
			if (val == -1) printf("value of %hu not found\n", code);
			else printf("value of %hu = %02X\n", code, val);
		}
	}
}

void appendToHT(Huffman_tree_node* t, unsigned short code_number, int length, int value) {
	for (int i = length - 1; i >= 0; i--) {
		unsigned short copy = code_number;
		copy >>= i;
		if ((copy & 0x0001) == 0) {
			if (t->zero == NULL) new_Huffman_tree_node(&t->zero);
			t = t->zero;
		}
		else {
			if (t->one == NULL) new_Huffman_tree_node(&t->one);
			t = t->one;
		}
	}
	t->value = value;
}

void buildHuffmanTree(Huffman *p) {
	unsigned short number = 0;
	bool zeroDispatched = false;
	int prev_size = 0;
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < p->table_sizes[i]; j++) {
			int k = p->code_lists[i][j];
			if (zeroDispatched)number++; else zeroDispatched = true;
			if (j == 0) number <<= (i + 1 - prev_size);
			appendToHT(p->root, number, i + 1, k);
		}
		if (p->table_sizes[i] != 0)
			prev_size = i + 1;
	}
}

void update_QTable(short unsigned *length, int i, int j, DQT* p, FILE *input, FILE *output) {
	p->QTable[i][j] = 0;
	fread(&p->QTable[i][j], 1, 1, input);
	p->QTable_index++;
	fprintf(output, "%02X ", p->QTable[i][j]);
	if (p->QTable_index % 8 == 0) fprintf(output, "\n");
	(*length)--;
}

void zigzag(short unsigned *length, DQT *p, FILE *input, FILE *output) {
	int i = 0, j = 0;
	update_QTable(length, i, j, p, input, output);
	do {
		if ((j + 1) == 8)i++; //if there is no move to the right
		else j++; //if there is move to the right
		update_QTable(length, i, j, p, input, output);
		while (j != 0 && i != 7) {
			j--; i++;
			update_QTable(length, i, j, p, input, output);
		}
		if ((i + 1) == 8) j++;//if there is no move down
		else i++;//if there is move down
		update_QTable(length, i, j, p, input, output);
		while (i != 0 && j != 7) {
			j++; i--;
			update_QTable(length, i, j, p, input, output);
		}
	} while (i * 8 + j != 63);
}

unsigned short parseTwoBytes(FILE* input, FILE* output) {
	unsigned short result = 0;
	unsigned short result_part = 0;
	fread(&result_part, 1, 1, input);
	result |= (result_part) << 8;
	fread(&result_part, 1, 1, input);
	result |= (result_part & 0x00ff);
	return result;
}

unsigned short getTwoByteLength(FILE* input, FILE* output) {
	unsigned short length = parseTwoBytes(input, output);
	fprintf(output, "\n%hu //length\n", length);
	return length;
}

void handleQuantizationTable(FILE* input, FILE* output) {
	unsigned short length = getTwoByteLength(input, output);
	length -= 2;
	// ^ first two bytes defining length are also included here
	//so subtraction is needed
	while (length != 0) {
		new_DQT_node();
		DQT* p = dqt_list.tail;
		unsigned char info;
		fread(&info, 1, 1, input);
		length--;
		p->precision = info & 0xf0;
		p->id = info & 0x0f;
		fprintf(output, "%02x //precision: %02x, id: %02x\n", info, p->precision, p->id);
		zigzag(&length, p, input, output);
	}
}

void parseHuffman(FILE* input, FILE* output) {
	unsigned short length = getTwoByteLength(input, output);
	unsigned char info;
	length -= 3;
	fread(&info, 1, 1, input);
	new_Huffman_node();
	Huffman* p = huffman_list.tail;
	p->huffman_class = (info & 0xf0) >> 4;
	p->id = info & 0x0f;
	for (int i = 0; i < 16; i++) {
		p->table_sizes[i] = 0;
		fread(&p->table_sizes[i], 1, 1, input);
	}
	p->general_size = length - 16;
	for (int i = 0; i < 16; i++) {
		p->code_lists[i] = malloc(sizeof(int) * p->table_sizes[i]);
		for (int j = 0; j < p->table_sizes[i]; j++) {
			p->code_lists[i][j] = 0;
			fread(&p->code_lists[i][j], 1, 1, input);
		}
	}
	fprintf(output, "%02x // dest id: %02x, class: %02x\n", info, p->id, p->huffman_class);
	for (int i = 0; i < 16; i++) {
		fprintf(output, "Codes of length %02x bits (%hu total): ", (unsigned short)(i + 1), p->table_sizes[i]);
		for (int j = 0; j < p->table_sizes[i]; j++) {
			fprintf(output, "%02X ", p->code_lists[i][j]);
		}
		fprintf(output, "\n");
	}
	fprintf(output, "Total number of codes: %hu\n", p->general_size);
	buildHuffmanTree(p);
}

void handleApp0(FILE* input, FILE* output) {
	//TO-DO
}

void handleApp1(FILE* input, FILE* output) {
	//TO-DO
}

void handleApp2(FILE* input, FILE* output) {
	//TO-DO
}

void handleComment(FILE* input, FILE* output) {
	//TO-DO
}

DQT* getQTable(int id) {
	for (DQT *p = dqt_list.head->next; p != NULL; p = p->next) {
		if (p->id == id) return p;
	}
	return NULL;
}

Huffman_tree_node* getRoot(int destId, int class) {
	for (Huffman *p = huffman_list.head->next; p != NULL; p = p->next)
		if (p->huffman_class == class && p->id == destId) return p->root;
	return NULL;
}

void scanPreparator(FILE* input, FILE* output) {
	scanDesc.number_of_components = 0;
	fread(&scanDesc.number_of_components, 1, 1, input);
	scanDesc.components_permutation = malloc(sizeof(int)*scanDesc.number_of_components);
	for (int i = 0; i < scanDesc.number_of_components; i++) {
		scanDesc.components_permutation[i] = 0;
		fread(&scanDesc.components_permutation[i], 1, 1, input);
		char entropy_tables;
		fread(&entropy_tables, 1, 1, input);
		int ac = entropy_tables & 0x0f;
		int dc = (entropy_tables >> 4) & 0x0f;
		COMP_PERM().ac_tree = getRoot(ac, 1);
		COMP_PERM().dc_tree = getRoot(dc, 0);
		COMP_PERM().quantization_table =
			getQTable(COMP_PERM().quantization_selector);
		COMP_PERM().ac = ac;
		COMP_PERM().dc = dc;
	}
	for (int i = 0; i < scanDesc.number_of_components; i++) {
		fprintf(output, "%d %d %d %d // component id, dc, ac, quantization_table\n",
			COMP_PERM().id,
			COMP_PERM().dc,
			COMP_PERM().ac,
			COMP_PERM().quantization_selector);
	}
	fread(&scanDesc.ss, 1, 1, input);
	fread(&scanDesc.se, 1, 1, input);
	fread(&scanDesc.ah_ai, 1, 1, input);
	fprintf(output, "%02X, %02X, %02X // ss, se, ah_ai\n\n", scanDesc.ss, scanDesc.se, scanDesc.ah_ai);
	c0Components.image = malloc(sizeof(int) * c0Components.max_Y);
	for (int i = 0; i<c0Components.max_Y; i++)
		c0Components.image[i] = malloc(sizeof(int) * c0Components.max_X);
}

void print_bin_padded(int number, char *end_str) {
	#define PRECISION 16
	int byte[PRECISION], r = PRECISION - 1;
	for (int i = 0; i < PRECISION; i++)byte[i] = 0;
	while (number != 0) {
		byte[r] = number % 2;
		r--;
		number /= 2;
	}
	for (int i = 0; i <= PRECISION-1; i++) printf("%d", byte[i]);
	printf("%s", end_str);
}

void fuel_word(unsigned short *word,
	int *word_content_counter,
	unsigned char* remainder,
	int *remainder_content_counter,
	FILE* input,
	FILE* output) {
	int end = 16 - *word_content_counter;
	for (int i = 0; i < end; i++) {
		if (*remainder_content_counter == 0) {
			fread(remainder, 1, 1, input);
			*remainder_content_counter = 8;
			if ((*remainder) == 0xff) {
				unsigned char redundant_zero;
				fread(&redundant_zero, 1, 1, input);
				if (redundant_zero == 0xd9) {
					*remainder = redundant_zero;
					return;
				}
			}
		}
		unsigned short transit = (*remainder & 0x80) >> 7;
		(*remainder) <<= 1;
		(*remainder_content_counter)--;
		transit <<= (15 - (*word_content_counter));
		(*word) |= transit;
		(*word_content_counter)++;
	}
	*word_content_counter = 16;
}

void initMCUs() {
	mcu_list.head = malloc(sizeof(MCUnode));
	mcu_list.head->next = NULL;
	mcu_list.tail = mcu_list.head;
}

void newMCU(int x, int y) {
	MCUnode *p = malloc(sizeof(MCUnode));
	mcu_list.tail->next = p;
	mcu_list.tail = p;
	p->next = NULL;
	p->x = x; p->y = y;
	p->current_pos = 0;
	p->current_table = (double *)p->YValues[0];
}

int zigzagOrder(int pos) {
	int zigzacTable[64];
	int m = 8;
	for (int i = 0, n = 0; i < m * 2; i++)
		for (int j = (i < m) ? 0 : i - m + 1; j <= i && j < m; j++)
			zigzacTable[n++] = (i & 1) ? j*(m - 1) + i : (i - j)*m + j;
	return zigzacTable[pos];
}

unsigned short readNBits(FILE* input, FILE* output,
	unsigned short *word, int *word_len,
	unsigned char *remainder, int *rem_len,
	int N) {

	//001111111xxxxxxx => 0000000 001111111
	// ^ 9 ; 16 - 9 = 7
	unsigned short val = (*word) >> (16 - N);
	(*word_len) = 16 -  N;
	//001111111xxxxxxx =>xxxxxxx 000000000
	(*word) <<= N;
	fuel_word(word, word_len,
		remainder, rem_len,
		input, output);
	return val;
}

bool foundCode(Huffman_tree_node* tree,
	unsigned short *word,
	int *word_content_counter,
	unsigned char* remainder,
	int *remainder_content_counter,
	FILE* input,FILE* output,
	bool *block_completed,bool calculating_dc) {
	if (mcu_list.tail->current_pos == 64) {
		*block_completed = true;
		return true;
	}
	int huffman_value;
	bool found = false;
	for (int j = 15; j >= 0; j--) {
		huffman_value = decodeHuffman(tree, *word >> j, 16 - j);
		*word_content_counter = 16 - j;
		if (huffman_value != -1) {
			found = true;
			(*word) <<= (*word_content_counter);
			(*word_content_counter) = 16 - (*word_content_counter);
			break;
		}
	}
	fuel_word(word, word_content_counter,
		remainder, remainder_content_counter,
		input, output);
	if ((*remainder) == 0xd9) {
		shouldContinue = false;
	}
	if (found) {
		unsigned short zerosAmount = (unsigned short)(huffman_value & 0xf0) >> 4;
		unsigned short value_bin_length = (unsigned short) huffman_value & 0x0f;
		unsigned short read_val = readNBits(input, output,
			word, word_content_counter,
			remainder, remainder_content_counter,
			value_bin_length);
		MCUnode *p = mcu_list.tail;
		if (huffman_value == 0 && !calculating_dc) {
				*block_completed = true;
				for (int i = p->current_pos; i < 64; i++)
					p->current_table[zigzagOrder(i)] = 0;
		}
		else {
			//read val = zzzz vvvv 
			for (int i = 0; i < zerosAmount; i++) {
				int pos = zigzagOrder(p->current_pos);
				p->current_table[pos] = 0;
				p->current_pos++;
			}
			//0000000 001111111 => 00000000 0000000 0
			unsigned short sign = read_val >> (value_bin_length - 1);
			int dct_coeff;
			if (sign == 1) dct_coeff = read_val;
			else { //0 eqals -1 in this coding
				//0000000000000001 =>len = 3 =>0000000000000100 
				unsigned short min_positive = 0x1 << (value_bin_length-1);
				//1111111111111111 => len = 3 => 0000000000000011
				unsigned short max_negative = 0xffff >> (16 - value_bin_length + 1);
				dct_coeff = -(min_positive + (max_negative - read_val));
			}
			int pos = zigzagOrder(p->current_pos);
			p->current_table[pos] = (double)dct_coeff;
			p->current_pos++;
			*block_completed = false;
		}
	}
	return found;
}

void scanImage(FILE* input, FILE* output) {
	unsigned short length = getTwoByteLength(input, output);
	scanPreparator(input, output);
	unsigned char remainder = 0;
	int word_content_counter = 16, remainder_content_counter = 0;
	unsigned short word = parseTwoBytes(input, output);
	for (int i = 0; i < scanDesc.number_of_components; i++) COMP_PERM().block_part_parsed = 0;
	initMCUs();
	for (int y = 0; y < c0Components.max_Y / 16; y++) {
		for (int x = 0; x < c0Components.max_X / 16; x++) {
			newMCU(x, y);
			MCUnode *p = mcu_list.tail;
			bool block_completed;
			for (int i = 0; i < 4; i++) {
				p->current_pos = 0;
				p->current_table = p->YValues[i];
				block_completed = false;
				if (!foundCode(c0Components.components[1].dc_tree,
					&word, &word_content_counter,
					&remainder, &remainder_content_counter,
					input, output ,&block_completed,true))
				{
					if (remainder == 0xd9) { shouldContinue = false; break; }
					else {
						printf("Value not found Ydc\n");
						exit(1);
					}
				}
				block_completed = false;
				while (!block_completed){
					if (!foundCode(c0Components.components[1].ac_tree,
						&word, &word_content_counter,
						&remainder, &remainder_content_counter,
						input, output, &block_completed,false))
					{
						if (remainder == 0xd9) { shouldContinue = false; break; }
						else {
							printf("Value not found Yac\n");
							exit(1);
						}
					}
				}
				for (int k = 0; k < 8; k++) {
					for (int j = 0; j < 8; j++) {
						int pos = zigzagOrder(k * 8 + j);
						p->current_table[k * 8 + j] *= c0Components.components[1].quantization_table->QTable[k][j];
					}
				}
			}
			for (int i = 2; i < 4; i++) {
				p->current_pos = 0;
				p->current_table = p->CrValues[i-2];
				block_completed = false;
				if (!foundCode(c0Components.components[i].dc_tree,
					&word, &word_content_counter,
					&remainder, &remainder_content_counter,
					input, output,&block_completed,true))
				{
					if (remainder == 0xd9) { shouldContinue = false; break; }
					else {
						printf("Value not found Cdc\n");
						exit(1);
					}
				}
				block_completed = false;
				while (!block_completed) {
					if (!foundCode(c0Components.components[i].ac_tree,
						&word, &word_content_counter,
						&remainder, &remainder_content_counter,
						input, output, &block_completed,false))
					{
						if (remainder == 0xd9) { shouldContinue = false; break; }
						else {
							printf("Value not found Cac\n");
							exit(1);
						}
					}
				}
				for (int k = 0; k < 8; k++) {
					for (int l = 0; l < 8; l++) {
						p->current_table[k * 8 + l] *= c0Components.components[i].quantization_table->QTable[k][l];
					}
				}
			}
		}
	}
	shouldContinue = false;
}

void handleScanComponentsC0(FILE* input, FILE* output) {
	unsigned short length = getTwoByteLength(input, output);
	c0Components.precision = 0;
	fread(&c0Components.precision, 1, 1, input);
	fprintf(output, "%02X //precision(P)\n", c0Components.precision);
	c0Components.max_Y = parseTwoBytes(input, output);
	//c0Components.max_Y = c0Components.max_Y % 8 ==0 ? c0Components.max_Y : c0Components.max_Y + ( 8 - c0Components.max_Y %8);
	//^ this is for heights different from multiple of 8
	fprintf(output, "%d // Y - max number of lines\n", c0Components.max_Y);
	c0Components.max_X = parseTwoBytes(input, output);
	//c0Components.max_X = c0Components.max_X % 8 ==0 ? c0Components.max_X : c0Components.max_X + ( 8 - c0Components.max_X %8);
	//^ this is for widths different from multiple of 8
	fprintf(output, "%d // X - max number of samples per line\n", c0Components.max_X);
	fread(&c0Components.number_of_components, 1, 1, input);
	fprintf(output, "%d // number of components\n", c0Components.number_of_components);
	c0Components.components = malloc(sizeof(Component) * (c0Components.number_of_components + 1));
	c0Components.H_max = c0Components.V_max = 0;
	for (int i = 0; i < c0Components.number_of_components; i++) {
		int id = 0;
		fread(&id, 1, 1, input);
		fprintf(output, "%d ", id);
		c0Components.components[id].id = id;
		char samplings;
		fread(&samplings, 1, 1, input);
		c0Components.components[id].horizontal_sampling = (samplings >> 4) & 0x0f;
		c0Components.components[id].vertical_sampling = samplings & 0x0f;
		if (c0Components.components[id].horizontal_sampling > c0Components.H_max)
			c0Components.H_max = c0Components.components[id].horizontal_sampling;
		if (c0Components.components[id].vertical_sampling > c0Components.V_max)
			c0Components.V_max = c0Components.components[id].vertical_sampling;
		fprintf(output, "%dx%d", c0Components.components[id].vertical_sampling, c0Components.components[id].horizontal_sampling);
		c0Components.components[id].quantization_selector = 0;
		fread(&c0Components.components[id].quantization_selector, 1, 1, input);
		fprintf(output, " %d\n", c0Components.components[id].quantization_selector);
	}
	fprintf(output, "H max: %d, V max: %d\n", c0Components.H_max, c0Components.V_max);
}

void transform_to_hex(int argc, char*argv[]) {
	FILE *input = fopen(argv[1], "rb");
	FILE *output = fopen("abc.txt", "wb");
	unsigned char byte, prev_byte = 0;
	while (shouldContinue) {
		fread(&byte, 1, 1, input);
		if (byte == 0xff) fprintf(output, "\n");
		fprintf(output, "%02X ", byte);
		if (prev_byte == 0xff) {
			switch (byte) {
			case 0xd9:
				shouldContinue = false;
				break;
			case 0xe0:
				handleApp0(input, output);
				break;
			case 0xe1:
				handleApp1(input, output);
				break;
			case 0xe2:
				handleApp2(input, output);
				break;
			case 0xfe:
				handleComment(input, output);
				break;
			case 0xdb:
				handleQuantizationTable(input, output);
				break;
			case 0xc4:
				parseHuffman(input, output);
				break;
			case 0xc0:
				handleScanComponentsC0(input, output);
				break;
			case 0xda:
				scanImage(input,output);
				break;
			default: break;
			}
		}
		prev_byte = byte;
	}
	fclose(input);
	fclose(output);
	for (DQT* p = dqt_list.head->next; p != NULL; p = p->next) {
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				printf("%02X ", p->QTable[i][j]);
			}
			printf("\n");
		}
		printf("=====\n");
	}
}

void init(int argc, char*argv[]) {
	if (argc != 2) {
		printf("Usage: program.exe file.jpg");
		getchar();
		exit(-1);
	}
	init_DQT_list();
	init_Huffman_list();
}

void test() {
	searchHuffmanTree(0, 0, 2, 3);
	searchHuffmanTree(0, 0, 2, 1);
	searchHuffmanTree(0, 1, 65506, 16);
	searchHuffmanTree(1, 1, 505, 9);
	searchHuffmanTree(1, 1, 65489, 16);
}

void interactive_test() {
	do {
		printf("Type id, class, len, code: ");
		int id, class, code = 0, len;
		char buffer[16];
		scanf("%d %d %d %s", &id, &class, &len, buffer); getchar();
		for (int i = len - 1; i >= 0; i--) {
			if (buffer[i] == '1') code += (int)pow(2, len - 1 - i);
		}
		searchHuffmanTree(id, class, code, len);
		print_bin_rek(code);
		printf("\n");
	} while (1);
}

void print_matrix(double A[8][8]) {
	for (int i = 0; i< 8; i++) {
		for (int j = 0; j < 8; j++) {
			printf("(%.3lf,%02X) ", A[i][j], (short)A[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void multiply(double Res[8][8], double M1[8][8], double M2[8][8]) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			Res[i][j] = 0;
			for (int k = 0; k< 8; k++) {
				Res[i][j] += M1[i][k] * M2[k][j];
			}
		}
	}
}

void set_U(double U[8][8]) {
	for (int j = 0; j < 8; j++) {
		U[0][j] = 0.5 * sqrt(2) / 2; //setting first row
	}
	for (int i = 1; i<8; i++) {
		for (int j = 0; j < 8; j++) {
			U[i][j] = 0.5 * cos(i*M_PI / 16 + j * 2 * i*M_PI / 16);
		}
	}
}

void set_U_transposed(double U[8][8], double U_t[8][8]) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			U_t[i][j] = U[j][i];
		}
	}
}

void partialDCT(double R[8][8], double Q[8][8]) {
	double  U[8][8], U_t[8][8], Inter[8][8];
	set_U(U);
	set_U_transposed(U, U_t);
	multiply(Inter, U_t, R);
	multiply(Q, Inter, U);
}

double **YCbCr[3];
double **RGB[3];

void calculateRelativeDCs() {
	double diff_y = 0, diff_cb=0, diff_cr=0;
	for (MCUnode *p = mcu_list.head->next; p != NULL; p = p->next) {
		for (int i = 0; i < 4; i++) {
			p->YValues[i][0] += diff_y;
			diff_y = p->YValues[i][0];
		}
		p->CrValues[0][0] += diff_cb;
		diff_cb = p->CrValues[0][0];
		p->CrValues[1][0] += diff_cr;
		diff_cr = p->CrValues[1][0];
	}
}

void IDCT() {
	double R[8][8];
	for (MCUnode *p = mcu_list.head->next; p != NULL; p = p->next) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 64; j++) {
				R[j/8][j%8] = p->YValues[i][j];
			}
			partialDCT(R,p->inversedYValues[i]);
		}
		
		for (int i = 0; i < 64; i++) {
			R[i / 8][i % 8] = p->CrValues[0][i];
		}
		partialDCT(R, p->inversedCrValues[0]);
		
		for (int i = 0; i < 64; i++) {
			R[i / 8][i % 8] = p->CrValues[1][i];
		}
		partialDCT(R, p->inversedCrValues[1]);
	}
}

void decompress() {
	calculateRelativeDCs();
	IDCT();
	for (int i = 0; i < 3; i++) {
		YCbCr[i] = malloc(sizeof(double) * c0Components.max_Y);
		RGB[i] = malloc(sizeof(double) *  c0Components.max_Y);
		for (int j = 0; j < c0Components.max_Y; j++) {
			YCbCr[i][j] = malloc(sizeof(double) * c0Components.max_X);
			RGB[i][j] = malloc(sizeof(double) *  c0Components.max_X);
		}
	}

	for (MCUnode *p = mcu_list.head->next; p != NULL; p = p->next) {
		for (int k = 0; k < 4; k++) {
			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < 8; j++) {
					YCbCr[0][(p->y) * 16 + (k / 2) * 8 + i][(p->x) * 16 + (k % 2) * 8 + j] = p-> inversedYValues[k][i][j];
				}
			}
		}
		for(int i = 0; i < 8; i++){
			for (int j = 0; j< 8; j++){
				for (int k = 0; k < 2; k++){
					for (int l =0; l < 2; l++){
						YCbCr[1][p->y*16 + 2*i + k][p->x*16 + 2*j + l] = p->inversedCrValues[0][i][j];
						YCbCr[2][p->y*16 + 2*i + k][p->x*16 + 2*j + l] = p->inversedCrValues[1][i][j];
					}
				}
			}
		}
	}
	//Red = Y + 1.402 * Cr + 128
	//Green = Y - 0.3437 * Cb - 0.7143 * Cr + 128
	//Blue = Y + 1.772 * Cb + 128
	for (int i = 0; i < c0Components.max_Y; i++) {
		for (int j = 0; j < c0Components.max_X; j++) {
			RGB[0][i][j] = min(ceil(YCbCr[0][i][j] + YCbCr[2][i][j] * 1.402 + 128), 255);
			RGB[1][i][j] = min(ceil(YCbCr[0][i][j] - 0.3437 * YCbCr[1][i][j] - 0.7143*YCbCr[2][i][j] + 128),255);
			RGB[2][i][j] = min(ceil(YCbCr[0][i][j] + 1.772 * YCbCr[1][i][j] + 128),255);
			for (int k = 0; k < 3; k++){
				RGB[k][i][j] = max(RGB[k][i][j],0);
			}
		}
	}
	FILE* res_file = fopen("result.txt", "wb");
	fprintf(res_file, "%d %d\n", c0Components.max_X, c0Components.max_Y);
	for (int k = 0; k < 3; k++) {
		for (int i = 0; i < c0Components.max_Y; i++) {
			for (int j = 0; j < c0Components.max_X; j++) {
				fprintf(res_file, "%d ",(int) RGB[k][i][j]);
			}
			fprintf(res_file, "\n");
		}
		fprintf(res_file, "\n");
	}
	fclose(res_file);
	printf("Done");
}

int main(int argc, char *argv[]) {
	init(argc, argv);
	transform_to_hex(argc, argv);
	getchar();
	decompress();
	getchar();
}