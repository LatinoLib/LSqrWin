/*==========================================================================;
 *
 *  File:          LSqrDotNet.cs
 *  Version:       1.0
 *  Desc:		   C# wrapper for LSQR DLL
 *  Author:        Miha Grcar 
 *  Created on:    Oct-2007
 *  Last modified: Oct-2007
 *  License:       Common Public License (CPL)
 *  Note:
 *      See ReadMe.txt for additional info and acknowledgements.
 * 
 ***************************************************************************/

using System;
using System.Runtime.InteropServices;

namespace LSqrDotNet
{
    /* .-----------------------------------------------------------------------
       |		 
       |  Class LSqrDll
       |
       '-----------------------------------------------------------------------
    */
    public class LSqrDll
    {
    #if DEBUG
        private const string LSQR_DLL = "LSqrDebug.dll";
    #else
        private const string LSQR_DLL = "LSqr.dll";
    #endif
        // *** external functions ***
        [DllImport(LSQR_DLL)]
        public static extern int NewMatrix(int row_count);
        [DllImport(LSQR_DLL)]
        public static extern void DeleteMatrix(int id);
        [DllImport(LSQR_DLL)]
        public static extern void InsertValue(int mat_id, int row_idx, int col_idx, double val);
        [DllImport(LSQR_DLL)]
        public static extern IntPtr DoLSqr(int mat_id, int mat_transp_id, double[] init_sol, double[] rhs, int max_iter);
        // *** wrappers for external DoLSqr ***
        public static double[] DoLSqr(int num_cols, LSqrSparseMatrix mat, LSqrSparseMatrix mat_transp, double[] rhs, int max_iter)
        {
            return DoLSqr(num_cols, mat, mat_transp, /*init_sol=*/null, rhs, max_iter);
        }
        public static double[] DoLSqr(int num_cols, LSqrSparseMatrix mat, LSqrSparseMatrix mat_transp, double[] init_sol, double[] rhs, int max_iter)
        {
            IntPtr sol_ptr = DoLSqr(mat.Id, mat_transp.Id, init_sol, rhs, max_iter);
            GC.KeepAlive(mat); // avoid premature garbage collection
            GC.KeepAlive(mat_transp);
            double[] sol = new double[num_cols];
            Marshal.Copy(sol_ptr, sol, 0, sol.Length);
            Marshal.FreeHGlobal(sol_ptr);
            return sol;
        }
    }

    /* .-----------------------------------------------------------------------
       |		 
       |  Class LSqrSparseMatrix
       |
       '-----------------------------------------------------------------------
    */
    public class LSqrSparseMatrix : IDisposable
    {
        private int m_id;
        public LSqrSparseMatrix(int row_count)
        {
            m_id = LSqrDll.NewMatrix(row_count);
        }
        ~LSqrSparseMatrix()
        {
            Dispose();
        }
        public int Id
        {
            get { return m_id; }
        }
        public void InsertValue(int row_idx, int col_idx, double val)
        {
            LSqrDll.InsertValue(m_id, row_idx, col_idx, val);
        }
        public static LSqrSparseMatrix FromDenseMatrix(double[,] mat)
        {
            LSqrSparseMatrix lsqr_mat = new LSqrSparseMatrix(mat.GetLength(0));
            for (int row = 0; row < mat.GetLength(0); row++)
            {
                for (int col = 0; col < mat.GetLength(1); col++)
                {
                    if (mat[row, col] != 0)
                    {
                        lsqr_mat.InsertValue(row, col, mat[row, col]);
                    }
                }
            }
            return lsqr_mat;
        }
        public static LSqrSparseMatrix TransposeFromDenseMatrix(double[,] mat)
        {
            LSqrSparseMatrix lsqr_mat = new LSqrSparseMatrix(mat.GetLength(1));
            for (int col = 0; col < mat.GetLength(1); col++)
            {
                for (int row = 0; row < mat.GetLength(0); row++)
                {
                    if (mat[row, col] != 0)
                    {
                        lsqr_mat.InsertValue(col, row, mat[row, col]);
                    }
                }
            }
            return lsqr_mat;
        }
        public void Dispose()
        {
            if (m_id >= 0)
            {
                LSqrDll.DeleteMatrix(m_id);
                m_id = -1;
            }
        }
    }
}