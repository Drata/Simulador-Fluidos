#include "solver.h"
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#define MAX_CELLS (N+2) * (N+2)

void Solver::Init(unsigned N, float dt, float diff, float visc)
{
	this->dt = dt;
	this->diff = diff;
	this->visc = visc;
	this->N = N;
}

/*
----------------------------------------------------------------------
free/clear/allocate simulation data
----------------------------------------------------------------------
*/
void Solver::FreeData(void)
{
	//Liberamos la memoria reservada de los buffers previos.
	if (this->u_prev != NULL)
	{
		free(this->u_prev);
	}

	if (this->v_prev != NULL)
	{
		free(this->v_prev);
	}

	if (this->dens_prev != NULL)
	{
		free(this->dens_prev);
	}

	//Liberamos la memoria reservada de los buffers del estado actual.
	if (this->u != NULL)
	{
		free(this->u);
	}

	if (this->v != NULL)
	{
		free(this->v);
	}

	if (this->dens != NULL)
	{
		free(this->dens);
	}
}

void Solver::ClearData(void)
{
	for(int i = 0; i < MAX_CELLS; i++) 
	{
		this->u_prev[i] = 0;
		this->v_prev[i] = 0;
		this->dens_prev[i] = 0;

		this->u[i] = 0;
		this->v[i] = 0;
		this->dens[i] = 0;
	}
}

bool Solver::AllocateData(void)
{
	//Reservamos memoria para los buffers previos.
	this->u_prev = (float *)malloc(MAX_CELLS * sizeof(float));
	
	if (this->u_prev == NULL) 
	{
		return false;
	}

	this->v_prev = (float *)malloc(MAX_CELLS * sizeof(float));
	
	if (this->v_prev == NULL)
	{
		return false;
	}
	
	this->dens_prev = (float *)malloc(MAX_CELLS * sizeof(float));

	if (this->dens_prev == NULL)
	{
		return false;
	}

	//Reservamos memoria para los buffers del estado actual.
	this->u = (float *)malloc(MAX_CELLS * sizeof(float));

	if (this->u == NULL)
	{
		return false;
	}

	this->v = (float *)malloc(MAX_CELLS * sizeof(float));

	if (this->v == NULL)
	{
		return false;
	}

	this->dens = (float *)malloc(MAX_CELLS * sizeof(float));

	if (this->dens == NULL)
	{
		return false;
	}

	//Limpiamos la memoria reservada.
	ClearData();

	return true;
}

void Solver::ClearPrevData() 
{
	for (unsigned int i = 0; i < MAX_CELLS; i++)
	{
		this->u_prev[i] = 0;
		this->v_prev[i] = 0;
		this->dens_prev[i] = 0;

	}
}

void Solver::AddDensity(unsigned x, unsigned y, float source)
{
	this->dens_prev[XY_TO_ARRAY(x, y)] += source;
}

void Solver::AddVelocity(unsigned x, unsigned y, float forceX, float forceY)
{
	this->u_prev[XY_TO_ARRAY(x, y)] += forceX;
	this->v_prev[XY_TO_ARRAY(x, y)] += forceY;
}

void Solver::Solve()
{
	VelStep();
	DensStep();
}

void Solver::DensStep()
{
	AddSource(dens, dens_prev);			//Adding input density (dens_prev) to final density (dens).
	SWAP(dens_prev, dens)				//Swapping matrixes, because we want save the next result in dens, not in dens_prev.
	Diffuse(0, dens, dens_prev);		//Writing result in dens because we made the swap before. bi = dens_prev. The initial trash in dens matrix, doesnt matter, because it converges anyways.
	SWAP(dens_prev, dens)				//Swapping matrixes, because we want save the next result in dens, not in dens_prev.
	Advect(0, dens, dens_prev, u, v);	//Advect phase, result in dens.
}

void Solver::VelStep()
{
	AddSource(u, u_prev);
	AddSource(v, v_prev);
	SWAP (u_prev,u)			
	SWAP (v_prev, v)
	Diffuse(1, u, u_prev);  
	Diffuse(2, v, v_prev); 
	Project(u, v, u_prev, v_prev);		//Mass conserving.
	SWAP (u_prev,u)			
	SWAP (v_prev,v)
	Advect(1, u, u_prev, u_prev, v_prev);
	Advect(2, v, v_prev, u_prev, v_prev);
	Project(u, v, u_prev, v_prev);		//Mass conserving.
}

void Solver::AddSource(float * base, float * source)
{
	FOR_EACH_CELL
		base[XY_TO_ARRAY(i, j)] += source[XY_TO_ARRAY(i, j)];
	END_FOR
}


void Solver::SetBounds(int b, float * x)
{
	//Comprobamos el caso.
	switch(b) 
	{
		case 0:

			//Recorremos los bordes.
			for(int i = 0; i < N + 2; i++) 
			{
				x[XY_TO_ARRAY(0, i)] = x[XY_TO_ARRAY(1, i)];
				x[XY_TO_ARRAY(i, 0)] = x[XY_TO_ARRAY(i, 1)];
				x[XY_TO_ARRAY(N + 1, i)] = x[XY_TO_ARRAY(N, i)];
				x[XY_TO_ARRAY(i, N + 1)] = x[XY_TO_ARRAY(i, N)];
			}

			break;
		case 1:

			//Recorremos los bordes.
			for (int i = 0; i < N + 2; i++)
			{
				x[XY_TO_ARRAY(0, i)] = x[XY_TO_ARRAY(1, i)];
				x[XY_TO_ARRAY(i, 0)] = -x[XY_TO_ARRAY(i, 1)];
				x[XY_TO_ARRAY(N + 1, i)] = x[XY_TO_ARRAY(N, i)];
				x[XY_TO_ARRAY(i, N + 1)] = -x[XY_TO_ARRAY(i, N)];
			}

			break;
		case 2:

			//Recorremos los bordes.
			for (int i = 0; i < N + 2; i++)
			{
				x[XY_TO_ARRAY(0, i)] = -x[XY_TO_ARRAY(1, i)];
				x[XY_TO_ARRAY(i, 0)] = x[XY_TO_ARRAY(i, 1)];
				x[XY_TO_ARRAY(N + 1, i)] = -x[XY_TO_ARRAY(N, i)];
				x[XY_TO_ARRAY(i, N + 1)] = x[XY_TO_ARRAY(i, N)];
			}

			break;
	}

	//Establecemos las esquinas.
	x[XY_TO_ARRAY(0, 0)] = 0;
	x[XY_TO_ARRAY(0, N + 1)] = 0;
	x[XY_TO_ARRAY(N + 1, 0)] = 0;
	x[XY_TO_ARRAY(N + 1, N + 1)] = 0;

}

/*
https://www.youtube.com/watch?v=62_RUX_hrT4
https://es.wikipedia.org/wiki/M%C3%A9todo_de_Gauss-Seidel <- Solución de valores independientes.
Despreciando posibles valores de x no contiguos, se simplifica mucho. Mirar diapositivas y la solución de Gauss Seidel de términos independientes.
Gauss Seidel -> Matrix x and x0
*/
void Solver::LinSolve(int b, float * x, float * x0, float aij, float aii)
{
	memccpy(x, x0, MAX_CELLS, sizeof(float));

	for (int k = 0; k < 30; k++) 
	{
		FOR_EACH_CELL
			x[XY_TO_ARRAY(i, j)] = (1 / aii)*(aij*(x[XY_TO_ARRAY(i, j - 1)] + 
				x[XY_TO_ARRAY(i - 1, j)] + 
				x[XY_TO_ARRAY(i + 1, j)] + 
				x[XY_TO_ARRAY(i, j + 1)]) + 
				x0[XY_TO_ARRAY(i, j)]);
		END_FOR

		SetBounds(b, x);
	}
}

/*
Nuestra función de difusión solo debe resolver el sistema de ecuaciones simplificado a las celdas contiguas de la casilla que queremos resolver,
por lo que solo con la entrada de dos valores, debemos poder obtener el resultado.
*/
void Solver::Diffuse(int b, float * x, float * x0)
{
	float aij, aii;

	aij = this->dt * this->diff * N * N;
 	aii = 1 + this->dt * 4 * this->diff * N * N;

	LinSolve(b, x, x0, aij, aii);
}

/*
d is overwrited with the initial d0 data and affected by the u & v vectorfield.
Hay que tener en cuenta que el centro de las casillas representa la posición entera dentro de la casilla, por lo que los bordes estan
en las posiciones x,5.
*/
void Solver::Advect(int b, float * d, float * d0, float * u, float * v)
{
	// Recorremos todas las celdas.
	FOR_EACH_CELL
		float prevU = (float)i - u[XY_TO_ARRAY(i, j)] * dt * N;
		float prevV = (float)j - v[XY_TO_ARRAY(i, j)] * dt * N;

		if (prevU < 0.5f) 
		{
			prevU = 0.5f;
		}
		else if (prevU > N + 0.5f)
		{
			prevU = N + 0.5f;
		}

		if (prevV < 0.5f) 
		{
			prevV = 0.5f;
		}
		else if (prevV > N + 0.5f)
		{
			prevV = N + 0.5f;
		}

		int integerU = (int)prevU;
		int integerV = (int)prevV;

		float difU = fabs(prevU - integerU);
		float difV = fabs(prevV - integerV);

		d[XY_TO_ARRAY(i, j)] = ((1 - difU)* (1 - difV)) * d0[XY_TO_ARRAY(integerU, integerV)]
			                 + ((1 - difU)* difV) * d0[XY_TO_ARRAY(integerU, integerV + 1)]
			                 + (difU * (1 - difV)) * d0[XY_TO_ARRAY(integerU + 1, integerV)]
			                 + (difU * difV) * d0[XY_TO_ARRAY(integerU + 1, integerV + 1)];
	END_FOR
}

/*
Se encarga de estabilizar el fluido y hacerlo conservativo de masa. Se usa solo en las matrices de velocidades.
No necesaria implementación por su complejidad.
*/
void Solver::Project(float * u, float * v, float * p, float * div)
{
	FOR_EACH_CELL
		div[XY_TO_ARRAY(i, j)] = -0.5f*(u[XY_TO_ARRAY(i + 1, j)] - u[XY_TO_ARRAY(i - 1, j)] + v[XY_TO_ARRAY(i, j + 1)] - v[XY_TO_ARRAY(i, j - 1)]) / N;
		p[XY_TO_ARRAY(i, j)] = 0;
	END_FOR
	SetBounds(0, div);
	SetBounds(0, p);

	LinSolve(0, p, div, 1, 4);

	//Aproximamos: Laplaciano de q a su gradiente.
	FOR_EACH_CELL
		u[XY_TO_ARRAY(i, j)] -= 0.5f*N*(p[XY_TO_ARRAY(i + 1, j)] - p[XY_TO_ARRAY(i - 1, j)]);
		v[XY_TO_ARRAY(i, j)] -= 0.5f*N*(p[XY_TO_ARRAY(i, j + 1)] - p[XY_TO_ARRAY(i, j - 1)]);
	END_FOR
	SetBounds(1, u);
	SetBounds(2, v);
}