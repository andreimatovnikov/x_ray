#include "lodepng.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char* load_png_file  (const char *filename, int *width, int *height) {
	unsigned char *image = NULL;
	int error = lodepng_decode32_file(&image, width, height, filename);
	if (error) {
		printf("error %u: %s\n", error, lodepng_error_text(error));
		return NULL;
	}

	return (image);
}

void output_file(const char* filename, const unsigned char* image, unsigned int width,
	unsigned int height) {
	unsigned char* picture;
	size_t size;
	int error = lodepng_encode32(&picture, &size, image, width, height);
	if (!error) {
		lodepng_save_file(picture, size, filename);
	}
	if (error)
		printf("error %u: %s\n", error, lodepng_error_text(error));
	free(picture);
}

struct pixel { 
	char R;
	char G;
	char B; 
	char alpha;
};


void remove_noises(unsigned char* cw, int width, int height)
{
	for (int i = 0; i < 4 * width * height; i += 4)
	{
		if (cw[i + 3] < 30) cw[i + 3] = 0;
		if (cw[i + 3] > 230) cw[i + 3] = 255;
	}
}

typedef struct Node {
	struct Node* p;

	unsigned char input_a;
	double output_r;
	double output_g;
	double output_b;

	int rank;
	int size;
} mst_node;



mst_node* make_set(double R, double G, double B, unsigned char cur_alpha)
{
	mst_node* elem = (mst_node*)malloc(sizeof(mst_node));
	elem->p = (mst_node*)malloc(sizeof(mst_node));
	elem->p = elem;
	elem->output_r = R;
	elem->output_g = G;
	elem->output_b = B;
	elem->input_a = cur_alpha;
	elem->rank = 0;
	elem->size = 1;
	return elem;
}

mst_node* find_set(mst_node* elem)
{
	if (elem->p == elem)
		return elem;
	return elem->p = find_set(elem->p);
}

void union_sets(mst_node* elem1, mst_node* elem2)
{
	elem1 = find_set(elem1);
	elem2 = find_set(elem2);

	if (elem1 == elem2) return;

	if (elem1->rank < elem2->rank)
	{
		elem2->input_a = elem1->input_a;

		elem2->output_r = elem1->output_r;
		elem2->output_g = elem1->output_g;
		elem2->output_b = elem1->output_b;

		elem2->size += elem1->size;
		elem1->p = elem2;
	}
	else
	{
		elem1->input_a = elem2->input_a;

		elem1->output_r = elem2->output_r;
		elem1->output_g = elem2->output_g;
		elem1->output_b = elem2->output_b;

		elem1->size += elem2->size;
		elem2->p = elem1;
		if (elem1->rank == elem2->rank)
			++elem2->rank;
	}
}



void fill_graph(mst_node** node, int width, int height, double eps)
{
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{

			if (j - 1>= 0 && abs(node[i * width + j - 1]->input_a -
				node[i * width + j]->input_a) <= eps)
				union_sets(node[i * width + j - 1], node[i * width + j]);

			if (j + 1 < width && abs(node[i * width + j + 1]->input_a -
				node[i * width + j]->input_a) <= eps)
				union_sets(node[i * width + j + 1], node[i * width + j]);

			if (i - 1>= 0 && abs(node[(i - 1) * width + j]->input_a -
				node[i * width + j]->input_a) <= eps)
				union_sets(node[(i - 1) * width + j], node[i * width + j]);

			if (i + 1 < height && abs(node[(i + 1) * width + j]->input_a -
				node[i * width + j]->input_a) <= eps)
				union_sets(node[(i + 1) * width + j], node[i * width + j]);
		}
	}
}


int main() {

	int width = 0, height = 0;
	int k = 0;

	char *filename = "scull.png";
	char *picture = load_png_file(filename, &width, &height);

	if (picture == NULL) {
		printf("I can't read the picture %s. Error.\n", filename);
		return -1;
	}

	unsigned char* cw = malloc(4 * width * height * sizeof(unsigned char));

	for (int i = 0; i < 4 * height * width; i += 4) {
		struct pixel P;
		P.R = picture[i];
		P.G = picture[i + 1];
		P.B = picture[i + 2];
		P.alpha = picture[i + 3];

		cw[i] = 255;
		cw[i + 1] = 255;
		cw[i + 2] = 255;
		cw[i + 3] = ((P.R + P.G + P.B + P.alpha) / 4) % 256;
		k++;
	}
	remove_noises(cw, width, height);

	mst_node** nod = (mst_node**)malloc((width * height) * sizeof(mst_node*));

	unsigned char R, G, B;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			R = rand() % 256;
			G = rand() % 256;
			B = rand() % 256;
			nod[i * width + j] = make_set(R, G, B, cw[4 * (i * width + j) + 3]);
		}
	}

	fill_graph(nod, width, height, 20);

	char* output = "devided_scull.png";
	for (unsigned int i = 0; i < width * height; ++i)
	{
		mst_node* node = find_set(nod[i]);
		if (node->size  < 5)
		{
			cw[i * 4] =  0;
			cw[i * 4 + 1] = 0;
			cw[i * 4 + 2] = 0;
			cw[i * 4 + 3] = 255;
		}
		else
		{
			cw[i * 4] = node->output_r;
			cw[i * 4 + 1] = node->output_g;
			cw[i * 4 + 2] = node->output_b;
			cw[i * 4 + 3] = 255;
		}
	}

	output_file(output, cw, width, height);

	free(picture);
	free(cw);
	for (int i = 0; i < width * height; ++i)
		free(nod[i]);
	free(nod);

	return 0;
}
