//ID:guest377
/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i ,j, ii, jj;
	int tmp1, tmp2, tmp3, tmp4;
	int deta;
	deta = 0;
	tmp1 = tmp2 = tmp3 = tmp4 = 0;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	if (M == 32 && N == 32)//32 * 32（divide to 8 * 8）
	{
		for (i = 0; i < N; i += 8)
		{
			for (j = 0; j < M; j += 8)
			{
				if (i == j)
				{
					for (ii = i; ii < i + 8; ++ii)
					{
						for (jj = j; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
				}
				else
				{
					for (ii = i; ii < i + 8; ++ii)
						for (jj = j; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
				}
			}
		}
	}
	else if (N == 64 && M == 64)//64 * 64（divide to 8 * 8）
	{
		for (i = 0; i < N; i += 8)
		{
			for (j = 0; j < N; j += 8)
			{
				if (i != j)
				{
					//左上
					for (ii = i; ii < i + 4; ++ii)
						for (jj = j; jj < j + 4; ++jj)
						{
							B[jj][ii] = A[ii][jj];
							if (ii < i + 2)
								A[ii][jj] = A[ii + 2][jj + 4];
						}
					//左下
					for (ii = i + 4; ii < i + 8; ++ii)
						for (jj = j; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
					//右下
					for (ii = i + 4; ii < i + 8; ++ii)
						for (jj = j + 4; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
					//右上
					for (ii = i; ii < i + 2; ++ii)
					{
						for (jj = j; jj < j + 4; ++jj)
							B[jj + 4][ii + 2] = A[ii][jj];
						for (jj = j + 4; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
					}
				}
				else
				{
					tmp1 = A[i][i + 4];
					tmp2 = A[i][i + 5];
					tmp3 = A[i][i + 6];
					tmp4 = A[i][i + 7];
					//左上
					for (ii = i; ii < i + 4; ++ii)
					{
						for (jj = j; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//左下
					for (ii = i + 4; ii < i + 8; ++ii)
					{
						for (jj = j; jj < ii - 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii - 3; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii - 4][ii] = A[ii][ii - 4];
					}
					//右下
					for (ii = i + 4; ii < i + 8; ++ii)
					{
						for (jj = j + 4; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//右上
					B[i + 4][i] = tmp1;
					B[i + 5][i] = tmp2;
					B[i + 6][i] = tmp3;
					B[i + 7][i] = tmp4;
					for (ii = i + 1; ii < i + 4; ++ii)
					{
						for (jj = j + 4; jj < ii + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 5; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii + 4][ii] = A[ii][ii + 4];
					}
				}
			}
		}
	}
	else if (M == 61 && N == 67)//61 * 67（divide to 8 * 8 by a offset—deta）
	{
		for (i = 0; i < 64; i += 8)
		{
			for (j = 0; j < 48; j += 8)
			{
				for (ii = i; ii < i + 8; ++ii)
				{
					for (jj = j + deta; jj < j + deta + 8; ++jj)
						B[jj][ii] = A[ii][jj];
					deta = (deta + 3) % 8;
				}
			}
			for (ii = i; ii < i + 8; ++ii)
			{
				for (jj = 0; jj < deta; ++jj)
					B[jj][ii] = A[ii][jj];
				for (jj = 61 - (13 - deta); jj < 61; ++jj)
					B[jj][ii] = A[ii][jj];
				deta = (deta + 3) % 8;
			}
		}
		//last section of matrix
		for (j = 0; j < 56; j += 8)
			for (ii = 64; ii < 67; ++ii)
				for (jj = j; jj < j + 8; ++jj)
					B[jj][ii] = A[ii][jj];
		for (ii = 64; ii < 67; ++ii)
			for (jj = 56; jj < 61; ++ jj)
				B[jj][ii] = A[ii][jj];
	}
    ENSURES(is_transpose(M, N, A, B));
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 
char trans1_desc[] = "Just for a test";
void trans1(int M, int N, int A[N][M], int B[M][N])
{
    int i ,j;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	
	for (i = 0; i < N; ++i)
		for (j = i + 1; j < M; ++j)
			B[j][i] = A[i][j];
	for (i = 0; i < N; ++i)
		for (j = 0; j < i; ++j)
			B[j][i] = A[i][j];
	for (i = 0; i < N; ++i)
		B[i][i] = A[i][i];

    ENSURES(is_transpose(M, N, A, B));
}

char trans2_desc[] = "<<<32 * 32>>>";
void trans2(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i ,j, block;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	if (M == 32 && N == 32)
	{
		block = 8;
		for (i = 0; i < N; i += block)
		{
			for (j = 0; j < M; j += block)
			{
				if (i == j)
				{
					for (ii = i; ii < i + block; ++ii)
					{
						for (jj = j; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + block; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
				}
				else
				{
					for (ii = i; ii < i + block; ++ii)
						for (jj = j; jj < j + block; ++jj)
							B[jj][ii] = A[ii][jj];
				}
			}
		}
	}

    ENSURES(is_transpose(M, N, A, B));
}

char trans3_desc[] = "<<<64 * 64>>V0>>";
void trans3(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i ,j, block;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	if (M == 64 && N == 64)
	{
		block = 8;
		for (i = 0; i < N; i += block)
		{
			for (j = 0; j < N; j += block)
			{
				for (ii = i; ii < i + 4; ++ii)
					for (jj = j; jj < j + 4; ++jj)
						B[jj][ii] = A[ii][jj];
				for (ii = i; ii < i + 4; ++ii)
					for (jj = j + 4; jj < j + block; ++jj)
						B[jj][ii] = A[ii][jj];
				for (ii = i + 4; ii < i + block; ++ii)
					for (jj = j + 4; jj < j + block; ++jj)
						B[jj][ii] = A[ii][jj];
				for (ii = i + 4; ii < i + block; ++ii)
					for (jj = j; jj < j + 4; ++jj)
						B[jj][ii] = A[ii][jj];
			}
		}
	}

    ENSURES(is_transpose(M, N, A, B));
}

char trans4_desc[] = "<<<64 * 64>>V1>>";
void trans4(int M, int N, int A[N][M], int B[M][N])
{
   int ii, jj, i ,j, block;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	if (M == 64 && N == 64)
	{
		block = 8;
		for (i = 0; i < N; i += block)
		{
			for (j = 0; j < N; j += block)
			{
				if (i != j)
				{
					//左上
					for (ii = i; ii < i + 4; ++ii)
						for (jj = j; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
					//右上
					for (ii = i; ii < i + 4; ++ii)
						for (jj = j + 4; jj < j + block; ++jj)
							B[jj][ii] = A[ii][jj];
					//右下
					for (ii = i + 4; ii < i + block; ++ii)
						for (jj = j + 4; jj < j + block; ++jj)
							B[jj][ii] = A[ii][jj];
					//左下
					for (ii = i + 4; ii < i + block; ++ii)
						for (jj = j; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
				}
				else
				{
					//左上
					for (ii = i; ii < i + 4; ++ii)
					{
						for (jj = j; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//左下
					for (ii = i + 4; ii < i + block; ++ii)
					{
						for (jj = j; jj < ii - 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii - 3; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii - 4][ii] = A[ii][ii - 4];
					}
					//右下
					for (ii = i + 4; ii < i + block; ++ii)
					{
						for (jj = j + 4; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + block; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//右上
					for (ii = i; ii < i + 4; ++ii)
					{
						for (jj = j + 4; jj < ii + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 5; jj < j + block; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii + 4][ii] = A[ii][ii + 4];
					}
				}
			}
		}
	}

    ENSURES(is_transpose(M, N, A, B));
}

char trans5_desc[] = "<<<64 * 64>>V2>>";
void trans5(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i ,j;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	if (M == 64 && N == 64)
	{
		for (i = 0; i < N; i += 8)
		{
			for (j = 0; j < N; j += 8)
			{
				if (i != j)
				{
					//左上
					for (ii = i; ii < i + 4; ++ii)
						for (jj = j; jj < j + 4; ++jj)
						{
							B[jj][ii] = A[ii][jj];
							if (ii < i + 2)
								A[ii][jj] = A[ii + 2][jj + 4];
						}
					//左下
					for (ii = i + 4; ii < i + 8; ++ii)
						for (jj = j; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
					//右下
					for (ii = i + 4; ii < i + 8; ++ii)
						for (jj = j + 4; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
					//右上
					for (ii = i; ii < i + 2; ++ii)
					{
						for (jj = j; jj < j + 4; ++jj)
							B[jj + 4][ii + 2] = A[ii][jj];
						for (jj = j + 4; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
					}
				}
				else
				{
					//左上
					for (ii = i; ii < i + 4; ++ii)
					{
						for (jj = j; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//左下
					for (ii = i + 4; ii < i + 8; ++ii)
					{
						for (jj = j; jj < ii - 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii - 3; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii - 4][ii] = A[ii][ii - 4];
					}
					//右下
					for (ii = i + 4; ii < i + 8; ++ii)
					{
						for (jj = j + 4; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//右上
					for (ii = i; ii < i + 4; ++ii)
					{
						for (jj = j + 4; jj < ii + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 5; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii + 4][ii] = A[ii][ii + 4];
					}
				}
			}
		}
	}
    ENSURES(is_transpose(M, N, A, B));
}

char trans6_desc[] = "<<<64 * 64>>V3>>";
void trans6(int M, int N, int A[N][M], int B[M][N])
{
	int ii, jj, i ,j;
	int tmp1, tmp2, tmp3, tmp4;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	if (M == 64 && N == 64)
	{
		for (i = 0; i < N; i += 8)
		{
			for (j = 0; j < N; j += 8)
			{
				if (i != j)
				{
					//左上
					for (ii = i; ii < i + 4; ++ii)
						for (jj = j; jj < j + 4; ++jj)
						{
							B[jj][ii] = A[ii][jj];
							if (ii < i + 2)
								A[ii][jj] = A[ii + 2][jj + 4];
						}
					//左下
					for (ii = i + 4; ii < i + 8; ++ii)
						for (jj = j; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
					//右下
					for (ii = i + 4; ii < i + 8; ++ii)
						for (jj = j + 4; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
					//右上
					for (ii = i; ii < i + 2; ++ii)
					{
						for (jj = j; jj < j + 4; ++jj)
							B[jj + 4][ii + 2] = A[ii][jj];
						for (jj = j + 4; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
					}
				}
				else
				{
					tmp1 = A[i][i + 4];
					tmp2 = A[i][i + 5];
					tmp3 = A[i][i + 6];
					tmp4 = A[i][i + 7];
					//左上
					for (ii = i; ii < i + 4; ++ii)
					{
						for (jj = j; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//左下
					for (ii = i + 4; ii < i + 8; ++ii)
					{
						for (jj = j; jj < ii - 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii - 3; jj < j + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii - 4][ii] = A[ii][ii - 4];
					}
					//右下
					for (ii = i + 4; ii < i + 8; ++ii)
					{
						for (jj = j + 4; jj < ii; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 1; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii][ii] = A[ii][ii];
					}
					//右上
					B[i + 4][i] = tmp1;
					B[i + 5][i] = tmp2;
					B[i + 6][i] = tmp3;
					B[i + 7][i] = tmp4;
					for (ii = i + 1; ii < i + 4; ++ii)
					{
						for (jj = j + 4; jj < ii + 4; ++jj)
							B[jj][ii] = A[ii][jj];
						for (jj = ii + 5; jj < j + 8; ++jj)
							B[jj][ii] = A[ii][jj];
						B[ii + 4][ii] = A[ii][ii + 4];
					}
				}
			}
		}
	}
    ENSURES(is_transpose(M, N, A, B));
}

char trans7_desc[] = "<<<61 * 67>>V0>>";
void trans7(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	for (i = 0; i < N; ++i)
		for (j = 0; j < i; ++j)
			B[j][i] = A[i][j];
	for (i = 0; i < N; ++i)
		for (j = i + 1; j < M; ++j)
			B[j][i] = A[i][j];
	for (i = 0; i < N; ++i)
		B[i][i] = A[i][i];
	ENSURES(is_transpose(M, N, A, B));
}

char trans8_desc[] = "<<<61 * 67>>V1>>";
void trans8(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	for (i = 0; i < M; ++i)
		for (j = 0; j < N; ++j)
			B[i][j] = A[j][i];

    ENSURES(is_transpose(M, N, A, B));
}

char trans9_desc[] = "<<<61 * 67>>V2>>";
void trans9(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;
	int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	i = 0;
	while (i < N * M)
	{
		j = i;
		while (j < N * M && j - i <= 7)
		{
			switch (j - i)
			{
				case 0:tmp1 = A[j / M][j % M];break;
				case 1:tmp2 = A[j / M][j % M];break;
				case 2:tmp3 = A[j / M][j % M];break;
				case 3:tmp4 = A[j / M][j % M];break;
				case 4:tmp5 = A[j / M][j % M];break;
				case 5:tmp6 = A[j / M][j % M];break;
				case 6:tmp7 = A[j / M][j % M];break;
				case 7:tmp8 = A[j / M][j % M];break;
			}
			++j;
		}
		--j;
		while (i <= j)
		{
			switch (j - i)
			{
				case 7:B[i % M][i / M] = tmp1;break;
				case 6:B[i % M][i / M] = tmp2;break;
				case 5:B[i % M][i / M] = tmp3;break;
				case 4:B[i % M][i / M] = tmp4;break;
				case 3:B[i % M][i / M] = tmp5;break;
				case 2:B[i % M][i / M] = tmp6;break;
				case 1:B[i % M][i / M] = tmp7;break;
				case 0:B[i % M][i / M] = tmp8;break;
			}
			++i;
		}
	}
	ENSURES(is_transpose(M, N, A, B));
}

char trans10_desc[] = "<<<61 * 67>>V3>>";
void trans10(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, ii, jj;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	
	for (i = 0; i < 64; i += 8)
	{
		for (j = 0; j < 56; j +=8)
		{
			for (ii = i; ii < i + 8; ++ii)
				for (jj = j; jj < j + 8; ++jj)
					B[jj][ii] = A[ii][jj];
		}
		for (ii = i; ii < i + 8; ++ii)
			for (jj = 56; jj < 61; ++jj)
				B[jj][ii] = A[ii][jj];
	}
	for (j = 0; j < 56; j += 8)
	{
		for (ii = 64; ii < 67; ++ii)
			for (jj = j; jj < j + 8; ++jj)
				B[jj][ii] = A[ii][jj];
	}
	for (ii = 64; ii < 67; ++ii)
		for (jj = 56; jj < 61; ++jj)
			B[jj][ii] = A[ii][jj];

	
	ENSURES(is_transpose(M, N, A, B));
}

char trans11_desc[] = "<<<61 * 67>>V4>>";
void trans11(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, ii, jj;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	
	for (i = 0; i < 56; i += 8)
	{
		for (j = 0; j < 48; j +=8)
		{
			for (ii = i; ii < i + 8; ++ii)
				for (jj = j; jj < j + 8; ++jj)
					B[jj][ii] = A[ii][jj];
		}
		for (ii = i; ii < i + 8; ++ii)
			for (jj = 48; jj < 61; ++jj)
				B[jj][ii] = A[ii][jj];
	}
	for (j = 0; j < 48; j += 8)
	{
		for (ii = 56; ii < 67; ++ii)
			for (jj = j; jj < j + 8; ++jj)
				B[jj][ii] = A[ii][jj];
	}
	for (ii = 56; ii < 67; ++ii)
		for (jj = 48; jj < 61; ++jj)
			B[jj][ii] = A[ii][jj];

	
	ENSURES(is_transpose(M, N, A, B));
}

char trans12_desc[] = "<<<61 * 67>>V5>>";
void trans12(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, ii, jj;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	
	for (i = 0; i < 56; i += 8)
	{
		for (j = 0; j < 48; j +=8)
		{
			for (ii = i; ii < i + 8; ++ii)
				for (jj = j; jj < j + 8; ++jj)
					B[jj][ii] = A[ii][jj];
		}
	}
	for (j = 0; j < 48; j += 8)
	{
		for (ii = 56; ii < 67; ++ii)
			for (jj = j; jj < j + 8; ++jj)
				B[jj][ii] = A[ii][jj];
	}
	for (ii = 56; ii < 67; ++ii)
		for (jj = 48; jj < 61; ++jj)
			B[jj][ii] = A[ii][jj];
	for (i = 0; i < 56; i += 8)
	{
		for (ii = i; ii < i + 8; ++ii)
			for (jj = 48; jj < 61; ++jj)
				B[jj][ii] = A[ii][jj];
	}
	
	
	ENSURES(is_transpose(M, N, A, B));
}


char trans13_desc[] = "<<<61 * 67>>V6>>";
void trans13(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, ii, jj, deta = 0;
	REQUIRES(M > 0);
    REQUIRES(N > 0);
	for (i = 0; i < 64; i += 8)
	{
		for (j = 0; j < 48; j += 8)
		{
			for (ii = i; ii < i + 8; ++ii)
			{
				for (jj = j + deta; jj < j + deta + 8; ++jj)
					B[jj][ii] = A[ii][jj];
				deta = (deta + 3) % 8;
			}
		}
		for (ii = i; ii < i + 8; ++ii)
		{
			for (jj = 0; jj < deta; ++jj)
				B[jj][ii] = A[ii][jj];
			for (jj = 61 - (13 - deta); jj < 61; ++jj)
				B[jj][ii] = A[ii][jj];
			deta = (deta + 3) % 8;
		}
	}
	for (j = 0; j < 56; j += 8)
		for (ii = 64; ii < 67; ++ii)
			for (jj = j; jj < j + 8; ++jj)
				B[jj][ii] = A[ii][jj];
	for (ii = 64; ii < 67; ++ii)
		for (jj = 56; jj < 61; ++ jj)
			B[jj][ii] = A[ii][jj];
	
	ENSURES(is_transpose(M, N, A, B));
}




/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
	/* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    
	registerTransFunction(trans, trans_desc); 
	registerTransFunction(trans1, trans1_desc); 
	registerTransFunction(trans2, trans2_desc); 
	/*registerTransFunction(trans3, trans3_desc); 
	registerTransFunction(trans4, trans4_desc);
	registerTransFunction(trans5, trans5_desc);
	*/
	registerTransFunction(trans6, trans6_desc);
	/*registerTransFunction(trans7, trans7_desc);
	registerTransFunction(trans8, trans8_desc);
	registerTransFunction(trans9, trans9_desc);
	registerTransFunction(trans10, trans10_desc);
	registerTransFunction(trans11, trans11_desc);
	registerTransFunction(trans12, trans12_desc);
	*/
	registerTransFunction(trans13, trans13_desc);
	/*registerTransFunction(trans14, trans14_desc);
	registerTransFunction(trans15, trans15_desc);
	registerTransFunction(trans16, trans16_desc);
	registerTransFunction(trans17, trans17_desc);
	*/

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
			if (A[i][j] != B[j][i]) {
				return 0;
			}
		}
    }
    return 1;
}

