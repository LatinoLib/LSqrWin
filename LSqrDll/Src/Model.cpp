/*
* Model.cpp
*    Code for test model
*/

#include <math.h>
#include "Model.hpp"
#include "LsqrTypes.hpp"
#include "test_lsqr.hpp"
#define PI      (4.0 * atan(1.0))
#define sqr(a)  ( (a) * (a) )

void dvec_scale( double, dvec * );
//void test_mult( long, dvec *, dvec *, void *);

void lstp_mult( dvec  *h,
	        dvec  *x, 
                dvec  *y )
/*
*     ------------------------------------------------------------------
*     HPROD  applies a Householder transformation stored in H to get
*     Y = ( I - 2*HZ*HZ(transpose) ) * X.
*     ------------------------------------------------------------------
*/
{
  long    vec_indx,
          vec_length;

  double  dwork;
  
  vec_length = h->length;
  dwork = 0.0;

  for( vec_indx = 0; vec_indx < vec_length; vec_indx++ )
    dwork += h->elements[vec_indx] * x->elements[vec_indx];
  
  dwork += dwork;
  
  for( vec_indx = 0; vec_indx < vec_length; vec_indx++ )
    y->elements[vec_indx] = x->elements[vec_indx] - dwork * 
h->elements[vec_indx];
  
  return;
}



void lstp( long       duplicate_param,
	   long       power_param,
	   double     damp_param,
	   dvec       *act_sol_vec,
	   dvec       *act_rhs_vec, 
           prod_data  *test_prod, 
	   double     *act_mat_cond_num, 
	   double     *act_resid_norm )
/*     
*     ------------------------------------------------------------------
*     LSTP  generates a sparse least-squares test problem of the form
*
*                (   A    )*X = ( B ) 
*                ( DAMP*I )     ( 0 )
*
*     having a specified solution X.  The matrix A is constructed
*     in the form  A = HY*D*HZ,  where D is an 'num_rows' by 'num_cols'
*     diagonal matrix, and HY and HZ are Householder transformations.
*
*     The first 4 parameters are input to LSTP.  The remainder are
*     output.  LSTP is intended for use with 'num_rows' >= 'num_cols'.
*     ------------------------------------------------------------------
*/
{
  double   dvec_norm2( dvec * );

  long     num_rows,
	   num_cols,
	   row_indx,
           col_indx,
           lwork;

  double   mnorm,
           nnorm,
           dwork;

  num_rows = act_rhs_vec->length;
  num_cols = act_sol_vec->length;
/*  
*     Make two vectors of norm 1.0 for the Householder transformations.
*/
  for( row_indx = 0; row_indx < num_rows; row_indx++ )
    test_prod->hy_vec->elements[row_indx] =
       sin( (double)(row_indx + 1) * 4.0 * PI / ( (double)(num_rows) ) );
  
  for( col_indx = 0; col_indx < num_cols; col_indx++ )
    test_prod->hz_vec->elements[col_indx] =
       cos( (double)(col_indx + 1) * 4.0 * PI / ( (double)(num_cols) ) );
  
  mnorm = dvec_norm2( test_prod->hy_vec );
  nnorm = dvec_norm2( test_prod->hz_vec );
  dvec_scale( (-1.0 / mnorm), test_prod->hy_vec );
  dvec_scale( (-1.0 / nnorm), test_prod->hz_vec );
/*  
*     Set the diagonal matrix D.  These are the singular values of A.
*/
  for( col_indx = 0; col_indx < num_cols; col_indx++ )
    {
      lwork = ( col_indx + duplicate_param ) / duplicate_param;
      dwork = (double) lwork * (double) duplicate_param;
      dwork /= (double) num_cols;
      test_prod->d_vec->elements[col_indx] = pow( dwork, ( (double) power_param 
) );
    }

  (*act_mat_cond_num) =
     sqrt((sqr(test_prod->d_vec->elements[num_cols - 1]) + sqr(damp_param)) / 
          (sqr(test_prod->d_vec->elements[0])            + sqr(damp_param)));
/*  
*     Compute the residual vector, storing it in  B.
*     It takes the form  HY*( S )
*                           ( T )
*     where S is obtained from  D*S = DAMP**2 * HZ * X and T can be anything.
*/
  lstp_mult( test_prod->hz_vec, act_sol_vec, act_rhs_vec );
  
  for( col_indx = 0; col_indx < num_cols; col_indx++ )
    act_rhs_vec->elements[col_indx] *= sqr( damp_param ) /
      test_prod->d_vec->elements[col_indx];
  
  dwork = 1.0;
  for( row_indx = num_cols; row_indx < num_rows; row_indx++ )
    {
      lwork = row_indx - (num_cols - 1);
      act_rhs_vec->elements[row_indx] = ( dwork * (double)(lwork) ) /
	( (double)(num_rows) );
      dwork = -dwork;
    }
  
  lstp_mult( test_prod->hy_vec, act_rhs_vec, act_rhs_vec );
/*
*     Now compute the true B = RESIDUAL + A*X.
*/
  mnorm = dvec_norm2( act_rhs_vec );
  nnorm = dvec_norm2( act_sol_vec );
  (*act_resid_norm) = sqrt( sqr( mnorm ) + sqr( damp_param ) * sqr( nnorm ) );
  
  test_mult( 0, act_sol_vec, act_rhs_vec, test_prod );

  return;
}


void test_mult( long  mode,
	        dvec  *x, 
                dvec  *y,
                void  *prod )

/*
*     ------------------------------------------------------------------
*     This is the matrix-vector product routine required by LSQR
*     for a test matrix of the form  A = HY*D*HZ.  The quantities
*     defining D, HY, HZ and the work array W are in the structure data.
*     ------------------------------------------------------------------
*/
{
  long       vec_indx,
             num_rows,
             num_cols;

  prod_data  *data;
  
  data = (prod_data *) prod;  

  num_cols = x->length;
  num_rows = y->length;
/*  
*     Compute  Y = Y + A*X
*/
  if( mode == 0 )
    {
      lstp_mult( data->hz_vec, x, data->work_vec );
      
      for( vec_indx = 0; vec_indx < num_cols; vec_indx++ )
	data->work_vec->elements[vec_indx] *= data->d_vec->elements[vec_indx];
      
      for( vec_indx = num_cols; vec_indx < num_rows; vec_indx++ )
	data->work_vec->elements[vec_indx] = 0.0;
      
      lstp_mult( data->hy_vec, data->work_vec, data->work_vec );

      for( vec_indx = 0; vec_indx < num_rows; vec_indx++ )
	y->elements[vec_indx] += data->work_vec->elements[vec_indx];
    }
/*  
*     Compute  X = X + A^T*Y
*/  
  if( mode == 1 )
    {
      lstp_mult( data->hy_vec, y, data->work_vec );
      
      for( vec_indx = 0; vec_indx < num_cols; vec_indx++ )
	data->work_vec->elements[vec_indx] *= data->d_vec->elements[vec_indx];
      
      lstp_mult( data->hz_vec, data->work_vec, data->work_vec );

      for( vec_indx = 0; vec_indx < num_cols; vec_indx++ )
	x->elements[vec_indx] += data->work_vec->elements[vec_indx];
    }
  
  return;
}

void Model::matVecMult( long  mode, dvec  *x, dvec  *y){
  //  prod_data *prod
  test_mult (mode, x, y, prod);
  return;
}
